subdirs="asm botlib cgame client game q3_ui qcommon server renderergl2 renderer
sys sys/sdl cmd/asm cmd/lcc ui"
for d in $subdirs; do
	d="src/$d"
	if [ ! -d $d ]; then
		echo "$d is missing. I can't believe you've done this."
		exit 1
	fi

	filt="$d/*.c $d/*.h"
	for f in $filt; do
		uncrustify -q -c .uncrustify -f $f -o $f
	done
	echo -n .
done
echo

