/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Credits:
	www.lighthouse3d.com
**************************************************************************/

#ifndef PARSING_TEXTFILE_H
#define PARSING_TEXTFILE_H 1

char *textFileRead(const char *fn);
char *dataFileRead(const char *fn);
int textFileWrite(const char *fn, char *s);

#endif /* PARSING_TEXTFILE_H */
