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
#include "scripting/avm1/toplevel/FileReference.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::file_reference` crate.

using AVM1FileRef = AVM1FileReference;
using AVM1FileReference;

constexpr auto protoFlags = AVM1PropFlags::DontEnum;

static constexpr auto protoDecls =
{
	AVM1_GETTER_TYPE_PROTO(AVM1FileRef, creationDate, CreationDate, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1FileRef, creator, Creator, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1FileRef, modificationDate, ModificationDate, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1FileRef, name, Name, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1FileRef, postData, PostData, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1FileRef, size, Size, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1FileRef, type, Type, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1FileRef, browse, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1FileRef, cancel, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1FileRef, download, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1FileRef, upload, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1FileRef::createClass
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
	ctx.definePropsOn(_class->ctor, {});
	return _class;
}

AVM1_FUNCTION_BODY(AVM1FileRef, ctor)
{
	_this.constCast() = NEW_GC_PTR(act.getGcCtx(), AVM1FileRef(act));
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, CreationDate)
{
	const auto& info = _this->info;
	if (!info.hasValue() || info->creationDate.isNull())
		return AVM1Value::undefinedVal;
	return info->creationDate;
}

AVM1_GETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, Creator)
{
	return _this->info.andThen([&](const auto& info)
	{
		return info.creator;
	}).transformOr(AVM1Value::undefinedVal);
}

AVM1_GETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, ModificationDate)
{
	if (!_this->info.hasValue() || _this->info->modifyDate.isNull())
		return AVM1Value::undefinedVal;
	return info->modifyDate;
}

AVM1_GETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, Name)
{
	return _this->info.andThen([&](const auto& info)
	{
		return info.name;
	}).transformOr(AVM1Value::undefinedVal);
}

AVM1_GETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, PostData)
{
	return _this->postData.valueOr("");
}

AVM1_SETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, PostData)
{
	_this->postData = value.toString(act);
}

AVM1_GETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, Size)
{
	return _this->info.andThen([&](const auto& info)
	{
		return info.size;
	}).transformOr(AVM1Value::undefinedVal, [&](size_t size)
	{
		return number_t(size);
	});
}

AVM1_GETTER_TYPE_BODY(AVM1FileRef, AVM1FileRef, Type)
{
	return _this->info.andThen([&](const auto& info)
	{
		return info.fileType;
	}).transformOr(AVM1Value::undefinedVal);
}

AVM1_FUNCTION_TYPE_BODY(AVM1FileRef, AVM1FileRef, browse)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `FileReference.browse()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1FileRef, AVM1FileRef, cancel)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `FileReference.cancel()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1FileRef, AVM1FileRef, download)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `FileReference.download()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1FileRef, AVM1FileRef, upload)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `FileReference.upload()` is a stub."
	);
	return AVM1Value::undefinedVal;
}
