/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2023-2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <algorithm>
#include <array>
#include <list>

#include "display_object/DisplayObject.h"
#include "display_object/DisplayObjectContainer.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "gc/ptr.h"
#include "tiny_string.h"
#include "utils/optional.h"
#include "utils/span.h"

using namespace lightspark;

DisplayObjectContainer::DisplayObjectContainer(ASWorker* wrk, Class_base* c):InteractiveObject(wrk,c)
	,mouseChildren(true)
	,boundsrectXmin(0)
	,boundsrectYmin(0)
	,boundsrectXmax(0)
	,boundsrectYmax(0)
	,boundsRectDirty(true)
	,boundsrectVisibleXmin(0)
	,boundsrectVisibleYmin(0)
	,boundsrectVisibleXmax(0)
	,boundsrectVisibleYmax(0)
	,boundsRectVisibleDirty(true)
	,initializingFrame(false)
	,tabChildren(true)
	,isInaccessibleParent(false)
{
	subtype=SUBTYPE_DISPLAYOBJECTCONTAINER;
}

void DisplayObjectContainer::markAsChanged()
{
	for (auto& child : dynamicDisplayList)
		child.markAsChanged();
}

void DisplayObjectContainer::markBoundsRectDirtyChildren()
{
	markBoundsRectDirty();
	for (auto& child : dynamicDisplayList)
	{
		auto container = child.as<DisplayObjectContainer>();
		if (container != nullptr)
			container->markBoundsRectDirtyChildren();
	}
}

bool DisplayObjectContainer::hasChildAt(int32_t depth) const
{
	auto it = mapDepthToChild.find(depth);
	return it != mapDepthToChild.end();
}

const DisplayObject& DisplayObjectContainer::getChildAt(int32_t depth) const
{
	return mapDepthToChild.at(depth).get();
}

DisplayObject& DisplayObjectContainer::getChildAt(int32_t depth)
{
	return mapDepthToChild.at(depth).get();
}

DisplayObject* DisplayObjectContainer::tryGetChildAt(int32_t depth)
{
	auto it = mapDepthToChild.find(depth);
	return it != mapDepthToChild.end() ? &it->get() : nullptr;
}

bool DisplayObjectContainer::hasChildByName
(
	const tiny_string& name,
	bool caseSensitive
) const
{
	for (const auto& pair : mapDepthToChild)
	{
		const auto& child = pair.second.get();
		const auto& childName = child.getName();
		if (childName.empty())
			continue;
		if (name.equalsWithCase(childName, caseSensitive))
			return true;
	}
	return false;
}

DisplayObject* DisplayObjectContainer::getChildByName
(
	const tiny_string& name,
	bool caseSensitive
) const
{
	for (const auto& pair : mapDepthToChild)
	{
		const auto& child = pair.second.get();
		const auto& childName = child.getName();
		if (childName.empty())
			continue;
		if (name.equalsWithCase(childName, caseSensitive))
			return &child;
	}
	return nullptr;
}


void DisplayObjectContainer::setupClipActionsAt
(
	int32_t depth,
	const CLIPACTIONS& actions
)
{
	auto child = tryGetChildAt(depth);
	if (child == nullptr)
	{
		LOG
		(
			LOG_ERROR,
			"setupClipActionsAt: "
			"no child at depth " << depth
		);
		return;
	}

	auto childClip = child->as<MovieClip>();
	if (childClip != nullptr)
		childClip->setupActions(actions);
}

void DisplayObjectContainer::checkRatioForChildAt
(
	int32_t depth,
	uint32_t ratio,
	bool inskipping
)
{
	auto child = tryGetChildAt(depth);
	if (child == nullptr)
	{
		LOG
		(
			LOG_ERROR,
			"checkRatioForLegacyChildAt: "
			"no child at that depth " << depth << ' ' <<
			toDebugString()
		);
		return;
	}
	child->checkRatio(ratio, inskipping);
	hasChanged = true;
}

void DisplayObjectContainer::checkColorTransformForChildAt
(
	int32_t depth,
	const CXFORMWITHALPHA& ct
)
{
	auto child = tryGetChildAt(depth);
	if (child == nullptr)
		return;
	child->setColorTransform(ct);
}

void DisplayObjectContainer::removeChildName(DisplayObject& child)
{
	if (child.getName().empty())
		return;

	auto obj = base.toASObject();
	if (obj.isNull() || !child.isAS3())
		return;

	auto sys = base.getSys();
	auto wrk = obj->getInstanceWorker();

	//The variable is not deleted, but just set to null
	//This is a tested behavior
	multiname objName(nullptr);
	objName.name_type = multiname::NAME_STRING;
	objName.name_s_id = sys->getUniqueStringId(child.getName());
	objName.ns.emplace_back(sys, BUILTIN_STRINGS::EMPTY, NAMESPACE);
	auto atom = asAtomHandler::invalidAtom;
	auto ret = obj->getVariableByMultiname
	(
		atom,
		objName,
		(
			GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE |
			GET_VARIABLE_OPTION::DONT_CALL_GETTER
		),
		wrk
	);

	bool isGetter = ret & GET_VARIABLE_RESULT::GETVAR_ISGETTER;

	// don't try to remove child name if property is builtin function
	if (isGetter && asAtomHandler::is<SyntheticFunction>(atom))
	{
		auto f = asAtomHandler::as<IFunction>(atom);
		f->callGetter(atom, asAtomHandler::getClosureAtom
		(
			atom,
			asAtomHandler::fromObject(obj)
		), wrk);
	}
	else if (isGetter)
		return;

	if (asAtomHandler::getObject(atom) == nullptr)
		return;

	auto newObj =
	(
		_isAS3 ?
		asAtomHandler::nullAtom :
		asAtomHandler::undefinedAtom
	);

	for (auto& _child : dynamicDisplayList)
	{
		if (&_child == &child || _child.getName() != child.getName())
			continue;

		auto childObj = _child.toASObject();
		if (childObj.isNull())
			continue;

		// set to DisplayObject with same name and lowest depth
		childObj->incRef();
		newObj = asAtomHandler::fromObjectNoPrimitive(childObj.getPtr());
	}

	obj->setVariableByMultiname
	(
		objName,
		newObj,
		CONST_NOT_ALLOWED,
		nullptr,
		wrk
	);
	ASATOM_DECREF(atom);
}

void DisplayObjectContainer::deleteChildAt(int32_t depth, bool inskipping)
{
	auto child = tryGetChildAt(depth);
	if (child == nullptr)
		return;
	if (!inskipping && child->is<SimpleButton>())
	{
		auto inserted = mapFrameDepthToChildRemembered.emplace
		(
			depth,
			*child
		);

		auto insertedObj = inserted.first->second;
		if (&insertedObj.get() != child)
			insertedObj = *child;
	}
	if (!inskipping)
		child->setMarkedForTimelineDeletion(true);
	child->afterTimelineDeletion(inskipping);

	// This also removes it from `mapDepthToChild`.
	removeChild(*child, false, inskipping);
}

void DisplayObjectContainer::insertChildAt
(
	int32_t depth,
	DisplayObject& obj,
	bool inskipping,
	bool fromTag
)
{
	if (hasLegacyChildAt(depth))
	{
		LOG
		(
			LOG_ERROR,
			"insertLegacyChildAt: "
			"there is already one child at that depth " <<
			toDebugString() << ' ' <<
			obj->toDebugString() << ' ' << depth
		);
		return;
	}

	using Iter = decltype(dynamicDisplayList.begin());
	auto skipChildrenPlacedByAS = [&](Iter it)
	{
		auto end = dynamicDisplayList.end();
		size_t i;
		for (i = 0; it != end && it->getDepth() >= 0; ++it, ++i);
		return i;
	};

	// find DisplayObject to insert obj after
	DisplayObject* preObj = nullptr;
	for (const auto& pair : mapDepthToChild)
	{
		if (pair.first > depth)
			break;
		preObj = &pair.second;
	}

	auto it = preObj != nullptr std::find
	(
		dynamicDisplayList.begin(),
		dynamicDisplayList.end(),
		preObj
	) : dynamicDisplayList.begin();

	size_t insertPos = it - dynamicDisplayList.begin();
	if (it != dynamicDisplayList.end() && fromTag)
		insertPos += skipChildrenPlacedByAS(++it);

	addChildAt(obj, insertPos, inskipping);

	obj.setDepth(depth);
	mapDepthToChild.emplace(depth, obj);
	obj.afterTimelineCreation();
}

DisplayObject* DisplayObjectContainer::findChildByTagID(uint32_t id) const
{
	for (const auto& pair : mapChildToDepth)
	{
		const auto& child = pair.first.get();
		if (child.getTagID() == id)
			return &child;
	}
	return nullptr;
}

void DisplayObjectContainer::transformChildAt
(
	int32_t depth,
	const MATRIX& mat
)
{
	auto child = tryGetChildAt(depth);
	if (child == nullptr)
		return;
	child->setLegacyMatrix(mat);
}

uint32_t DisplayObjectContainer::getMaxChildDepth() const
{
	int32_t max = -1;
	for (const auto& pair : mapDepthToChild)
		max = std::max(pair.first, max);
	return max >= 0 ? max : UINT32_MAX;
}

void DisplayObjectContainer::checkClipDepth()
{
	DisplayObject* clipObj = nullptr;
	for (const auto& pair : mapDepthToChild)
	{
		auto depth = pair.first;
		const DisplayObject& obj = pair.second;
		if (obj.getClipDepth())
			clipobj = obj;
		else if (clipObj != nullptr && clipObj->getClipDepth() > depth)
			obj.setClipMask(clipObj);
		else
			obj.setClipMask(nullptr);
	}
}

std::vector<DisplayObject&> DisplayObjectContainer::cloneDisplayList() const
{
	Locker l(mutexDisplayList);
	std::vector<DisplayObject&> ret(dynamicDisplayList);
	return ret;
}

bool DisplayObjectContainer::fillTabStopsAutomatic
(
	std::map<int32_t, DisplayObject&>& distanceMap,
	bool& hasTabIndices
) const
{
	if (!hasTabChildren())
		return false;

	bool needsFilling = base.is<Stage>() || base.isRoot() ||
	(
		base.isVisible() &&
		base.isOnStage() &&
		!base.is<SimpleButton>() &&
		!base.is<Button>() &&
		base.tryAs
		<
			InteractiveObject
		>().transformOr(false, [&](const auto& obj)
		{
			hasTabIndices = obj.getTabIndex() >= 0;
			return obj.isTabbable() && hasTabIndices;
		})
	);

	if (!needsFilling)
		return false;

	bool filled = false;
	for (const auto& child : dynamicDisplayList)
	{
		if (!child.isVisible() || !child.isOnStage())
			continue;

		auto intrObj = child.as<InteractiveObject>();
		if (intrObj == nullptr)
			continue;
		bool tabEnabled = intrObj->isTabbable();

		if (intrObj->getTabIndex() >= 0 && tabEnabled)
		{
			hasTabIndices = true;
			return false;
		}

		auto container = child.as<DisplayObjectContainer>();
		if (container == nullptr)
			continue;

		bool _filled = container->fillTabStopsAutomatic
		(
			distanceMap,
			hasTabIndices
		);

		filled |= _filled;
		if (_filled && hasTabIndices)
			return false;
		else if (_filled || !tabEnabled)
			continue;

		auto point = intrObj->localToGlobal(Vector2Twips());
		auto stageWidth = sys->stage->internalGetWidth();
		distanceMap.emplace(point.y * stageWidth + point.x, child);
		filled = true;
	}
	return filled;
}
void DisplayObjectContainer::fillTabStopsByTabIndex
(
	std::map<int32_t, DisplayObject&>& indexMap
) const
{
	if (!hasTabChildren())
		return false;

	bool needsFilling = base.is<Stage>() || base.isRoot() ||
	(
		base.isVisible() &&
		base.isOnStage() &&
		base.tryAs
		<
			InteractiveObject
		>().transformOr(false, [&](const auto& obj)
		{
			auto tabIndex = obj.getTabIndex();
			if (tabIndex >= 0)
				indexMap.emplace(tabIndex, this);
			return obj.isTabbable();
		})
	);

	if (!needsFilling)
		return;

	for (const auto& child : dynamicDisplayList)
	{
		if (!child.isVisible() || !child.isOnStage())
			continue;

		if (!child.is<SimpleButton>() && !child.is<Button>())
			continue;
		auto intrObj = child.as<InteractiveObject>();
		if (intrObj == nullptr || !intrObj->isTabbable())
			continue;

		auto container = child.as<DisplayObjectContainer>();
		if (container != nullptr)
			container->fillTabStopsByTabIndex(indexMap);
		if (intrObj->getTabIndex() < 0)
			continue;
		indexMap.emplace(intrObj->getTabIndex(), intrObj);
	}
}

void DisplayObjectContainer::dumpDisplayList(size_t level)
{
	std::string indent(2 * level, ' ');
	for (const auto& child : dynamicDisplayList)
	{
		auto pos = child.getXY();
		auto size = child.getNominalSize();
		bool hasScrollRect = child.getScrollRect().hasValue();
		LOG
		(
			LOG_INFO,
			indent << child.toDebugString() <<
			" (" << pos.x << ',' << pos.y << ") " <<
			size.x << 'x' << size.y << ' ' <<
			child.isVisible() ? "v" : "" <<
			child.isMask() ? "m" : "" <<
			child.hasFilters() ? "f" : "" <<
			hasScrollRect ? "s " : " " <<
			"cd=" << child.getClipDepth() << ' ' <<
			"a=" << child.getAlpha() << " '" <<
			child.getName() << "' " <<
			"depth:" << child.getDepth() << ' ' <<
			"blendmode:" << child.getBlendMode() <<
			child.getCachedBitmapPreference() ? " cached" ? ""
		);

		auto container = child.as<DisplayObjectContainer>();
		if (container != nullptr)
			container->dumpDisplayList(level + 1);
	}
	
	for (const auto& pair : mapDepthToChild)
	{
		LOG
		(
			LOG_INFO,
			indent << "legacy:" <<
			pair.first << ' ' <<
			pair.second.toDebugString()
		);
	}
}

void DisplayObjectContainer::constructionComplete(bool _explicit, bool forInitAction)
{
	if (!_isAS3)
		return;

	auto list = cloneDisplayList();
	for (auto& child : list)
		child.initFrame();
}

void DisplayObjectContainer::requestInvalidation
(
	InvalidateQueue* q,
	bool forceTextureRefresh
)
{
	if (!forceTextureRefresh)
	{
		base.setHasChanged(true);
		q->addToInvalidateQueue(this);
		base.requestInvalidationFilterParent(q);
		return;
	}

	// Use a copy of the display list to avoid deadlocks when computing
	// `boundsRect` for cached bitmaps.
	auto list = cloneDisplayList();
	for (auto& child : list)	
		child.markAsChanged(q, true);

	base.setNeedsTextureRecalculation();
	base.setHasChanged(true);
	q->addToInvalidateQueue(this);
	requestInvalidationFilterParent(q);
}

void DisplayObjectContainer::removeChildrenMarkedForDeletion()
{
	auto& list = childrenMarkedForDeletion;
	for (auto it = list.begin(); it != list.end(); it = list.erase(it))
		deleteChildAt(*it, false);
}

DisplayObject* DisplayObjectContainer::getLastFrameChildAtDepth
(
	int32_t depth,
	uint32_t id
)
{
	auto it = mapFrameDepthToChildRemembered.find(depth);
	if (it == mapFrameDepthToChildRemembered.end())
		return nullptr;

	DisplayObject& obj = it->second;
	mapFrameDepthToLegacyChildRemembered.erase(it);

	if (obj.getTagID() != id)
		return nullptr;

	obj.setMarkedForTimelineDeletion(false);
	obj.setColorTransform(ColorTransform());
	return &obj;
}

bool DisplayObjectContainer::removeChildDeletionMark(int32_t depth)
{
	auto it = childrenMarkedForDeletion.find(depth);
	if (it != legacyChildrenMarkedForDeletion.end())
	{
		legacyChildrenMarkedForDeletion.erase(it);
		return true;
	}
	return false;
}

void DisplayObjectContainer::requestInvalidationIncludingChildren(InvalidateQueue* q)
{
	for (auto& child : dynamicDisplayList)
		child.requestInvalidationIncludingChildren(q);
}

IDrawable* DisplayObjectContainer::invalidate(bool smoothing)
{
	IDrawable* ret = getFilterDrawable(smoothing);
	if (ret != nullptr)
	{
		Locker l(mutexDisplayList);
		ret->getState()->setupChildrenList(dynamicDisplayList);
		return ret;
	}

	const auto& matrix = base.getMatrix();
	auto bounds = computeBoundsForTransformedRect
	(
		base.boundsRect(false),
		MATRIX().scale(matrix.getScale())
	);
	
	auto ct = base.getColorTransform();
	base.resetNeedsTextureRecalculation();
	
	ret = new RefreshableDrawable
	(
		bounds.min,
		bounds.max.ceil(),
		matrix.getScale(),
		false,
		base.getCachedBitmapPref
		base.getCachedBitmapPreference(),
		base.getScaleFactor(),
		base.getConcatenatedAlpha(),
		ct,
		smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE,
		base.getBlendMode(),
		matrix
	);

	auto state = ret->getState();

	{
		Locker l(mutexDisplayList);
		state->setupChildrenList(dynamicDisplayList);
	}

	auto sys = base.getSys();
	if (_isAS3 || !base.isTabEnabled() || &base != sys->stage->getFocusTarget())
		return ret;

	// add rectangle to be rendered for focused Button/Sprite (AVM1 only)
	assert(base.is<Sprite>() || baseis<Button>());

	auto _bounds = base.tryBoundsRect(false);
	if (!_bounds.hasValue())
		return ret;
	
	if (state->tokens.stroketokens.isNull())
		state->tokens.stroketokens = _MR(new tokenListRef());
	state->renderWithNanoVG = true;
	state->tokens.isFilled = true;
	auto size = _bounds.size();
	auto& strokeTokens = state->tokens.stroketokens->tokens;
	strokeTokens.emplace_back(GeomToken(SET_STROKE).uval);
	strokeTokens.emplace_back(GeomToken(sys->avm1FocusRectLinestyle).uval);

	strokeTokens.emplace_back(GeomToken(MOVE).uval);
	strokeTokens.emplace_back(GeomToken(Vector2f
	(
		0,
		bounds.min.y - bounds.min.x
	).uval));

	strokeTokens.emplace_back(GeomToken(STRAIGHT).uval);
	strokeTokens.emplace_back(GeomToken(Vector2f(0, size.y)).uval);

	strokeTokens.emplace_back(GeomToken(STRAIGHT).uval);
	strokeTokens.emplace_back(GeomToken(size).uval);

	strokeTokens.emplace_back(GeomToken(STRAIGHT).uval);
	strokeTokens.emplace_back(GeomToken(Vector2f(size.x, 0)).uval);

	strokeTokens.emplace_back(GeomToken(STRAIGHT).uval);
	strokeTokens.emplace_back(GeomToken(Vector2f()).uval);

	strokeTokens.emplace_back(GeomToken(CLEAR_STROKE).uval);
	return ret;
}

void DisplayObjectContainer::addChildAt
(
	DisplayObject& child,
	size_t index,
	bool inskipping
)
{
	auto sys = base.getSys();
	auto parent = child.getParent();
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	bool hasParent =
	(
		parent != nullptr &&
		sys->isInResetParentList(child);
	);

	if (hasParent && parent == &base && getChildIndex(child) >= 0)
	{
		//Child already in this container, set to new position
		setChildIndexImpl(child, index);
		return;
	}

	if (hasParent && parent != &base)
		parent->removeChild(child, false, inskipping);

	sys->removeFromResetParentList(child);
	if (!child.isAS3())
		sys->stage->AVM1AddDisplayObject(child);

	bool wasOnStage = child.isOnStage();
	child.setPlaceFrame(0);
	child.setParent(&base, false);
	if (!_isAS3)
		child.setRemovedByAVM1(false);

	{
		Locker l(mutexDisplayList);
		auto it =
		(
			index >= dynamicDisplayList.size() ?
			//We insert the object in the back of the list
			dynamicDisplayList.end() :
			dynamicDisplayList.begin() + index
		);
		dynamicDisplayList.insert(it, child);
	}

	handleAddedEvent(base, child, wasOnStage);

	if (base.isOnStage())
		base.requestInvalidation(sys);
}

void DisplayObjectContainer::handleAddedEvent
(
	const DisplayObject& parent,
	DisplayObject& child,
	bool wasOnStage
)
{
	handleAddedEventOnly(child);
	if (parent.isOnStage() && !wasOnStage)
		handleAddedToStageEvent(child);
}

void DisplayObjectContainer::handleAddedEventOnly(DisplayObject& child)
{
	auto obj = child.toASObject();
	if (obj.isNull())
		return;

	auto wrk = obj->getInstanceWorker();
	getVm(child.getSys())->tryAddEvent(_MR(Class<Event>::getInstanceS
	(
		wrk,
		"added",
		true
	)));
}

void DisplayObjectContainer::handleAddedToStageEvent(DisplayObject& child)
{
	handleAddedToStageEventOnly(child);

	auto container = child.as<DisplayObjectContainer>();
	if (container != nullptr)
	{
		auto list = container->cloneDisplayList();
		for (auto& _child : list)
			handleAddedToStageEvent(_child);
	}

	auto button = child.as<SimpleButton>();
	if (button == nullptr)
		return;
	
	auto _child = button->getCurrentStateObj();
	if (_child != nullptr)
		handleAddedToStageEvent(_child);
		

}

void DisplayObjectContainer::handleAddedToStageEventOnly
(
	DisplayObject& child
)
{
	auto obj = child.toASObject();
	if (obj.isNull())
		return;

	auto wrk = obj->getInstanceWorker();
	getVm(child.getSys())->tryAddEvent(_MR(Class<Event>::getInstanceS
	(
		wrk,
		"addedToStage",
		true
	)));
}

void DisplayObjectContainer::handleRemovedEvent
(
	DisplayObject& child,
	bool keepOnStage
)
{
	auto obj = child.toASObject();
	if (obj.isNull())
		return;

	auto sys = child.getSys();
	auto wrk = obj->getInstanceWorker();
	getVm(sys)->tryAddEvent(_MR(Class<Event>::getInstanceS
	(
		wrk,
		"removed",
		true
	)));

	bool isRemovedFromStage = !keepOnStage &&
	(
		child.isOnStage() ||
		child.getStage() != nullptr
	);

	if (isRemovedFromStage)
		handleRemovedFromStageEvent(child);
}

void DisplayObjectContainer::handleRemovedFromStageEvent(DisplayObject& child)
{
	auto obj = child.toASObject();
	if (obj.isNull())
		return;

	auto sys = child.getSys();
	auto engineData = sys->getEngineData();
	auto wrk = obj->getInstanceWorker();
	getVm(sys)->tryAddEvent(_MR(Class<Event>::getInstanceS
	(
		wrk,
		"removedToStage"
	)));

	if (!child.is<InteractiveObject>() || engineData == nullptr)
		return;
	engineData->InteractiveObjectRemovedFromStage();
	sys->stage->AVM1RemoveDisplayObject(child);
}

bool DisplayObjectContainer::removeChild
(
	DisplayObject& child,
	bool direct,
	bool inskipping,
	bool keepOnStage,
	bool removeName
)
{
	auto sys = base.getSys();
	auto parent = child.getParent();
	if (parent == nullptr || parent != &base)
		return false;

	if (!_isAS3)
		child.removeAVM1Listeners();

	{
		Locker l(mutexDisplayList);
		auto it = std::find
		(
			dynamicDisplayList.begin(),
			dynamicDisplayList.end(),
			child
		);
		if (it == dynamicDisplayList.end())
			return sys->isInResetParentList(child);
	}

	child.setMask(nullptr);
	removeFromDisplayList(child);
	handleRemovedEvent(child, keepOnstage);
	if (!direct && !inskipping && !isVmThread())
		sys->addDisplayObjectToResetParentList(child);
	else
	{
		child.setParent(nullptr);
		if (removeName)
			removeChildName(child);
	}
	base.markAsChanged(true);
	sys->stage->prepareForRemoval(child);
	checkClipDepth();
	return true;
}

void DisplayObjectContainer::removeFromDisplayList(DisplayObject& child)
{
	Locker l(mutexDisplayList);
	auto it = std::find
	(
		dynamicDisplayList.begin(),
		dynamicDisplayList.end(),
		child
	);
	
	//Erase this from the legacy child map (if it is in there)
	unmarkChild(child);
	dynamicDisplayList.erase(it);
}

void DisplayObjectContainer::removeAllChildren(bool sendevents, bool recursive)
{
	if (dynamicDisplayList.empty())
		return; // no need to request invalidation if this container is already empty

	auto list = cloneDisplayList();
	for (auto& child : list)
	{
		auto container = child.as<DisplayObjectContainer>();
		if (recursive && container != nullptr)
			container->removeAllChildren(recursive);
		removeChild(child, false, false, false);
	}

	base.requestInvalidation(base.getSys());
}

void DisplayObjectContainer::removeAVM1Listeners()
{
	if (_isAS3())
		return;

	Locker l(mutexDisplayList);
	for (auto& child : dynamicDisplayList)
		child.removeAVM1Listeners();
}

bool DisplayObjectContainer::contains(const DisplayObject& child) const
{
	if (&child == &base)
		return true;

	for (auto& _child : dynamicDisplayList)
	{
		if (&_child == &child)
			return true;
		auto container = _child.as<DisplayObjectContainer>();
		if (container != nullptr && container->contains(child))
			return true;
	}
	return false;
}

void DisplayObjectContainer::stopAllMovieClipsImpl()
{
	(void)base.tryAs<MovieClip>().andThen([](const auto& clip)
	{
		clip.setStopped();
		return clip;
	});

	for (auto& child : dynamicDisplayList)
	{
		auto clip = child.as<MovieClip>();
		auto container = child.as<DisplayObjectContainer>();
		if (clip != nullptr)
			clip->setStopped();
		if(container != nullptr)
			container->stopAllMovieClipsImpl();
	}
}

void DisplayObjectContainer::setChildIndexImpl
(
	DisplayObject& child,
	size_t index
)
{
	auto curIndex = getChildIndex(child);
	if(curIndex < 0)
		return;

	// Remove from the old position.
	dynamicDisplayList.erase
	(
		dynamicDisplayList.begin() +
		curIndex
	);

	//Erase the child from the child map (if it is in there)
	unmarkChild(child);
	
	if (index >= dynamicDisplayList.size())
		dynamicDisplayList.push_back(child);
	else
	{
		dynamicDisplayList.insert
		(
			dynamicDisplayList.begin() + index,
			child
		);
	}

	checkClipDepth();
	base.requestInvalidation(base.getSys());
}

std::vector<DisplayObject&> DisplayObjectContainer::getObjectsFromPoint
(
	const Vector2Twips& point
)
{
	std::vector<DisplayObject&> list;
	getObjectsFromPoint(point, list);
	return list;
}

void DisplayObjectContainer::getObjectsFromPoint
(
	const Vector2Twips& point,
	std::vector<DisplayObject&>& list
)
{
	const auto flags =
	(
		HitTestFlags::SkipMask |
		HitTestFlags::SkipInvisible |
		HitTestFlags::SkipChildren
	);

	auto obj = base.hitTestShape
	(
		point,
		base.globalToLocal(point, false),
		flags
	);

	if (obj != nullptr)
	{
		list.push_back(*obj);
		return;
	}

	Locker l(mutexDisplayList);
	for (auto& child : dynamicDisplayList)
	{
		obj = child.hitTestShape
		(
			point,
			child.globalToLocal(point, false),
			flags
		);

		if (obj != nullptr)
			list.push_back(*obj);
		auto container = child.as<DisplayObjectContainer>();
		if (container != nullptr)
			container->getObjectsFromPoint(point, list);
	}
}

void DisplayObjectContainer::unmarkChild(DisplayObject& child)
{
	auto it = mapChildToDepth.find(child);
	if (it != mapChildToDepth.end())
	{
		mapDepthToChild.erase(it->second);
		mapChildToDepth.erase(it);
	}
}

void DisplayObjectContainer::clearDisplayList()
{
	auto& list = dynamicDisplayList;
	for (auto it = list.rbegin(); it != list.rend(); it = list.erase(it))
	{
		removeChildName(*it);
		it->setParent(nullptr);
		it->getSys()->removeFromResetParentList(*it);
	}

}

void DisplayObjectContainer::declareFrame(bool implicit)
{
	if (!_isAS3)
		return;
	// elements of the dynamicDisplayList may be removed/added during declareFrame() calls,
	// so we create a temporary list containing all elements
	auto list = cloneDisplayList();
	for (auto& child : list)
		child.declareFrame(true);
}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void DisplayObjectContainer::initFrame()
{
	/* call our own constructor, if necassary */
	DisplayObject::initFrame();

	/* init the frames and call constructors of our children */

	// elements of the dynamicDisplayList may be removed during initFrame() calls,
	// so we create a temporary list containing all elements
	auto list = cloneDisplayList();
	for (auto& child : list)
		child.initFrame();
}

void DisplayObjectContainer::executeFrameScript()
{
	// elements of the dynamicDisplayList may be removed during executeFrameScript() calls,
	// so we create a temporary list containing all elements
	auto list = cloneDisplayList();
	for (auto& child : list)
		child.executeFrameScript();
}

void DisplayObjectContainer::afterTimelineCreation()
{
	auto list = cloneDisplayList();
	for (auto& child : list)
		child.afterTimelineCreation();
}

void DisplayObjectContainer::afterTimelineDeletion(bool inskipping)
{
	auto list = cloneDisplayList();
	for (auto& child : list)
		child.afterTimelineDeletion();
}

void DisplayObjectContainer::enterFrame(bool implicit)
{
	auto list = cloneDisplayList();
	for (auto& child : list)
	{
		if (base.getSkipFrame())
			child.setSkipFrame(true);
		child.enterFrame(implicit);
	}

	// Reset `SkipFrame` for anything that isn't a `MovieClip`
	// (`Loader`, `Sprite`, `{,Simple}Button`).
	if (!base.is<MovieClip>())
		base.setSkipFrame(false);
}

/* This is run in vm's thread context */
void DisplayObjectContainer::advanceFrame(bool implicit)
{
	if (!_isAS3 && implicit)
		return;

	// elements of the dynamicDisplayList may be removed during advanceFrame() calls,
	// so we create a temporary list containing all elements
	auto list = cloneDisplayList();
	for (auto& child : list)
		child.advanceFrame(implicit);
}
