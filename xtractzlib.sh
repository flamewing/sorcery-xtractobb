#!/bin/bash

find "$1" -iname '*.inf' | while read ff; do
	baseff="${ff%.inf}"
	if [[ ! -f "$baseff.cmp" ]]; then
		mv "$baseff" "$baseff.cmp"
	fi
	#printf "\x1f\x8b\x08\x00\x00\x00\x00\x00" | cat - "$baseff.cmp" | gzip -d > "$baseff"
	zlib-flate -uncompress < "$baseff.cmp" > "$baseff"
done

