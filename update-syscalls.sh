awk 'BEGIN{x=-1};
	/^equ/ { 
		printf("%s\t%-37s %d\n", $1, $2, x--) 
	}
	!/^equ/ { 
		print 
	}
' "$1" >"$1.tmp"
mv "$1.tmp" "$1"