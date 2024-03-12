#!/bin/sh
###########################################################################
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)
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
###########################################################################

make_excludes() {
	ret="(";
	while [ $# -gt 0 ]; do
		ret="$ret -path $1";
		if [ $# -gt 1 ]; then
			ret="$ret -o";
		fi
		shift;
	done
	echo "$ret )";
	return 0;
}

if [ "$1" = "-d" ]; then
	base_path="$2";
	shift 2;
fi

if [ -n "$base_path" ]; then
	path="$base_path/src/";
fi

exclude_list="$@"
forward_dir="forwards";
[ -z "$exclude_list" ] && read -d '' exclude_list << EOF
${path}$forward_dir
${path}interfaces
${path}3rdparty
${path}plugin
${path}plugin_ppapi
${path}allclasses.h
${path}compat.h
${path}errorconstants.h
${path}exceptions.h
${path}logger.h
${path}memory_support.h
${path}platforms/fastpaths.h
${path}smartrefs.h
EOF

excludes=$(make_excludes $exclude_list)
if [ -n "$base_path" ]; then
	find_path="$path"
else
	find_path="*"
	base_dir=$(dirname "$0")/..
	src_dir=$base_dir/src
	cd $src_dir
fi

find $find_path -type d -path "*/.*" -prune -o $excludes -prune -o -type f -name '*.h' -print;
