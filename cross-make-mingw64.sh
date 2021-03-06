#!/bin/sh

CMD_PREFIX="amd64-mingw32 x86_64-w64-mingw32 amd64-mingw32msvc";

if [ "X$CC" = "X" ]; then
    for check in $CMD_PREFIX; do
        full_check="${check}-gcc"
	if [ ! $(which "$full_check" 2>/dev/null) = "" ]; then
	    export CC="$full_check"
	fi
    done
fi

if [ "X$WINDRES" = "X" ]; then
    for check in $CMD_PREFIX; do
        full_check="${check}-windres"
	if [ ! $(which "$full_check" 2>/dev/null) = "" ]; then
	    export WINDRES="$full_check"
	fi
    done
fi

if [ "X$WINDRES" = "X" -o "X$CC" = "X" ]; then
    echo "Error: Must define or find WINDRES and CC"
    exit 1
fi

export PLATFORM=mingw32
export ARCH=amd64

echo make $*
exec make $*
