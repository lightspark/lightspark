/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_SELECTION_H
#define SCRIPTING_AVM1_TOPLEVEL_SELECTION_H 1

#include "display_object/TextField.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"

// Based on Ruffle's `avm1::globals::selection` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class GcContext;

class AVM1Selection : public AVM1Object, public TextSelection
{
public:
	AVM1Selection
	(
		AVM1DeclContext& ctx,
		const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
		const GcPtr<AVM1Object>& arrayProto
	);

	AVM1_FUNCTION_DECL(getBeginIndex);
	AVM1_FUNCTION_DECL(getEndIndex);
	AVM1_FUNCTION_DECL(getCaretIndex);
	AVM1_FUNCTION_DECL(getFocus);
	AVM1_FUNCTION_DECL(setFocus);
	AVM1_FUNCTION_DECL(setSelection);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_SELECTION_H */
