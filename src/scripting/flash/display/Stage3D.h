/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DISPLAY_STAGE3D_H
#define SCRIPTING_FLASH_DISPLAY_STAGE3D_H 1

#include "scripting/flash/events/flashevents.h"
#include "backends/rendering_context.h"

namespace lightspark
{

class Context3D;

class Stage3D: public EventDispatcher
{
friend class Stage;
protected:
	bool renderImpl(RenderContext &ctxt) const;
public:
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	Stage3D(ASWorker* wrk, Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(requestContext3D);
	ASFUNCTION_ATOM(requestContext3DMatchingProfiles);
	ASPROPERTY_GETTER_SETTER(number_t,x);
	ASPROPERTY_GETTER_SETTER(number_t,y);
	ASPROPERTY_GETTER_SETTER(bool,visible);
	ASPROPERTY_GETTER(_NR<Context3D>,context3D);
};



}

#endif /* SCRIPTING_FLASH_DISPLAY_STAGE3D_H */
