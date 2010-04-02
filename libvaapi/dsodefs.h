//   Copyright (C) 2006, 2007, 2008, 2009, 2010 Free Software
//   Foundation, Inc
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef DSODEFS_H
#define DSODEFS_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32)
	// #ifdef BUILDING_DLL
	#ifdef DLL_EXPORT
		#define DSOEXPORT __declspec(dllexport)
	#else
		// Temporarily commented because of VC++ compiler problems 
		#define DSOEXPORT // __declspec(dllimport)
	#endif

	#define DSOLOCAL
#elif defined(__OS2__)
	#ifdef BUILDING_DLL
		#define DSOEXPORT __declspec(dllexport)
	#else
		// Temporarily commented because of VC++ compiler problems 
		#define DSOEXPORT // __declspec(dllimport)
	#endif

	#define DSOLOCAL

#else
	#ifdef HAVE_GNUC_VISIBILITY
		#define DSOEXPORT __attribute__ ((visibility("default")))
		#define DSOLOCAL __attribute__ ((visibility("hidden")))
	#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550) /* Sun Studio >= 8 */
		#define DSOEXPORT __global
		#define DSOLOCAL __hidden
	#else
		#define DSOEXPORT
		#define DSOLOCAL
	#endif
#endif

#endif /* DSODEFS_H */
