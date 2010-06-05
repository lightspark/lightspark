/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

char *textFileRead(const char *fn)
{
	FILE *fp;
	char *content = NULL;

	int count=0;

	if (fn != NULL)
	{
		fp = fopen(fn,"rt");

		if (fp != NULL)
		{
			fseek(fp, 0, SEEK_END);
			count = ftell(fp);
			rewind(fp);
			if (count > 0)
			{
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}
	return content;
}

int textFileWrite(const char *fn, char *s)
{
	FILE *fp;
	int status = 0;

	if (fn != NULL)
	{
		fp = fopen(fn,"w");
		if (fp != NULL)
		{
			if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
	}
	return(status);
}


char *dataFileRead(const char *fn)
{
#ifdef LIGHTSPARK_PKG_BUILD
	char buf[PATH_MAX] = "/usr/share/lightspark/";
	strcat(buf, fn);
	return textFileRead(buf);
#else
	return textFileRead(fn);
#endif
}


