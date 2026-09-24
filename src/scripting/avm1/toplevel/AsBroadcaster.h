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

#ifndef SCRIPTING_AVM1_TOPLEVEL_ASBROADCASTER_H
#define SCRIPTING_AVM1_TOPLEVEL_ASBROADCASTER_H 1

#include "gc/resource.h"
#include "scripting/avm1/function.h"

// Based on Ruffle's `avm1::globals::as_broadcaster` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Object;
class AVM1Value;
class GcContext;

struct AsBroadcasterFuncs : public GcResource
{
	GcPtr<AVM1Object> addListener;
	GcPtr<AVM1Object> removeListener;
	GcPtr<AVM1Object> broadcastMessage;

	AsBroadcasterFuncs
	(
		GcContext& ctx,
		const GcPtr<AVM1Object>& _addListener,
		const GcPtr<AVM1Object>& _removeListener,
		const GcPtr<AVM1Object>& _broadcastMessage
	) :
	GcResource(ctx),
	addListener(_addListener),
	removeListener(_removeListener),
	broadcastMessage(_broadcastMessage) {}


	void init
	(
		GcContext& ctx,
		const GcPtr<AVM1Object>& broadcaster,
		const GcPtr<AVM1Object>& arrayProto
	) const;
};

class AsBroadcaster
{
public:
	using CreateClassType = std::pair
	<
		GcPtr<AsBroadcasterFuncs>,
		GcPtr<AVM1SystemClass>
	>;

	static CreateClassType createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(initialize);
	AVM1_FUNCTION_DECL(addListener);
	AVM1_FUNCTION_DECL(removeListener);
	AVM1_FUNCTION_DECL(broadcastMessage);
	AVM1_FUNCTION_DECL(broadcastImpl, const tiny_string& methodName);
{
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_ASBROADCASTER_H */
