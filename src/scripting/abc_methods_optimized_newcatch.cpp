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

#include "scripting/abc.h"
#include "scripting/abc_optimized.h"
#include "scripting/abc_optimized_newcatch.h"
#include "scripting/class.h"
#include "scripting/toplevel/toplevel.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_newcatch_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	Catchscope_object* obj = new_catchscopeObject(context->worker,context->mi, t);
	asAtom ret=asAtomHandler::fromObject(obj);
	if (ret.uintval != CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos).uintval)
	{
		REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	}
	LOG_CALL("getNewCatch_l " << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos))<<" "<<t<<" "<<context->exec_pos->local3.pos);
	++(context->exec_pos);
}
