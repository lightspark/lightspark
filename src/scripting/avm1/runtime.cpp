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

#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/scope.h"
#include "scripting/avm1/value.h"
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/display/MovieClip.h"
#include "swf.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::runtime::Avm1`.

AVM1Context::AVM1Context(SystemState* _sys, uint8_t _playerVersion) :
sys(_sys),
playerVersion(_playerVersion),
halted(false),
maxRecursionDepth(255),
hasMouseListener(false),
useSpecialInvalidBounds(false)
{
	AVM1Global* globals;
	std::tie(prototypes, globals, broadcasterFuncs) = createGlobals();

	globalScope = _MNR(new AVM1Scope(globals));
}

void AVM1Context::runStackFrameForAction
(
	MovieClip* activeClip,
	const tiny_string& name,
	const std::vector<uint8_t>& code
)
{
	using Identifier = AVM1Activation::Identifier;
	if (halted)
	{
		// We've been told to halt action execution.
		return;
	}

	AVM1Activation parentActivation
	(
		sys,
		Identifier("Actions Parent"),
		activeClip
	);

	AVM1Activation childActivation
	(
		parentActivation.getSys(),
		Identifier(parentActivation.getId(), name),
		activeClip->getSwfVersion(),
		_MNR(new AVM1Scope(parentActivation.getScope(), activeClip)),
		constPool,
		activeClip
	);

	try
	{
		childActivation.runActions(code);
	}
	catch (std::exception& e)
	{
		rootExceptionHandler(childActivation, e);
	}
}

void AVM1Context::runWithStackFrameForClip
(
	MovieClip* activeClip,
	std::function<void(AVM1Activation&)> func
)
{
	using Identifier = AVM1Activation::Identifier;

	AVM1Activation activation
	(
		sys,
		Identifier("Display Object"),
		activeClip->getSwfVersion(),
		_MNR(new AVM1Scope(globalScope, activeClip)),
		constPool,
		activeClip
	);
	func(activation);
}

void AVM1Context::runStackFrameForInitAction
(
	MovieClip* activeClip,
	const std::vector<uint8_t>& code
)
{
	using Identifier = AVM1Activation::Identifier;
	if (halted)
	{
		// We've been told to halt action execution.
		return;
	}

	AVM1Activation parentActivation
	(
		sys,
		Identifier("Init Parent"),
		activeClip
	);

	AVM1Activation childActivation
	(
		parentActivation.getSys(),
		Identifier(parentActivation.getId(), name),
		activeClip->getSwfVersion(),
		_MNR(new AVM1Scope(parentActivation.getScope(), activeClip)),
		constPool,
		activeClip
	);

	try
	{
		childActivation.runActions(code);
	}
	catch (std::exception& e)
	{
		rootExceptionHandler(childActivation, e);
	}
}

void AVM1Context::runStackFrameForMethod
(
	MovieClip* activeClip,
	AVM1Object* obj,
	const tiny_string& name,
	const std::vector<AVM1Value>& args
)
{
	using Identifier = AVM1Activation::Identifier;
	if (halted)
	{
		// We've been told to halt action execution.
		return;
	}

	AVM1Activation activation(sys, Identifier(name), activeClip);
	(void)obj->callMethod
	(
		name,
		args,
		activation,
		AVM1ExecutionReason::Special
	);
}

void AVM1Context::notifySystemListeners
(
	MovieClip* activeClip,
	const tiny_string& broadcasterName,
	const tiny_string& method,
	const std::vector<AVM1Value>& args
)
{
	using Identifier = AVM1Activation::Identifier;
	AVM1Activation activation
	(
		sys,
		Identifier("System Listeners"),
		activeClip
	);

	auto broadcaster = getGlobalObject()->getProp
	(
		sys,
		broadcasterName,
		activation
	)->as<AVM1Broadcaster>(activation);

	bool hasListener = broadcaster->tryBroadcast
	(
		sys,
		activation,
		args,
		method
	);

	if (broadcasterName == "Mouse")
		hasMouseListener = hasListener;
}

void AVM1Context::halt()
{
	if (halted)
		return;

	halted = true;
	LOG(LOG_ERROR, "Ending action execution for this movie.");
}

AVM1Value AVM1Context::pop()
{
	if (stack.empty())
	{
		LOG(LOG_ERROR, "AVM1Context::pop: Stack underflow.");
		return AVM1Value::undefinedValue;
	}

	AVM1Value value = stack.back();
	stack.pop_back();
	return value;
}

Optional<const AVM1Value&> AVM1Context::getReg(size_t idx) const
{
	if (idx >= registers.size())
		return {};
	return registers[idx];
}

Optional<AVM1Value&> AVM1Context::getReg(size_t idx)
{
	if (idx >= registers.size())
		return {};
	return registers[idx];
}

void AVM1Context::findDisplayObjectsPendingRemoval
(
	DisplayObject* obj,
	std::vector<_R<DisplayObject>>& vec
)
{
	auto parent = obj->as<DisplayObjectContainer>();
	if (parent == nullptr)
		return;

	std::vector<_R<DisplayObject>> dispList;
	parent->cloneDisplayList(dispList);

	for (auto& child : dispList)
	{
		if (child-isPendingAVM1Removal())
			vec.push_back(child);
		findDisplayObjectsPendingRemoval(child.getPtr(), vec);
	}
}

void AVM1Context::removePending()
{
	std::vector<_R<DisplayObject>> dispList;

	auto rootClip = sys->stage->getRoot();

	// Find all objects that need to be removed.
	if (!sys->stage->getRoot().isNull())
		findDisplayObjectsPendingRemoval(rootClip.getPtr(), dispList);

	for (auto& child : dispList)
	{
		// Get the object's parent.
		auto parent = child->getParent();
		// Remove it.
		parent->removeChildDirectly(sys, child.getPtr());
		// Update the pending removal state.
		parent->updatePendingRemovals();
	}
}

void AVM1Context::runFrame()
{
	// Remove any pending objects.
	removePending();

	// NOTE: AVM1 only executes the `Idle` phase, meaning all the work
	// that'd normally be done in phases is instead done all at once, in
	// the order that the SWF requests them.
	sys->setFramePhase(FramePhase::IDLE);

	// NOTE: AVM1's execution order is determined by the global execution
	// list, which is based on instantiation order.
	DisplayObject* prevClip = nullptr;
	for (auto clip = clipExecList; clip != nullptr; clip = clip->getNextAVM1Clip())
	{
		if (!clip->isAVM1Removed())
		{
			clip->AVM1runFrame(sys);
			prevClip = clip;
			continue;
		}

		auto nextClip = clip->getNextAVM1Clip();
		// Clean up any removed clips from this, or the previous frame.
		if (prevClip != nullptr)
			prevClip->setNextAVM1Clip(nextClip);
		else
			clipExecList = nextClip;
		clip->setNextAVM1Clip(nullptr);
	}

	// Fire any `onLoadInit` events, and remove any completed movie
	// loaders
	sys->loaderManager->clipOnLoad(sys->getActionQueue());

	sys->setFramePhase(FramePhase::IDLE);
}

void AVM1Context::addToExecList(MovieClip* clip)
{
	if (clip->getNextAVM1Clip() != nullptr)
		return;

	// NOTE: Adding while iterating is safe, as it doesn't modify any
	// active nodes.
	clip->setNextAVM1Clip(clipExecList.getPtr());
	clipExecList = clip;
}

_NR<AVM1Function> AVM1Context::getRegisteredCtor
(
	uint8_t swfVersion,
	const tiny_string& symbol
) const
{
	return getRegisteredCtor
	(
		swfVersion,
		sys->getUniqueStringId(symbol, swfVersion > 6)
	);
}

_NR<AVM1Function> AVM1Context::getRegisteredCtor
(
	uint8_t swfVersion,
	uint32_t symbol
) const
{
	const auto& ctorRegistry = getCtorRegistry(swfVersion);

	auto it = ctorRegistry.find(symbol);
	if (it == ctorRegistry.end())
		return nullptr;

	return it->second;
}

void AVM1Context::registerCtor
(
	uint8_t swfVersion,
	const tiny_string& symbol,
	const _NR<AVM1Function>& ctor
)
{
	return registeredCtor
	(
		swfVersion,
		sys->getUniqueStringId(symbol, swfVersion > 6),
		ctor
	);
}

void AVM1Context::registerCtor
(
	uint8_t swfVersion,
	uint32_t symbol,
	const _NR<AVM1Function>& ctor
)
{
	auto& ctorRegistry = getCtorRegistry(swfVersion);

	auto it = ctorRegistry.find(symbol);
	if (it == ctorRegistry.end())
		ctorRegistry.erase(it);

	if (!ctor.isNull())
		ctorRegistry.insert(std::make_pair(symbol, ctor));
}
