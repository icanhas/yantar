#!/bin/sh -e

# This is for platforms without all of the needed libs in their packages
# & ports systems (e.g.  OpenBSD, FreeBSD), or if we're cross-compiling.

if [ "X$procs" = "X" ]; then
	procs=4
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

rm -rf $prefix/lib/libvorbis* $prefix/lib/libogg* $prefix/lib/libfreetype* \
	$prefix/lib/libz.* $prefix/lib/libjpeg.* $prefix/lib/libcurl.* \
	$prefix/lib/libSDL* $prefix/include/SDL &&

mkdir -p /tmp/getlibs
(cd /tmp/getlibs

echo && echo mkzlib
curl -# http://zlib.net/zlib-1.2.7.tar.gz | gunzip | tar xf -
(cd zlib-1.2.7
./configure --prefix=$prefix --static
$make -s -j$procs $zcross \
	BINARY_PATH=$prefix/bin INCLUDE_PATH=$prefix/include \
	LIBRARY_PATH=$prefix/lib install) &&

echo && echo mklibogg
curl -# http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz | gunzip | tar xf -
(cd libogg-1.3.0
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -s -j$procs && $make -s install) &&

echo && echo mklibvorbis
curl -# http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz | gunzip | tar xf -
(cd libvorbis-1.3.3
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -s -j$procs
$make -s install) &&

echo && echo mklibfreetype
curl -# ftp://ftp.igh.cnrs.fr/pub/nongnu/freetype/freetype-2.4.10.tar.bz2 | bunzip2 | tar xf -
(cd freetype-2.4.10
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -j$procs
$make -s install &&
ln -sf /usr/$chain/include/freetype2/freetype /usr/$chain/include/freetype) &&

echo && echo mklibsdl
curl -# http://www.libsdl.org/release/SDL-1.2.15.tar.gz | gunzip | tar xf -
(cd SDL-1.2.15
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared --enable-render-d3d=no >/dev/null
$make -s -j$procs
$make -s install) &&

echo && echo mklibjpeg
curl -# http://ijg.org/files/jpegsrc.v8d.tar.gz | gunzip | tar xf -
(cd jpeg-8d
./configure --prefix=$prefix $cross --enable-static \
	--disable-shared >/dev/null
$make -s -j$procs
$make -s install) &&

echo && echo mklibcurl
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
