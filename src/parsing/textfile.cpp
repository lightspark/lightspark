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

#include "parsing/textfile.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include "compat.h"

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
				content = (char *) malloc(count + 1);
				count = fread(content, 1, count, fp);
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
	char *ret = 0;
	int max_path_len=0;
	const char *paths[] = {
		".",
		"..",
		"src",
		LS_DATADIR,
		"/usr/share/lightspark",
#ifdef _WIN32
		getExectuablePath(),
#endif
	};
	for(uint32_t i=0;i<sizeof(paths)/sizeof(const char*);i++)
		max_path_len=imax(strlen(paths[i]),max_path_len);
	max_path_len+=strlen(fn)+2;
	char* buf=new char[max_path_len];

	for (unsigned int i = 0; i < sizeof(paths)/sizeof(paths[0]); i++)
	{
		snprintf(buf, max_path_len, "%s/%s", paths[i], fn);
		ret = textFileRead(buf);
		if (ret)
		{
			delete[] buf;
			return ret;
		}
	}

	delete[] buf;
	return ret;
}


