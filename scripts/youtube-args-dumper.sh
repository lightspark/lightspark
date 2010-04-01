#!/bin/sh

echo "Dumping from " $1 >&2

wget $1 -q -O tmp

#cat tmp | grep SWF_ARGS | sed "s/.*{\(.*\)}.*/\1/g" | sed "s/\", /\"\n/g" | grep -v rv.[0-9] | sed "s/\"\(.*\)\": \"\(.*\)\"/\1\n\2/g" | ./urldecode.sh

cat tmp | egrep -o "<embed(.*?)/>" | sed "s/ /\n/g" | grep flashvars | grep -o '\\\".*\\\"' | sed 's@\\\"\(.*\)\\\"@\1@g' | sed "s/&/\n/g" | sed "s/=/\n/g" | ./urldecode.sh
