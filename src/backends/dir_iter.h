/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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
**************************************************************************/

#ifndef BACKENDS_DIR_ITER_H
#define BACKENDS_DIR_ITER_H 1

// Check if compiling for POSIX based systems.
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#ifdef _POSIX_VERSION
#include "backends/posix/dir_iter.h"
#endif
#endif

#endif /* BACKENDS_DIR_ITER_H */
