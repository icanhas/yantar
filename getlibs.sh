#!/bin/sh -e

# This is for platforms without all of the needed libs in their packages
# & ports systems (e.g.  OpenBSD, FreeBSD), or if we're cross-compiling.

if [ "X$procs" = "X" ]; then
	procs=3
fi
if [ "X$make" = "X" ]; then
	make="make"
fi
if [ "X$prefix" = "X" ]; then
	prefix="/usr"
fi
if [ ! "X$chain" = "X" ]; then
	# for configure scripts
	cross="--host=$chain --target=$chain"
	# for zlib configure
	zcross="-fwin32/Makefile.gcc PREFIX=$chain- RC=$chain-windres"
fi

rm -rf $prefix/lib/libvorbis.a $prefix/lib/libogg.a $prefix/lib/libfreetype.a \
	$prefix/lib/libz.a $prefix/lib/libjpeg.a $prefix/lib/libcurl.a \
	$prefix/lib/libSDL.a $prefix/include/SDL &&

mkdir -p /tmp/getlibs
(cd /tmp/getlibs

echo && echo mk libccco
curl -L -# http://downloads.sf.net/project/libccco/libccco-0.1.1.tar.bz2 | bunzip2 | tar xf -
(cd libccco-0.1.1
$make -s -j$procs install $libcccomkflags && $make tests $libcccomkflags)

echo && echo mk zlib
curl -# http://zlib.net/zlib-1.2.7.tar.gz | gunzip | tar xf -
(cd zlib-1.2.7
./configure --prefix=$prefix --static
$make -s -j$procs $zcross \
	BINARY_PATH=$prefix/bin INCLUDE_PATH=$prefix/include \
	LIBRARY_PATH=$prefix/lib install) &&

echo && echo mk libogg
curl -# http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz | gunzip | tar xf -
(cd libogg-1.3.0
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -s -j$procs && $make -s install) &&

echo && echo mk libvorbis
curl -# http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz | gunzip | tar xf -
(cd libvorbis-1.3.3
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -s -j$procs
$make -s install) &&

echo && echo mk libfreetype
curl -# ftp://ftp.igh.cnrs.fr/pub/nongnu/freetype/freetype-2.4.10.tar.bz2 | bunzip2 | tar xf -
(cd freetype-2.4.10
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -j$procs
$make -s install &&
ln -sf /usr/$chain/include/freetype2/freetype /usr/$chain/include/freetype) &&

echo && echo mk libsdl
curl -# http://www.libsdl.org/release/SDL-1.2.15.tar.gz | gunzip | tar xf -
(cd SDL-1.2.15
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared --enable-dlopen --enable-loadso \
	--enable-render-d3d=no --enable-sdl-dlopen \
	>/dev/null
$make -s -j$procs
$make -s install) &&

echo && echo mk libjpeg
curl -# http://ijg.org/files/jpegsrc.v8d.tar.gz | gunzip | tar xf -
(cd jpeg-8d
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -s -j$procs
$make -s install) &&

echo && echo mk libcurl
curl -# http://curl.haxx.se/download/curl-7.28.1.tar.bz2 | bunzip2 | tar xf -
(cd curl-7.28.1
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared --disable-ldap --disable-ldaps \
	--disable-pop3 --disable-telnet --disable-cookies \
	--disable-imap --disable-manual \
	--disable-smtp --without-ssl --without-libssh2 >/dev/null
$make -s -j$procs
$make -s install)

) && echo && echo done && echo

rm -rf /tmp/getlibs
