#!/bin/bash
echo -e "$(sed 'y/+/ /; s/%/\\x/g')"
