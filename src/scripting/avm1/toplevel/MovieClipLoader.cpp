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

#include "display_object/DisplayObject.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/AsBroadcaster.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::movie_clip_loader` crate.

using AVM1MovieClipLoader;

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

static constexpr auto protoDecls =
{
	AVM1Decl("loadClip", loadClip, protoFlags),
	AVM1Decl("unloadClip", unloadClip, protoFlags),
	AVM1Decl("getProgress", getProgress, protoFlags)
};

AVM1MovieClipLoader::AVM1MovieClipLoader(AVM1Activation& act) : AVM1Object
(
	act.getGcCtx(),
	act.getPrototypes().object->proto
)
{
}

GcPtr<AVM1SystemClass> AVM1MovieClipLoader::createClass
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

AVM1_FUNCTION_BODY(AVM1MovieClipLoader, ctor)
{
	_this->defineValue
	(
		"_listeners",
		NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, { _this })),
		AVM1PropFlags::DontEnum
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1MovieClipLoader, loadClip)
{
	if (args.size() < 2)
		return AVM1Value::undefinedVal;

	if (!args[0].is<tiny_string>())
		return false;

	tiny_string url;
	AVM1Value targetVal;
	AVM1_ARG_UNPACK(url)(targetVal);
	auto target = targetVal.visit(makeVisitor
	(
		#ifdef USE_STRING_ID
		[&](uint32_t)
		#else
		[&](const tiny_string&)
		#endif
		{
			return act.resolveTargetClip
			(
				act.getTargetOrRootClip(),
				targetVal,
				true
			);
		},
		[&](number_t levelID)
		{
			// NOTE: Levels are rounded down.
			// TODO: What happens with negative levels?
			return act.getOrCreateLevel(levelID).asNullable();
		},
		[&](const GcPtr<AVM1Object>& obj)
		{
			return obj->as<AVM1DisplayObject>();
		},
		[&](const GcPtr<AVM1MovieClipRef>&)
		{
			return targetVal.toObject(act)->as<AVM1DisplayObject>();
		},
		[&](const auto&) -> NullableGcPtr<AVM1DisplayObject>
		{
			return NullGc;
		}
	));

	if (target.isNull())
		return false;

	act.getSys()->loaderManager->loadMovieIntoClip
	(
		target,
		Request(RequestMethod::Get, url),
		{},
		AVM1MovieLoaderData(_this)
	);
	return true;
}

AVM1_FUNCTION_BODY(AVM1MovieClipLoader, unloadClip)
{
	if (args.size() < 1)
		return AVM1Value::undefinedVal;

	const auto& targetVal = args[0];
	auto target = targetVal.visit(makeVisitor
	(
		#ifdef USE_STRING_ID
		[&](uint32_t)
		#else
		[&](const tiny_string&)
		#endif
		{
			return act.resolveTargetClip
			(
				act.getTargetOrRootClip(),
				targetVal,
				true
			);
		},
		[&](number_t levelID)
		{
			// NOTE: Levels are rounded down.
			// TODO: What happens with negative levels?
			return act.getLevel(levelID);
		},
		[&](const GcPtr<AVM1Object>& obj)
		{
			return obj->as<AVM1DisplayObject>();
		},
		[&](const GcPtr<AVM1MovieClipRef>&)
		{
			return targetVal.toObject(act)->as<AVM1DisplayObject>();
		},
		[&](const auto&) -> NullableGcPtr<AVM1DisplayObject>
		{
			return NullGc;
		}
	));

	if (target.isNull())
		return false;

	// TODO: Figure out what the correct behaviour is. If `target` isn't
	// a `MovieClip`, does Flash Player also wait an extra frame before
	// calling `AVM1unload()`? Is `AVM1unloadMovie()` the correct function
	// to call?
	if (target->is<MovieClip>())
		target->as<MovieClip>()->AVM1unloadMovie(act.getCtx());
	else
		target->AVM1unload(act.getCtx());
	return true;
}

AVM1_FUNCTION_BODY(AVM1MovieClipLoader, getProgress)
{
	if (args.size() < 1)
		return AVM1Value::undefinedVal;

	const auto& targetVal = args[0];
	bool isValid = true;
	auto target = targetVal.visit(makeVisitor
	(
		#ifdef USE_STRING_ID
		[&](uint32_t)
		#else
		[&](const tiny_string&)
		#endif
		{
			return act.resolveTargetClip
			(
				act.getTargetOrRootClip(),
				targetVal,
				true
			);
		},
		[&](number_t levelID)
		{
			// NOTE: Levels are rounded down.
			// TODO: What happens with negative levels?
			return act.getLevel(levelID);
		},
		[&](const GcPtr<AVM1Object>& obj)
		{
			return obj->as<AVM1DisplayObject>();
		},
		[&](const GcPtr<AVM1MovieClipRef>&)
		{
			return targetVal.toObject(act)->as<AVM1DisplayObject>();
		},
		[&](const auto&) -> NullableGcPtr<AVM1DisplayObject>
		{
			isValid = false;
			return NullGc;
		}
	));

	if (!isValid)
		return AVM1Value::undefinedVal;

	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1Object(act.getGcCtx()));

	if (target.isNull())
		return ret;

	auto pair = target->tryAs<MovieClip>().andThen([&](const auto& clip)
	{
		return makeOptional(std::make_pair
		(
			clip.getBytesLoaded(),
			clip.getBytesTotal()
		));
	// TODO: Figure out what `bytes{Loaded,Total}` are, if `target` isn't
	// a `MovieClip`. For now, just set both to the size of the loaded
	// file.
	}).valueOr(std::make_pair
	(
		target->getMovie()->getSize(),
		target->getMovie()->getSize()
	));

	ret.defineValue("bytesLoaded", number_t(pair.first), AVM1PropFlags(0));
	ret.defineValue("bytesTotal", number_t(pair.second), AVM1PropFlags(0));
	return ret;
}
