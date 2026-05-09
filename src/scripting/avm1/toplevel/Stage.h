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

#ifndef SCRIPTING_AVM1_TOPLEVEL_STAGE_H
#define SCRIPTING_AVM1_TOPLEVEL_STAGE_H 1

#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::stage` crate.

namespace lightspark
{

class AVM1DeclContext;

class AVM1Stage : public AVM1Object
{
public:
	AVM1Stage
	(
		AVM1DeclContext& ctx,
		const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
		const GcPtr<AVM1Object>& arrayProto
	);

	AVM1_FUNCTION_DECL(getAlign);
	AVM1_FUNCTION_DECL(setAlign);
	AVM1_FUNCTION_DECL(getHeight);
	AVM1_FUNCTION_DECL(getScaleMode);
	AVM1_FUNCTION_DECL(setScaleMode);
	AVM1_FUNCTION_DECL(getDisplayState);
	AVM1_FUNCTION_DECL(setDisplayState);
	AVM1_FUNCTION_DECL(getShowMenu);
	AVM1_FUNCTION_DECL(setShowMenu);
	AVM1_FUNCTION_DECL(getWidth);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_STAGE_H */
