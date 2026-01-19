/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef SCRIPTING_ABC_OPTIMIZED_H
#define SCRIPTING_ABC_OPTIMIZED_H 1

#include "scripting/abcutils.h"
#include "scripting/abctypes.h"

#define REPLACELOCALRESULT(context,pos,ret) { \
	if (USUALLY_FALSE(context->exceptionthrown)) \
	{ \
		ASATOM_DECREF(ret); \
		return; \
	} \
	asAtom oldres = CONTEXT_GETLOCAL(context,pos);\
	asAtomHandler::set(CONTEXT_GETLOCAL(context,pos),ret);\
	ASATOM_DECREF(oldres);\
}

#endif /* SCRIPTING_ABC_OPTIMIZED_H */
