#!/bin/sh

echo "Dumping from " $1 >&2

wget $1 -q -O - | grep SWF_ARGS | sed "s/.*{\(.*\)}.*/\1/g" | sed "s/\", /\"\n/g" | grep -v rv.[0-9] | sed "s/\"\(.*\)\": \"\(.*\)\"/\1\n\2/g" | ./urldecode.sh
