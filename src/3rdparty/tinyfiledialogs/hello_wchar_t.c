/* SPDX-License-Identifier: Zlib
Copyright (c) 2014 - 2024 Guillaume Vareille http://ysengrin.com
	 ________________________________________________________________
	|                                                                |
	| 100% compatible C C++  ->  You can rename this .c file as .cpp |
	|________________________________________________________________|

********* TINY FILE DIALOGS OFFICIAL WEBSITE IS ON SOURCEFORGE *********
  _________
 /         \ hello_wchar_t.c v3.18.2 [Jun 10, 2024]
 |tiny file| Hello WCHAR_T windows only file created [November 9, 2014]
 | dialogs |
 \____  ___/ http://tinyfiledialogs.sourceforge.net
	  \|     git clone http://git.code.sf.net/p/tinyfiledialogs/code tinyfd
			  ____________________________________________
			 |                                            |
			 |   email: tinyfiledialogs at ysengrin.com   |
			 |____________________________________________|
	  ________________________________________________________________
	 |                                                                |
	 | this file is for windows only it uses wchar_t UTF-16 functions |
	 |________________________________________________________________|

If you like tinyfiledialogs, please upvote my stackoverflow answer
https://stackoverflow.com/a/47651444

- License -
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.  If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

  See compilation instructions at the end of this file

     __________________________________________
    |  ______________________________________  |
    | |                                      | |
    | | DO NOT USE USER INPUT IN THE DIALOGS | |
    | |______________________________________| |
    |__________________________________________|
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tinyfiledialogs.h"

#ifdef _MSC_VER
#pragma warning(disable:4996) /* silences warning about wcscpy*/
#endif

int main(void) /* WINDOWS ONLY */
{
	wchar_t * lPassword;
	wchar_t * lTheSaveFileName;
	wchar_t * lTheOpenFileName;
	wchar_t * lTheSelectFolderName;
	wchar_t * lTheHexColor;
	wchar_t * lWillBeGraphicMode;
	unsigned char lRgbColor[3];
	FILE * lIn;
	wchar_t lWcharBuff[1024];
	wchar_t lBuffer[1024];
	wchar_t const * lFilterPatterns[2] = { L"*.txt", L"*.text" };

	tinyfd_beep();

	lWillBeGraphicMode = tinyfd_inputBoxW(L"tinyfd_query", NULL, NULL);

	wcscpy(lBuffer, L"v");
	mbstowcs(lWcharBuff, tinyfd_version, strlen(tinyfd_version) + 1);
	wcscat(lBuffer, lWcharBuff);
	if (lWillBeGraphicMode)
	{
		wcscat(lBuffer, L"\ngraphic mode: ");
	}
	else
	{
		wcscat(lBuffer, L"\nconsole mode: ");
	}
	mbstowcs(lWcharBuff, tinyfd_response, strlen(tinyfd_response)+1);
	wcscat(lBuffer, lWcharBuff);
	wcscat(lBuffer, L"\n");
	mbstowcs(lWcharBuff, tinyfd_needs + 78, strlen(tinyfd_needs + 78) + 1);
	wcscat(lBuffer, lWcharBuff);

	tinyfd_messageBoxW(L"hello", lBuffer, L"ok", L"info", 0);

	tinyfd_notifyPopupW(L"the title", L"the message\n\tfrom outer-space", L"info");

	lPassword = tinyfd_inputBoxW(
		L"a password box", L"your password will be revealed later", NULL);

	if (!lPassword) return 1;

	lTheSaveFileName = tinyfd_saveFileDialogW(
		L"let us save this password",
		L"passwordFile.txt",
		2,
		lFilterPatterns,
		NULL);

	if (! lTheSaveFileName)
	{
		tinyfd_messageBoxW(
			L"Error",
			L"Save file name is NULL",
			L"ok",
			L"error",
			1);
		return 1 ;
	}

	lIn = _wfopen(lTheSaveFileName, L"wt, ccs=UNICODE");
	if (!lIn)
	{
		tinyfd_messageBoxW(
			L"Error",
			L"Can not open this file in write mode",
			L"ok",
			L"error",
			1);
		return 1 ;
	}
	fputws(lPassword, lIn);
	fclose(lIn);

	lTheOpenFileName = tinyfd_openFileDialogW(
		L"let us read the password back",
		L"",
		2,
		lFilterPatterns,
		NULL,
		0);

	if (! lTheOpenFileName)
	{
		tinyfd_messageBoxW(
			L"Error",
			L"Open file name is NULL",
			L"ok",
			L"error",
			1);
		return 1 ;
	}

	lIn = _wfopen(lTheOpenFileName, L"rt, ccs=UNICODE");

	if (!lIn)
	{
		tinyfd_messageBoxW(
			L"Error",
			L"Can not open this file in read mode",
			L"ok",
			L"error",
			1);
		return(1);
	}
	lBuffer[0] = '\0';
	fgetws(lBuffer, sizeof(lBuffer), lIn);
	fclose(lIn);

	tinyfd_messageBoxW(L"your password is",
			lBuffer, L"ok", L"info", 1);

	lTheSelectFolderName = tinyfd_selectFolderDialogW(
		L"let us just select a directory", L"C:\\");

	if (!lTheSelectFolderName)
	{
		tinyfd_messageBoxW(
			L"Error",
			L"Select folder name is NULL",
			L"ok",
			L"error",
			1);
		return 1;
	}

	tinyfd_messageBoxW(L"The selected folder is",
		lTheSelectFolderName, L"ok", L"info", 1);

	lTheHexColor = tinyfd_colorChooserW(
		L"choose a nice color",
		L"#FF0077",
		lRgbColor,
		lRgbColor);

	if (!lTheHexColor)
	{
		tinyfd_messageBoxW(
			L"Error",
			L"hexcolor is NULL",
			L"ok",
			L"error",
			1);
		return 1;
	}

	tinyfd_messageBoxW(L"The selected hexcolor is",
		lTheHexColor, L"ok", L"info", 1);

	tinyfd_messageBoxW(L"your password was", lPassword, L"ok", L"info", 1);

	return 0;
}

#ifdef _MSC_VER
#pragma warning(default:4996)
#endif


/*
MinGW needs gcc >= v4.9 otherwise some headers are incomplete
> gcc -o hello.exe hello.c tinyfiledialogs.c -LC:/mingw/lib -lcomdlg32 -lole32

TinyCC needs >= v0.9.27 (+ tweaks - contact me) otherwise some headers are missing
> tcc -o hello.exe hello.c tinyfiledialogs.c ^
	-isystem C:\tcc\winapi-full-for-0.9.27\include\winapi ^
	-lcomdlg32 -lole32 -luser32 -lshell32

Borland C: > bcc32c -o hello.exe hello.c tinyfiledialogs.c
OpenWatcom v2: create a character-mode executable project.

VisualStudio :
	Create a console application project,
	it links against comdlg32.lib & ole32.lib.

VisualStudio command line :
	> cl hello.c tinyfiledialogs.c comdlg32.lib ole32.lib user32.lib shell32.lib /W4
*/
