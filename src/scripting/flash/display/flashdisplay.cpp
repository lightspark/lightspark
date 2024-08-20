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

#include <list>

#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/MovieClip.h"
#include "scripting/flash/display/NativeWindow.h"
#include "scripting/flash/display/Stage3D.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/display/Graphics.h"
#include "swf.h"
#include "scripting/flash/system/flashsystem.h"
#include "parsing/streams.h"
#include "parsing/tags.h"
#include "compat.h"
#include "scripting/class.h"
#include "backends/cachedsurface.h"
#include "backends/geometry.h"
#include "backends/input.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/display/Bitmap.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/ui/keycodes.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include <algorithm>

#define FRAME_NOT_FOUND 0xffffffff //Used by getFrameIdBy*

using namespace std;
using namespace lightspark;

std::ostream& lightspark::operator<<(std::ostream& s, const DisplayObject& r)
{
	s << "[" << r.getClass()->class_name << "]";
	if(r.name != BUILTIN_STRINGS::EMPTY)
		s << " name: " << r.name;
	return s;
}



Sprite::Sprite(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c),TokenContainer(this),graphics(NullRef),soundstartframe(UINT32_MAX),streamingsound(false),hasMouse(false),initializingFrame(false)
	,dragged(false),buttonMode(false),useHandCursor(true)
{
	subtype=SUBTYPE_SPRITE;
}

bool Sprite::destruct()
{
	resetToStart();
	graphics.reset();
	hitArea.reset();
	hitTarget.reset();
	tokens=nullptr;
	dragged = false;
	buttonMode = false;
	useHandCursor = true;
	streamingsound=false;
	hasMouse=false;
	sound.reset();
	soundtransform.reset();
	return DisplayObjectContainer::destruct();
}

void Sprite::finalize()
{
	resetToStart();
	graphics.reset();
	hitArea.reset();
	hitTarget.reset();
	tokens=nullptr;
	sound.reset();
	soundtransform.reset();
	DisplayObjectContainer::finalize();
}

void Sprite::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if (graphics)
		graphics->prepareShutdown();
	if (hitArea)
		hitArea->prepareShutdown();
	if (hitTarget)
		hitTarget->prepareShutdown();
	if (sound)
		sound->prepareShutdown();
	if (soundtransform)
		soundtransform->prepareShutdown();
}

void Sprite::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("graphics","",c->getSystemState()->getBuiltinFunction(_getGraphics,0,Class<Graphics>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",c->getSystemState()->getBuiltinFunction(_startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",c->getSystemState()->getBuiltinFunction(_stopDrag),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, buttonMode,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, hitArea,Sprite);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, useHandCursor,Boolean);
	c->setDeclaredMethodByQName("soundTransform","",c->getSystemState()->getBuiltinFunction(getSoundTransform,0,Class<SoundTransform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("soundTransform","",c->getSystemState()->getBuiltinFunction(setSoundTransform),SETTER_METHOD,true);
}

ASFUNCTIONBODY_GETTER_SETTER(Sprite, buttonMode)
ASFUNCTIONBODY_GETTER_SETTER_CB(Sprite, useHandCursor,afterSetUseHandCursor)

void Sprite::afterSetUseHandCursor(bool /*oldValue*/)
{
	handleMouseCursor(hasMouse);
}

void Sprite::refreshSurfaceState()
{
	if (graphics)
		graphics->refreshSurfaceState();
}

IDrawable* Sprite::invalidate(bool smoothing)
{
	IDrawable* res = getFilterDrawable(smoothing);
	if (res)
	{
		Locker l(mutexDisplayList);
		res->getState()->setupChildrenList(dynamicDisplayList);
		return res;
	}

	if (graphics && graphics->hasBounds())
	{
		res = graphics->invalidate(smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE);
		if (res)
		{
			Locker l(mutexDisplayList);
			res->getState()->setupChildrenList(dynamicDisplayList);
		}
	}
	else
		res = DisplayObjectContainer::invalidate(smoothing);
	return res;
}

ASFUNCTIONBODY_ATOM(Sprite,_startDrag)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	bool lockCenter = false;
	const RECT* bounds = nullptr;
	ARG_CHECK(ARG_UNPACK(lockCenter,false));
	if(argslen > 1)
	{
		Rectangle* rect = Class<Rectangle>::cast(asAtomHandler::getObject(args[1]));
		if(!rect)
		{
			createError<ArgumentError>(wrk,kInvalidArgumentError,"Wrong type");
			return;
		}
		bounds = new RECT(rect->getRect());
	}

	Vector2f offset;
	if(!lockCenter)
	{
		offset = -th->getParent()->getLocalMousePos();
		offset += th->getXY();
	}

	th->incRef();
	wrk->getSystemState()->getInputThread()->startDrag(_MR(th), bounds, offset);
}

ASFUNCTIONBODY_ATOM(Sprite,_stopDrag)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	wrk->getSystemState()->getInputThread()->stopDrag(th);
}

ASFUNCTIONBODY_GETTER(Sprite, hitArea)

ASFUNCTIONBODY_ATOM(Sprite,_setter_hitArea)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	_NR<Sprite> value;
	ARG_CHECK(ARG_UNPACK(value));

	if (!th->hitArea.isNull())
		th->hitArea->hitTarget.reset();

	th->hitArea = value;
	if (!th->hitArea.isNull())
	{
		th->incRef();
		th->hitArea->hitTarget = _MNR(th);
	}
}

ASFUNCTIONBODY_ATOM(Sprite,getSoundTransform)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	if (th->sound && th->sound->soundTransform)
	{
		ret = asAtomHandler::fromObject(th->sound->soundTransform.getPtr());
		ASATOM_INCREF(ret);
	}
	else
		asAtomHandler::setNull(ret);
	if (!th->soundtransform)
	{
		if (th->sound)
			th->soundtransform = th->sound->soundTransform;
	}
	if (!th->soundtransform)
	{
		th->soundtransform = _MR(Class<SoundTransform>::getInstanceSNoArgs(wrk));
		if (th->sound)
			th->sound->soundTransform = th->soundtransform;
	}
	ret = asAtomHandler::fromObject(th->soundtransform.getPtr());
	ASATOM_INCREF(ret);
}
ASFUNCTIONBODY_ATOM(Sprite,setSoundTransform)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	if (argslen == 0 || !asAtomHandler::is<SoundTransform>(args[0]))
		return;
	ASATOM_INCREF(args[0]);
	th->soundtransform =  _MR(asAtomHandler::getObject(args[0])->as<SoundTransform>());
}

bool DisplayObjectContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	bool ret = false;

	if(dynamicDisplayList.empty())
		return false;
	if (visibleOnly)
	{
		if (!this->isVisible())
			return false;
		if (!boundsRectVisibleDirty)
		{
			xmin = boundsrectVisibleXmin;
			ymin = boundsrectVisibleYmin;
			xmax = boundsrectVisibleXmax;
			ymax = boundsrectVisibleYmax;
			return true;
		}
	}
	else if (!boundsRectDirty)
	{
		xmin = boundsrectXmin;
		ymin = boundsrectYmin;
		xmax = boundsrectXmax;
		ymax = boundsrectYmax;
		return true;
	}

	Locker l(mutexDisplayList);
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		number_t txmin,txmax,tymin,tymax;
		if((*it)->getBounds(txmin,txmax,tymin,tymax,(*it)->getMatrix(),visibleOnly))
		{
			if(ret==true)
			{
				xmin = min(xmin,txmin);
				xmax = max(xmax,txmax);
				ymin = min(ymin,tymin);
				ymax = max(ymax,tymax);
			}
			else
			{
				xmin=txmin;
				xmax=txmax;
				ymin=tymin;
				ymax=tymax;
				ret=true;
			}
		}
	}
	if (ret)
	{
		if (visibleOnly)
		{
			boundsrectVisibleXmin=xmin;
			boundsrectVisibleYmin=ymin;
			boundsrectVisibleXmax=xmax;
			boundsrectVisibleYmax=ymax;
			boundsRectVisibleDirty=false;
		}
		else
		{
			boundsrectXmin=xmin;
			boundsrectYmin=ymin;
			boundsrectXmax=xmax;
			boundsrectYmax=ymax;
			boundsRectDirty=false;
		}
	}
	return ret;
}

bool Sprite::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	bool ret = DisplayObjectContainer::boundsRect(xmin,xmax,ymin,ymax,visibleOnly);
	if (graphics && graphics->hasBounds())
	{
		if (!ret)
		{
			ret = this->graphics->boundsRect(xmin,xmax,ymin,ymax);
		}
		else
		{
			number_t gxmin,gxmax,gymin,gymax;
			if (this->graphics->boundsRect(gxmin,gxmax,gymin,gymax))
			{
				xmin = min(xmin,gxmin);
				xmax = max(xmax,gxmax);
				ymin = min(ymin,gymin);
				ymax = max(ymax,gymax);
				ret=true;
			}
		}
	}
	return ret;
}

void Sprite::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (graphics && graphics->hasBounds())
	{
		requestInvalidationFilterParent(q);
		incRef();
		q->addToInvalidateQueue(_MR(this));
	}
	DisplayObjectContainer::requestInvalidation(q,forceTextureRefresh);
}

void DisplayObjectContainer::LegacyChildEraseDeletionMarked()
{
	auto it = legacyChildrenMarkedForDeletion.begin();
	while (it != legacyChildrenMarkedForDeletion.end())
	{
		deleteLegacyChildAt(*it,false);
		it = legacyChildrenMarkedForDeletion.erase(it);
	}
}

void DisplayObjectContainer::rememberLastFrameChildren()
{
	assert(this->is<MovieClip>());
	for (auto it=mapDepthToLegacyChild.begin(); it != mapDepthToLegacyChild.end(); it++)
	{
		if (this->as<MovieClip>()->state.next_FP < it->second->placeFrame)
			continue;
		it->second->incRef();
		it->second->addStoredMember();
		mapFrameDepthToLegacyChildRemembered.insert(make_pair(it->first,it->second));
	}
}

void DisplayObjectContainer::clearLastFrameChildren()
{
	auto it=mapFrameDepthToLegacyChildRemembered.begin();
	while (it != mapFrameDepthToLegacyChildRemembered.end())
	{
		it->second->removeStoredMember();
		it = mapFrameDepthToLegacyChildRemembered.erase(it);
	}
}

DisplayObject* DisplayObjectContainer::getLastFrameChildAtDepth(int depth)
{
	auto it=mapFrameDepthToLegacyChildRemembered.find(depth);
	if (it != mapFrameDepthToLegacyChildRemembered.end())
		return it->second;
	return nullptr;
}

void DisplayObjectContainer::fillGraphicsData(Vector* v, bool recursive)
{
	if (!recursive)
		return;
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->fillGraphicsData(v,recursive);
}

bool DisplayObjectContainer::LegacyChildRemoveDeletionMark(int32_t depth)
{
	auto it = legacyChildrenMarkedForDeletion.find(depth);
	if (it != legacyChildrenMarkedForDeletion.end())
	{
		legacyChildrenMarkedForDeletion.erase(it);
		return true;
	}
	return false;
}

_NR<DisplayObject> DisplayObjectContainer::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;
	bool hit_this=false;
	//Test objects added at runtime, in reverse order
	Locker l(mutexDisplayList);
	auto j=dynamicDisplayList.rbegin();
	for(;j!=dynamicDisplayList.rend();++j)
	{
		//Don't check masks
		if((*j)->isMask() || (*j)->getClipDepth() > 0)
			continue;

		if(!(*j)->getMatrix().isInvertible())
			continue; /* The object is shrunk to zero size */

		const auto childPoint = (*j)->getMatrix().getInverted().multiply2D(localPoint);
		ret=(*j)->hitTest(globalPoint, childPoint,type,interactiveObjectsOnly);
		
		if (!ret.isNull())
		{
			if (interactiveObjectsOnly)
			{
				if (!mouseChildren) // When mouseChildren is false, we should get all events of our children
				{
					if (mouseEnabled)
					{
						this->incRef();
						ret =_MNR(this);
					}
					else
					{
						ret.reset();
						continue;
					}
				}
				else
				{
					if (mouseEnabled)
					{
						if (!ret->is<InteractiveObject>())
						{
							// we have hit a non-interactive object, so "this" may be the hit target
							// but we continue to search the children as there may be an InteractiveObject that is also hit
							hit_this=true;
							ret.reset();
							continue;
						}
						else if (!ret->as<InteractiveObject>()->isHittable(type))
						{
							// hit is a disabled InteractiveObject, so so "this" may be the hit target
							// but we continue to search the children as there may be an enabled InteractiveObject that is also hit
							hit_this=true;
							ret.reset();
							continue;
						}
					}
				}
			}
			break;
		}
	}
	if (hit_this && ret.isNull())
	{
		this->incRef();
		ret =_MNR(this);
	}
	return ret;
}

_NR<DisplayObject> Sprite::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
{
	//Did we hit a child?
	_NR<DisplayObject> ret = NullRef;
	if (dragged) // no hitting when in drag/drop mode
		return ret;
	ret = DisplayObjectContainer::hitTestImpl(globalPoint, localPoint, type,interactiveObjectsOnly);
	if (interactiveObjectsOnly && !hitArea.isNull() &&
		(ret.isNull() || !ret->is<Sprite>() || ret->as<Sprite>()->hitTarget.isNull())) // don't check our hitTarget if we already have a hit on another DisplayObject with a hitTarget
	{
		Vector2f hitPoint;
		// TODO: Add an overload for Vector2f.
		hitArea->globalToLocal(globalPoint.x, globalPoint.y, hitPoint.x, hitPoint.y);
		ret = hitArea->hitTestImpl(globalPoint, hitPoint, type,interactiveObjectsOnly);
		if (!ret.isNull())
		{
			this->incRef();
			ret = _MR(this);
		}
		return ret;
	}
	if (ret.isNull() && hitArea.isNull())
	{
		//The coordinates are locals
		if (graphics && graphics->hasBounds())
		{
			Vector2f hitPoint;
			// TODO: Add an overload for Vector2f.
			this->globalToLocal(globalPoint.x, globalPoint.y, hitPoint.x, hitPoint.y);
			if (graphics->hitTest(Vector2f(localPoint.x,localPoint.y)))
			{
				this->incRef();
				ret = _MR(this);
			}
		}
		if (!ret.isNull())  // we hit the sprite?
		{
			if (!hitTarget.isNull())
			{
				//Another Sprite has registered us
				//as its hitArea -> relay the hit
				ret = hitTarget;
			}
		}
	}

	return ret;
}

void Sprite::resetToStart()
{
	if (sound && this->getTagID() != UINT32_MAX)
	{
		sound->threadAbort();
	}
}

void Sprite::setSound(SoundChannel *s,bool forstreaming)
{
	sound = _MR(s);
	streamingsound = forstreaming;
	if (sound->soundTransform)
		this->soundtransform = sound->soundTransform;
	else
		sound->soundTransform = this->soundtransform;
}

SoundChannel* Sprite::getSoundChannel() const
{
	return sound.getPtr();
}

void Sprite::appendSound(unsigned char *buf, int len, uint32_t frame)
{
	if (sound)
		sound->appendStreamBlock(buf,len);
	if (soundstartframe == UINT32_MAX)
		soundstartframe=frame;
}

void Sprite::checkSound(uint32_t frame)
{
	if (sound && streamingsound && soundstartframe==frame)
		sound->play();
}

void Sprite::stopSound()
{
	if (sound)
		sound->threadAbort();
}

void Sprite::markSoundFinished()
{
	if (sound)
		sound->markFinished();
}

bool Sprite::boundsRectWithoutChildren(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	if (graphics && graphics->hasBounds())
		return graphics->boundsRect(xmin,xmax,ymin,ymax);
	return false;
}

void Sprite::fillGraphicsData(Vector* v, bool recursive)
{
	if (graphics && graphics->hasBounds())
		graphics->fillGraphicsData(v);
	DisplayObjectContainer::fillGraphicsData(v,recursive);
}

ASFUNCTIONBODY_ATOM(Sprite,_constructor)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	// it's possible that legacy MovieClips are defined as inherited directly from Sprite
	// so we have to set initializingFrame here
	th->initializingFrame = true; 
	DisplayObjectContainer::_constructor(ret,wrk,obj,nullptr,0);
	th->initializingFrame = false;
}

Graphics* Sprite::getGraphics()
{
	//Probably graphics is not used often, so create it here
	if(graphics.isNull())
		graphics=_MR(Class<Graphics>::getInstanceS(getInstanceWorker(),this));
	return graphics.getPtr();
}

void Sprite::handleMouseCursor(bool rollover)
{
	if (rollover)
	{
		hasMouse=true;
		if (buttonMode)
			getSystemState()->setMouseHandCursor(this->useHandCursor);
	}
	else
	{
		getSystemState()->setMouseHandCursor(false);
		hasMouse=false;
	}
}

string Sprite::toDebugString() const
{
	std::string res = DisplayObjectContainer::toDebugString();
	if (graphics && graphics->hasBounds())
		res += " hasgraphics";
	return res;
}

ASFUNCTIONBODY_ATOM(Sprite,_getGraphics)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	Graphics* g = th->getGraphics();
	g->incRef();
	ret = asAtomHandler::fromObject(g);
}

void DisplayObjectContainer::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("numChildren","",c->getSystemState()->getBuiltinFunction(_getNumChildren,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("getChildIndex","",c->getSystemState()->getBuiltinFunction(_getChildIndex,1,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setChildIndex","",c->getSystemState()->getBuiltinFunction(_setChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildAt","",c->getSystemState()->getBuiltinFunction(getChildAt,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildByName","",c->getSystemState()->getBuiltinFunction(getChildByName,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getObjectsUnderPoint","",c->getSystemState()->getBuiltinFunction(getObjectsUnderPoint,1,Class<Array>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChild","",c->getSystemState()->getBuiltinFunction(addChild,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChild","",c->getSystemState()->getBuiltinFunction(removeChild,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildAt","",c->getSystemState()->getBuiltinFunction(removeChildAt,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildren","",c->getSystemState()->getBuiltinFunction(removeChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChildAt","",c->getSystemState()->getBuiltinFunction(addChildAt,2,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildren","",c->getSystemState()->getBuiltinFunction(swapChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildrenAt","",c->getSystemState()->getBuiltinFunction(swapChildrenAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",c->getSystemState()->getBuiltinFunction(contains,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",c->getSystemState()->getBuiltinFunction(_setMouseChildren,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",c->getSystemState()->getBuiltinFunction(_getMouseChildren),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, tabChildren);
}

ASFUNCTIONBODY_GETTER_SETTER(DisplayObjectContainer, tabChildren)

DisplayObjectContainer::DisplayObjectContainer(ASWorker* wrk, Class_base* c):InteractiveObject(wrk,c),mouseChildren(true),
	boundsrectXmin(0),boundsrectYmin(0),boundsrectXmax(0),boundsrectYmax(0),boundsRectDirty(true),
	boundsrectVisibleXmin(0),boundsrectVisibleYmin(0),boundsrectVisibleXmax(0),boundsrectVisibleYmax(0),boundsRectVisibleDirty(true),
	tabChildren(true)
{
	subtype=SUBTYPE_DISPLAYOBJECTCONTAINER;
}

void DisplayObjectContainer::markAsChanged()
{
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
		(*it)->markAsChanged();
	DisplayObject::markAsChanged();
}

void DisplayObjectContainer::markBoundsRectDirtyChildren()
{
	markBoundsRectDirty();
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
	{
		if ((*it)->is<DisplayObjectContainer>())
			(*it)->as<DisplayObjectContainer>()->markBoundsRectDirtyChildren();
	}
}

bool DisplayObjectContainer::hasLegacyChildAt(int32_t depth)
{
	auto i = mapDepthToLegacyChild.find(depth);
	return i != mapDepthToLegacyChild.end();
}
DisplayObject* DisplayObjectContainer::getLegacyChildAt(int32_t depth)
{
	return mapDepthToLegacyChild.at(depth);
}


void DisplayObjectContainer::setupClipActionsAt(int32_t depth,const CLIPACTIONS& actions)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"setupClipActionsAt: no child at depth "<<depth);
		return;
	}
	DisplayObject* o = getLegacyChildAt(depth);
	if (o->is<MovieClip>())
		o->as<MovieClip>()->setupActions(actions);
}

void DisplayObjectContainer::checkRatioForLegacyChildAt(int32_t depth,uint32_t ratio,bool inskipping)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"checkRatioForLegacyChildAt: no child at that depth "<<depth<<" "<<this->toDebugString());
		return;
	}
	mapDepthToLegacyChild.at(depth)->checkRatio(ratio,inskipping);
	this->hasChanged=true;
}
void DisplayObjectContainer::checkColorTransformForLegacyChildAt(int32_t depth,const CXFORMWITHALPHA& colortransform)
{
	if(!hasLegacyChildAt(depth))
		return;
	DisplayObject* o = mapDepthToLegacyChild.at(depth);
	if (o->colorTransform.isNull())
		o->colorTransform=_NR<ColorTransform>(Class<ColorTransform>::getInstanceS(getInstanceWorker(),colortransform));
	else
		o->colorTransform->setProperties(colortransform);
	markAsChanged();
}

void DisplayObjectContainer::deleteLegacyChildAt(int32_t depth, bool inskipping)
{
	if(!hasLegacyChildAt(depth))
		return;
	DisplayObject* obj = mapDepthToLegacyChild.at(depth);
	if(obj->name != BUILTIN_STRINGS::EMPTY 
	   && !obj->markedForLegacyDeletion) // member variable was already reset in purgeLegacyChildren
	{
		//The variable is not deleted, but just set to null
		//This is a tested behavior
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=obj->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		setVariableByMultiname(objName,needsActionScript3() ? asAtomHandler::nullAtom : asAtomHandler::undefinedAtom, ASObject::CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
		
	}
	obj->placeFrame=UINT32_MAX;
	obj->afterLegacyDelete(inskipping);
	//this also removes it from depthToLegacyChild
	_removeChild(obj,false,inskipping);
}

void DisplayObjectContainer::insertLegacyChildAt(int32_t depth, DisplayObject* obj, bool inskipping, bool fromtag)
{
	if(hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"insertLegacyChildAt: there is already one child at that depth");
		return;
	}
	
	uint32_t insertpos = 0;
	// find DisplayObject to insert obj after
	DisplayObject* preobj=nullptr;
	for (auto it = mapDepthToLegacyChild.begin(); it != mapDepthToLegacyChild.end();it++)
	{
		if (it->first > depth)
			break;
		preobj = it->second;
	}
	if (preobj)
	{
		auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),preobj);
		if(it!=dynamicDisplayList.end())
		{
			insertpos = it-dynamicDisplayList.begin()+1;
			it++;
			if(fromtag)
			{
				while(it!=dynamicDisplayList.end() && !(*it)->legacy)
				{
					it++; // skip all children previously added through actionscript
					insertpos++;
				}
			}
		}
	}
	if (!this->loadedFrom->usesActionScript3 && obj->legacy && obj->name == BUILTIN_STRINGS::EMPTY && obj->is<InteractiveObject>())
	{
		// AVM1 seems to assign a unique name "instance{x}" to all children that
		// - are InteractiveObjects (?) and
		// - don't have a name and
		// - are added by tags
		
		int32_t instancenum = ATOMIC_INCREMENT(getSystemState()->instanceCounter);
		char buf[100];
		sprintf(buf,"instance%i",instancenum);
		tiny_string s(buf);
		obj->name = getSystemState()->getUniqueStringId(s);
	}
	if(obj->name != BUILTIN_STRINGS::EMPTY)
	{
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=obj->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		bool set=true;
		if (!loadedFrom->usesActionScript3 && !obj->legacy)
		{
			variable* v = this->findVariableByMultiname(objName,this->getClass(),nullptr,nullptr,true,loadedFrom->getInstanceWorker());
			if (v && asAtomHandler::is<DisplayObject>(v->var))
			{
				// it seems that in AVM1 the variable for a named child is not set if
				// - a variable with the same name already exists and
				// - that variable is a DisplayObject and
				// - the new displayobject was constructed from actionscript and
				// - the depth of the existing DisplayObject is lower than that of the new DisplayObject
				auto it = this->mapLegacyChildToDepth.find(asAtomHandler::as<DisplayObject>(v->var));
				if (it != this->mapLegacyChildToDepth.end() && it->second < depth)
					set = false;
			}
		}
		if (set)
		{
			obj->incRef();
			asAtom v = asAtomHandler::fromObject(obj);
			setVariableByMultiname(objName,v,ASObject::CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
		}
	}
	_addChildAt(obj,insertpos,inskipping);

	mapDepthToLegacyChild.insert(make_pair(depth,obj));
	mapLegacyChildToDepth.insert(make_pair(obj,depth));
	obj->afterLegacyInsert();
}
DisplayObject *DisplayObjectContainer::findLegacyChildByTagID(uint32_t tagid)
{
	auto it = mapLegacyChildToDepth.begin();
	while (it != mapLegacyChildToDepth.end())
	{
		if (it->first->getTagID() == tagid)
			return it->first;
		it++;
	}
	return nullptr;
}
int DisplayObjectContainer::findLegacyChildDepth(DisplayObject *obj)
{
	auto it = mapLegacyChildToDepth.find(obj);
	if (it != mapLegacyChildToDepth.end())
		return it->second;
	return 0;
}

void DisplayObjectContainer::transformLegacyChildAt(int32_t depth, const MATRIX& mat)
{
	if(!hasLegacyChildAt(depth))
		return;
	mapDepthToLegacyChild.at(depth)->setLegacyMatrix(mat);
}

void DisplayObjectContainer::purgeLegacyChildren()
{
	auto i = mapDepthToLegacyChild.begin();
	while( i != mapDepthToLegacyChild.end() )
	{
		if (i->first < 0 && is<MovieClip>() && i->second->placeFrame > as<MovieClip>()->state.FP)
		{
			legacyChildrenMarkedForDeletion.insert(i->first);
			DisplayObject* obj = i->second;
			obj->markedForLegacyDeletion=true;
			if(obj->name != BUILTIN_STRINGS::EMPTY)
			{
				multiname objName(nullptr);
				objName.name_type=multiname::NAME_STRING;
				objName.name_s_id=obj->name;
				objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
				setVariableByMultiname(objName,needsActionScript3() ? asAtomHandler::nullAtom : asAtomHandler::undefinedAtom,ASObject::CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
			}
		}
		i++;
	}
}
uint32_t DisplayObjectContainer::getMaxLegacyChildDepth()
{
	auto it = mapDepthToLegacyChild.begin();
	int32_t max =-1;
	while (it !=mapDepthToLegacyChild.end())
	{
		if (it->first > max)
			max = it->first;
		it++;
	}
	return max >= 0 ? max : UINT32_MAX;
}

void DisplayObjectContainer::checkClipDepth()
{
	DisplayObject* clipobj = nullptr;
	int depth = 0;
	for (auto it=mapDepthToLegacyChild.begin(); it != mapDepthToLegacyChild.end(); it++)
	{
		DisplayObject* obj = it->second;
		depth = it->first;
		if (obj->ClipDepth)
		{
			clipobj = obj;
		}
		else if (clipobj && clipobj->ClipDepth > depth)
		{
			clipobj->incRef();
			obj->setClipMask(_NR<DisplayObject>(clipobj));
		}
		else
			obj->setClipMask(NullRef);
	}
}

bool DisplayObjectContainer::destruct()
{
	// clear all member variables in the display list first to properly handle cyclic reference detection
	prepareDestruction();
	clearDisplayList();
	mouseChildren = true;
	boundsRectDirty = true;
	boundsRectVisibleDirty = true;
	tabChildren = true;
	legacyChildrenMarkedForDeletion.clear();
	mapDepthToLegacyChild.clear();
	mapLegacyChildToDepth.clear();
	return InteractiveObject::destruct();
}

void DisplayObjectContainer::finalize()
{
	// clear all member variables in the display list first to properly handle cyclic reference detection
	prepareDestruction();
	clearDisplayList();
	legacyChildrenMarkedForDeletion.clear();
	mapDepthToLegacyChild.clear();
	mapLegacyChildToDepth.clear();
	InteractiveObject::finalize();
}

void DisplayObjectContainer::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	InteractiveObject::prepareShutdown();
	for (int i = dynamicDisplayList.size()-1; i >= 0; i--)
	{
		DisplayObject* d = dynamicDisplayList[i];
		dynamicDisplayList.pop_back();
		d->setParent(nullptr);
		getSystemState()->removeFromResetParentList(d);
		d->prepareShutdown();
		d->removeStoredMember();
	}
}

bool DisplayObjectContainer::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = InteractiveObject::countCylicMemberReferences(gcstate);
	Locker l(mutexDisplayList);
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
	{
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;
}

void DisplayObjectContainer::cloneDisplayList(std::vector<Ref<DisplayObject> >& displayListCopy)
{
	Locker l(mutexDisplayList);
	displayListCopy.reserve(dynamicDisplayList.size());
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
	{
		(*it)->incRef();
		displayListCopy.push_back(_MR(*it));
	}
}

InteractiveObject::InteractiveObject(ASWorker* wrk, Class_base* c):DisplayObject(wrk,c),mouseEnabled(true),doubleClickEnabled(false),accessibilityImplementation(NullRef),contextMenu(NullRef),tabEnabled(false),tabIndex(-1)
{
	subtype=SUBTYPE_INTERACTIVE_OBJECT;
}

void InteractiveObject::setOnStage(bool staged, bool force,bool inskipping)
{
	if (!staged)
		getSystemState()->stage->checkResetFocusTarget(this);
	DisplayObject::setOnStage(staged,force,inskipping);
}
InteractiveObject::~InteractiveObject()
{
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_setMouseEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	assert_and_throw(argslen==1);
	th->mouseEnabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_getMouseEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	asAtomHandler::setBool(ret,th->mouseEnabled);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_setDoubleClickEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	assert_and_throw(argslen==1);
	th->doubleClickEnabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_getDoubleClickEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	asAtomHandler::setBool(ret,th->doubleClickEnabled);
}

bool InteractiveObject::destruct()
{
	if (contextMenu)
	{
		contextMenu->removeStoredMember();
		contextMenu.fakeRelease();
	}
	mouseEnabled = true;
	doubleClickEnabled =false;
	accessibilityImplementation.reset();
	focusRect.reset();
	tabEnabled = false;
	tabIndex = -1;
	return DisplayObject::destruct();
}
void InteractiveObject::finalize()
{
	if (contextMenu)
	{
		contextMenu->removeStoredMember();
		contextMenu.fakeRelease();
	}
	accessibilityImplementation.reset();
	focusRect.reset();
	DisplayObject::finalize();
}

void InteractiveObject::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObject::prepareShutdown();
	if (contextMenu)
		contextMenu->prepareShutdown();
	if (accessibilityImplementation)
		accessibilityImplementation->prepareShutdown();
	if (focusRect)
		focusRect->prepareShutdown();
}

bool InteractiveObject::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = DisplayObject::countCylicMemberReferences(gcstate);
	if (contextMenu)
		ret = contextMenu->countAllCylicMemberReferences(gcstate) || ret;
	if (accessibilityImplementation)
		ret = accessibilityImplementation->countAllCylicMemberReferences(gcstate) || ret;
	if (focusRect)
		ret = focusRect->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void InteractiveObject::defaultEventBehavior(Ref<Event> e)
{
	if(mouseEnabled && e->type == "contextMenu")
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_OPEN_CONTEXTMENU;
		event.user.data1 = (void*)this;
		SDL_PushEvent(&event);
	}
}

_NR<InteractiveObject> InteractiveObject::getCurrentContextMenuItems(std::vector<_R<NativeMenuItem>>& items)
{
	if (this->contextMenu.isNull() || !this->contextMenu->is<ContextMenu>())
	{
		if (this->getParent())
			return getParent()->getCurrentContextMenuItems(items);
		ContextMenu::getVisibleBuiltinContextMenuItems(nullptr,items,getInstanceWorker());
	}
	else
		this->contextMenu->as<ContextMenu>()->getCurrentContextMenuItems(items);
	this->incRef();
	return _MR<InteractiveObject>(this);
}

void InteractiveObject::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("mouseEnabled","",c->getSystemState()->getBuiltinFunction(_setMouseEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseEnabled","",c->getSystemState()->getBuiltinFunction(_getMouseEnabled,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",c->getSystemState()->getBuiltinFunction(_setDoubleClickEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",c->getSystemState()->getBuiltinFunction(_getDoubleClickEnabled,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, accessibilityImplementation,AccessibilityImplementation);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, contextMenu,ASObject);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, tabEnabled,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, tabIndex,Integer);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, focusRect,ASObject);
}

ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, accessibilityImplementation)
ASFUNCTIONBODY_GETTER_SETTER_CB(InteractiveObject, contextMenu,onContextMenu)
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabEnabled)
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabIndex)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(InteractiveObject, focusRect) // stub

void InteractiveObject::onContextMenu(_NR<ASObject> oldValue)
{
	if (oldValue)
	{
		oldValue.fakeRelease();
		oldValue->removeStoredMember();
	}
	if (this->contextMenu->is<ContextMenu>())
	{
		this->contextMenu->as<ContextMenu>()->owner = this;
		this->contextMenu->addStoredMember();
	}
}

void DisplayObjectContainer::dumpDisplayList(unsigned int level)
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
		    ((*it)->isMask() ? "m" : "") <<((*it)->hasFilters() ? "f" : "") <<((*it)->scrollRect.getPtr() ? "s" : "") << " cd=" <<(*it)->ClipDepth<<" "<<
		    "a=" << (*it)->clippedAlpha() <<" '"<<getSystemState()->getStringFromUniqueId((*it)->name)<<"'"<<" depth:"<<(*it)->getDepth()<<" blendmode:"<<(*it)->getBlendMode()<<
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

void DisplayObjectContainer::setOnStage(bool staged, bool force,bool inskipping)
{
	if(staged!=onStage||force)
	{
		//Make a copy of display list before calling setOnStage
		std::vector<_R<DisplayObject>> displayListCopy;
		cloneDisplayList(displayListCopy);
		InteractiveObject::setOnStage(staged,force,inskipping);
		//Notify children
		//calling InteractiveObject::setOnStage may have changed the onStage state of the children,
		//but the addedToStage/removedFromStage event must always be dispatched
		auto it=displayListCopy.begin();
		for(;it!=displayListCopy.end();++it)
			(*it)->setOnStage(staged,force,inskipping);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_constructor)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	InteractiveObject::_constructor(ret,wrk,obj,nullptr,0);
	if (th->needsActionScript3())
	{
		std::vector<_R<DisplayObject>> list;
		th->cloneDisplayList(list);
		for (auto child : list)
			child->initFrame();
	}
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getNumChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	asAtomHandler::setInt(ret,wrk,(int32_t)th->dynamicDisplayList.size());
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getMouseChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	asAtomHandler::setBool(ret,th->mouseChildren);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_setMouseChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	th->mouseChildren=asAtomHandler::Boolean_concrete(args[0]);
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
	incRef();
	q->addToInvalidateQueue(_MR(this));
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
	
	ColorTransformBase ct;
	if (this->colorTransform)
		ct=*this->colorTransform.getPtr();
	
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
	return res;
}
void DisplayObjectContainer::invalidateForRenderToBitmap(RenderDisplayObjectToBitmapContainer* container)
{
	DisplayObject::invalidateForRenderToBitmap(container);
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		(*it)->invalidateForRenderToBitmap(container);
	}
}
void DisplayObjectContainer::_addChildAt(DisplayObject* child, unsigned int index, bool inskipping)
{
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	if(child->getParent() && !getSystemState()->isInResetParentList(child))
	{
		//Child already in this container, set to new position
		if(child->getParent()==this)
		{
			setChildIndexIntern(child,index);
			return;
		}
		else
		{
			child->getParent()->_removeChild(child,inskipping,this->isOnStage());
		}
	}
	getSystemState()->removeFromResetParentList(child);
	if(!child->needsActionScript3())
		getSystemState()->stage->AVM1AddDisplayObject(child);
	child->setParent(this);
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
	if (!onStage || child != getSystemState()->mainClip)
		child->setOnStage(onStage,false,inskipping);
	
	if (isOnStage())
		this->requestInvalidation(getSystemState());
}

void DisplayObjectContainer::handleRemovedEvent(DisplayObject* child, bool keepOnStage, bool inskipping)
{
	_R<Event> e=_MR(Class<Event>::getInstanceS(child->getInstanceWorker(),"removed"));
	if (isVmThread())
		ABCVm::publicHandleEvent(child, e);
	else
	{
		child->incRef();
		getVm(getSystemState())->addEvent(_MR(child), e);
	}
	if (!keepOnStage && (child->isOnStage() || !child->getStage().isNull()))
		child->setOnStage(false, true, inskipping);
}

bool DisplayObjectContainer::_removeChild(DisplayObject* child,bool direct,bool inskipping, bool keeponstage)
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

	{
		Locker l(mutexDisplayList);
		auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);

		if (direct || !this->isOnStage() || inskipping )
			child->setParent(nullptr);
		else if (!isOnStage() || !isVmThread())
			getSystemState()->addDisplayObjectToResetParentList(child);
		child->setMask(NullRef);
		
		//Erase this from the legacy child map (if it is in there)
		umarkLegacyChild(child);
		dynamicDisplayList.erase(it);
	}
	handleRemovedEvent(child, keeponstage, inskipping);
	this->hasChanged=true;
	this->requestInvalidation(getSystemState());
	child->setParent(nullptr);
	getSystemState()->stage->prepareForRemoval(child);
	checkClipDepth();
	return true;
}

void DisplayObjectContainer::_removeAllChildren()
{
	Locker l(mutexDisplayList);
	auto it=dynamicDisplayList.begin();
	while (it!=dynamicDisplayList.end())
	{
		DisplayObject* child = *it;
		child->setOnStage(false,false);
		getSystemState()->addDisplayObjectToResetParentList(child);
		child->setMask(NullRef);
		if (!needsActionScript3())
			child->removeAVM1Listeners();

		//Erase this from the legacy child map (if it is in there)
		umarkLegacyChild(child);
		it = dynamicDisplayList.erase(it);
		getSystemState()->stage->prepareForRemoval(child);
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

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,contains)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(!asAtomHandler::is<DisplayObject>(args[0]))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	bool res=th->_contains(d);
	asAtomHandler::setBool(ret,res);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==2);
	if(asAtomHandler::isClass(args[0]) || asAtomHandler::isNull(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	int index=asAtomHandler::toInt(args[1]);

	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	assert_and_throw(index >= 0 && (size_t)index<=th->dynamicDisplayList.size());
	d->incRef();
	th->_addChildAt(d,index);

	//Notify the object
	d->incRef();
	getVm(wrk->getSystemState())->addEvent(_MR(d),_MR(Class<Event>::getInstanceS(wrk,"added")));

	//incRef again as the value is getting returned
	d->incRef();
	ret = asAtomHandler::fromObject(d);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::isClass(args[0]) || asAtomHandler::isNull(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	d->incRef();
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	d->incRef();
	getVm(wrk->getSystemState())->addEvent(_MR(d),_MR(Class<Event>::getInstanceS(wrk,"added")));

	d->incRef();
	ret = asAtomHandler::fromObject(d);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChild)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(!asAtomHandler::is<DisplayObject>(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	//As we return the child we have to incRef it
	d->incRef();

	if(!th->_removeChild(d))
	{
		createError<ArgumentError>(wrk,2025,"removeChild: child not in list");
		return;
	}

	ret = asAtomHandler::fromObject(d);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	int32_t index=asAtomHandler::toInt(args[0]);

	DisplayObject* child=nullptr;
	{
		Locker l(th->mutexDisplayList);
		if(index>=int(th->dynamicDisplayList.size()) || index<0)
		{
			createError<RangeError>(wrk,2025,"removeChildAt: invalid index");
			return;
		}
		auto it=th->dynamicDisplayList.begin();
		for(int32_t i=0;i<index;i++)
			++it;
		child=(*it);
	}
	//As we return the child we incRef it
	child->incRef();
	th->_removeChild(child);
	ret = asAtomHandler::fromObject(child);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChildren)
{
	uint32_t beginindex;
	uint32_t endindex;
	ARG_CHECK(ARG_UNPACK(beginindex,0)(endindex,0x7fffffff));
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	{
		Locker l(th->mutexDisplayList);
		if (endindex > th->dynamicDisplayList.size())
			endindex = (uint32_t)th->dynamicDisplayList.size();
		auto it = th->dynamicDisplayList.begin()+beginindex;
		while (it != th->dynamicDisplayList.begin()+endindex)
		{
			(*it)->removeStoredMember();
			it++;
		}
		th->dynamicDisplayList.erase(th->dynamicDisplayList.begin()+beginindex,th->dynamicDisplayList.begin()+endindex);
	}
	th->requestInvalidation(th->getSystemState());
}
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_setChildIndex)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	_NR<DisplayObject> ch;
	int32_t index;
	ARG_CHECK(ARG_UNPACK(ch)(index))
	if (ch.isNull() || ch->getParent() != th)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError);
		return;
	}
	Locker l(th->mutexDisplayList);
	if (index < 0 || index > (int)th->dynamicDisplayList.size())
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	DisplayObject* child = ch.getPtr();
	th->setChildIndexIntern(child, index);
}
void DisplayObjectContainer::setChildIndexIntern(DisplayObject *child, int index)
{
	int curIndex = this->getChildIndex(child);
	if(curIndex == index || curIndex < 0)
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

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,swapChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==2);
	
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[1]));

	if (asAtomHandler::getObject(args[0]) == asAtomHandler::getObject(args[1]))
	{
		// Must return, otherwise crashes trying to erase the
		// same object twice
		return;
	}

	//Cast to object
	DisplayObject* child1=asAtomHandler::as<DisplayObject>(args[0]);
	DisplayObject* child2=asAtomHandler::as<DisplayObject>(args[1]);

	{
		Locker l(th->mutexDisplayList);
		auto it1=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child1);
		auto it2=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child2);
		if(it1==th->dynamicDisplayList.end() || it2==th->dynamicDisplayList.end())
		{
			createError<ArgumentError>(wrk,2025,"Argument is not child of this object");
			return;
		}

		std::iter_swap(it1, it2);
	}
	//Erase both children from the legacy child map
	th->umarkLegacyChild(child1);
	th->umarkLegacyChild(child2);
	
	th->checkClipDepth();
	th->requestInvalidation(th->getSystemState());
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,swapChildrenAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	int index1;
	int index2;
	ARG_CHECK(ARG_UNPACK(index1)(index2));

	if ((index1 < 0) || (index1 > (int)th->dynamicDisplayList.size()) ||
		(index2 < 0) || (index2 > (int)th->dynamicDisplayList.size()))
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	if (index1 == index2)
	{
		return;
	}

	{
		Locker l(th->mutexDisplayList);
		std::iter_swap(th->dynamicDisplayList.begin() + index1, th->dynamicDisplayList.begin() + index2);
	}
	//Erase both children from the legacy child map
	th->umarkLegacyChild(*(th->dynamicDisplayList.begin() + index1));
	th->umarkLegacyChild(*(th->dynamicDisplayList.begin() + index2));
	
	th->checkClipDepth();
	th->requestInvalidation(th->getSystemState());
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getChildByName)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	uint32_t wantedName=asAtomHandler::toStringId(args[0],wrk);
	auto it=th->dynamicDisplayList.begin();
	ASObject* res=nullptr;
	for(;it!=th->dynamicDisplayList.end();++it)
	{
		if((*it)->name==wantedName)
		{
			res=(*it);
			break;
		}
	}
	if(res)
	{
		res->incRef();
		ret = asAtomHandler::fromObject(res);
	}
	else
		asAtomHandler::setUndefined(ret);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	unsigned int index=asAtomHandler::toInt(args[0]);
	if(index>=th->dynamicDisplayList.size())
	{
		createError<RangeError>(wrk,2025,"getChildAt: invalid index");
		return;
	}
	auto it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		++it;
	(*it)->incRef();
	ret = asAtomHandler::fromObject(*it);
}

int DisplayObjectContainer::getChildIndex(DisplayObject* child)
{
	auto it = dynamicDisplayList.begin();
	int ret = 0;
	do
	{
		if(it == dynamicDisplayList.end())
		{
			createError<ArgumentError>(getInstanceWorker(),2025,"getChildIndex: child not in list");
			return -1;
		}
		if(*it == child)
			break;
		ret++;
		++it;
	}
	while(1);
	return ret;
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getChildIndex)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	//Cast to object
	DisplayObject* d= asAtomHandler::as<DisplayObject>(args[0]);

	asAtomHandler::setInt(ret,wrk,th->getChildIndex(d));
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getObjectsUnderPoint)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	_NR<Point> point;
	ARG_CHECK(ARG_UNPACK(point));
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	if (!point.isNull())
		th->getObjectsFromPoint(point.getPtr(),res);
	ret = asAtomHandler::fromObject(res);
}

void DisplayObjectContainer::getObjectsFromPoint(Point* point, Array *ar)
{
	number_t xmin,xmax,ymin,ymax;
	MATRIX m;
	{
		Locker l(mutexDisplayList);
		auto it = dynamicDisplayList.begin();
		while (it != dynamicDisplayList.end())
		{
			(*it)->incRef();
			if ((*it)->getBounds(xmin,xmax,ymin,ymax,m))
			{
				if (xmin <= point->getX() && xmax >= point->getX()
						&& ymin <= point->getY() && ymax >= point->getY())
				{
					(*it)->incRef();
					ar->push(asAtomHandler::fromObject(*it));
				}
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
		c->setParent(nullptr);
		getSystemState()->removeFromResetParentList(c);
		dynamicDisplayList.pop_back();
		it = dynamicDisplayList.rbegin();
		c->removeStoredMember();
	}
}


void GradientType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"linear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RADIAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"radial"),CONSTANT_TRAIT);
}

void BlendMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ADD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"add"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"alpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DARKEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"darken"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DIFFERENCE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"difference"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ERASE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"erase"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HARDLIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"hardlight"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVERT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invert"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LAYER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"layer"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LIGHTEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"lighten"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MULTIPLY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"multiply"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OVERLAY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"overlay"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"screen"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SUBTRACT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"subtract"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHADER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"shader"),CONSTANT_TRAIT);
}

void SpreadMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("PAD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pad"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REFLECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"reflect"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REPEAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"repeat"),CONSTANT_TRAIT);
}

void InterpolationMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("RGB",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rgb"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINEAR_RGB",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"linearRGB"),CONSTANT_TRAIT);
}

void GraphicsPathCommand::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CUBIC_CURVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CURVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINE_TO",nsNameAndKind(),asAtomHandler::fromUInt(2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MOVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_OP",nsNameAndKind(),asAtomHandler::fromUInt(0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WIDE_LINE_TO",nsNameAndKind(),asAtomHandler::fromUInt(5),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WIDE_MOVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(4),CONSTANT_TRAIT);
}

void GraphicsPathWinding::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("EVEN_ODD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"evenOdd"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NON_ZERO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"nonZero"),CONSTANT_TRAIT);
}

void PixelSnapping::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALWAYS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"always"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"never"),CONSTANT_TRAIT);

}

void CapsStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"round"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQUARE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"square"),CONSTANT_TRAIT);
}

void JointStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BEVEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bevel"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MITER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"miter"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"round"),CONSTANT_TRAIT);
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
	/* init the frames and call constructors of our children first */

	// elements of the dynamicDisplayList may be removed during initFrame() calls,
	// so we create a temporary list containing all elements
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->initFrame();
	/* call our own constructor, if necassary */
	DisplayObject::initFrame();
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

multiname *DisplayObjectContainer::setVariableByMultiname(multiname& name, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk)
{
// TODO I disable this for now as gamesmenu from homestarrunner doesn't work with it (I don't know which swf file required this...)
//	if (asAtomHandler::is<DisplayObject>(o))
//	{
//		// it seems that setting a new value for a named existing dynamic child removes the child from the display list
//		variable* v = findVariableByMultiname(name,this->getClass());
//		if (v && v->kind == TRAIT_KIND::DYNAMIC_TRAIT && asAtomHandler::is<DisplayObject>(v->var))
//		{
//			DisplayObject* obj = asAtomHandler::as<DisplayObject>(v->var);
//			if (!obj->legacy)
//			{
//				if (v->var.uintval == o.uintval)
//					return nullptr;
//				obj->incRef();
//				_removeChild(obj);
//			}
//		}
//	}
	return InteractiveObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
}

bool DisplayObjectContainer::deleteVariableByMultiname(const multiname &name, ASWorker* wrk)
{
	variable* v = findVariableByMultiname(name,this->getClass(),nullptr,nullptr,false,wrk);
	if (v && v->kind == TRAIT_KIND::DYNAMIC_TRAIT && asAtomHandler::is<DisplayObject>(v->var))
	{
		DisplayObject* obj = asAtomHandler::as<DisplayObject>(v->var);
		if (!obj->legacy)
		{
			_removeChild(obj);
		}
	}
	return InteractiveObject::deleteVariableByMultiname(name,wrk);
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

AVM1Movie::AVM1Movie(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c)
{
	subtype=SUBTYPE_AVM1MOVIE;
}

void AVM1Movie::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(AVM1Movie,_constructor)
{
	DisplayObject::_constructor(ret,wrk,obj,nullptr,0);
}

void Shader::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(Shader,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "Shader class is unimplemented.");
}

void BitmapDataChannel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALPHA",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::ALPHA),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BLUE",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::BLUE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREEN",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::GREEN),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RED",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::RED),CONSTANT_TRAIT);
}

unsigned int BitmapDataChannel::channelShift(uint32_t channelConstant)
{
	unsigned int shift;
	switch (channelConstant)
	{
		case BitmapDataChannel::ALPHA:
			shift = 24;
			break;
		case BitmapDataChannel::RED:
			shift = 16;
			break;
		case BitmapDataChannel::GREEN:
			shift = 8;
			break;
		case BitmapDataChannel::BLUE:
		default: // check
			shift = 0;
			break;
	}

	return shift;
}

void LineScaleMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("HORIZONTAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"horizontal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VERTICAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"vertical"),CONSTANT_TRAIT);
}

void ActionScriptVersion::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ACTIONSCRIPT2",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ACTIONSCRIPT3",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)3),CONSTANT_TRAIT);
}

void AVM1scriptToExecute::execute()
{
	std::map<uint32_t, asAtom> scopevariables;
	if (actions)
		ACTIONRECORD::executeActions(clip,avm1context,*actions, startactionpos,scopevariables);
	if (this->event_name_id != UINT32_MAX)
	{
		asAtom func=asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id= this->event_name_id;
		clip->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,clip->getInstanceWorker());
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtom ret=asAtomHandler::invalidAtom;
			asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			asAtomHandler::as<AVM1Function>(func)->decRef();
		}
	}
	clip->decRef(); // was increffed in AVM1AddScriptEvents
}
