#!/bin/bash
# For all files in this directory (except this one),
# convert LF to CRLF (if not already).

SELF=convert_lf_to_crlf.sh

for file in *; do
    if [[ $file == $SELF ]]; then
        continue
    fi
    cat $file | tr -d '\r' | perl -pe 's|\n|\r\n|' > $file.bak
    cat $file.bak > $file
    rm $file.bak -f
done
