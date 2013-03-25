#!/bin/sh -e

# You need to have mingw, curl, and libtool installed, or this will fail.

# amd64 chains are given precedence here
prefixes="i386-mingw32 i486-mingw32 i586-mingw32 i686-mingw32 \
	i586-mingw32msvc i686-w64-mingw32 mingw32"

for try in $prefixes; do
	if [ ! $(which "${try}-gcc" 2>/dev/null) = "" ]; then
		chain=$try
		CC=$try-gcc
		AS=$try-as
		LD=$try-ld
		AR=$try-ar
		RANLIB=$try-ranlib
	fi
	# known path on Debian
	if [ -d /usr/$try ]; then
		prefix=/usr/$try
	fi
	# OpenBSD, FreeBSD
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

libcccomkflags="OS=windows PREFIX=$prefix"

export chain
export prefix
export libcccomkflags
export CC
export AS
export LD
export AR
export RANLIB
./getlibs.sh
