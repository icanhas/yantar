#!/bin/bash -e

# This script fetches, builds, and installs the prerequisite libraries
# to cross-compile yantar using mingw.  You need to have mingw, curl,
# and libtool installed, or this will fail. 
# Also, you need to run this with root privileges.

chain=
prefix=
procs=4
# amd64 chains are given precedence here
prefixes="i386-mingw32 i486-mingw32 i586-mingw32 i686-mingw32 \
	i586-mingw32msvc i686-w64-mingw32 amd64-mingw32 \
	x86_64-w64-mingw32 amd64-mingw32msvc mingw32"
	
if [ "X$make" = "X" ]; then
	make="make"
fi

function determine(){
	for try in $prefixes; do
		if [ ! $(which "${try}-gcc" 2>/dev/null) = "" ]; then
			chain=$try
		fi
		# known path on Debian
		if [ -d /usr/$try ]; then
			prefix=/usr/$try
		fi
		# OpenBSD
		if [ -d /usr/local/$try ]; then
			prefix=/usr/local/$try
		fi
	done
	
	if [ "X$chain" = "X" ]; then
		echo Do you even have mingw installed?
		exit 1
	fi
	if [ "X$prefix" = "X" ]; then
		echo Could not find mingw directory.
		exit 1
	fi
}

function mkzlib(){
	echo && echo mkzlib
	curl -# http://zlib.net/zlib-1.2.7.tar.gz | gunzip | tar xf -
	(cd zlib-1.2.7
	./configure --static
	$make --silent -j$procs -fwin32/Makefile.gcc PREFIX=$chain- RC=$chain-windres \
		BINARY_PATH=$prefix/bin INCLUDE_PATH=$prefix/include \
		LIBRARY_PATH=$prefix/lib install)
}

function mklibogg(){
	echo && echo mklibogg
	curl -# http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz | gunzip | tar xf -
	(cd libogg-1.3.0
	./configure --prefix=$prefix --host=$chain --target=$chain --enable-static \
		--disable-shared >/dev/null
	$make --silent -j$procs && $make --silent install)
}

function mklibvorbis(){
	echo && echo mklibvorbis
	curl -# http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz | gunzip | tar xf -
	(cd libvorbis-1.3.3
	./configure --prefix=$prefix --host=$chain --target=$chain --enable-static \
		--disable-shared >/dev/null
	$make --silent -j$procs
	$make --silent install)
}

function mklibfreetype(){
	echo && echo mklibfreetype
	curl -# ftp://ftp.igh.cnrs.fr/pub/nongnu/freetype/freetype-2.4.10.tar.bz2 | bunzip2 | tar xf -
	(cd freetype-2.4.10
	./configure --prefix=$prefix --host=$chain --target=$chain --enable-static \
		--disable-shared >/dev/null
	$make -j$procs
	$make --silent install &&
	ln -sf /usr/$chain/include/freetype2/freetype /usr/$chain/include/freetype)
}

function mklibsdl(){
	echo && echo mklibsdl
	curl -# http://www.libsdl.org/release/SDL-1.2.15.tar.gz | gunzip | tar xf -
	(cd SDL-1.2.15
	./configure --prefix=$prefix --host=$chain --target=$chain --enable-static \
		--disable-shared --enable-render-d3d=no >/dev/null
	$make --silent -j$procs
	$make --silent install)
}

function mklibjpeg(){
	echo && echo mklibjpeg
	curl -# http://ijg.org/files/jpegsrc.v8d.tar.gz | gunzip | tar xf -
	(cd jpeg-8d
	./configure --prefix=$prefix --host=$chain --target=$chain --enable-static \
		--disable-shared >/dev/null
	$make --silent -j$procs
	$make --silent install)
}

function mklibcurl(){
	echo && echo mklibcurl
	curl -# http://curl.haxx.se/download/curl-7.28.1.tar.bz2 | bunzip2 | tar xf -
	(cd curl-7.28.1
	./configure --prefix=$prefix --host=$chain --target=$chain --enable-static \
		--disable-shared --enable-ldap=no --enable-ldaps=no >/dev/null
	$make --silent -j$procs
	$make --silent install)
}

mkdir getlibs

(cd getlibs
determine &&
rm -rf $prefix/lib/libvorbis* $prefix/lib/libogg* $prefix/lib/libfreetype* \
	$prefix/lib/libz.* $prefix/lib/libjpeg.* $prefix/lib/libcurl.* \
	$prefix/lib/libSDL* $prefix/include/SDL &&
mkzlib &&
mklibogg &&
mklibvorbis &&
mklibfreetype &&
mklibjpeg &&
mklibsdl &&
mklibcurl
) && echo && echo done && echo

rm -rf getlibs
