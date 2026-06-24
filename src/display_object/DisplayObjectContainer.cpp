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

void DisplayObjectContainer::removeChildName(DisplayObject& obj)
{
	if (obj.getName().empty)
		return;

	//The variable is not deleted, but just set to null
	//This is a tested behavior
	multiname objName(nullptr);
	objName.name_type=multiname::NAME_STRING;
	objName.name_s_id=obj->name;
	objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	asAtom a=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT r = getVariableByMultiname(a,objName,GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE|GET_VARIABLE_OPTION::DONT_CALL_GETTER,getInstanceWorker());
	if (r&GET_VARIABLE_RESULT::GETVAR_ISGETTER)
	{
		// don't try to remove child name if property is builtin function
		if (asAtomHandler::is<SyntheticFunction>(a))
		{
			IFunction* f = asAtomHandler::as<IFunction>(a);
			asAtom closure = asAtomHandler::getClosureAtom(a,asAtomHandler::fromObject(this));
			f->callGetter(a,closure,this->getInstanceWorker());
		}
		else
			a = asAtomHandler::invalidAtom;
	}
	if (asAtomHandler::getObject(a))
	{
		asAtom newobj = needsActionScript3() ? asAtomHandler::nullAtom : asAtomHandler::undefinedAtom;
		for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); ++it)
		{
			if ((*it) != obj && (*it)->name==obj->name)
			{
				// set to DisplayObject with same name and lowest depth
				(*it)->incRef();
				newobj=asAtomHandler::fromObjectNoPrimitive(*it);
				break;
			}
		}
		setVariableByMultiname(objName,newobj, CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
	}
	ASATOM_DECREF(a);
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
	tiny_string indent(std::string(2*level, ' '));
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		Vector2f pos = (*it)->getXY();
		LOG(LOG_INFO, indent << (*it)->toDebugString() <<
		    " (" << pos.x << "," << pos.y << ") " <<
		    (*it)->getNominalWidth() << "x" << (*it)->getNominalHeight() << " " <<
		    ((*it)->isVisible() ? "v" : "") <<
						  ((*it)->isMask() ? "m" : "") <<((*it)->hasFilters() ? "f" : "") <<(asAtomHandler::is<Rectangle>((*it)->scrollRect) ? "s" : "") << " cd=" <<(*it)->ClipDepth<<" "<<
			"a=" << (*it)->clippedAlpha() <<" '"<<
			((*it)->name != UINT32_MAX ? getSystemState()->getStringFromUniqueId((*it)->name) : "")<<"'"
			<<" depth:"<<(*it)->getDepth()<<" blendmode:"<<(*it)->getBlendMode()<<
		    ((*it)->cacheAsBitmap ? " cached" : ""));

		if ((*it)->is<DisplayObjectContainer>())
		{
			(*it)->as<DisplayObjectContainer>()->dumpDisplayList(level+1);
		}
	}
	auto i = mapDepthToLegacyChild.begin();
	while( i != mapDepthToLegacyChild.end() )
	{
		LOG(LOG_INFO, indent << "legacy:"<<i->first <<" "<<i->second->toDebugString());
		i++;
	}
}

void DisplayObjectContainer::constructionComplete(bool _explicit, bool forInitAction)
{
	if (!_isAS3)
		return;

	auto list = cloneDisplayList();
	initializingFrame = true;
	for (auto& child : list)
		child.initFrame();
	initializingFrame = false;
}

void DisplayObjectContainer::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	DisplayObject::requestInvalidation(q);
	if (forceTextureRefresh)
	{
		std::vector<_R<DisplayObject>> tmplist;
		cloneDisplayList(tmplist); // use copy of displaylist to avoid deadlock when computing boundsrect for cached bitmaps
		auto it=tmplist.begin();
		for(;it!=tmplist.end();++it)
		{
			(*it)->hasChanged = true;
			(*it)->requestInvalidation(q,forceTextureRefresh);
		}
	}
	if (forceTextureRefresh)
		this->setNeedsTextureRecalculation();
	hasChanged=true;
	q->addToInvalidateQueue(this);
	requestInvalidationFilterParent(q);
}
void DisplayObjectContainer::requestInvalidationIncludingChildren(InvalidateQueue* q)
{
	DisplayObject::requestInvalidationIncludingChildren(q);
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		(*it)->requestInvalidationIncludingChildren(q);
	}
}
IDrawable* DisplayObjectContainer::invalidate(bool smoothing)
{
	IDrawable* res = getFilterDrawable(smoothing);
	if (res)
	{
		Locker l(mutexDisplayList);
		res->getState()->setupChildrenList(dynamicDisplayList);
		return res;
	}
	number_t x,y;
	number_t width,height;
	number_t bxmin=0,bxmax=0,bymin=0,bymax=0;
	boundsRect(bxmin,bxmax,bymin,bymax,false);
	MATRIX matrix = getMatrix();
	
	bool isMask=false;
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);
	
	ColorTransformBase ct=this->colorTransform;
	this->resetNeedsTextureRecalculation();
	
	res = new RefreshableDrawable(x, y, ceil(width), ceil(height)
								   , matrix.getScaleX(), matrix.getScaleY()
								   , isMask, cacheAsBitmap
								   , getScaleFactor(),getConcatenatedAlpha()
								   , ct, smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS:SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
	{
		Locker l(mutexDisplayList);
		res->getState()->setupChildrenList(dynamicDisplayList);
	}
	if (!this->needsActionScript3()
			&& asAtomHandler::AVM1toBool(tabEnabled,getInstanceWorker(),loadedFrom->version)
			&& this == getSystemState()->stage->getFocusTarget())
	{
		// add rectangle to be rendered for focused SimpleButton/Sprite (AVM1 only)
		assert (this->is<Sprite>() || this->is<SimpleButton>());
		number_t bxmin,bxmax,bymin,bymax;
		if(boundsRect(bxmin,bxmax,bymin,bymax,false))
		{
			if (!res->getState()->tokens.stroketokens)
				res->getState()->tokens.stroketokens=_MR(new tokenListRef());
			res->getState()->renderWithNanoVG=true;
			res->getState()->tokens.isFilled=true;
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(SET_STROKE).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(getSystemState()->avm1FocusRectLinestyle).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(MOVE).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(Vector2(0.0 , (bymin-bxmin))).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(STRAIGHT).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(Vector2(0.0, (bymax-bymin))).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(STRAIGHT).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(Vector2((bxmax-bxmin), (bymax-bymin))).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(STRAIGHT).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(Vector2((bxmax-bxmin), 0.0)).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(STRAIGHT).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(Vector2(0.0, 0.0)).uval);
			res->getState()->tokens.stroketokens->tokens.push_back(GeomToken(CLEAR_STROKE).uval);
		}
	}
	return res;
}
void DisplayObjectContainer::invalidateForRenderToBitmap(BitmapContainerRenderData* container,bool smoothing)
{
	DisplayObject::invalidateForRenderToBitmap(container, smoothing);
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		(*it)->invalidateForRenderToBitmap(container, smoothing);
	}
}
void DisplayObjectContainer::_addChildAt(DisplayObject* child, unsigned int index, bool inskipping)
{
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	if(child->getParent() && !getSystemState()->isInResetParentList(child))
	{
		if(child->getParent()==this)
		{
			if (this->getChildIndex(child) >= 0)
			{
				//Child already in this container, set to new position
				setChildIndexIntern(child,index);
				child->decRef();
				return;
			}
		}
		else
		{
			child->getParent()->_removeChild(child,false,inskipping);
		}
	}
	getSystemState()->removeFromResetParentList(child);
	if(!child->needsActionScript3())
		getSystemState()->stage->AVM1AddDisplayObject(child);
	child->setParent(this,false);
	{
		Locker l(mutexDisplayList);
		//We insert the object in the back of the list
		if(index >= dynamicDisplayList.size())
			dynamicDisplayList.push_back(child);
		else
		{
			auto it=dynamicDisplayList.begin();
			for(unsigned int i=0;i<index;i++)
				++it;
			dynamicDisplayList.insert(it,child);
		}
		child->addStoredMember();
	}
	_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"added",true));
	if(isVmThread())
		ABCVm::publicHandleEvent(child,e);
	else
	{
		child->incRef();
		getVm(getSystemState())->addEvent(_MR(child),e);
	}

	if (!onStage || child != getSystemState()->mainClip)
		child->setOnStage(onStage,false,inskipping);
	
	if (isOnStage())
		this->requestInvalidation(getSystemState());
}

void DisplayObjectContainer::handleRemovedEvent(DisplayObject* child, bool keepOnStage, bool inskipping, bool sendevents)
{
	if (sendevents)
	{
		_R<Event> e=_MR(Class<Event>::getInstanceS(child->getInstanceWorker(),"removed",true));
		if (isVmThread())
			ABCVm::publicHandleEvent(child, e);
		else
		{
			child->incRef();
			getVm(getSystemState())->addEvent(_MR(child), e);
		}
	}
	if (!keepOnStage && (child->isOnStage() || !child->getStage().isNull()))
		child->setOnStage(false, true, inskipping);
}

bool DisplayObjectContainer::_removeChild(DisplayObject* child, bool direct, bool inskipping, bool keeponstage, bool sendevents, bool removeName)
{
	if(!child->getParent() || child->getParent()!=this)
		return false;
	if (!needsActionScript3())
		child->removeAVM1Listeners();

	{
		Locker l(mutexDisplayList);
		auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
		if(it==dynamicDisplayList.end())
			return getSystemState()->isInResetParentList(child);
	}
	child->setMask(NullRef);
	_removeFromDisplayList(child);
	handleRemovedEvent(child, keeponstage, inskipping, sendevents);
	if (!direct && !inskipping && !isVmThread())
		getSystemState()->addDisplayObjectToResetParentList(child);
	else
	{
		child->setParent(nullptr,false);
		if (removeName)
			removeChildName(child);
	}
	this->hasChanged=true;
	this->requestInvalidation(getSystemState());
	getSystemState()->stage->prepareForRemoval(child);
	checkClipDepth();
	return true;
}
void DisplayObjectContainer::_removeFromDisplayList(DisplayObject* child)
{
	Locker l(mutexDisplayList);
	auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
	
	//Erase this from the legacy child map (if it is in there)
	umarkLegacyChild(child);
	dynamicDisplayList.erase(it);
}

void DisplayObjectContainer::_removeAllChildren(bool sendevents, bool recursive)
{
	mutexDisplayList.lock();
	vector<DisplayObject*> childrenToRemove = dynamicDisplayList;
	mutexDisplayList.unlock();
	if (dynamicDisplayList.empty())
		return; // no need to request invalidation if this container is already empty
	auto it = childrenToRemove.begin();
	while (it != childrenToRemove.end())
	{
		if (recursive && (*it)->is<DisplayObjectContainer>())
			(*it)->as<DisplayObjectContainer>()->_removeAllChildren(sendevents,recursive);
		_removeChild(*it,false,false,false,sendevents);
		it++;
	}
	this->requestInvalidation(getSystemState());
}

void DisplayObjectContainer::removeAVM1Listeners()
{
	if (needsActionScript3())
		return;
	Locker l(mutexDisplayList);
	auto it=dynamicDisplayList.begin();
	while (it!=dynamicDisplayList.end())
	{
		(*it)->removeAVM1Listeners();
		it++;
	}
	DisplayObject::removeAVM1Listeners();
}

bool DisplayObjectContainer::_contains(DisplayObject* d)
{
	if(d==this)
		return true;

	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		if(*it==d)
			return true;
		if((*it)->is<DisplayObjectContainer>() && (*it)->as<DisplayObjectContainer>()->_contains(d))
			return true;
	}
	return false;
}

void DisplayObjectContainer::stopAllMovieClipsIntern()
{
	if (is<MovieClip>())
		as<MovieClip>()->setStopped();
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		if ((*it)->is<MovieClip>())
			(*it)->as<MovieClip>()->setStopped();
		if((*it)->is<DisplayObjectContainer>())
			(*it)->as<DisplayObjectContainer>()->stopAllMovieClipsIntern();
	}
}

void DisplayObjectContainer::setChildIndexIntern(DisplayObject *child, int index)
{
	int curIndex = this->getChildIndex(child);
	if(curIndex < 0)
		return;
	auto itrem = this->dynamicDisplayList.begin()+curIndex;
	this->dynamicDisplayList.erase(itrem); //remove from old position

	auto it=this->dynamicDisplayList.begin();
	int i = 0;
	//Erase the child from the legacy child map (if it is in there)
	this->umarkLegacyChild(child);
	
	for(;it != this->dynamicDisplayList.end(); ++it)
		if(i++ == index)
		{
			this->dynamicDisplayList.insert(it, child);
			this->checkClipDepth();
			this->requestInvalidation(this->getSystemState());
			return;
		}
	this->dynamicDisplayList.push_back(child);
	this->checkClipDepth();
	this->requestInvalidation(this->getSystemState());
}

void DisplayObjectContainer::getObjectsFromPoint(const Vector2f& point, Array *ar)
{
	Locker l(mutexDisplayList);
	if (getClipDepth())
		return;
	if (!hitTestMask(point,HIT_TYPE::GENERIC_HIT_EXCLUDE_CHILDREN))
		return;
	if  (!isVisible())
		return;
	Vector2f localpoint = this->globalToLocal(point,false);
	auto obj = this->hitTest(point,localpoint,HIT_TYPE::GENERIC_HIT_EXCLUDE_CHILDREN,false);
	if (obj)
	{
		obj->incRef();
		ar->push(asAtomHandler::fromObject(obj.getPtr()));
	}
	else
	{
		auto it = dynamicDisplayList.begin();
		while (it != dynamicDisplayList.end())
		{
			if ((*it)->getClipDepth())
			{
				it++;
				continue;
			}
			localpoint = (*it)->globalToLocal(point, false);
			auto obj = (*it)->hitTest(point, localpoint,HIT_TYPE::GENERIC_HIT_EXCLUDE_CHILDREN,false);
			if (obj)
			{
				obj->incRef();
				ar->push(asAtomHandler::fromObject(obj.getPtr()));
			}
			if ((*it)->is<DisplayObjectContainer>())
				(*it)->as<DisplayObjectContainer>()->getObjectsFromPoint(point,ar);
			it++;
		}
	}
}

void DisplayObjectContainer::umarkLegacyChild(DisplayObject* child)
{
	auto it = mapLegacyChildToDepth.find(child);
	if (it != mapLegacyChildToDepth.end())
	{
		mapDepthToLegacyChild.erase(it->second);
		mapLegacyChildToDepth.erase(it);
	}
}

void DisplayObjectContainer::clearDisplayList()
{
	auto it = dynamicDisplayList.rbegin();
	while (it != dynamicDisplayList.rend())
	{
		DisplayObject* c = (*it);
		dynamicDisplayList.pop_back();
		removeChildName(c);
		c->setParent(nullptr,true);
		c->removeStoredMember();
		getSystemState()->removeFromResetParentList(c);
		it = dynamicDisplayList.rbegin();
	}
}

void DisplayObjectContainer::declareFrame(bool implicit)
{
	if (needsActionScript3())
	{
		// elements of the dynamicDisplayList may be removed/added during declareFrame() calls,
		// so we create a temporary list containing all elements
		std::vector<_R<DisplayObject>> tmplist;
		cloneDisplayList(tmplist);
		auto it=tmplist.begin();
		for(;it!=tmplist.end();it++)
			(*it)->declareFrame(true);
	}
	DisplayObject::declareFrame(implicit);
}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void DisplayObjectContainer::initFrame()
{
	// it's possible that legacy MovieClips are defined as inherited directly from Sprite
	// so we have to set initializingFrame here
	initializingFrame=true;

	/* call our own constructor, if necassary */
	DisplayObject::initFrame();

	/* init the frames and call constructors of our children */

	// elements of the dynamicDisplayList may be removed during initFrame() calls,
	// so we create a temporary list containing all elements
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->initFrame();
	initializingFrame=false;
}

void DisplayObjectContainer::executeFrameScript()
{
	// elements of the dynamicDisplayList may be removed during executeFrameScript() calls,
	// so we create a temporary list containing all elements
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->executeFrameScript();
}

void DisplayObjectContainer::afterLegacyInsert()
{
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->afterLegacyInsert();
}

void DisplayObjectContainer::afterLegacyDelete(bool inskipping)
{
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->afterLegacyDelete(inskipping);
}

void DisplayObjectContainer::enterFrame(bool implicit)
{
	std::vector<_R<DisplayObject>> list;
	cloneDisplayList(list);
	for (auto child : list)
	{
		child->skipFrame = skipFrame ? true : child->skipFrame;
		child->enterFrame(implicit);
	}
	if (!is<MovieClip>()) // reset skipFrame for everything that is not a MovieClip (Loader/Sprite/SimpleButton)
		skipFrame = false;
}

/* This is run in vm's thread context */
void DisplayObjectContainer::advanceFrame(bool implicit)
{
	if (needsActionScript3() || !implicit)
	{
		// elements of the dynamicDisplayList may be removed during advanceFrame() calls,
		// so we create a temporary list containing all elements
		std::vector<_R<DisplayObject>> tmplist;
		cloneDisplayList(tmplist);
		auto it=tmplist.begin();
		for(;it!=tmplist.end();it++)
			(*it)->advanceFrame(implicit);
	}
	else
		InteractiveObject::advanceFrame(implicit);
}
