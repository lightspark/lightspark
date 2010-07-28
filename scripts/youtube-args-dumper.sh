#!/bin/sh
#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#**************************************************************************


echo "Dumping from " $1 >&2

wget $1 -q -O - | egrep -o "<embed(.*?)/>" | sed "s/ /\n/g" | grep flashvars | grep -o '\\\".*\\\"' | sed 's@\\\"\(.*\)\\\"@\1@g' | sed "s/&/\n/g" | sed "s/=/\n/g" | `dirname "$0"`/urldecode.sh
