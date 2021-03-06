/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "qfiles.h"
#include "common.h"
#include "ref.h"
extern Refimport ri;

#ifdef USE_INTERNAL_JPEG
#  define JPEG_INTERNALS
#endif

#include <jpeglib.h>

#ifndef USE_INTERNAL_JPEG
#  if JPEG_LIB_VERSION < 80
#    error Need system libjpeg >= 80
#  endif
#endif

static void
R_JPGErrorExit(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message)(cinfo, buffer);

	/* Let the memory manager delete any temp files before we die */
	jpeg_destroy(cinfo);

	ri.Error(ERR_FATAL, "%s", buffer);
}

static void
R_JPGOutputMessage(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	/* Create the message */
	(*cinfo->err->format_message)(cinfo, buffer);

	ri.Printf(PRINT_ALL, "%s\n", buffer);
}

void
R_LoadJPG(const char *filename, unsigned char **pic, int *width, int *height)
{
	struct jpeg_decompress_struct cinfo = {NULL};
	struct jpeg_error_mgr jerr;

	JSAMPARRAY buffer;		/* Output row buffer */
	unsigned int	row_stride;	/* physical row width in output buffer */
	unsigned int	pixelcount, memcount;
	unsigned int	sindex, dindex;
	byte	*out;
	int	len;
	union {
		byte	*b;
		void	*v;
	} fbuffer;
	byte *buf;

	/* Read the JPEG file */
	len = ri.fsreadfile (( char* )filename, &fbuffer.v);
	if(!fbuffer.b || len < 0){
		return;
	}

	/* Step 1: allocate and initialize JPEG decompression object */
	/* Set up the error handler first */
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = R_JPGErrorExit;
	cinfo.err->output_message = R_JPGOutputMessage;

	/* Initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */
	jpeg_mem_src(&cinfo, fbuffer.b, len);

	/* Step 3: read file parameters with jpeg_read_header() */
	(void)jpeg_read_header(&cinfo, TRUE);

	/* Step 4: set parameters for decompression */

	/* Make sure it always converts images to RGB color space. This will
	 * automatically convert 8-bit greyscale images to RGB as well. */
	cinfo.out_color_space = JCS_RGB;

	/* Step 5: Start decompressor */
	(void)jpeg_start_decompress(&cinfo);

	/* JSAMPLEs per row in output buffer */
	pixelcount = cinfo.output_width * cinfo.output_height;

	if(!cinfo.output_width || !cinfo.output_height
	   || ((pixelcount * 4) / cinfo.output_width) / 4 != cinfo.output_height
	   || pixelcount > 0x1FFFFFFF || cinfo.output_components != 3){
		/* Free to make sure we don't leak memory */
		ri.fsfreefile (fbuffer.v);
		jpeg_destroy_decompress(&cinfo);

		ri.Error(ERR_DROP, "LoadJPG: %s has an invalid image format: "
				   "%dx%d*4=%d, components: %d", filename, cinfo.output_width,
			cinfo.output_height, pixelcount * 4, cinfo.output_components);
	}

	memcount	= pixelcount * 4;
	row_stride	= cinfo.output_width * cinfo.output_components;

	out = ri.Malloc(memcount);

	*width	= cinfo.output_width;
	*height = cinfo.output_height;

	/* Step 6: while (scan lines remain to be read)
	 *           jpeg_read_scanlines(...); */

	/* Use the library's state variable cinfo.output_scanline as the loop
	 * counter */
	while(cinfo.output_scanline < cinfo.output_height){
		buf = ((out+(row_stride*cinfo.output_scanline)));
		buffer = &buf;
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
	}

	buf = out;

	/* Expand from RGB to RGBA */
	sindex	= pixelcount * cinfo.output_components;
	dindex	= memcount;

	do {
		buf[--dindex]	= 255;
		buf[--dindex]	= buf[--sindex];
		buf[--dindex]	= buf[--sindex];
		buf[--dindex]	= buf[--sindex];
	} while(sindex);

	*pic = out;

	/* Step 7: Finish decompression */

	jpeg_finish_decompress(&cinfo);

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);
	ri.fsfreefile (fbuffer.v);

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero). */
}

/* Expanded data destination object for stdio output */
typedef struct {
	struct jpeg_destination_mgr pub;	/* public fields */

	byte	* outfile;	/* target stream */
	int	size;
} my_destination_mgr;

typedef my_destination_mgr *my_dest_ptr;

/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

static void
init_destination(j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr)cinfo->dest;

	dest->pub.next_output_byte	= dest->outfile;
	dest->pub.free_in_buffer	= dest->size;
}


/*
 * Called whenever buffer fills up
 * */
static boolean
empty_output_buffer(j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr)cinfo->dest;

	jpeg_destroy_compress(cinfo);

	/* Make crash fatal or we would probably leak memory. */
	ri.Error(ERR_FATAL, "Output buffer for encoded JPEG image has"
			    " insufficient size of %d bytes", dest->size);

	return FALSE;
}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 */
static void
term_destination(j_compress_ptr cinfo)
{
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */
static void
jpegDest(j_compress_ptr cinfo, byte* outfile, int size)
{
	my_dest_ptr dest;

	/* The destination object is made permanent so that multiple JPEG images
	 * can be written to the same file without re-executing jpeg_stdio_dest.
	 * This makes it dangerous to use this manager and a different destination
	 * manager serially with the same JPEG object, because their private object
	 * sizes may be different.  Caveat programmer.
	 */
	if(cinfo->dest == NULL){	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr*)
			      (*cinfo->mem->alloc_small)((j_common_ptr)cinfo,
			JPOOL_PERMANENT, sizeof(my_destination_mgr));
	}

	dest = (my_dest_ptr)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
	dest->size = size;
}

/*
 * Encodes JPEG from image in image_buffer and writes to buffer.
 * Expects RGB input data
 */
size_t
RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality,
		   int image_width, int image_height, byte *image_buffer, int padding)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	my_dest_ptr dest;
	int row_stride;	/* physical row width in image buffer */
	size_t outcount;

	/* Step 1: allocate and initialize JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = R_JPGErrorExit;
	cinfo.err->output_message = R_JPGOutputMessage;

	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	jpegDest(&cinfo, buffer, bufSize);

	/* Step 3: set parameters for compression */
	cinfo.image_width	= image_width;	/* image width and height, in pixels */
	cinfo.image_height	= image_height;
	cinfo.input_components	= 3;		/* # of color components per pixel */
	cinfo.in_color_space	= JCS_RGB;	/* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
	/* If quality is set high, disable chroma subsampling */
	if(quality >= 85){
		cinfo.comp_info[0].h_samp_factor	= 1;
		cinfo.comp_info[0].v_samp_factor	= 1;
	}

	/* Step 4: Start compressor */
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written)
	 *           jpeg_write_scanlines(...); */
	row_stride = image_width * cinfo.input_components + padding;	/* JSAMPLEs per row in image_buffer */

	while(cinfo.next_scanline < cinfo.image_height){
		row_pointer[0] = &image_buffer[((cinfo.image_height - 1)
						* row_stride) - cinfo.next_scanline * row_stride];
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */
	jpeg_finish_compress(&cinfo);

	dest = (my_dest_ptr)cinfo.dest;
	outcount = dest->size - dest->pub.free_in_buffer;

	/* Step 7: release JPEG compression object */
	jpeg_destroy_compress(&cinfo);

	return outcount;
}

void
RE_SaveJPG(char * filename, int quality, int image_width, int image_height, byte *image_buffer, int padding)
{
	byte *out;
	size_t bufSize;

	bufSize = image_width * image_height * 3;
	out = ri.hunkalloctemp(bufSize);

	bufSize = RE_SaveJPGToBuffer(out, bufSize, quality, image_width, image_height, image_buffer, padding);
	ri.fswritefile(filename, out, bufSize);

	ri.hunkfreetemp(out);
}
