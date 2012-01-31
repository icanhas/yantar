subdirs="asm botlib cgame client game null q3_ui qcommon server sys tools/asm
ui"
for d in $subdirs; do
	d="src/$d"
	if [ ! -d $d ]; then
		echo "$d is missing. I can't believe you've done this."
		exit 1
	fi

	filt="$d/*.c $d/*.h"
	echo -n .
	for f in $filt; do
		uncrustify -q -c .uncrustify -f $f -o $f
	done
done
echo

