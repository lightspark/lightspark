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

#include <initializer_list>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/AsBroadcaster.h"
#include "scripting/avm1/toplevel/FileReferenceList.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::file_reference_list` crate.

using AVM1FileRefList = AVM1FileReferenceList;
using AVM1FileReferenceList;

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1FileRefList, fileList, FileList, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1FileRefList, browse, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1FileRefList::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto,
	const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
	const GcPtr<AVM1Object>& arrayProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	bcastFuncs->init(ctx.getGcCtx(), _class->proto, arrayProto);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1FileRefList, ctor)
{
	_this.constCast() = NEW_GC_PTR
	(
		act.getGcCtx(),
		AVM1FileRefList(act)
	);
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1FileRefList, AVM1FileRefList, FileList)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `FileReferenceList.fileList`'s getter is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_SETTER_TYPE_BODY(AVM1FileRefList, AVM1FileRefList, FileList)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `FileReferenceList.fileList`'s setter is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1FileRefList, AVM1FileRefList, browse)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `FileReferenceList.browse()` is a stub."
	);
	return AVM1Value::undefinedVal;
}
