#!/bin/sh
#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2009,2010,2011 Alessandro Pignotti (a.pignotti@sssup.it)
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
sh "$basedir"/youtube-args-dumper.sh "$1" > "$paramfile"

wget "$(sh "$basedir"/youtube-get-player.sh "$1")" -O "$playerfile"

$LIGHTSPARK "$playerfile" -p "$paramfile"
rm "$playerfile" "$paramfile"
