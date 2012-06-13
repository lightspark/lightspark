#!/bin/sh
#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2009-2012 Alessandro Pignotti (a.pignotti@sssup.it)
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


PATH=$(pwd)/$(dirname $0)/../$(uname -m)/$(sed -n 's/CMAKE_BUILD_TYPE:STRING=\(.*\)/\1/ p' CMakeCache.txt)/bin:$PATH

if test "x$LIGHTSPARK" = "x"; then
    LIGHTSPARK="lightspark"
fi

if test "$1" = "-g"; then
    LIGHTSPARK="gdb --args $LIGHTSPARK"
    shift 1
fi

paramfile="$(mktemp)"
playerfile="$(mktemp)"

basedir="$(dirname "$0")"

if test $# -eq 0; then
	URL="http://www.youtube.com/watch?v=YE7VzlLtp-4"
else
	URL=$1
fi

sh "$basedir"/youtube-args-dumper.sh "$URL" > "$paramfile"
wget "$(sh "$basedir"/youtube-get-player.sh "$URL")" -O "$playerfile"

$LIGHTSPARK "$playerfile" -p "$paramfile" --url "$URL"
rm "$playerfile" "$paramfile"
