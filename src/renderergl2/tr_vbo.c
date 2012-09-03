/*
 * Copyright (C) 2007-2009 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * This file is part of XreaL source code.
 *
 * XreaL source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * XreaL source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XreaL source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "tr_local.h"

VBO_t*
R_CreateVBO(const char *name, byte * vertexes, int vertexesSize, vboUsage_t usage)
{
	int glUsage;
	VBO_t *vbo;

	switch(usage){
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW_ARB;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW_ARB;
		break;
	default:
		Com_Errorf(ERR_FATAL, "bad vboUsage_t given: %i", usage);
		return nil;
	}

	if(strlen(name) >= MAX_QPATH){
		ri.Error(ERR_DROP, "R_CreateVBO: \"%s\" is too long\n", name);
	}
	if(tr.numVBOs == MAX_VBOS){
		ri.Error(ERR_DROP, "R_CreateVBO: MAX_VBOS hit\n");
	}

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	vbo =  ri.Hunk_Alloc(sizeof(*vbo), h_low);
	tr.vbos[tr.numVBOs] = vbo;
	tr.numVBOs++;
	memset(vbo, 0, sizeof(*vbo));
	Q_strncpyz(vbo->name, name, sizeof(vbo->name));

	vbo->vertexesSize = vertexesSize;
	qglGenBuffersARB(1, &vbo->vertexesVBO);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo->vertexesVBO);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, vertexesSize, vertexes, glUsage);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	GL_CheckErrors();
	glState.currentVBO = nil;
	return vbo;
}

VBO_t*
R_CreateVBO2(const char *name, int nverts, srfVert_t *verts,
	unsigned int flags, vboUsage_t usage)
{
	int i;
	byte *data;
	int dataSize, dataOfs;
	int glUsage;
	VBO_t *vbo;

	switch(usage){
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW_ARB;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW_ARB;
		break;
	default:
		Com_Errorf(ERR_FATAL, "bad vboUsage_t given: %i", usage);
		return nil;
	}

	if(nverts < 1)
		return nil;

	if(strlen(name) >= MAX_QPATH){
		ri.Error(ERR_DROP, "R_CreateVBO2: \"%s\" is too long\n", name);
	}

	if(tr.numVBOs == MAX_VBOS){
		ri.Error(ERR_DROP, "R_CreateVBO2: MAX_VBOS hit\n");
	}

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	vbo = ri.Hunk_Alloc(sizeof(*vbo), h_low);
	tr.vbos[tr.numVBOs] = vbo;
	tr.numVBOs++;
	memset(vbo, 0, sizeof(*vbo));
	Q_strncpyz(vbo->name, name, sizeof(vbo->name));

	if(usage == VBO_USAGE_STATIC){
		/* since these vertex attributes are never altered, interleave them */
		vbo->ofs_xyz = 0;
		dataSize = sizeof(verts[0].xyz);
		if(flags & ATTR_NORMAL){
			vbo->ofs_normal = dataSize;
			dataSize += sizeof(verts[0].normal);
		}
		if(flags & ATTR_TANGENT){
			vbo->ofs_tangent = dataSize;
			dataSize += sizeof(verts[0].tangent);
		}
		if(flags & ATTR_BITANGENT){
			vbo->ofs_bitangent = dataSize;
			dataSize += sizeof(verts[0].bitangent);
		}
		if(flags & ATTR_TEXCOORD){
			vbo->ofs_st = dataSize;
			dataSize += sizeof(verts[0].st);
		}
		if(flags & ATTR_LIGHTCOORD){
			vbo->ofs_lightmap = dataSize;
			dataSize += sizeof(verts[0].lightmap);
		}
		if(flags & ATTR_COLOR){
			vbo->ofs_vertexcolor = dataSize;
			dataSize += sizeof(verts[0].vertexColors);
		}
		if(flags & ATTR_LIGHTDIRECTION){
			vbo->ofs_lightdir = dataSize;
			dataSize += sizeof(verts[0].lightdir);
		}

		vbo->stride_xyz = dataSize;
		vbo->stride_normal	= dataSize;
		vbo->stride_tangent	= dataSize;
		vbo->stride_bitangent	= dataSize;
		vbo->stride_st		= dataSize;
		vbo->stride_lightmap	= dataSize;
		vbo->stride_vertexcolor = dataSize;
		vbo->stride_lightdir	= dataSize;

		/* create VBO */
		dataSize *= nverts;
		data = ri.Hunk_AllocateTempMemory(dataSize);
		dataOfs = 0;

		/* ri.Printf(PRINT_ALL, "CreateVBO: %d, %d %d %d %d %d, %d %d %d %d %d\n", dataSize, vbo->ofs_xyz, vbo->ofs_normal, vbo->ofs_st, vbo->ofs_lightmap, vbo->ofs_vertexcolor,
		 * vbo->stride_xyz, vbo->stride_normal, vbo->stride_st, vbo->stride_lightmap, vbo->stride_vertexcolor); */

		for(i = 0; i < nverts; i++){
			/* xyz */
			memcpy(data + dataOfs, &verts[i].xyz, sizeof(verts[i].xyz));
			dataOfs += sizeof(verts[i].xyz);
			/* normal */
			if(flags & ATTR_NORMAL){
				memcpy(data + dataOfs, &verts[i].normal, sizeof(verts[i].normal));
				dataOfs += sizeof(verts[i].normal);
			}
			/* tangent */
			if(flags & ATTR_TANGENT){
				memcpy(data + dataOfs, &verts[i].tangent, sizeof(verts[i].tangent));
				dataOfs += sizeof(verts[i].tangent);
			}
			/* bitangent */
			if(flags & ATTR_BITANGENT){
				memcpy(data + dataOfs, &verts[i].bitangent, sizeof(verts[i].bitangent));
				dataOfs += sizeof(verts[i].bitangent);
			}
			/* vertex texcoords */
			if(flags & ATTR_TEXCOORD){
				memcpy(data + dataOfs, &verts[i].st, sizeof(verts[i].st));
				dataOfs += sizeof(verts[i].st);
			}
			/* feed vertex lightmap texcoords */
			if(flags & ATTR_LIGHTCOORD){
				memcpy(data + dataOfs, &verts[i].lightmap, sizeof(verts[i].lightmap));
				dataOfs += sizeof(verts[i].lightmap);
			}
			/* feed vertex colors */
			if(flags & ATTR_COLOR){
				memcpy(data + dataOfs, &verts[i].vertexColors, sizeof(verts[i].vertexColors));
				dataOfs += sizeof(verts[i].vertexColors);
			}
			/* feed vertex light directions */
			if(flags & ATTR_LIGHTDIRECTION){
				memcpy(data + dataOfs, &verts[i].lightdir, sizeof(verts[i].lightdir));
				dataOfs += sizeof(verts[i].lightdir);
			}
		}
	}else{
		/* since these vertex attributes may be changed, put them in flat arrays */
		dataSize = sizeof(verts[0].xyz);
		if(flags & ATTR_NORMAL){
			dataSize += sizeof(verts[0].normal);
		}
		if(flags & ATTR_TANGENT){
			dataSize += sizeof(verts[0].tangent);
		}
		if(flags & ATTR_BITANGENT){
			dataSize += sizeof(verts[0].bitangent);
		}
		if(flags & ATTR_TEXCOORD){
			dataSize += sizeof(verts[0].st);
		}
		if(flags & ATTR_LIGHTCOORD){
			dataSize += sizeof(verts[0].lightmap);
		}
		if(flags & ATTR_COLOR){
			dataSize += sizeof(verts[0].vertexColors);
		}
		if(flags & ATTR_LIGHTDIRECTION){
			dataSize += sizeof(verts[0].lightdir);
		}

		/* create VBO */
		dataSize *= nverts;
		data = ri.Hunk_AllocateTempMemory(dataSize);
		dataOfs = 0;

		vbo->ofs_xyz = 0;
		vbo->ofs_normal = 0;
		vbo->ofs_tangent = 0;
		vbo->ofs_bitangent = 0;
		vbo->ofs_st = 0;
		vbo->ofs_lightmap = 0;
		vbo->ofs_vertexcolor = 0;
		vbo->ofs_lightdir = 0;
		vbo->stride_xyz = sizeof(verts[0].xyz);
		vbo->stride_normal = sizeof(verts[0].normal);
		vbo->stride_tangent = sizeof(verts[0].tangent);
		vbo->stride_bitangent = sizeof(verts[0].bitangent);
		vbo->stride_vertexcolor = sizeof(verts[0].vertexColors);
		vbo->stride_st = sizeof(verts[0].st);
		vbo->stride_lightmap = sizeof(verts[0].lightmap);
		vbo->stride_lightdir = sizeof(verts[0].lightdir);

		/* ri.Printf(PRINT_ALL, "2CreateVBO: %d, %d %d %d %d %d, %d %d %d %d %d\n", dataSize, vbo->ofs_xyz, vbo->ofs_normal, vbo->ofs_st, vbo->ofs_lightmap, vbo->ofs_vertexcolor,
		 * vbo->stride_xyz, vbo->stride_normal, vbo->stride_st, vbo->stride_lightmap, vbo->stride_vertexcolor); */

		/* xyz */
		for(i = 0; i < nverts; i++){
			memcpy(data + dataOfs, &verts[i].xyz, sizeof(verts[i].xyz));
			dataOfs += sizeof(verts[i].xyz);
		}
		/* normal */
		if(flags & ATTR_NORMAL){
			vbo->ofs_normal = dataOfs;
			for(i = 0; i < nverts; i++){
				memcpy(data + dataOfs, &verts[i].normal, sizeof(verts[i].normal));
				dataOfs += sizeof(verts[i].normal);
			}
		}
		/* tangent */
		if(flags & ATTR_TANGENT){
			vbo->ofs_tangent = dataOfs;
			for(i = 0; i < nverts; i++){
				memcpy(data + dataOfs, &verts[i].tangent, sizeof(verts[i].tangent));
				dataOfs += sizeof(verts[i].tangent);
			}
		}
		/* bitangent */
		if(flags & ATTR_BITANGENT){
			vbo->ofs_bitangent = dataOfs;
			for(i = 0; i < nverts; i++){
				memcpy(data + dataOfs, &verts[i].bitangent, sizeof(verts[i].bitangent));
				dataOfs += sizeof(verts[i].bitangent);
			}
		}
		/* vertex texcoords */
		if(flags & ATTR_TEXCOORD){
			vbo->ofs_st = dataOfs;
			for(i = 0; i < nverts; i++){
				memcpy(data + dataOfs, &verts[i].st, sizeof(verts[i].st));
				dataOfs += sizeof(verts[i].st);
			}
		}
		/* feed vertex lightmap texcoords */
		if(flags & ATTR_LIGHTCOORD){
			vbo->ofs_lightmap = dataOfs;
			for(i = 0; i < nverts; i++){
				memcpy(data + dataOfs, &verts[i].lightmap, sizeof(verts[i].lightmap));
				dataOfs += sizeof(verts[i].lightmap);
			}
		}
		/* feed vertex colors */
		if(flags & ATTR_COLOR){
			vbo->ofs_vertexcolor = dataOfs;
			for(i = 0; i < nverts; i++){
				memcpy(data + dataOfs, &verts[i].vertexColors, sizeof(verts[i].vertexColors));
				dataOfs += sizeof(verts[i].vertexColors);
			}
		}
		/* feed vertex lightdirs */
		if(flags & ATTR_LIGHTDIRECTION){
			vbo->ofs_lightdir = dataOfs;
			for(i = 0; i < nverts; i++){
				memcpy(data + dataOfs, &verts[i].lightdir, sizeof(verts[i].lightdir));
				dataOfs += sizeof(verts[i].lightdir);
			}
		}
	}


	vbo->vertexesSize = dataSize;
	qglGenBuffersARB(1, &vbo->vertexesVBO);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo->vertexesVBO);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, dataSize, data, glUsage);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glState.currentVBO = nil;
	GL_CheckErrors();
	ri.Hunk_FreeTempMemory(data);
	return vbo;
}

IBO_t*
R_CreateIBO(const char *name, byte *indexes, int indexesSize, vboUsage_t usage)
{
	int glUsage;
	IBO_t *ibo;

	switch(usage){
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW_ARB;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW_ARB;
		break;
	default:
		Com_Errorf(ERR_FATAL, "bad vboUsage_t given: %i", usage);
		return nil;
	}

	if(strlen(name) >= MAX_QPATH){
		ri.Error(ERR_DROP, "R_CreateIBO: \"%s\" is too long\n", name);
	}
	if(tr.numIBOs == MAX_IBOS){
		ri.Error(ERR_DROP, "R_CreateIBO: MAX_IBOS hit\n");
	}

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	ibo = ri.Hunk_Alloc(sizeof(*ibo), h_low);
	tr.ibos[tr.numIBOs] = ibo;
	tr.numIBOs++;
	Q_strncpyz(ibo->name, name, sizeof(ibo->name));

	ibo->indexesSize = indexesSize;
	qglGenBuffersARB(1, &ibo->indexesVBO);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->indexesVBO);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexesSize, indexes, glUsage);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	GL_CheckErrors();
	glState.currentIBO = nil;
	return ibo;
}

IBO_t*
R_CreateIBO2(const char *name, int ntris, srfTriangle_t *triangles, vboUsage_t usage)
{
	int i, j;
	byte *indexes;
	int indexesSize, indexesOfs;
	int glUsage;
	srfTriangle_t *tri;
	glIndex_t index;
	IBO_t *ibo;

	switch(usage){
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW_ARB;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW_ARB;
		break;
	default:
		Com_Errorf(ERR_FATAL, "bad vboUsage_t given: %i", usage);
		return nil;
	}

	if(ntris < 1)
		return nil;

	if(strlen(name) >= MAX_QPATH){
		ri.Error(ERR_DROP, "R_CreateIBO2: \"%s\" is too long\n", name);
	}

	if(tr.numIBOs == MAX_IBOS){
		ri.Error(ERR_DROP, "R_CreateIBO2: MAX_IBOS hit\n");
	}

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	ibo = ri.Hunk_Alloc(sizeof(*ibo), h_low);
	tr.ibos[tr.numIBOs] = ibo;
	tr.numIBOs++;
	Q_strncpyz(ibo->name, name, sizeof(ibo->name));

	indexesSize = ntris * 3 * sizeof(int);
	indexes = ri.Hunk_AllocateTempMemory(indexesSize);
	indexesOfs = 0;
	for(i = 0, tri = triangles; i < ntris; i++, tri++)
		for(j = 0; j < 3; j++){
			index = tri->indexes[j];
			memcpy(indexes + indexesOfs, &index, sizeof(glIndex_t));
			indexesOfs += sizeof(glIndex_t);
		}

	ibo->indexesSize = indexesSize;
	qglGenBuffersARB(1, &ibo->indexesVBO);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->indexesVBO);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexesSize, indexes, glUsage);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	GL_CheckErrors();
	glState.currentIBO = nil;
	ri.Hunk_FreeTempMemory(indexes);
	return ibo;
}

void
R_BindVBO(VBO_t *vbo)
{
	if(vbo == nil){
		/* R_BindNullVBO(); */
		ri.Error(ERR_DROP, "R_BindNullVBO: null VBO");
		return;
	}

	if(r_logFile->integer){
		GLimp_LogComment(va("--- R_BindVBO(%s) ---\n", vbo->name));
	}

	if(glState.currentVBO != vbo){
		glState.currentVBO = vbo;
		glState.vertexAttribPointersSet = 0;
		glState.vertexAttribsInterpolation = 0;
		glState.vertexAttribsOldFrame	= 0;
		glState.vertexAttribsNewFrame	= 0;
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo->vertexesVBO);
		backEnd.pc.c_vboVertexBuffers++;
	}
}

void
R_BindNullVBO(void)
{
	GLimp_LogComment("--- R_BindNullVBO ---\n");

	if(glState.currentVBO){
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glState.currentVBO = nil;
	}
	GL_CheckErrors();
}

void
R_BindIBO(IBO_t *ibo)
{
	if(ibo == nil){
		/* R_BindNullIBO(); */
		ri.Error(ERR_DROP, "R_BindIBO: null IBO");
		return;
	}

	if(r_logFile->integer){
		GLimp_LogComment(va("--- R_BindIBO(%s) ---\n", ibo->name));
	}

	if(glState.currentIBO != ibo){
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->indexesVBO);
		glState.currentIBO = ibo;
		backEnd.pc.c_vboIndexBuffers++;
	}
}

void
R_BindNullIBO(void)
{
	GLimp_LogComment("--- R_BindNullIBO ---\n");

	if(glState.currentIBO){
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		glState.currentIBO = nil;
		glState.vertexAttribPointersSet = 0;
	}
}

void
R_InitVBOs(void)
{
	int dataSize;
	byte *data;
	VBO_t *pv;

	ri.Printf(PRINT_ALL, "------- R_InitVBOs -------\n");

	tr.numVBOs = 0;
	tr.numIBOs = 0;

	dataSize = sizeof(tess.xyz[0]);
	dataSize += sizeof(tess.normal[0]);
	dataSize += sizeof(tess.tangent[0]);
	dataSize += sizeof(tess.bitangent[0]);
	dataSize += sizeof(tess.vertexColors[0]);
	dataSize += sizeof(tess.texCoords[0][0]) * 2;
	dataSize += sizeof(tess.lightdir[0]);
	dataSize *= SHADER_MAX_VERTEXES;

	data = ri.Malloc(dataSize);
	memset(data, 0, dataSize);
	tess.vbo = R_CreateVBO("tessVertexArray_VBO", data, dataSize, VBO_USAGE_DYNAMIC);
	ri.Free(data);

	pv = tess.vbo;
	pv->ofs_xyz = 0;
	pv->ofs_normal = pv->ofs_xyz + sizeof(tess.xyz[0])*SHADER_MAX_VERTEXES;
	pv->ofs_tangent = pv->ofs_normal + sizeof(tess.normal[0])*SHADER_MAX_VERTEXES;
	pv->ofs_bitangent = pv->ofs_tangent + sizeof(tess.tangent[0])*SHADER_MAX_VERTEXES;
	/* these next two are actually interleaved */
	pv->ofs_st = pv->ofs_bitangent + sizeof(tess.bitangent[0])*SHADER_MAX_VERTEXES;
	pv->ofs_lightmap = pv->ofs_st + sizeof(tess.texCoords[0][0]);
	pv->ofs_vertexcolor = pv->ofs_st + sizeof(tess.texCoords[0][0]) * 2*SHADER_MAX_VERTEXES;
	pv->ofs_lightdir = pv->ofs_vertexcolor + sizeof(tess.vertexColors[0])*SHADER_MAX_VERTEXES;

	pv->stride_xyz = sizeof(tess.xyz[0]);
	pv->stride_normal = sizeof(tess.normal[0]);
	pv->stride_tangent = sizeof(tess.tangent[0]);
	pv->stride_bitangent = sizeof(tess.bitangent[0]);
	pv->stride_vertexcolor = sizeof(tess.vertexColors[0]);
	pv->stride_st = sizeof(tess.texCoords[0][0]) * 2;
	pv->stride_lightmap = sizeof(tess.texCoords[0][0]) * 2;
	pv->stride_lightdir = sizeof(tess.lightdir[0]);

	dataSize = sizeof(tess.indexes[0]) * SHADER_MAX_INDEXES;
	data = ri.Malloc(dataSize);
	memset(data, 0, dataSize);
	tess.ibo = R_CreateIBO("tessVertexArray_IBO", data, dataSize, VBO_USAGE_DYNAMIC);
	ri.Free(data);

	R_BindNullVBO();
	R_BindNullIBO();
	GL_CheckErrors();
}

void
R_ShutdownVBOs(void)
{
	int i;
	VBO_t	*vbo;
	IBO_t	*ibo;

	ri.Printf(PRINT_ALL, "------- R_ShutdownVBOs -------\n");

	R_BindNullVBO();
	R_BindNullIBO();

	for(i = 0; i < tr.numVBOs; i++){
		vbo = tr.vbos[i];

		if(vbo->vertexesVBO){
			qglDeleteBuffersARB(1, &vbo->vertexesVBO);
		}

		/* ri.Free(vbo); */
	}

	for(i = 0; i < tr.numIBOs; i++){
		ibo = tr.ibos[i];

		if(ibo->indexesVBO){
			qglDeleteBuffersARB(1, &ibo->indexesVBO);
		}

		/* ri.Free(ibo); */
	}

	tr.numVBOs = 0;
	tr.numIBOs = 0;
}

void
R_VBOList_f(void)
{
	int i;
	int vertexesSize = 0;
	int indexesSize = 0;
	VBO_t *vbo;
	IBO_t *ibo;

	ri.Printf(PRINT_ALL, " size          name\n");
	ri.Printf(PRINT_ALL, "----------------------------------------------------------\n");

	for(i = 0; i < tr.numVBOs; i++){
		vbo = tr.vbos[i];

		ri.Printf(PRINT_ALL, "%d.%02d MB %s\n", vbo->vertexesSize / (1024 * 1024),
			(vbo->vertexesSize % (1024 * 1024)) * 100 / (1024 * 1024), vbo->name);

		vertexesSize += vbo->vertexesSize;
	}

	for(i = 0; i < tr.numIBOs; i++){
		ibo = tr.ibos[i];

		ri.Printf(PRINT_ALL, "%d.%02d MB %s\n", ibo->indexesSize / (1024 * 1024),
			(ibo->indexesSize % (1024 * 1024)) * 100 / (1024 * 1024), ibo->name);

		indexesSize += ibo->indexesSize;
	}

	ri.Printf(PRINT_ALL, " %i total VBOs\n", tr.numVBOs);
	ri.Printf(PRINT_ALL, " %d.%02d MB total vertices memory\n", vertexesSize / (1024 * 1024),
		(vertexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
	ri.Printf(PRINT_ALL, " %i total IBOs\n", tr.numIBOs);
	ri.Printf(PRINT_ALL, " %d.%02d MB total triangle indices memory\n", indexesSize / (1024 * 1024),
		(indexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
}

/*
 * RB_UpdateVBOs: Tr3B: update the default VBO to replace the client side vertex arrays
 */
void
RB_UpdateVBOs(unsigned int attribBits)
{
	VBO_t *pv;

	GLimp_LogComment("--- RB_UpdateVBOs ---\n");

	backEnd.pc.c_dynamicVboDraws++;

	pv = tess.vbo;
	/* update the default VBO */
	if(tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES){
		R_BindVBO(pv);

		if(attribBits & ATTR_BITS){
			if(attribBits & ATTR_POSITION){
				/* ri.Printf(PRINT_ALL, "offset %d, size %d\n", pv->ofs_xyz, tess.numVertexes * sizeof(tess.xyz[0])); */
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_xyz,
					tess.numVertexes * sizeof(tess.xyz[0]),
					tess.xyz);
			}

			if(attribBits & ATTR_TEXCOORD || attribBits & ATTR_LIGHTCOORD){
				/* these are interleaved, so we update both if either need it
				 * ri.Printf(PRINT_ALL, "offset %d, size %d\n", pv->ofs_st, tess.numVertexes * sizeof(tess.texCoords[0][0]) * 2); */
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_st,
					tess.numVertexes * sizeof(tess.texCoords[0][0]) * 2,
					tess.texCoords);
			}

			if(attribBits & ATTR_NORMAL){
				/* ri.Printf(PRINT_ALL, "offset %d, size %d\n", pv->ofs_normal, tess.numVertexes * sizeof(tess.normal[0])); */
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_normal,
					tess.numVertexes * sizeof(tess.normal[0]),
					tess.normal);
			}

			if(attribBits & ATTR_TANGENT){
				/* ri.Printf(PRINT_ALL, "offset %d, size %d\n", pv->ofs_tangent, tess.numVertexes * sizeof(tess.tangent[0])); */
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_tangent,
					tess.numVertexes * sizeof(tess.tangent[0]),
					tess.tangent);
			}

			if(attribBits & ATTR_BITANGENT){
				/* ri.Printf(PRINT_ALL, "offset %d, size %d\n", pv->ofs_bitangent, tess.numVertexes * sizeof(tess.bitangent[0])); */
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_bitangent,
					tess.numVertexes * sizeof(tess.bitangent[0]),
					tess.bitangent);
			}

			if(attribBits & ATTR_COLOR){
				/* ri.Printf(PRINT_ALL, "offset %d, size %d\n", pv->ofs_vertexcolor, tess.numVertexes * sizeof(tess.vertexColors[0])); */
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_vertexcolor,
					tess.numVertexes * sizeof(tess.vertexColors[0]),
					tess.vertexColors);
			}

			if(attribBits & ATTR_LIGHTDIRECTION){
				/* ri.Printf(PRINT_ALL, "offset %d, size %d\n", pv->ofs_lightdir, tess.numVertexes * sizeof(tess.lightdir[0])); */
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_lightdir,
					tess.numVertexes * sizeof(tess.lightdir[0]),
					tess.lightdir);
			}
		}else{
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_xyz,
				tess.numVertexes * sizeof(tess.xyz[0]),
				tess.xyz);
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_st,
				tess.numVertexes * sizeof(tess.texCoords[0][0]) * 2,
				tess.texCoords);
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_normal,
				tess.numVertexes * sizeof(tess.normal[0]),
				tess.normal);
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_tangent,
				tess.numVertexes * sizeof(tess.tangent[0]),
				tess.tangent);
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_bitangent,
				tess.numVertexes * sizeof(tess.bitangent[0]),
				tess.bitangent);
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_vertexcolor,
				tess.numVertexes * sizeof(tess.vertexColors[0]),
				tess.vertexColors);
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, pv->ofs_lightdir,
				tess.numVertexes * sizeof(tess.lightdir[0]),
				tess.lightdir);
		}

	}

	/* update the default IBO */
	if(tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES){
		R_BindIBO(tess.ibo);

		qglBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, tess.numIndexes * sizeof(tess.indexes[0]),
			tess.indexes);
	}
}
