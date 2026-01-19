/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "compat.h"
#include "swf.h"
#include "scripting/flash/display/DisplayObject.h"
#include "backends/rendering_context.h"
#include "backends/cachedsurface.h"
#include "backends/input.h"
#include "scripting/argconv.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Matrix3D.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/filters/BevelFilter.h"
#include "scripting/flash/filters/BlurFilter.h"
#include "scripting/flash/filters/ColorMatrixFilter.h"
#include "scripting/flash/filters/ConvolutionFilter.h"
#include "scripting/flash/filters/DropShadowFilter.h"
#include "scripting/flash/filters/GlowFilter.h"
#include "scripting/flash/filters/GradientBevelFilter.h"
#include "scripting/flash/filters/GradientGlowFilter.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/avm1/avm1display.h"
#include "utils/array.h"
#include <algorithm>
#include <array>

// adobe seems to use twips as the base of the internal coordinate system, so we have to "round" coordinates to twips
// TODO I think we should also use a twips-based coordinate system
#define ROUND_TO_TWIPS(v) v = number_t(int(v*20))/20.0

using namespace lightspark;
using namespace std;

ATOMIC_INT32(DisplayObject::instanceCount);

Vector2f DisplayObject::getXY()
{
	Vector2f ret;
	ret.x = getMatrix().getTranslateX();
	ret.y = getMatrix().getTranslateY();
	return ret;
}

bool DisplayObject::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, const MATRIX& m, bool visibleOnly)
{
	if(!legacy && !isConstructed())
		return false;

	bool ret=boundsRect(xmin,xmax,ymin,ymax,visibleOnly);
	if(ret)
	{
		number_t tmpX[4];
		number_t tmpY[4];
		m.multiply2D(xmin,ymin,tmpX[0],tmpY[0]);
		m.multiply2D(xmax,ymin,tmpX[1],tmpY[1]);
		m.multiply2D(xmax,ymax,tmpX[2],tmpY[2]);
		m.multiply2D(xmin,ymax,tmpX[3],tmpY[3]);
		auto retX=minmax_element(tmpX,tmpX+4);
		auto retY=minmax_element(tmpY,tmpY+4);
		xmin=*retX.first;
		xmax=*retX.second;
		ymin=*retY.first;
		ymax=*retY.second;
	}
	return ret;
}

number_t DisplayObject::getNominalWidth()
{
	number_t xmin, xmax, ymin, ymax;

	if(!isConstructed())
		return 0;

	bool ret=boundsRect(xmin,xmax,ymin,ymax,false);
	return ret?(xmax-xmin):0;
}

number_t DisplayObject::getNominalHeight()
{
	number_t xmin, xmax, ymin, ymax;

	if(!isConstructed())
		return 0;

	bool ret=boundsRect(xmin,xmax,ymin,ymax,false);
	return ret?(ymax-ymin):0;
}

bool DisplayObject::inMask() const
{
	if (mask || this->getClipDepth())
		return true;
	if (parent)
		return parent->inMask();
	return false;
}
bool DisplayObject::belongsToMask() const
{
	if (ismaskCount)
		return true;
	if (parent)
		return parent->belongsToMask();
	return false;
}

void DisplayObject::addBroadcastEventListener()
{
	if (broadcastEventListenerCount==0)
		getSystemState()->registerFrameListener(this);
	++broadcastEventListenerCount;
}

void DisplayObject::removeBroadcastEventListener()
{
	assert(broadcastEventListenerCount);
	--broadcastEventListenerCount;
	if (broadcastEventListenerCount==0)
		getSystemState()->unregisterFrameListener(this);
}

DisplayObject::DisplayObject(ASWorker* wrk, Class_base* c):EventDispatcher(wrk,c),matrix(Class<Matrix>::getInstanceS(wrk)),tx(0),ty(0),rotation(0),
	sx(1),sy(1),blendMode(BLENDMODE_NORMAL),
	isLoadedRoot(false),inInitFrame(false),
	filterlistHasChanged(false),ismaskCount(0),maxfilterborder(0),ClipDepth(0),hasDefaultName(false),
	hiddenPrevDisplayObject(nullptr),hiddenNextDisplayObject(nullptr),avm1PrevDisplayObject(nullptr),avm1NextDisplayObject(nullptr),parent(nullptr),cachedSurface(new CachedSurface()),
	constructed(false),useLegacyMatrix(true),
	needsTextureRecalculation(true),textureRecalculationSkippable(false),
	avm1mouselistenercount(0),avm1framelistenercount(0),broadcastEventListenerCount(0),
	onStage(false),visible(true),
	mask(nullptr),clipMask(nullptr),
	invalidateQueueNext(),
	loaderInfo(nullptr),
	scrollRect(asAtomHandler::undefinedAtom),
	loadedFrom(nullptr),hasChanged(true),legacy(false),placeFrame(UINT32_MAX),markedForLegacyDeletion(false),cacheAsBitmap(false),placedByActionScript(false),skipFrame(false),
	name(UINT32_MAX),
	opaqueBackground(asAtomHandler::nullAtom),
	metaData(asAtomHandler::nullAtom)
{
	subtype=SUBTYPE_DISPLAYOBJECT;
	if (wrk->rootClip)
		loadedFrom = wrk->rootClip->applicationDomain.getPtr();
}

void DisplayObject::markAsChanged()
{
	hasChanged = true;
	if (onStage)
		requestInvalidation(getSystemState());
	else
		requestInvalidationFilterParent();
}

DisplayObject::~DisplayObject()
{
}

void DisplayObject::finalize()
{
	getSystemState()->stage->AVM1RemoveDisplayObject(this);
	getSystemState()->stage->removeHiddenObject(this);
	removeAVM1Listeners();
	EventDispatcher::finalize();
	setParent(nullptr);
	eventparentmap.clear();
	if (mask)
		mask->removeStoredMember();
	mask=nullptr;
	if (clipMask)
		clipMask->removeStoredMember();
	clipMask=nullptr;
	matrix.reset();
	if (loaderInfo)
		loaderInfo->removeStoredMember();
	loaderInfo=nullptr;
	colorTransform.reset();
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
	scalingGrid.reset();
	ASATOM_REMOVESTOREDMEMBER(opaqueBackground);
	opaqueBackground=asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(metaData);
	metaData=asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(scrollRect);
	scrollRect=asAtomHandler::undefinedAtom;
	for (auto it = avm1variables.begin(); it != avm1variables.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->removeStoredMember();
	}
	avm1variables.clear();
	for (auto it = avm1locals.begin(); it != avm1locals.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
	}
	avm1locals.clear();
	variablebindings.clear();
	loadedFrom=getSystemState()->mainClip->applicationDomain.getPtr();
	hasChanged = true;
	needsTextureRecalculation=true;
	avm1mouselistenercount=0;
	avm1framelistenercount=0;
	if (broadcastEventListenerCount)
		getSystemState()->unregisterFrameListener(this);
	broadcastEventListenerCount=0;
	filters.reset();
	EventDispatcher::finalize();
}

bool DisplayObject::destruct()
{
	// TODO make all DisplayObject derived classes reusable
	getSystemState()->stage->AVM1RemoveDisplayObject(this);
	getSystemState()->stage->removeHiddenObject(this);
	removeAVM1Listeners();
	filterlistHasChanged=false;
	maxfilterborder=0;
	setParent(nullptr);
	eventparentmap.clear();
	if (mask)
		mask->removeStoredMember();
	mask=nullptr;
	ismaskCount=0;
	if (clipMask)
		clipMask->removeStoredMember();
	clipMask=nullptr;
	matrix.reset();
	if (loaderInfo)
		loaderInfo->removeStoredMember();
	loaderInfo=nullptr;
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
	colorTransform.reset();
	scalingGrid.reset();
	ASATOM_REMOVESTOREDMEMBER(opaqueBackground);
	opaqueBackground=asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(metaData);
	metaData=asAtomHandler::nullAtom;
	ASATOM_REMOVESTOREDMEMBER(scrollRect);
	scrollRect=asAtomHandler::undefinedAtom;
	loadedFrom=getSystemState()->mainClip->applicationDomain.getPtr();
	hasChanged = true;
	needsTextureRecalculation=true;
	hasDefaultName=false;
	tx=0;
	ty=0;
	rotation=0;
	sx=1;
	sy=1;
	blendMode=BLENDMODE_NORMAL;
	isLoadedRoot=false;
	inInitFrame=false;
	ClipDepth=0;
	constructed=false;
	useLegacyMatrix=true;
	onStage=false;
	visible=true;
	avm1mouselistenercount=0;
	avm1framelistenercount=0;
	if (broadcastEventListenerCount)
		getSystemState()->unregisterFrameListener(this);
	broadcastEventListenerCount=0;
	filters.reset();
	legacy=false;
	markedForLegacyDeletion=false;
	cacheAsBitmap=false;
	name=UINT32_MAX;
	for (auto it = avm1variables.begin(); it != avm1variables.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->removeStoredMember();
	}
	avm1variables.clear();
	for (auto it = avm1locals.begin(); it != avm1locals.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
	}
	avm1locals.clear();
	variablebindings.clear();
	placeFrame=UINT32_MAX;
	return EventDispatcher::destruct();
}

void DisplayObject::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	removeAVM1Listeners();
	broadcastEventListenerCount=0;

	if (clipMask)
		clipMask->prepareShutdown();
	if (matrix)
		matrix->prepareShutdown();
	if (loaderInfo)
		loaderInfo->prepareShutdown();
	if (invalidateQueueNext)
		invalidateQueueNext->prepareShutdown();
	if (accessibilityProperties)
		accessibilityProperties->prepareShutdown();
	if (colorTransform)
		colorTransform->prepareShutdown();
	if (scalingGrid)
		scalingGrid->prepareShutdown();
	if (filters)
		filters->prepareShutdown();
	ASObject* op = asAtomHandler::getObject(opaqueBackground);
	if (op)
		op->prepareShutdown();
	ASObject* me = asAtomHandler::getObject(metaData);
	if (me)
		me->prepareShutdown();
	ASObject* sr = asAtomHandler::getObject(scrollRect);
	if (sr)
		sr->prepareShutdown();
	while (!avm1variables.empty())
	{
		ASObject* o = asAtomHandler::getObject(avm1variables.begin()->second);
		avm1variables.erase(avm1variables.begin());
		if (o)
		{
			o->prepareShutdown();
			o->removeStoredMember();
		}
	}
	while (!avm1locals.empty())
	{
		ASObject* o = asAtomHandler::getObject(avm1locals.begin()->second);
		avm1locals.erase(avm1locals.begin());
		if (o)
		{
			o->prepareShutdown();
			o->removeStoredMember();
		}
	}
	setMask(NullRef);
	setClipMask(NullRef);
	// setParent(nullptr);
	// getSystemState()->removeFromResetParentList(this);
	// onStage=false;
}

bool DisplayObject::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (skipCountCylicMemberReferences(gcstate))
		return gcstate.hasMember(this);
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	for (auto it = avm1variables.begin(); it != avm1variables.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			ret = o->countAllCylicMemberReferences(gcstate) || ret;
	}
	for (auto it = avm1locals.begin(); it != avm1locals.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			ret = o->countAllCylicMemberReferences(gcstate) || ret;
	}
	ASObject* op = asAtomHandler::getObject(opaqueBackground);
	if (op)
		ret = op->countAllCylicMemberReferences(gcstate) || ret;
	ASObject* me = asAtomHandler::getObject(metaData);
	if (me)
		ret = me->countAllCylicMemberReferences(gcstate) || ret;
	ASObject* sr = asAtomHandler::getObject(scrollRect);
	if (sr)
		ret = sr->countAllCylicMemberReferences(gcstate) || ret;
	if (mask)
		ret = mask->countAllCylicMemberReferences(gcstate) || ret;
	if (clipMask)
		ret = clipMask->countAllCylicMemberReferences(gcstate) || ret;
	if (loaderInfo)
		ret = loaderInfo->countAllCylicMemberReferences(gcstate) || ret;
	if (parent)
		ret = parent->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void DisplayObject::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED);
	c->setDeclaredMethodByQName("loaderInfo","",c->getSystemState()->getBuiltinFunction(_getLoaderInfo,0,Class<LoaderInfo>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getWidth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",c->getSystemState()->getBuiltinFunction(_getScaleX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",c->getSystemState()->getBuiltinFunction(_setScaleX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",c->getSystemState()->getBuiltinFunction(_getScaleY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",c->getSystemState()->getBuiltinFunction(_setScaleY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleZ","",c->getSystemState()->getBuiltinFunction(_getScaleZ,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleZ","",c->getSystemState()->getBuiltinFunction(_setScaleZ),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(_getX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(_getY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",c->getSystemState()->getBuiltinFunction(_getZ,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",c->getSystemState()->getBuiltinFunction(_setZ),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getHeight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",c->getSystemState()->getBuiltinFunction(_getVisible,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",c->getSystemState()->getBuiltinFunction(_setVisible),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",c->getSystemState()->getBuiltinFunction(_getRotation,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",c->getSystemState()->getBuiltinFunction(_setRotation),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,name,ASString);
	c->setDeclaredMethodByQName("parent","",c->getSystemState()->getBuiltinFunction(_getParent,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("root","",c->getSystemState()->getBuiltinFunction(_getRoot,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",c->getSystemState()->getBuiltinFunction(_getBlendMode,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",c->getSystemState()->getBuiltinFunction(_setBlendMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",c->getSystemState()->getBuiltinFunction(_getScale9Grid,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",c->getSystemState()->getBuiltinFunction(_setScale9Grid),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stage","",c->getSystemState()->getBuiltinFunction(_getStage,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",c->getSystemState()->getBuiltinFunction(_getMask,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",c->getSystemState()->getBuiltinFunction(_setMask),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",c->getSystemState()->getBuiltinFunction(_getAlpha,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",c->getSystemState()->getBuiltinFunction(_setAlpha),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",c->getSystemState()->getBuiltinFunction(_getBounds,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getRect","",c->getSystemState()->getBuiltinFunction(_getBounds,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseX","",c->getSystemState()->getBuiltinFunction(_getMouseX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseY","",c->getSystemState()->getBuiltinFunction(_getMouseY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",c->getSystemState()->getBuiltinFunction(localToGlobal,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("globalToLocal","",c->getSystemState()->getBuiltinFunction(globalToLocal,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTestObject","",c->getSystemState()->getBuiltinFunction(hitTestObject,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTestPoint","",c->getSystemState()->getBuiltinFunction(hitTestPoint,2,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(_getTransform,0,Class<Transform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(_setTransform),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,accessibilityProperties,AccessibilityProperties);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,cacheAsBitmap,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,filters,Array);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,scrollRect,Rectangle);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, rotationX,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, rotationY,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, opaqueBackground,ASObject);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, metaData,ASObject);

	c->addImplementedInterface(InterfaceClass<IBitmapDrawable>::getClass(c->getSystemState()));
	IBitmapDrawable::linkTraits(c);
}

ASFUNCTIONBODY_SETTER_STRINGID_CB(DisplayObject,name,onSetName)
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,accessibilityProperties)
ASFUNCTIONBODY_GETTER_SETTER_ATOMTYPE_CB(DisplayObject,scrollRect,asAtomHandler::undefinedAtom,onSetScrollRect)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, rotationX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, rotationY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, metaData)

ASFUNCTIONBODY_ATOM(DisplayObject,_getter_name)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->needsActionScript3() && (th->name == BUILTIN_STRINGS::EMPTY || th->name==UINT32_MAX) )
		ret = asAtomHandler::nullAtom;
	else if (th->name==UINT32_MAX)
		ret = asAtomHandler::undefinedAtom;
	else
		ret = asAtomHandler::fromStringID(th->name);

}

void DisplayObject::onSetName(uint32_t oldName)
{
	if (!needsActionScript3() && oldName != name)
	{
		auto parent = getParent();
		if (parent != nullptr)
		{
			multiname m(nullptr);
			m.name_type = multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id = name;

			ASWorker* wrk = parent->getInstanceWorker();
			incRef();
			asAtom val = asAtomHandler::fromObject(this);
			// use internal version to avoid any unwanted sideeffects (events/addChild/removeChild)
			parent->setVariableByMultiname_intern(m, val, CONST_NOT_ALLOWED, nullptr,nullptr, wrk);

			if (oldName != BUILTIN_STRINGS::EMPTY)
			{
				m.name_s_id = oldName;
				m.ns.push_back(nsNameAndKind());
				parent->deleteVariableByMultiname_intern(m, wrk);
			}
		}
	}
}
void DisplayObject::onSetScrollRect(asAtom oldValue)
{
	if (oldValue.uintval == this->scrollRect.uintval)
		return;
	if (asAtomHandler::is<Rectangle>(oldValue))
		asAtomHandler::as<Rectangle>(oldValue)->removeUser(this);
	if (asAtomHandler::is<Rectangle>(scrollRect))
	{
		// not mentioned in the specs, but setting scrollRect actually creates a clone of the provided rect,
		// so that any future changes to the scrollRect have no effect on the provided rect and vice versa
		Rectangle* res=Class<Rectangle>::getInstanceS(this->getInstanceWorker());
		res->x=asAtomHandler::as<Rectangle>(scrollRect)->x;
		res->y=asAtomHandler::as<Rectangle>(scrollRect)->y;
		res->width=asAtomHandler::as<Rectangle>(scrollRect)->width;
		res->height=asAtomHandler::as<Rectangle>(scrollRect)->height;

		ASATOM_REMOVESTOREDMEMBER(scrollRect);
		this->scrollRect = asAtomHandler::fromObjectNoPrimitive(res);
		res->addStoredMember();
		res->addUser(this);
	}
	if ((!asAtomHandler::is<Rectangle>(scrollRect) && !asAtomHandler::is<Rectangle>(oldValue)) ||
		(!asAtomHandler::is<Rectangle>(scrollRect) && asAtomHandler::is<Rectangle>(oldValue)) ||
		(!asAtomHandler::is<Rectangle>(scrollRect) && !asAtomHandler::is<Rectangle>(oldValue) &&
		 (asAtomHandler::as<Rectangle>(scrollRect)->x != asAtomHandler::as<Rectangle>(oldValue)->x ||
		  asAtomHandler::as<Rectangle>(scrollRect)->y != asAtomHandler::as<Rectangle>(oldValue)->y ||
		  asAtomHandler::as<Rectangle>(scrollRect)->width != asAtomHandler::as<Rectangle>(oldValue)->width ||
		  asAtomHandler::as<Rectangle>(scrollRect)->height != asAtomHandler::as<Rectangle>(oldValue)->height)))
	{
		hasChanged = true;
		if (onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}
void DisplayObject::updatedRect()
{
	assert(asAtomHandler::is<Rectangle>(scrollRect));
	hasChanged = true;
	if (onStage)
		requestInvalidation(getSystemState());
	else
		requestInvalidationFilterParent();

}

ASFUNCTIONBODY_ATOM(DisplayObject,_getter_opaqueBackground)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	ret = th->opaqueBackground;
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setter_opaqueBackground)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 1)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0]))
		th->opaqueBackground = asAtomHandler::nullAtom;
	else
	{
		// convert argument to uint and ignore alpha component
		th->opaqueBackground =asAtomHandler::fromUInt(asAtomHandler::toUInt(args[0])&0xffffff) ;
	}
	th->requestInvalidation(wrk->getSystemState());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getter_filters)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->filters.isNull())
		th->filters = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	th->filters->incRef();
	ret = asAtomHandler::fromObject(th->filters.getPtr());
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setter_filters)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 1)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);

	th->filters =ArgumentConversionAtom<_NR<Array>>::toConcrete(wrk,args[0],th->filters);
	th->maxfilterborder=0;
	th->filterlistHasChanged=true;
	if (!th->filters.isNull())
	{
		for (uint32_t i = 0; i < th->filters->size(); i++)
		{
			asAtom f = asAtomHandler::invalidAtom;
			th->filters->at_nocheck(f,i);
			if (asAtomHandler::is<BitmapFilter>(f))
				th->maxfilterborder = max(th->maxfilterborder,asAtomHandler::as<BitmapFilter>(f)->getMaxFilterBorder());
		}
	}
	th->requestInvalidation(wrk->getSystemState());
}
bool DisplayObject::needsCacheAsBitmap() const
{
	return cacheAsBitmap
		   || blendMode == BLENDMODE_MULTIPLY
		   || blendMode == BLENDMODE_ADD
		   || blendMode == BLENDMODE_SCREEN
		   || blendMode == BLENDMODE_DARKEN
		   || blendMode == BLENDMODE_DIFFERENCE
		   || blendMode == BLENDMODE_HARDLIGHT
		   || blendMode == BLENDMODE_LIGHTEN
		   || blendMode == BLENDMODE_OVERLAY
		   || blendMode == BLENDMODE_ERASE
		   || hasFilters();
}
bool DisplayObject::hasFilters() const
{
	return filters && filters->size();
}

void DisplayObject::requestInvalidationFilterParent(InvalidateQueue* q)
{
	if (mask == this)
		return;
	if (cachedSurface->cachedFilterTextureID != UINT32_MAX
		|| (!cachedSurface->isInitialized && (this->hasFilters()
											 || this->inMask()
											 || isShaderBlendMode(getBlendMode()))
		))
	{
		if (cachedSurface->getState())
			cachedSurface->getState()->needsFilterRefresh=true;
		this->hasChanged=true;
		requestInvalidationIncludingChildren(q);
	}
	DisplayObject* p = getParent();
	while (p)
	{
		if (p->cachedSurface->cachedFilterTextureID != UINT32_MAX
			|| (!p->cachedSurface->isInitialized && (p->hasFilters()
													|| p->inMask()
													|| isShaderBlendMode(p->getBlendMode()))
			))
		{
			p->requestInvalidationFilterParent(q);
			break;
		}
		p = p->getParent();
	}
}
void DisplayObject::requestInvalidationIncludingChildren(InvalidateQueue* q)
{
	this->hasChanged=true;
	if (q)
	{
		this->incRef();
		q->addToInvalidateQueue(_MR(this));
		if(mask)
			mask->requestInvalidationIncludingChildren(q);
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,_getter_cacheAsBitmap)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	ret = asAtomHandler::fromBool(th->cacheAsBitmap);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setter_cacheAsBitmap)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 1)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->cacheAsBitmap != asAtomHandler::toInt(args[0]))
	{
		th->hasChanged=true;
		th->cacheAsBitmap = asAtomHandler::toInt(args[0]);
		th->requestInvalidation(wrk->getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getTransform)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	Transform* tf = Class<Transform>::getInstanceS(wrk,th);
	th->addOwnedObject(tf);
	ret = asAtomHandler::fromObject(tf);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setTransform)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<Transform> trans;
	ARG_CHECK(ARG_UNPACK(trans));
	if (!trans.isNull())
	{
		th->setMatrix(trans->owner->matrix);
		th->colorTransform = trans->owner->colorTransform;
		th->hasChanged=true;
	}
}

void DisplayObject::setMatrix(_NR<Matrix> m)
{
	bool mustInvalidate=false;
	if (m.isNull())
	{
		if (!matrix.isNull())
		{
			mustInvalidate=true;
			geometryChanged();
		}
		matrix= NullRef;
	}
	else
	{
		Locker locker(spinlock);
		if (matrix.isNull())
			matrix= _MR(Class<Matrix>::getInstanceS(this->getInstanceWorker()));
		if(matrix->matrix!=m->matrix)
		{
			matrix->matrix=m->matrix;
			extractValuesFromMatrix();
			geometryChanged();
			mustInvalidate=true;
		}
	}
	if(mustInvalidate)
	{
		hasChanged=true;
		if (onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

void DisplayObject::setMatrix3D(_NR<Matrix3D> m)
{
	bool mustInvalidate=false;
	if (!m.isNull())
	{
		Locker locker(spinlock);
		matrix= NullRef;
		float rawdata[16];
		m->getRawDataAsFloat(rawdata);
		this->tx = rawdata[12];
		this->ty = rawdata[13];
		this->tz = rawdata[14];
		this->sx = rawdata[0];
		this->sy = rawdata[5];
		this->sz = rawdata[10];
		LOG(LOG_NOT_IMPLEMENTED,"not all values of Matrix3D are handled in DisplayObject");
		geometryChanged();
		mustInvalidate=true;
	}
	if(mustInvalidate)
	{
		hasChanged=true;
		if (onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

void DisplayObject::setLegacyMatrix(const lightspark::MATRIX& m)
{
	if(!useLegacyMatrix)
		return;
	bool mustInvalidate=false;
	{
		Locker locker(spinlock);
		if (matrix.isNull())
			matrix= _MR(Class<Matrix>::getInstanceS(this->getInstanceWorker()));
		if(m.getTranslateX() != tx ||
			m.getTranslateY() != ty ||
			m.getScaleX() != sx ||
			m.getScaleY() != sy ||
			m.getRotation() != rotation)
		{
			matrix->matrix=m;
			extractValuesFromMatrix();
			geometryChanged();
			afterSetLegacyMatrix();
			mustInvalidate=true;
		}
	}
	if(mustInvalidate)
	{
		hasChanged=true;
		if (onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

void DisplayObject::setFilters(const FILTERLIST& filterlist)
{
	if (filterlist.NumberOfFilters)
	{
		if (filters.isNull())
			filters = _MR(Class<Array>::getInstanceS(getInstanceWorker()));
		else
		{
			// check if filterlist has really changed
			if (!filterlistHasChanged && filters->size() == filterlist.NumberOfFilters)
			{
				for (uint32_t i =0; i < filters->size(); i++)
				{
					asAtom f=filters->at(i);
					if (!asAtomHandler::is<BitmapFilter>(f))
					{
						filterlistHasChanged=true;
						break;
					}
					if (!asAtomHandler::as<BitmapFilter>(f)->compareFILTER(filterlist.Filters[i]))
					{
						filterlistHasChanged=true;
						break;
					}
				}
				if (!filterlistHasChanged)
					return;
			}
		}
		filterlistHasChanged=true;
		maxfilterborder=0;
		filters->resize(0);
		uint8_t n = filterlist.NumberOfFilters;
		for (uint8_t i=0; i < n; i++)
		{
			FILTER* flt = &filterlist.Filters[i];
			BitmapFilter* f = nullptr;
			switch(flt->FilterID)
			{
				case FILTER::FILTER_DROPSHADOW:
					f=Class<DropShadowFilter>::getInstanceS(getInstanceWorker(),flt->DropShadowFilter);
					break;
				case FILTER::FILTER_BLUR:
					f=Class<BlurFilter>::getInstanceS(getInstanceWorker(),flt->BlurFilter);
					break;
				case FILTER::FILTER_GLOW:
					f=Class<GlowFilter>::getInstanceS(getInstanceWorker(),flt->GlowFilter);
					break;
				case FILTER::FILTER_BEVEL:
					f=Class<BevelFilter>::getInstanceS(getInstanceWorker(),flt->BevelFilter);
					break;
				case FILTER::FILTER_GRADIENTGLOW:
					f=Class<GradientGlowFilter>::getInstanceS(getInstanceWorker(),flt->GradientGlowFilter);
					break;
				case FILTER::FILTER_CONVOLUTION:
					f=Class<ConvolutionFilter>::getInstanceS(getInstanceWorker(),flt->ConvolutionFilter);
					break;
				case FILTER::FILTER_COLORMATRIX:
					f=Class<ColorMatrixFilter>::getInstanceS(getInstanceWorker(),flt->ColorMatrixFilter);
					break;
				case FILTER::FILTER_GRADIENTBEVEL:
					f=Class<GradientBevelFilter>::getInstanceS(getInstanceWorker(),flt->GradientBevelFilter);
					break;
				default:
					LOG(LOG_ERROR,"Unsupported Filter Id " << (int)flt->FilterID);
					break;
			}
			if (f)
			{
				filters->push(asAtomHandler::fromObject(f));
				maxfilterborder = max(maxfilterborder,f->getMaxFilterBorder());
			}
		}
		hasChanged=true;
		setNeedsTextureRecalculation();
		requestInvalidation(getSystemState());
	}
	else
	{
		maxfilterborder=0;
		if (!filters.isNull() && filters->size())
		{
			filters->resize(0);
			hasChanged=true;
			setNeedsTextureRecalculation();
			requestInvalidation(getSystemState());
		}
	}
}

void DisplayObject::setFilter(BitmapFilter* filter)
{
	if (filters.isNull())
		filters = _MR(Class<Array>::getInstanceS(getInstanceWorker()));
	filters->resize(0);

	filter->incRef();
	filters->push(asAtomHandler::fromObject(filter));
	maxfilterborder = filter->getMaxFilterBorder();

	hasChanged=true;
	filterlistHasChanged=true;
	requestInvalidation(getSystemState());
}

void DisplayObject::refreshSurfaceState()
{
}
void DisplayObject::setupSurfaceState(IDrawable* d)
{
	SurfaceState* state = d->getState();
	assert(state);
#ifndef NDEBUG
	state->src=this; // keep track of the DisplayObject when debugging
#endif
	if (this->mask)
		state->mask = this->mask->getCachedSurface();
	else
		state->mask.reset();
	state->clipdepth = this->getClipDepth();
	state->depth = this->getDepth();
	state->isMask = this->ismaskCount || this->getClipDepth();
	state->visible = this->visible;
	state->alpha = this->clippedAlpha();
	state->allowAsMask = this->allowAsMask();
	state->maxfilterborder = this->getMaxFilterBorder();
	if (this->filterlistHasChanged)
	{
		state->filters.clear();
		if (this->filters)
		{
			state->filters.reserve(this->filters->size()*2);
			for (uint32_t i = 0; i < this->filters->size(); i++)
			{
				asAtom f = asAtomHandler::invalidAtom;
				this->filters->at_nocheck(f,i);
				if (asAtomHandler::is<BitmapFilter>(f))
				{
					auto filter = asAtomHandler::as<BitmapFilter>(f);
					FilterData fdata;
					filter->getRenderFilterGradient
					(
						fdata.gradientColors,
						fdata.gradientStops
					);
					uint32_t step = 0;
					while (true)
					{
						filter->getRenderFilterArgs(step,fdata.filterdata);
						state->filters.push_back(fdata);
						if (fdata.filterdata[0] == 0)
							break;
						step++;
					}
				}
			}
		}
	}
	this->boundsRectWithoutChildren(state->bounds.min.x, state->bounds.max.x, state->bounds.min.y, state->bounds.max.y, false);
	if (asAtomHandler::is<Rectangle>(scrollRect))
		state->scrollRect=asAtomHandler::as<Rectangle>(scrollRect)->getRect();
	else
		state->scrollRect= RECT();
	if (this->is<RootMovieClip>())
	{
		state->hasOpaqueBackground = true;
		state->renderWithNanoVG = true;
		state->opaqueBackground=this->as<RootMovieClip>()->getBackground();
		this->boundsRect(state->bounds.min.x, state->bounds.max.x, state->bounds.min.y, state->bounds.max.y, false);
	}
	else
	{
		state->hasOpaqueBackground = !asAtomHandler::isNull(this->opaqueBackground);
		if (state->hasOpaqueBackground)
			state->opaqueBackground=RGB(asAtomHandler::toUInt(this->opaqueBackground));
	}
	currentrendermatrix=state->matrix;
}

void DisplayObject::setMask(_NR<DisplayObject> m)
{
	bool mustInvalidate=(mask!=m.getPtr() || (m && m->hasChanged));

	if(mask)
	{
		//Remove previous mask
		assert(mask->ismaskCount);
		mask->ismaskCount--;
		mask->removeStoredMember();
		mask=nullptr;
	}

	mask=m.getPtr();
	if(mask)
	{
		//Use new mask
		mask->ismaskCount++;
		mask->incRef();
		mask->addStoredMember();
	}

	if(mustInvalidate)
	{
		hasChanged=true;
		if (onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

void DisplayObject::setClipMask(_NR<DisplayObject> m)
{
	if (clipMask)
		clipMask->removeStoredMember();
	clipMask = m.getPtr();
	if (clipMask)
	{
		clipMask->incRef();
		clipMask->addStoredMember();
	}
}
void DisplayObject::setBlendMode(UI8 blendmode)
{
	if (blendmode <= 1 || blendmode > 14)
		this->blendMode = BLENDMODE_NORMAL;
	else
	{
		this->blendMode = (AS_BLENDMODE)(uint8_t)blendmode;
	}
}

bool DisplayObject::isShaderBlendMode(AS_BLENDMODE bl)
{
	// TODO add shader for other extended blendmodes
	return bl == AS_BLENDMODE::BLENDMODE_OVERLAY
		   || bl == BLENDMODE_HARDLIGHT
		   || bl == BLENDMODE_DARKEN
		   || bl == BLENDMODE_LIGHTEN;
}

MATRIX DisplayObject::getConcatenatedMatrix(bool includeRoot, bool fromcurrentrendering) const
{
	if(!parent || (!includeRoot && parent == getSystemState()->mainClip))
		return fromcurrentrendering ? currentrendermatrix : getMatrix();
	else
		return parent->getConcatenatedMatrix(includeRoot,fromcurrentrendering).multiplyMatrix(fromcurrentrendering ? currentrendermatrix : getMatrix());
}

/* Return alpha value between 0 and 1. (The stored alpha value is not
 * necessary bounded.) */
float DisplayObject::clippedAlpha() const
{
	float a = 1.0;
	if (!this->colorTransform.isNull())
	{
		a = this->colorTransform->alphaMultiplier;
		if (this->colorTransform->alphaOffset != 0)
			a += this->colorTransform->alphaOffset /256.;
	}
	return dmin(dmax(a, 0.), 1.);
}

float DisplayObject::getConcatenatedAlpha() const
{
	if(!parent)
		return clippedAlpha();
	else
		return parent->getConcatenatedAlpha()*clippedAlpha();
}

float DisplayObject::getScaleFactor() const
{
	return TWIPS_SCALING_FACTOR;
}

void DisplayObject::onNewEvent(Event* ev)
{
	Locker locker(spinlock);
	if (parent && parent->getInDestruction())
		return;
	if (parent)
	{
		eventparentmap[ev] =parent;
		parent->incRef();
	}
}

void DisplayObject::afterHandleEvent(Event *ev)
{
	Locker locker(spinlock);
	auto it = eventparentmap.find(ev);
	if (it != eventparentmap.end())
	{
		it->second->decRef();
		eventparentmap.erase(it);
	}
}

tiny_string DisplayObject::AVM1GetPath(bool dotnotation)
{
	tiny_string res;
	if (getParent())
		res = getParent()->AVM1GetPath(dotnotation);
	if (this->name != BUILTIN_STRINGS::EMPTY && this->name != UINT32_MAX)
	{
		if (dotnotation)
		{
			if (!res.empty())
				res += ".";
		}
		else
			res += "/";
		res += getSystemState()->getStringFromUniqueId(this->name);
	}
	else if (is<RootMovieClip>())
	{
		if (dotnotation)
		{
			int lvl =as<RootMovieClip>()->AVM1getLevel();
			res += "_level";
			res += Integer::toString(lvl);
		}
	}
	return res;
}

void DisplayObject::afterLegacyInsert()
{
}

MATRIX DisplayObject::getMatrix(bool includeRotation) const
{
	Locker locker(spinlock);
	//Start from the residual matrix and construct the whole one
	MATRIX ret;
	if (!matrix.isNull())
		ret=matrix->matrix;
	ret.scale(sx,sy);
	if (includeRotation && !std::isnan(rotation))
		ret.rotate(rotation*M_PI/180.0);
	ret.translate(tx,ty);
	return ret;
}

bool DisplayObject::isConstructed() const
{
	return ACQUIRE_READ(constructed);
}

void DisplayObject::extractValuesFromMatrix()
{
	//Extract the base components from the matrix and leave in
	//it only the residual components
	assert(!matrix.isNull());
	tx=matrix->matrix.getTranslateX();
	ty=matrix->matrix.getTranslateY();
	sx=matrix->matrix.getScaleX();
	sy=matrix->matrix.getScaleY();
	rotation=matrix->matrix.getRotation();
	//Deapply translation
	matrix->matrix.translate(-tx,-ty);
	//Deapply rotation
	matrix->matrix.rotate(-rotation*M_PI/180.0);
	//Deapply scaling
	matrix->matrix.scale(1.0/sx,1.0/sy);
}

bool DisplayObject::skipRender() const
{
	return !isMask() && !ClipDepth && (visible==false || clippedAlpha()==0.0);
}

bool DisplayObject::defaultRender(RenderContext& ctxt)
{
	const Transform2D& t = ctxt.transformStack().transform();
	const CachedSurface* surface=ctxt.getCachedSurface(this);
	/* surface is only modified from within the render thread
	 * so we need no locking here */
	if(!surface->isValid || !surface->isInitialized || !surface->tex || !surface->tex->isValid())
		return true;
	if (surface->tex->width == 0 || surface->tex->height == 0)
		return true;

	Rectangle* r = this->scalingGrid.getPtr();
	if (!r && getParent())
		r = getParent()->scalingGrid.getPtr();
	ctxt.lsglLoadIdentity();
	ColorTransformBase ct = t.colorTransform;
	MATRIX m = t.matrix;
	ctxt.renderTextured(*surface->tex, surface->getState()->alpha, RenderContext::RGB_MODE,
						ct, false,0.0,RGB(),surface->getState()->smoothing,m,r->getRect(),t.blendmode);
	return false;
}

void DisplayObject::computeBoundsForTransformedRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
		number_t& outXMin, number_t& outYMin, number_t& outWidth, number_t& outHeight,
		const MATRIX& m) const
{
	//As the transformation is arbitrary we have to check all the four vertices
	number_t coords[8];
	m.multiply2D(xmin,ymin,coords[0],coords[1]);
	m.multiply2D(xmin,ymax,coords[2],coords[3]);
	m.multiply2D(xmax,ymax,coords[4],coords[5]);
	m.multiply2D(xmax,ymin,coords[6],coords[7]);
	//Now find out the minimum and maximum that represent the complete bounding rect
	number_t minx=coords[6];
	number_t maxx=coords[6];
	number_t miny=coords[7];
	number_t maxy=coords[7];
	for(int i=0;i<6;i+=2)
	{
		if(coords[i]<minx)
			minx=coords[i];
		else if(coords[i]>maxx)
			maxx=coords[i];
		if(coords[i+1]<miny)
			miny=coords[i+1];
		else if(coords[i+1]>maxy)
			maxy=coords[i+1];
	}
	outXMin=minx;
	outYMin=miny;
	outWidth = maxx - minx;
	outHeight = maxy - miny;
}

IDrawable* DisplayObject::invalidate(bool smoothing)
{
	//Not supposed to be called
	throw RunTimeException("DisplayObject::invalidate");
}
void DisplayObject::invalidateForRenderToBitmap(BitmapContainerRenderData* container,bool smoothing)
{
	if (this->mask)
		this->mask->invalidateForRenderToBitmap(container,smoothing);
	IDrawable* d = this->invalidate(smoothing);
	if (d)
	{
		setupSurfaceState(d);
		if (getNeedsTextureRecalculation() || !d->isCachedSurfaceUsable(this))
		{
			this->incRef();
			AsyncDrawJob* j = new AsyncDrawJob(d,_MR(this));
			j->execute();
			j->threadAbort(); // avoid addUploadJob in jobFence()
			container->uploads.push_back(j);
		}
		else
		{
			RefreshableSurface s;
			this->incRef();
			s.displayobject = _MR(this);
			s.drawable = d;
			container->surfacesToRefresh.push_back(s);
		}
	}
}

void DisplayObject::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	//Let's invalidate also the mask
	if(mask && mask != this && (mask->hasChanged || forceTextureRefresh))
		mask->requestInvalidation(q,forceTextureRefresh);
}

void DisplayObject::updateCachedSurface(IDrawable *d)
{
	// this is called only from rendering thread, so no locking done here
	cachedSurface->SetState(d->getState());
	cachedSurface->isValid=true;
	cachedSurface->isInitialized=true;
	cachedSurface->wasUpdated=true;
}
//TODO: Fix precision issues, Adobe seems to do the matrix mult with twips and rounds the results,
//this way they have less pb with precision.
Vector2f DisplayObject::localToGlobal(const Vector2f& point, bool fromcurrentrendering) const
{
	return getConcatenatedMatrix(true, fromcurrentrendering).multiply2D(point);
}
//TODO: Fix precision issues
Vector2f DisplayObject::globalToLocal(const Vector2f& point, bool fromcurrentrendering) const
{
	return getConcatenatedMatrix(true, fromcurrentrendering).getInverted().multiply2D(point);
}
void DisplayObject::localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout, bool fromcurrentrendering) const
{
	Vector2f global = localToGlobal(Vector2f(xin, yin), fromcurrentrendering);
	xout = global.x;
	yout = global.y;
}
void DisplayObject::globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout, bool fromcurrentrendering) const
{
	Vector2f local = globalToLocal(Vector2f(xin, yin), fromcurrentrendering);
	xout = local.x;
	yout = local.y;
}
void DisplayObject::setOnStage(bool staged, bool force,bool inskipping)
{
	bool changed = false;
	//TODO: When removing from stage released the cachedTex
	if(staged!=onStage)
	{
		//Our stage condition changed, send event
		onStage=staged;
		if(staged==true)
		{
			hasChanged=true;
			requestInvalidation(getSystemState());
		}
		if(getVm(getSystemState())==nullptr)
			return;
		changed = true;
		this->requestInvalidationFilterParent();
	}
	if (force || changed)
	{
		/*NOTE: By tests we can assert that added/addedToStage is dispatched
		  immediately when addChild is called. On the other hand setOnStage may
		  be also called outside of the VM thread (for example by Loader::execute)
		  so we have to check isVmThread and act accordingly. If in the future
		  asynchronous uses of setOnStage are removed the code can be simplified
		  by removing the !isVmThread case.
		*/
		if(onStage==true)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"addedToStage"));
			// root clips are added to stage after the builtin MovieClip is constructed, but before the constructor call is completed.
			// So the EventListeners for "addedToStage" may not be registered yet and we can't execute the event directly
			if (!this->is<RootMovieClip>())
			{
				if(isVmThread())
					ABCVm::publicHandleEvent(this,e);
				else
				{
					this->incRef();
					getVm(getSystemState())->addEvent(_MR(this),e);
				}
			}
		}
		else if(onStage==false)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"removedFromStage"));
			if(isVmThread())
				ABCVm::publicHandleEvent(this,e);
			else
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),e);
			}
			if (this->is<InteractiveObject>() && getSystemState()->getEngineData())
				getSystemState()->getEngineData()->InteractiveObjectRemovedFromStage();
			getSystemState()->stage->AVM1RemoveDisplayObject(this);
		}
	}
}

bool DisplayObject::isVisible() const
{
	if (visible)
		return parent ? parent->isVisible() : true;
	return visible;
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK (val));
	if (!th->loadedFrom->usesActionScript3) // AVM1 uses alpha values from 0-100
		val /= 100.0;

	if (th->colorTransform.isNull())
		th->colorTransform = _NR<ColorTransform>(Class<ColorTransform>::getInstanceS(th->getInstanceWorker()));

	/* The stored value is not clipped, _getAlpha will return the
	 * stored value even if it is outside the [0, 1] range. */
	if(th->colorTransform->alphaMultiplier != val)
	{
		th->colorTransform->alphaMultiplier = val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(wrk->getSystemState(),false);
		else
			th->requestInvalidationFilterParent();

	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->loadedFrom->usesActionScript3)
		asAtomHandler::setNumber(ret, !th->colorTransform.isNull() ? th->colorTransform->alphaMultiplier : 1.0);
	else // AVM1 uses alpha values from 0-100
		asAtomHandler::setNumber(ret, !th->colorTransform.isNull() ? th->colorTransform->alphaMultiplier*100.0 : 100.0);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMask)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if(!th->mask)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->mask->incRef();
	ret = asAtomHandler::fromObject(th->mask);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setMask)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::is<DisplayObject>(args[0]))
	{
		//We received a valid mask object
		DisplayObject* newMask=asAtomHandler::as<DisplayObject>(args[0]);
		newMask->incRef();
		th->setMask(_MR(newMask));
	}
	else
		th->setMask(NullRef);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->sx);
}

void DisplayObject::setScaleX(number_t val)
{
	if (std::isnan(val))
		return;
	//Apply the difference
	if(sx!=val)
	{
		sx=val;
		hasChanged=true;
		geometryChanged();
		if(onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleX(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->sy);
}

void DisplayObject::setScaleY(number_t val)
{
	if (std::isnan(val))
		return;
	//Apply the difference
	if(sy!=val)
	{
		sy=val;
		hasChanged=true;
		geometryChanged();
		if(onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleY(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScaleZ)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->sz);
}

void DisplayObject::setScaleZ(number_t val)
{
	if (std::isnan(val))
		return;
	ROUND_TO_TWIPS(val);
	//Apply the difference
	if(sz!=val)
	{
		sz=val;
		hasChanged=true;
		geometryChanged();
		if(onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

void DisplayObject::setVisible(bool v)
{
	visible=v;
	requestInvalidation(getSystemState());
}

void DisplayObject::setLoaderInfo(LoaderInfo* li)
{
	assert(loaderInfo==nullptr);
	loaderInfo=li;
	if (li)
	{
		loaderInfo = li;
		loaderInfo->incRef();
		loaderInfo->addStoredMember();
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setScaleZ)
{
	LOG(LOG_NOT_IMPLEMENTED,"DisplayObject.scaleZ is set, but not used anywhere");
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleZ(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->tx);
}

void DisplayObject::setX(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	if (std::isnan(val))
	{
		if (needsActionScript3())
			val = 0;
		else
			return;
	}
	ROUND_TO_TWIPS(val);
	//Apply translation, it's trivial
	if(tx!=val)
	{
		tx=val;
		hasChanged=true;
		geometryChanged();
		if(onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

void DisplayObject::setY(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	if (std::isnan(val))
	{
		if (needsActionScript3())
			val = 0;
		else
			return;
	}
	ROUND_TO_TWIPS(val);
	//Apply translation, it's trivial
	if(ty!=val)
	{
		ty=val;
		hasChanged=true;
		geometryChanged();
		if(onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

void DisplayObject::setZ(number_t val)
{
	LOG(LOG_NOT_IMPLEMENTED,"setting DisplayObject.z has no effect");

	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	if (std::isnan(val))
		return;
	ROUND_TO_TWIPS(val);
	//Apply translation, it's trivial
	if(tz!=val)
	{
		tz=val;
		hasChanged=true;
		geometryChanged();
		if(onStage)
			requestInvalidation(getSystemState());
		else
			requestInvalidationFilterParent();
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	th->setX(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->ty);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	th->setY(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getZ)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->tz);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setZ)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	th->setZ(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getBounds)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);

	MATRIX m;
	if (asAtomHandler::is<DisplayObject>(args[0]))
	{
		DisplayObject* target=asAtomHandler::as<DisplayObject>(args[0]);
		//Compute the transformation matrix
		DisplayObject* cur=th;
		while(cur!=nullptr && cur!=target)
		{
			m = cur->getMatrix().multiplyMatrix(m);
			cur=cur->parent;
		}
		if(cur==nullptr)
		{
			//We crawled all the parent chain without finding the target
			//The target is unrelated, compute it's transformation matrix
			const MATRIX& targetMatrix=target->getConcatenatedMatrix(false,false);
			//If it's not invertible just use the previous computed one
			if(targetMatrix.isInvertible())
				m = targetMatrix.getInverted().multiplyMatrix(m);
		}
	}

	Rectangle* res=Class<Rectangle>::getInstanceS(wrk);
	number_t x1,x2,y1,y2;
	bool r=th->getBounds(x1,x2,y1,y2, m);
	if(r)
	{
		//Bounds are in the form [XY]{min,max}
		//convert it to rect (x,y,width,height) representation
		res->x=x1;
		res->width=x2-x1;
		res->y=y1;
		res->height=y2-y1;
	}
	else
	{
		res->x=0;
		res->width=0;
		res->y=0;
		res->height=0;
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getLoaderInfo)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);

	/* According to tests returning root.loaderInfo is the correct
	 * behaviour, even though the documentation states that only
	 * the main class should have non-null loaderInfo. */
	_NR<RootMovieClip> r=th->getRoot();
	if(r.isNull() || !r->loaderInfo)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	r->loaderInfo->incRef();
	ret = asAtomHandler::fromObject(r->loaderInfo);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getStage)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<Stage> res=th->getStage();
	if(res.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}

	res->incRef();
	ret = asAtomHandler::fromObject(res.getPtr());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScale9Grid)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->scalingGrid.getPtr())
	{
		th->scalingGrid->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(th->scalingGrid.getPtr());
	}
	else
		asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setScale9Grid)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	ARG_CHECK(ARG_UNPACK(th->scalingGrid));
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getBlendMode)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	tiny_string res;
	switch (th->blendMode)
	{
		case BLENDMODE_LAYER: res = "layer"; break;
		case BLENDMODE_MULTIPLY: res = "multiply"; break;
		case BLENDMODE_SCREEN: res = "screen"; break;
		case BLENDMODE_LIGHTEN: res = "lighten"; break;
		case BLENDMODE_DARKEN: res = "darken"; break;
		case BLENDMODE_DIFFERENCE: res = "difference"; break;
		case BLENDMODE_ADD: res = "add"; break;
		case BLENDMODE_SUBTRACT: res = "subtract"; break;
		case BLENDMODE_INVERT: res = "invert"; break;
		case BLENDMODE_ALPHA: res = "alpha"; break;
		case BLENDMODE_ERASE: res = "erase"; break;
		case BLENDMODE_OVERLAY: res = "overlay"; break;
		case BLENDMODE_HARDLIGHT: res = "hardlight"; break;
		default: res = "normal"; break;
	}

	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setBlendMode)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	tiny_string val;
	ARG_CHECK(ARG_UNPACK(val));

	AS_BLENDMODE oldblendmode= th->blendMode;
	th->blendMode = BLENDMODE_NORMAL;
	if (val == "add") th->blendMode = BLENDMODE_ADD;
	else if (val == "alpha") th->blendMode = BLENDMODE_ALPHA;
	else if (val == "darken") th->blendMode = BLENDMODE_DARKEN;
	else if (val == "difference") th->blendMode = BLENDMODE_DIFFERENCE;
	else if (val == "erase") th->blendMode = BLENDMODE_ERASE;
	else if (val == "hardlight") th->blendMode = BLENDMODE_HARDLIGHT;
	else if (val == "invert") th->blendMode = BLENDMODE_INVERT;
	else if (val == "layer") th->blendMode = BLENDMODE_LAYER;
	else if (val == "lighten") th->blendMode = BLENDMODE_LIGHTEN;
	else if (val == "multiply") th->blendMode = BLENDMODE_MULTIPLY;
	else if (val == "overlay") th->blendMode = BLENDMODE_OVERLAY;
	else if (val == "screen") th->blendMode = BLENDMODE_SCREEN;
	else if (val == "subtract") th->blendMode = BLENDMODE_SUBTRACT;
	if (oldblendmode != th->blendMode)
	{
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(wrk->getSystemState());
		else
			th->requestInvalidationFilterParent();
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,localToGlobal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t tempx, tempy;

	th->localToGlobal(pt->getX(), pt->getY(), tempx, tempy,false);

	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(wrk,tempx, tempy));
}

ASFUNCTIONBODY_ATOM(DisplayObject,globalToLocal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t tempx, tempy;

	th->globalToLocal(pt->getX(), pt->getY(), tempx, tempy,false);

	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(wrk,tempx, tempy));
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setRotation)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	//Apply the difference
	if(th->rotation!=val)
	{
		val = fmod(val+180.0, 360.0) - 180.0;
		if (val < -180.0)
			val += 360.0;
		th->rotation=val;
		th->hasChanged=true;
		th->geometryChanged();
		if(th->onStage)
			th->requestInvalidation(wrk->getSystemState());
	}
}

void DisplayObject::setParent(DisplayObjectContainer *p)
{
	Locker locker(spinlock);
	if(parent!=p)
	{
		if (parent)
			parent->removeStoredMember();
		if (p)
		{
			// mark old parent as dirty
			geometryChanged();
			getSystemState()->removeFromResetParentList(this);
			getSystemState()->stage->removeHiddenObject(this);
			p->incRef();
			p->addStoredMember();
		}
		parent=p;
		hasChanged=true;
		geometryChanged();
		if(onStage && !getSystemState()->isShuttingDown())
			requestInvalidation(getSystemState());
	}
}

void DisplayObject::setScalingGrid()
{
	RECT* r = loadedFrom->ScalingGridsLookup(this->getTagID());
	if (r)
	{
		this->scalingGrid = _MR(Class<Rectangle>::getInstanceS(getInstanceWorker()));
		this->scalingGrid->x=r->Xmin/20.0;
		this->scalingGrid->y=r->Ymin/20.0;
		this->scalingGrid->width=(r->Xmax-r->Xmin)/20.0;
		this->scalingGrid->height=(r->Ymax-r->Ymin)/20.0;
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getParent)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if(!th->parent || th->parent->isInaccessibleParent)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->parent->incRef();
	ret = asAtomHandler::fromObject(th->parent);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getRoot)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<DisplayObject> res;

	if (th->isLoadedRootObject())
		res = _NR<DisplayObject>(th);
	else if (th->is<Stage>())
		// according to spec, the root of the stage is the stage itself
		res = _NR<DisplayObject>(th);
	else
		res =th->getRoot();
	if(res.isNull())
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	res->incRef(); // one ref will be removed during destruction of res

	res->incRef();
	ret = asAtomHandler::fromObject(res.getPtr());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getRotation)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->rotation);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setVisible)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	bool newval = asAtomHandler::Boolean_concrete(args[0]);
	if (newval != th->visible)
	{
		th->setVisible(newval);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getVisible)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setBool(ret,th->visible);
}

number_t DisplayObject::computeHeight()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix(false));

	return (ret)?(y2-y1):0;
}

void DisplayObject::geometryChanged()
{
	if (this->is<DisplayObjectContainer>())
	{
		this->as<DisplayObjectContainer>()->markBoundsRectDirtyChildren();
	}
	DisplayObjectContainer* p = this->getParent();
	while (p)
	{
		p->markBoundsRectDirty();
		p=p->getParent();
	}
}

number_t DisplayObject::computeWidth()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix(false));

	return (ret)?(x2-x1):0;
}

int DisplayObject::getRawDepth()
{
	return (parent != nullptr) ? parent->findLegacyChildDepth(this) : 0;
}

int DisplayObject::getDepth()
{
	return getRawDepth() + 16384;
}

int DisplayObject::getClipDepth() const
{
	return ClipDepth ? ClipDepth + 16384 : 0;
}

_NR<RootMovieClip> DisplayObject::getRoot()
{
	if(!parent)
		return NullRef;

	return parent->getRoot();
}

_NR<Stage> DisplayObject::getStage()
{
	if(!parent)
		return NullRef;

	return parent->getStage();
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getWidth)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->computeWidth());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setWidth)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t newwidth=asAtomHandler::toNumber(args[0]);
	if (std::isnan(newwidth))
		return;
	if (std::isinf(newwidth) && th->needsActionScript3())
		newwidth = 0;
	ROUND_TO_TWIPS(newwidth);

	number_t xmin,xmax,y1,y2;
	bool hasBounds = th->boundsRect(xmin,xmax,y1,y2,false);

	number_t width = hasBounds ? xmax - xmin : 0;
	number_t height = hasBounds ? y2 - y1 : 0;
	auto aspectRatio = height / width;
	Vector2f targetScale;

	if (width != 0)
		targetScale = Vector2f(newwidth / width, newwidth / height);

	Vector2f prevScale(th->sx, th->sy);
	auto rotation = th->getRotation() * (M_PI / 180.0);
	auto cos = std::abs(std::cos(rotation));
	auto sin = std::abs(std::sin(rotation));

	Vector2f newScale
	(
		// x.
		aspectRatio * (cos * targetScale.x + sin * targetScale.y) /
		((cos + aspectRatio * sin) * (aspectRatio * cos + sin)),
		// y.
		(sin * prevScale.x + aspectRatio * cos * prevScale.y) /
		(aspectRatio * cos + sin)
	);
	if (th->useLegacyMatrix)
		th->useLegacyMatrix = false;
	th->setScaleX(std::isfinite(newScale.x) ? newScale.x : 0);
	th->setScaleY(std::isfinite(newScale.y) ? newScale.y : 0);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getHeight)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->computeHeight());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setHeight)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t newheight=asAtomHandler::toNumber(args[0]);
	if (std::isnan(newheight))
		return;
	if (std::isinf(newheight) && th->needsActionScript3())
		newheight = 0;
	ROUND_TO_TWIPS(newheight);

	number_t x1,x2,ymin,ymax;
	bool hasBounds = th->boundsRect(x1,x2,ymin,ymax,false);

	number_t width = hasBounds ? x2 - x1 : 0;
	number_t height = hasBounds ? ymax - ymin : 0;
	auto aspectRatio = height / width;
	Vector2f targetScale;

	if (height != 0)
		targetScale = Vector2f(newheight / width, newheight / height);

	Vector2f prevScale(th->sx, th->sy);
	auto rotation = th->getRotation() * (M_PI / 180.0);
	auto cos = std::abs(std::cos(rotation));
	auto sin = std::abs(std::sin(rotation));

	Vector2f newScale
	(
		// x.
		(aspectRatio * cos * prevScale.x + sin * prevScale.y) /
		(aspectRatio * cos + sin),
		// y.
		aspectRatio * (sin * targetScale.x + cos * targetScale.y) /
		((cos + aspectRatio * sin) * (aspectRatio * cos + sin))
	);
	if (th->useLegacyMatrix)
		th->useLegacyMatrix = false;
	th->setScaleX(std::isfinite(newScale.x) ? newScale.x : 0);
	th->setScaleY(std::isfinite(newScale.y) ? newScale.y : 0);
}

Vector2f DisplayObject::getLocalMousePos()
{
	return getConcatenatedMatrix().getInverted().multiply2D(getSystemState()->getInputThread()->getMousePos());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMouseX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->getLocalMousePos().x);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMouseY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, th->getLocalMousePos().y);
}

bool DisplayObject::hitTestMask(const Vector2f& globalPoint, HIT_TYPE type)
{
	const auto& mask = this->mask ? this->mask : this->clipMask;
	if(mask)
	{
		//Compute the coordinates local to the mask
		const MATRIX& maskMatrix = mask->getConcatenatedMatrix(false,false);
		if(!maskMatrix.isInvertible())
		{
			//If the matrix is not invertible the mask as collapsed to zero size
			//If the mask is zero sized then the object is not visible
			return false;
		}
		const auto maskPoint = maskMatrix.getInverted().multiply2D(globalPoint);
		if(mask->hitTest(globalPoint, maskPoint, GENERIC_HIT_INVISIBLE,false).isNull())
			return false;
	}
	return true;
}

_NR<DisplayObject> DisplayObject::hitTest(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
{
	if((!((visible && this->clippedAlpha()>0.0) || type == GENERIC_HIT_INVISIBLE) || !isConstructed()) && (!isMask() || !getClipDepth()))
		return NullRef;

	//First check if there is any mask on this object, if so the point must be inside the mask to go on
	if (type != GENERIC_HIT_EXCLUDE_CHILDREN
		&& !hitTestMask(globalPoint,type))
		return NullRef;
	return hitTestImpl(globalPoint, localPoint, type,interactiveObjectsOnly);
}

/* Display objects have no children in general,
 * so we skip to calling the constructor, if necessary.
 * This is called in vm's thread context */
void DisplayObject::initFrame()
{
	if(!inInitFrame && !isConstructed() && getClass() && needsActionScript3())
	{
		inInitFrame=true;
		handleConstruction();

		/*
		 * Legacy objects have their display list properties set on creation, but
		 * the related events must only be sent after the constructor is sent.
		 * This is from "Order of Operations".
		 */
		if(parent)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"added",true));
			ABCVm::publicHandleEvent(this,e);
		}
		if(onStage)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"addedToStage"));
			ABCVm::publicHandleEvent(this,e);
		}
		inInitFrame=false;
	}
}

void DisplayObject::executeFrameScript()
{
}

bool DisplayObject::needsActionScript3() const
{
	return !this->loadedFrom || this->loadedFrom->usesActionScript3;
}

void DisplayObject::constructionComplete(bool _explicit, bool forInitAction)
{
	RELEASE_WRITE(constructed,true);
	if (getParent())
		setNameOnParent();
	if (!placedByActionScript && needsActionScript3() && getParent() != nullptr)
	{
		_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"added",true));
		if (isVmThread())
			ABCVm::publicHandleEvent(this, e);
		else
		{
			incRef();
			getVm(getSystemState())->addEvent(_MR(this), e);
		}
		if (isOnStage())
			setOnStage(true, true);
	}
}
void DisplayObject::setNameOnParent()
{
	if( this->name != BUILTIN_STRINGS::EMPTY
		&& !this->hasDefaultName
		&& this->name != UINT32_MAX
	)
	{
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=this->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		asAtom obj = asAtomHandler::invalidAtom;
		getParent()->getVariableByMultiname(obj,objName,GET_VARIABLE_OPTION::DONT_CALL_GETTER,getInstanceWorker());
		if (!asAtomHandler::is<DisplayObject>(obj)
			|| asAtomHandler::as<DisplayObject>(obj)->getDepth() >= this->getDepth())
		{
			this->incRef();
			asAtom v = asAtomHandler::fromObject(this);
			getParent()->setVariableByMultiname(objName,v,CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
		}
		ASATOM_DECREF(obj);
	}
}
void DisplayObject::beforeConstruction(bool _explicit)
{
	skipFrame |= needsActionScript3() && _explicit;
	placedByActionScript |= _explicit;
	if (needsActionScript3() && getParent() == nullptr && this != getSystemState()->mainClip)
		getSystemState()->stage->addHiddenObject(this);
}
void DisplayObject::afterConstruction(bool _explicit)
{
//	hasChanged=true;
//	needsTextureRecalculation=true;
//	if(onStage)
//		requestInvalidation(getSystemState());
}

void DisplayObject::applyFilters(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley)
{
	if (filters)
	{
		for (uint32_t i = 0; i < filters->size(); i++)
		{
			asAtom f = asAtomHandler::invalidAtom;
			filters->at_nocheck(f,i);
			if (asAtomHandler::is<BitmapFilter>(f))
				asAtomHandler::as<BitmapFilter>(f)->applyFilter(target, source, sourceRect, xpos, ypos, scalex, scaley, this);
		}
	}
}

void DisplayObject::setNeedsTextureRecalculation(bool skippable)
{
	textureRecalculationSkippable=skippable;
	needsTextureRecalculation=true;
	if (!cachedSurface->isChunkOwner)
		cachedSurface->tex=nullptr;
	cachedSurface->isChunkOwner=true;
}

string DisplayObject::toDebugString() const
{
	std::string res = EventDispatcher::toDebugString();
	res += "tag=";
	char buf[100];
	sprintf(buf,"%u pa=%p",getTagID(),getParent());
	res += buf;
	if (onStage)
		res += " onstage";
	return res;
}
IDrawable* DisplayObject::getFilterDrawable(bool smoothing)
{
	if (!hasFilters())
		return nullptr;
	number_t x,y;
	number_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(!boundsRect(bxmin,bxmax,bymin,bymax,false))
	{
		//No contents, nothing to do
		return nullptr;
	}
	MATRIX matrix = getMatrix();

	bool isMask=false;
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);

	if (isnan(width) || isnan(height))
	{
		// on stage with invalid concatenatedMatrix. Create a trash initial texture
		width = 1;
		height = 1;
	}
	if (width >= 8192 || height >= 8192 || (width * height) >= 16777216)
		return nullptr;

	if(width==0 || height==0)
		return nullptr;
	ColorTransformBase ct;
	if (this->colorTransform)
		ct = *this->colorTransform.getPtr();
	this->resetNeedsTextureRecalculation();
	return new RefreshableDrawable(x, y, ceil(width), ceil(height)
				, matrix.getScaleX(), matrix.getScaleY()
				, isMask, cacheAsBitmap
				, this->getScaleFactor(), getConcatenatedAlpha()
				, ct, smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS:SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
}

bool DisplayObject::findParent(DisplayObject *d) const
{
	if (this == d)
		return true;
	if (!parent)
		return false;
	return parent->findParent(d);
}

int DisplayObject::getParentDepth() const
{
	int i;
	DisplayObjectContainer* p;
	for (i = 0, p = parent; p != nullptr; p = p->parent, ++i);
	return i;
}

int DisplayObject::findParentDepth(DisplayObject* d) const
{
	if (this != d)
	{
		int i;
		DisplayObjectContainer* p;
		for (i = 0, p = parent; p != nullptr && p != d; p = p->parent, ++i);
		return i;
	}
	return -1;
}

DisplayObjectContainer* DisplayObject::getAncestor(int depth) const
{
	if (depth < 0)
		return (DisplayObjectContainer*)this;
	if (!depth)
		return parent;
	if (parent == nullptr)
		return nullptr;
	return parent->getAncestor(--depth);
}

DisplayObjectContainer* DisplayObject::findCommonAncestor(DisplayObject* d, int& depth, bool init) const
{
	const DisplayObject* a = this;
	const DisplayObject* b = d;
	if (init)
	{
		depth = 0;
		if ((a->getParentDepth() - b->getParentDepth()) < 0)
			std::swap(a, b);
	}
	if (a->parent == nullptr || b == nullptr)
		return a->parent == nullptr ? (DisplayObjectContainer*)a : nullptr;
	if (b->findParent(a->parent))
		return a->parent;
	return a->parent->findCommonAncestor((DisplayObject*)b, ++depth, false);
}

// Compute the minimal, axis aligned bounding box in global
// coordinates
bool DisplayObject::boundsRectGlobal(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool fromcurrentrendering)
{
	number_t x1, x2, y1, y2, tmpx,tmpy;
	if (!boundsRect(x1, x2, y1, y2,false))
		return false;

	MATRIX m = getConcatenatedMatrix(true, fromcurrentrendering);

	// check all four corners to get the bounding box taking rotation into account
	m.multiply2D(x1, y1, tmpx,tmpy);
	xmin = tmpx;
	xmax = tmpx;
	ymin = tmpy;
	ymax = tmpy;
	m.multiply2D(x2, y1, tmpx,tmpy);
	xmin = dmin(xmin, tmpx);
	xmax = dmax(xmax, tmpx);
	ymin = dmin(ymin, tmpy);
	ymax = dmax(ymax, tmpy);
	m.multiply2D(x1, y2, tmpx,tmpy);
	xmin = dmin(xmin, tmpx);
	xmax = dmax(xmax, tmpx);
	ymin = dmin(ymin, tmpy);
	ymax = dmax(ymax, tmpy);
	m.multiply2D(x2, y2, tmpx,tmpy);
	xmin = dmin(xmin, tmpx);
	xmax = dmax(xmax, tmpx);
	ymin = dmin(ymin, tmpy);
	ymax = dmax(ymax, tmpy);
	return true;
}

bool DisplayObject::skipCountCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.stopped)
		return true;
	if (!getSystemState()->isShuttingDown())
	{
		if (broadcastEventListenerCount || isOnStage() || hiddenPrevDisplayObject || hiddenNextDisplayObject)
		{
			// no need to count as we have at least one reference left if this object is still on stage or hidden
			gcstate.ignoreCount(this);
		}
		if(gcstate.isIgnored(this))
			return true;
		DisplayObject* p = parent;
		while (p)
		{
			if (p->hasBroadcastListeners() || gcstate.isIgnored(p) || p->hiddenPrevDisplayObject || p->hiddenNextDisplayObject)
			{
				// no need to count as the parent is already ignored
				gcstate.ignoreCount(this);
				return true;
			}
			p = p->parent;
		}
	}
	return false;
}

void DisplayObject::handleConstruction()
{
	asAtom o = asAtomHandler::fromObject(this);
	getClass()->handleConstruction(o,nullptr,0,true);
}

bool DisplayObject::boundsRectGlobal(RectF& rect, bool fromcurrentrendering)
{
	return boundsRectGlobal(rect.min.x, rect.max.y, rect.min.y, rect.max.y, fromcurrentrendering);
}

ASFUNCTIONBODY_ATOM(DisplayObject,hitTestObject)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<DisplayObject> another;
	ARG_CHECK(ARG_UNPACK(another));
	number_t xmin = 0, xmax, ymin = 0, ymax;
	if (!th->boundsRectGlobal(xmin, xmax, ymin, ymax, false))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	number_t xmin2, xmax2, ymin2, ymax2;
	if (!another->boundsRectGlobal(xmin2, xmax2, ymin2, ymax2, false))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	number_t intersect_xmax = dmin(xmax, xmax2);
	number_t intersect_xmin = dmax(xmin, xmin2);
	number_t intersect_ymax = dmin(ymax, ymax2);
	number_t intersect_ymin = dmax(ymin, ymin2);
	asAtomHandler::setBool(ret,(intersect_xmax > intersect_xmin) &&
			  (intersect_ymax > intersect_ymin));
}

ASFUNCTIONBODY_ATOM(DisplayObject,hitTestPoint)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t x;
	number_t y;
	bool shapeFlag;
	ARG_CHECK(ARG_UNPACK (x) (y) (shapeFlag, false));

	number_t xmin, xmax, ymin, ymax;
	if (!th->boundsRectGlobal(xmin, xmax, ymin, ymax,false))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}
	if (!th->needsActionScript3())
		th->getSystemState()->mainClip->localToGlobal(x,y,x,y,false);

	bool insideBoundingBox = (xmin <= x) && (x <= xmax) && (ymin <= y) && (y <= ymax);

	if (!shapeFlag)
	{
		asAtomHandler::setBool(ret,insideBoundingBox);
		return;
	}
	else
	{
		if (!insideBoundingBox || !th->isOnStage())
		{
			asAtomHandler::setBool(ret,false);
			return;
		}

		number_t localX;
		number_t localY;
		th->globalToLocal(x, y, localX, localY,false);
		if (std::isnan(localX) || std::isnan(localY))
		{
			asAtomHandler::setBool(ret,false);
			return;
		}

		_NR<DisplayObject> hit = th->hitTest(Vector2f(x, y), Vector2f(localX, localY),
						     HIT_TYPE::GENERIC_HIT_INVISIBLE,false);

		asAtomHandler::setBool(ret,!hit.isNull());
	}
}

bool DisplayObject::isMouseEvent(uint32_t nameID) const
{
	return is<InteractiveObject>() &&
	(
		nameID == BUILTIN_STRINGS::STRING_ONMOUSEMOVE ||
		nameID == BUILTIN_STRINGS::STRING_ONMOUSEDOWN ||
		nameID == BUILTIN_STRINGS::STRING_ONMOUSEUP ||
		nameID == BUILTIN_STRINGS::STRING_ONPRESS ||
		nameID == BUILTIN_STRINGS::STRING_ONMOUSEWHEEL ||
		nameID == BUILTIN_STRINGS::STRING_ONROLLOVER ||
		nameID == BUILTIN_STRINGS::STRING_ONROLLOUT ||
		nameID == BUILTIN_STRINGS::STRING_ONRELEASEOUTSIDE ||
		nameID == BUILTIN_STRINGS::STRING_ONRELEASE
	);
}

asAtom DisplayObject::getPropertyByIndex(size_t idx, ASWorker* wrk)
{
	auto sys = getSystemState();
	asAtom ret = asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::fromObject(this);
	switch (idx)
	{
		case 0:// x
			_getX(ret,wrk,obj,nullptr,0);
			break;
		case 1:// y
			_getY(ret,wrk,obj,nullptr,0);
			break;
		case 2:// xscale
			AVM1_getScaleX(ret,wrk,obj,nullptr,0);
			break;
		case 3:// xscale
			AVM1_getScaleY(ret,wrk,obj,nullptr,0);
			break;
		case 4:// currentframe
			if (is<MovieClip>())
				MovieClip::_getCurrentFrame(ret,wrk,obj,nullptr,0);
			break;
		case 5:// totalframes
			if (is<MovieClip>())
				MovieClip::_getTotalFrames(ret,wrk,obj,nullptr,0);
			break;
		case 6:// alpha
			AVM1_getAlpha(ret,wrk,obj,nullptr,0);
			break;
		case 7:// visible
			_getVisible(ret,wrk,obj,nullptr,0);
			break;
		case 8:// width
			_getWidth(ret,wrk,obj,nullptr,0);
			break;
		case 9:// height
			_getHeight(ret,wrk,obj,nullptr,0);
			break;
		case 10:// rotation
			_getRotation(ret,wrk,obj,nullptr,0);
			break;
		case 11:// target
			ret = asAtomHandler::fromString(sys, AVM1GetPath(false));
			break;
		case 12:// framesloaded
			if (is<MovieClip>())
				MovieClip::_getFramesLoaded(ret,wrk,obj,nullptr,0);
			break;
		case 13:// _name
			if (this->name == UINT32_MAX)
				ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
			else
				_getter_name(ret,wrk,obj,nullptr,0);
			break;
		case 14:// _droptarget
			if (is<AVM1MovieClip>())
			{
				DisplayObject* droptarget = as<AVM1MovieClip>()->getDroptarget();
				if (droptarget)
					ret = asAtomHandler::fromString(sys, droptarget->AVM1GetPath(false));
				else
					ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
			}
			break;
		case 15:// url
			ret = asAtomHandler::fromString(sys, sys->mainClip->getOrigin().getURL());
			break;
		case 16:// _highquality
			ret = asAtomHandler::fromUInt(wrk->getSystemState()->highquality);
			break;
		case 17:// focusrect
			if (is<InteractiveObject>())
				InteractiveObject::AVM1_getfocusrect(ret,wrk,obj,nullptr,0);
			break;
		case 18:// _soundbuftime
			ret = asAtomHandler::fromInt(wrk->getSystemState()->static_SoundMixer_bufferTime);
			break;
		case 19:// quality
			AVM1_getQuality(ret,wrk,obj,nullptr,0);
			break;
		case 20:// xmouse
			_getMouseX(ret,wrk,obj,nullptr,0);
			break;
		case 21:// ymouse
			_getMouseY(ret,wrk,obj,nullptr,0);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED, "getPropertyByIndex: Unknown property index " << idx);
			break;
	}
	return ret;
}

void DisplayObject::setPropertyByIndex(size_t idx, const asAtom& val, ASWorker* wrk)
{
	asAtom ret = asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::fromObject(this);

	asAtom value = val;
	switch (idx)
	{
		case 0:// x
			_setX(ret,wrk,obj,&value,1);
			break;
		case 1:// y
			_setY(ret,wrk,obj,&value,1);
			break;
		case 2:// xscale
			AVM1_setScaleX(ret,wrk,obj,&value,1);
			break;
		case 3:// xscale
			AVM1_setScaleY(ret,wrk,obj,&value,1);
			break;
		case 6:// alpha
			AVM1_setAlpha(ret,wrk,obj,&value,1);
			break;
		case 7:// visible
			_setVisible(ret,wrk,obj,&value,1);
			break;
		case 8:// width
			_setWidth(ret,wrk,obj,&value,1);
			break;
		case 9:// height
			_setHeight(ret,wrk,obj,&value,1);
			break;
		case 10:// rotation
			_setRotation(ret,wrk,obj,&value,1);
			break;
		case 13:// name
			_setter_name(ret,wrk,obj,&value,1);
			break;
		case 17:// focusrect
			if (is<InteractiveObject>())
				InteractiveObject::AVM1_setfocusrect(ret,wrk,obj,&value,1);
			break;
		case 19:// quality
			AVM1_setQuality(ret,wrk,obj,&value,1);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED, "setPropertyByIndex: Unknown property index " << idx);
			break;
	}
	ASATOM_DECREF(value);
}

// NOTE: Can't use `tiny_string` here because it isn't `constexpr`.
static constexpr auto propTable = makeArray
(
	"_x",
	"_y",
	"_xscale",
	"_yscale",
	"_currentframe",
	"_totalframes",
	"_alpha",
	"_visible",
	"_width",
	"_height",
	"_rotation",
	"_target",
	"_framesloaded",
	"_name",
	"_droptarget",
	"_url",
	"_highquality",
	"_focusrect",
	"_soundbuftime",
	"_quality",
	"_xmouse",
	"_ymouse"
);

bool DisplayObject::hasPropertyName(const tiny_string& name) const
{
	return getPropertyIndex(name) != size_t(-1);
}

size_t DisplayObject::getPropertyIndex(const tiny_string& name) const
{
	auto it = std::find_if
	(
		propTable.begin(),
		propTable.end(),
		[&](const char* prop)
		{
			return name.caselessEquals(prop);
		}
	);
	if (it == propTable.end())
		return size_t(-1);
	return std::distance(propTable.begin(), it);
}

asAtom DisplayObject::getPropertyByName(const tiny_string& name, ASWorker* wrk)
{
	return getPropertyByIndex(getPropertyIndex(name), wrk);
}

void DisplayObject::setPropertyByName(const tiny_string& name, const asAtom& value, ASWorker* wrk)
{
	setPropertyByIndex(getPropertyIndex(name), value, wrk);
}

static int parseLevelId(const tiny_string& digits)
{
	bool isNegative = digits.startsWith("-");

	int levelID = 0;
	for (size_t i = isNegative; i < digits.numChars() && isdigit(digits[i]); ++i)
		levelID = (levelID * 10) + (digits[i] - '0');

	return isNegative ? -levelID : levelID;
}

// TODO: Move this into `AVM1context`, once `originalclip` is moved into
// there.
DisplayObject* DisplayObject::getLevel(int levelID) const
{
	return getSystemState()->stage->AVM1getLevelRoot(levelID);
}

asAtom DisplayObject::resolvePathProperty(const tiny_string& name, ASWorker* wrk)
{
	auto sys = getSystemState();
	bool caseSensitive = wrk->AVM1isCaseSensitive();

	if (name.equalsWithCase("_root", caseSensitive))
		return asAtomHandler::fromObject(AVM1getRoot());
	else if (name.equalsWithCase("_parent", caseSensitive))
	{
		if (parent != nullptr)
			return asAtomHandler::fromObject(parent);
		return asAtomHandler::undefinedAtom;
	}
	else if (name.equalsWithCase("_global", caseSensitive))
		return asAtomHandler::fromObject(sys->avm1global);

	// Resolve level names (`_level<depth>`).
	auto prefix = name.substr(0, 6);
	bool isLevelName =
	(
		prefix.equalsWithCase("_level", caseSensitive) ||
		// NOTE: `_flash` is an alias for `_level`, which is a relic of
		// the earliest versions of Flash Player.
		prefix.equalsWithCase("_flash", caseSensitive)
	);

	if (!isLevelName)
		return asAtomHandler::invalidAtom;

	auto levelClip = getLevel(parseLevelId(name.substr(6, UINT32_MAX)));
	if (levelClip != nullptr)
		return asAtomHandler::fromObject(levelClip);
	return asAtomHandler::undefinedAtom;

}

GET_VARIABLE_RESULT DisplayObject::AVM1getVariableByMultiname
(
	asAtom& ret,
	const multiname& name,
	GET_VARIABLE_OPTION opt,
	ASWorker* wrk,
	bool isSlashPath
)
{
	// Property search order for `DisplayObject`s.
	// 1. Expandos (user defined properties on the underlying object).
	auto result = ASObject::AVM1getVariableByMultiname
	(
		ret,
		name,
		opt,
		wrk,
		isSlashPath
	);

	if (asAtomHandler::isValid(ret))
		return result;

	result = GETVAR_NORMAL;

	auto s = name.normalizedName(wrk);
	bool caseSensitive = wrk->AVM1isCaseSensitive();
	bool isInternalProp = s.startsWith("_") && !s.startsWith("__");
	// 2. Path properties. i.e. `_root`, `_parent`, `_level<depth>`
	// (honours case sensitivity).
	if (isInternalProp)
		ret = resolvePathProperty(name.normalizedName(wrk), wrk);

	if (asAtomHandler::isValid(ret))
	{
		if ((opt & GET_VARIABLE_OPTION::NO_INCREF)==0)
			ASATOM_INCREF(ret);
		return result;
	}

	// 3. Child `DisplayObject`s with the supplied instance name.
	auto child =
	(
		is<DisplayObjectContainer>() ?
		as<DisplayObjectContainer>()->getLegacyChildByName
		(
			name.normalizedName(wrk),
			caseSensitive
		) : nullptr
	);
	if (child != nullptr)
	{
		// NOTE: If the object can't be represented as an AVM1 object,
		// such as `Shape`, then any attempt to access it'll return the
		// parent instead.
		if (!isSlashPath && child->is<Shape>())
			ret = asAtomHandler::fromObject(child->getParent());
		else
			ret = asAtomHandler::fromObject(child);
		if ((opt & GET_VARIABLE_OPTION::NO_INCREF)==0)
			ASATOM_INCREF(ret);
		return result;
	}

	// 4. Internal properties, such as `_x`, and `_y` (always case
	// insensitive).
	if (isInternalProp)
	{
		ret = getPropertyByName(s, wrk);
		result = GETVAR_ISINCREFFED;
	}

	return result;
}

bool DisplayObject::AVM1setLocalByMultiname
(
	multiname& name,
	asAtom& value,
	CONST_ALLOWED_FLAG allowConst,
	ASWorker* wrk
)
{
	assert(!needsActionScript3());
	auto sys = getSystemState();

	bool alreadySet = false;
	bool caseSensitive = wrk->AVM1isCaseSensitive();
	auto s = name.normalizedName(wrk);
	auto nameID = sys->getUniqueStringId(s, caseSensitive);

	// If a `TextField` was bound to this property, update it's text.
	AVM1UpdateVariableBindings(nameID, value);

	// Property search order for `DisplayObject`s.
	// 1. Expandos (user defined properties on the underlying object).
	bool hasprop = ASObject::hasPropertyByMultiname(name, true, false, wrk);
	if (hasprop)
	{
		EventDispatcher::setVariableByMultiname
		(
			name,
			value,
			allowConst,
			&alreadySet,
			wrk
		);
	}
	// 2. Internal properties, such as `_x`, and `_y`.
	else if (hasPropertyName(s))
		setPropertyByName(s, value, wrk);
	// 3. Prototype.
	else if (hasprop_prototype())
	{
		auto proto = getprop_prototype();
		alreadySet = proto->AVM1setLocalByMultiname(name, value, allowConst, wrk);
	}
	else
	{
		EventDispatcher::setVariableByMultiname
		(
			name,
			value,
			allowConst,
			&alreadySet,
			wrk
		);
	}

	bool isFrameEvent =
	(
		name.name_s_id == BUILTIN_STRINGS::STRING_ONENTERFRAME ||
		name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD
	);

	if (isFrameEvent)
	{
		if (asAtomHandler::isFunction(value))
		{
			sys->registerFrameListener(this);
			sys->stage->AVM1AddEventListener(this);
			avm1framelistenercount++;
			setIsEnumerable(name, false);
		}
		else // value is not a function, we remove the FrameListener
		{
			if(hasprop)
			{
				avm1framelistenercount--;
				sys->stage->AVM1RemoveEventListener(this);
				if (avm1framelistenercount==0)
					sys->unregisterFrameListener(this);
			}
		}
	}
	else if (isMouseEvent(name.name_s_id))
	{
		if (asAtomHandler::isFunction(value))
		{
			as<InteractiveObject>()->setMouseEnabled(true);
			sys->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
			avm1mouselistenercount++;
			setIsEnumerable(name, false);
		}
		else // value is not a function, we remove the MouseListener
		{
			if(hasprop)
			{
				avm1mouselistenercount--;
				if (avm1mouselistenercount==0)
					sys->stage->AVM1RemoveMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				setIsEnumerable(name, false);
			}
		}
	}
	return alreadySet;
}

bool DisplayObject::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	if (needsActionScript3())
	{
		return ASObject::hasPropertyByMultiname
		(
			name,
			considerDynamic,
			considerPrototype,
			wrk
		);
	}
	if (ASObject::hasPropertyByMultiname
	(
		name,
		considerDynamic,
		considerPrototype,
		wrk
	))
		return true;

	auto s = name.normalizedName(wrk);
	bool caseSensitive = wrk->AVM1isCaseSensitive();
	bool isInternalProp = s.startsWith("_");
	if (isInternalProp && hasPropertyName(s))
		return true;

	if (isOnStage() && is<DisplayObjectContainer>())
	{
		auto container = as<DisplayObjectContainer>();
		if (container->hasLegacyChildByName(s, caseSensitive))
			return true;
	}

	return
	(
		isInternalProp &&
		asAtomHandler::isValid(resolvePathProperty(s, wrk))
	);
}

void DisplayObject::AVM1registerPrototypeListeners()
{
	assert(!needsActionScript3());
	ASObject* pr = this->getprop_prototype();
	while (pr)
	{
		multiname name(nullptr);
		name.name_type = multiname::NAME_STRING;
		name.name_s_id = BUILTIN_STRINGS::STRING_ONENTERFRAME;
		if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
		{
			getSystemState()->registerFrameListener(this);
			getSystemState()->stage->AVM1AddEventListener(this);
			avm1framelistenercount++;
		}
		name.name_s_id = BUILTIN_STRINGS::STRING_ONLOAD;
		if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
		{
			getSystemState()->registerFrameListener(this);
			getSystemState()->stage->AVM1AddEventListener(this);
			avm1framelistenercount++;
		}
		if (this->is<InteractiveObject>())
		{
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEMOVE;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEDOWN;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEUP;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONPRESS;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEWHEEL;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONROLLOVER;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONROLLOUT;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONRELEASEOUTSIDE;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONRELEASE;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
				avm1mouselistenercount++;
			}
		}
		pr = pr->getprop_prototype();
	}
}
void DisplayObject::AVM1unregisterPrototypeListeners()
{
	assert(!needsActionScript3());
	ASObject* pr = this->getprop_prototype();
	while (pr)
	{
		multiname name(nullptr);
		name.name_type = multiname::NAME_STRING;
		name.name_s_id = BUILTIN_STRINGS::STRING_ONENTERFRAME;
		if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
		{
			getSystemState()->stage->AVM1RemoveEventListener(this);
			avm1framelistenercount--;
		}
		name.name_s_id = BUILTIN_STRINGS::STRING_ONLOAD;
		if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
		{
			getSystemState()->stage->AVM1RemoveEventListener(this);
			avm1framelistenercount--;
		}
		if (this->is<InteractiveObject>())
		{
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEMOVE;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEDOWN;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEUP;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONPRESS;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEWHEEL;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONROLLOVER;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONROLLOUT;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONRELEASEOUTSIDE;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			name.name_s_id = BUILTIN_STRINGS::STRING_ONRELEASE;
			if (pr->hasPropertyByMultiname(name,true,false,getInstanceWorker()))
				avm1mouselistenercount--;
			if (avm1mouselistenercount==0)
				getSystemState()->stage->AVM1RemoveMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
		}
		pr = pr->getprop_prototype();
	}
}
bool DisplayObject::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	bool res = EventDispatcher::deleteVariableByMultiname(name,wrk);
	if (!this->loadedFrom->usesActionScript3)
	{
		if (name.name_s_id == BUILTIN_STRINGS::STRING_ONENTERFRAME ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD)
		{
			avm1framelistenercount--;
			getSystemState()->stage->AVM1RemoveEventListener(this);
			if (avm1framelistenercount==0)
				getSystemState()->unregisterFrameListener(this);
		}
		if (isMouseEvent(name.name_s_id))
		{
			this->as<InteractiveObject>()->setMouseEnabled(false);
			avm1mouselistenercount--;
			if (avm1mouselistenercount==0)
				getSystemState()->stage->AVM1RemoveMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
		}
		tiny_string s = name.normalizedNameUnresolved(getSystemState()).lowercase();
		AVM1SetVariable(s,asAtomHandler::undefinedAtom,false);
	}
	return res;
}
void DisplayObject::removeAVM1Listeners()
{
	if (needsActionScript3())
		return;
	getSystemState()->stage->AVM1RemoveMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
	getSystemState()->stage->AVM1RemoveKeyboardListener(asAtomHandler::fromObjectNoPrimitive(this));
	getSystemState()->stage->AVM1RemoveFocusListener(asAtomHandler::fromObjectNoPrimitive(this));
	getSystemState()->unregisterFrameListener(this);
	AVM1unregisterPrototypeListeners();
	avm1mouselistenercount=0;
	avm1framelistenercount=0;
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setInt(ret,round(th->sx*100.0));
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleX(val/100.0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setInt(ret,round(th->sy*100.0));
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleY(val/100.0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getParent)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	DisplayObject* p = th->parent;
	if(!p || p->is<Stage>())
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	p->incRef();
	ret = asAtomHandler::fromObject(p);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getRoot)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	DisplayObject* root=th->AVM1getRoot();
	root->incRef();
	ret = asAtomHandler::fromObject(root);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getURL)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	ret = asAtomHandler::fromString(wrk->getSystemState(),th->loadedFrom->getOrigin().getURL());
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_hitTest)
{
	asAtomHandler::setBool(ret,false);
	if (argslen==1)
	{
		if (!asAtomHandler::is<Undefined>(args[0]))
		{
			if (asAtomHandler::is<DisplayObject>(obj))
			{
				if (asAtomHandler::is<DisplayObject>(args[0]))
				{
					hitTestObject(ret,wrk,obj,args,1);
				}
				else
				{
					tiny_string s = asAtomHandler::toString(args[0],wrk);
					DisplayObject* path = s.empty() ? nullptr : asAtomHandler::as<DisplayObject>(obj)->AVM1GetClipFromPath(s);
					if (path)
					{
						asAtom pathobj = asAtomHandler::fromObject(path);
						hitTestObject(ret,wrk,obj,&pathobj,1);
					}
					else
						LOG(LOG_ERROR,"AVM1_hitTest:clip not found:"<<asAtomHandler::toDebugString(args[0]));
				}
			}
			else
				LOG(LOG_NOT_IMPLEMENTED,"AVM1_hitTest:object is no MovieClip:"<<asAtomHandler::toDebugString(obj));
		}
	}
	else
		hitTestPoint(ret,wrk,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_localToGlobal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);


	ASObject* pt=asAtomHandler::toObject(args[0],wrk);

	asAtom x = asAtomHandler::fromInt(0);
	asAtom y = asAtomHandler::fromInt(0);
	multiname mx(nullptr);
	mx.name_type=multiname::NAME_STRING;
	mx.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
	mx.isAttribute = false;
	pt->getVariableByMultiname(x,mx,GET_VARIABLE_OPTION::NONE,wrk);
	multiname my(nullptr);
	my.name_type=multiname::NAME_STRING;
	my.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
	my.isAttribute = false;
	pt->getVariableByMultiname(y,my,GET_VARIABLE_OPTION::NONE,wrk);

	number_t tempx, tempy;

	th->localToGlobal(asAtomHandler::toNumber(x), asAtomHandler::toNumber(y), tempx, tempy,false);
	ASATOM_DECREF(x);
	ASATOM_DECREF(y);
	asAtomHandler::setNumber(x,tempx);
	asAtomHandler::setNumber(y,tempy);
	bool alreadyset=false;
	pt->setVariableByMultiname(mx,x,CONST_ALLOWED,&alreadyset,wrk);
	if (alreadyset)
		ASATOM_DECREF(x);
	pt->setVariableByMultiname(my,y,CONST_ALLOWED,&alreadyset,wrk);
	if (alreadyset)
		ASATOM_DECREF(y);
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_globalToLocal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);


	ASObject* pt=asAtomHandler::toObject(args[0],wrk);

	asAtom x = asAtomHandler::fromInt(0);
	asAtom y = asAtomHandler::fromInt(0);
	multiname mx(nullptr);
	mx.name_type=multiname::NAME_STRING;
	mx.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
	mx.isAttribute = false;
	pt->getVariableByMultiname(x,mx,GET_VARIABLE_OPTION::NONE,wrk);
	multiname my(nullptr);
	my.name_type=multiname::NAME_STRING;
	my.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
	my.isAttribute = false;
	pt->getVariableByMultiname(y,my,GET_VARIABLE_OPTION::NONE,wrk);

	number_t tempx, tempy;

	th->globalToLocal(asAtomHandler::toNumber(x), asAtomHandler::toNumber(y), tempx, tempy,false);
	ASATOM_DECREF(x);
	ASATOM_DECREF(y);
	asAtomHandler::setNumber(x,tempx);
	asAtomHandler::setNumber(y,tempy);
	bool alreadyset=false;
	pt->setVariableByMultiname(mx,x,CONST_ALLOWED,&alreadyset,wrk);
	if (alreadyset)
		ASATOM_DECREF(x);
	pt->setVariableByMultiname(my,y,CONST_ALLOWED,&alreadyset,wrk);
	if (alreadyset)
		ASATOM_DECREF(y);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBytesLoaded)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setUInt(ret,th->loaderInfo ? th->loaderInfo->getBytesLoaded() : 0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBytesTotal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setUInt(ret,th->loaderInfo ? th->loaderInfo->getBytesTotal() : 0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getQuality)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->stage->quality.uppercase());
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setQuality)
{
	if (argslen > 0)
		wrk->getSystemState()->stage->quality = asAtomHandler::toString(args[0],wrk).uppercase();
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret, !th->colorTransform.isNull() ? th->colorTransform->alphaMultiplier*100.0 : 100.0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK (val));
	val /= 100.0;

	if (th->colorTransform.isNull())
		th->colorTransform = _NR<ColorTransform>(Class<ColorTransform>::getInstanceS(th->getInstanceWorker()));

	if(th->colorTransform->alphaMultiplier != val)
	{
		th->colorTransform->alphaMultiplier = val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(wrk->getSystemState());
		else
			th->requestInvalidationFilterParent();
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBounds)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);

	DisplayObject* target= th;
	bool currentNewInvalidBounds = wrk->getSystemState()->useNewInvalidBounds;
	if(argslen>=1) // contrary to spec adobe allows getBounds with zero parameters
	{
		if (asAtomHandler::is<Undefined>(args[0]) || asAtomHandler::is<Null>(args[0]))
			return;
		asAtom t = args[0];
		if (!asAtomHandler::is<DisplayObject>(t))
		{
			tiny_string path = asAtomHandler::toString(args[0],wrk).lowercase();
			DisplayObject* d = th->AVM1GetClipFromPath(path);
			if (!d)
			{
				LOG(LOG_ERROR,"AVM1_getBounds:path not found:"<<asAtomHandler::toDebugString(args[0])<<" on "<<th->toDebugString());
				ASATOM_DECREF(t);
				return;
			}
			target = d;
		}
		else
			target = asAtomHandler::as<DisplayObject>(t);
		if (target!=th && (target->loadedFrom->version >= 8 || wrk->getSystemState()->getSwfVersion() >= 8))
			currentNewInvalidBounds = wrk->getSystemState()->useNewInvalidBounds=true;
	}
	ASObject* o =  new_asobject(wrk);
	ret = asAtomHandler::fromObject(o);
	//Compute the transformation matrix
	MATRIX m;
	DisplayObject* cur=th;
	while(cur!=nullptr && cur!=target)
	{
		m = cur->getMatrix().multiplyMatrix(m);
		cur=cur->parent;
	}
	if(cur==nullptr)
	{
		//We crawled all the parent chain without finding the target
		//The target is unrelated, compute it's transformation matrix
		const MATRIX& targetMatrix=target->getConcatenatedMatrix();
		//If it's not invertible just use the previous computed one
		if(targetMatrix.isInvertible())
			m = targetMatrix.getInverted().multiplyMatrix(m);
	}

	number_t x1=0,x2=0,y1=0,y2=0;
	if (th == target)
		wrk->getSystemState()->useNewInvalidBounds=false;
	th->getBounds(x1,x2,y1,y2, m);
	wrk->getSystemState()->useNewInvalidBounds=currentNewInvalidBounds;

	asAtom v=asAtomHandler::invalidAtom;
	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=wrk->getSystemState()->getUniqueStringId("xMin");
	v = asAtomHandler::fromNumber(x1);
	o->setVariableByMultiname(name,v,CONST_ALLOWED,nullptr,wrk);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId("xMax");
	v = asAtomHandler::fromNumber(x2);
	o->setVariableByMultiname(name,v,CONST_ALLOWED,nullptr,wrk);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId("yMin");
	v = asAtomHandler::fromNumber(y1);
	o->setVariableByMultiname(name,v,CONST_ALLOWED,nullptr,wrk);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId("yMax");
	v = asAtomHandler::fromNumber(y2);
	o->setVariableByMultiname(name,v,CONST_ALLOWED,nullptr,wrk);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_swapDepths)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (argslen < 1)
		throw RunTimeException("AVM1: invalid number of arguments for swapDepths");
	DisplayObject* child1 = th;
	DisplayObject* child2 = nullptr;
	if (asAtomHandler::is<DisplayObject>(args[0]))
		child2 = asAtomHandler::as<DisplayObject>(args[0]);
	else
	{
		if (th->getParent() && th->getParent()->hasLegacyChildAt(asAtomHandler::toInt(args[0])))
			child2 = th->getParent()->getLegacyChildAt(asAtomHandler::toInt(args[0]));
	}
	if (th->getParent() && child1 && child2)
	{
		asAtom newargs[2];
		newargs[0] = asAtomHandler::fromObject(child1);
		newargs[1] = asAtomHandler::fromObject(child2);
		asAtom obj = asAtomHandler::fromObject(th->getParent());
		DisplayObjectContainer::swapChildren(ret,wrk,obj,newargs,2);
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getDepth)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	int r=0;
	if (th->getParent())
		r=th->getParent()->findLegacyChildDepth(th);
	ret = asAtomHandler::fromInt(r);
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_toString)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	tiny_string s = th->AVM1GetPath();
	if (s.empty())
		DisplayObject::_toString(ret,wrk,obj,args,argslen);
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),s);
}

void DisplayObject::AVM1SetupMethods(Class_base* c)
{
	// setup all methods and properties available for MovieClips in AVM1

	c->destroyContents();
	c->borrowedVariables.destroyContents();
	c->setDeclaredMethodByQName("hitTest","",c->getSystemState()->getBuiltinFunction(AVM1_hitTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",c->getSystemState()->getBuiltinFunction(AVM1_localToGlobal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("globalToLocal","",c->getSystemState()->getBuiltinFunction(AVM1_globalToLocal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getBytesLoaded","",c->getSystemState()->getBuiltinFunction(AVM1_getBytesLoaded),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getBytesTotal","",c->getSystemState()->getBuiltinFunction(AVM1_getBytesTotal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",c->getSystemState()->getBuiltinFunction(AVM1_getBounds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapDepths","",c->getSystemState()->getBuiltinFunction(AVM1_swapDepths),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDepth","",c->getSystemState()->getBuiltinFunction(AVM1_getDepth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMask","",c->getSystemState()->getBuiltinFunction(_setMask),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(_getTransform),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(_setTransform),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(AVM1_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,scrollRect,Rectangle);
	c->prototype->setDeclaredMethodByQName("hitTest","",c->getSystemState()->getBuiltinFunction(AVM1_hitTest),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("localToGlobal","",c->getSystemState()->getBuiltinFunction(AVM1_localToGlobal),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("globalToLocal","",c->getSystemState()->getBuiltinFunction(AVM1_globalToLocal),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getBytesLoaded","",c->getSystemState()->getBuiltinFunction(AVM1_getBytesLoaded),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getBytesTotal","",c->getSystemState()->getBuiltinFunction(AVM1_getBytesTotal),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getBounds","",c->getSystemState()->getBuiltinFunction(AVM1_getBounds),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getRect","",c->getSystemState()->getBuiltinFunction(AVM1_getBounds),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("swapDepths","",c->getSystemState()->getBuiltinFunction(AVM1_swapDepths),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getDepth","",c->getSystemState()->getBuiltinFunction(AVM1_getDepth),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("setMask","",c->getSystemState()->getBuiltinFunction(_setMask),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(_getTransform),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(_setTransform),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(AVM1_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("blendMode","",c->getSystemState()->getBuiltinFunction(_getBlendMode,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("blendMode","",c->getSystemState()->getBuiltinFunction(_setBlendMode),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("cacheAsBitmap","",c->getSystemState()->getBuiltinFunction(_getter_cacheAsBitmap,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("cacheAsBitmap","",c->getSystemState()->getBuiltinFunction(_setter_cacheAsBitmap),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("filters","",c->getSystemState()->getBuiltinFunction(_getter_filters,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("filters","",c->getSystemState()->getBuiltinFunction(_setter_filters),SETTER_METHOD,false);

}
DisplayObject *DisplayObject::AVM1GetClipFromPath(tiny_string &path, asAtom* member)
{
	if (path.empty() || path == "this")
	{
		if (member)
			*member=asAtomHandler::invalidAtom;
		return this;
	}
	if (path =="_root")
	{
		if (member)
			*member=asAtomHandler::invalidAtom;
		return this->AVM1getRoot();
	}
	if (path =="_parent")
	{
		if (member)
			*member=asAtomHandler::invalidAtom;
		return getParent();
	}
	if (path.startsWith("/"))
	{
		tiny_string newpath = path.substr_bytes(1,path.numBytes()-1);
		MovieClip* root = getRoot().getPtr();
		if (root)
			return root->AVM1GetClipFromPath(newpath,member);
		LOG(LOG_ERROR,"AVM1: no root movie clip for path:"<<path<<" "<<this->toDebugString());
		if (member)
			*member=asAtomHandler::invalidAtom;
		return nullptr;
	}
	if (path.startsWith("../"))
	{
		tiny_string newpath = path.substr_bytes(3,path.numBytes()-3);
		if (this->getParent() && this->getParent()->is<MovieClip>())
			return this->getParent()->as<MovieClip>()->AVM1GetClipFromPath(newpath,member);
		LOG(LOG_ERROR,"AVM1: no parent clip for path:"<<path<<" "<<this->toDebugString());
		if (member)
			*member=asAtomHandler::invalidAtom;
		return nullptr;
	}
	uint32_t pos = path.find("/");
	uint32_t pos2 = path.find(":");
	if (pos2 < pos)
		pos = pos2;
	tiny_string subpath = (pos == tiny_string::npos) ? path : path.substr_bytes(0,pos);
	if (subpath.empty())
	{
		if (member)
			*member=asAtomHandler::invalidAtom;
		return nullptr;
	}
	// path "/stage" is mapped to the root movie (?)
	if (this == getSystemState()->mainClip && subpath == "stage")
	{
		if (member)
			*member=asAtomHandler::invalidAtom;
		return this;
	}
	uint32_t posdot = subpath.find(".");
	if (posdot != tiny_string::npos)
	{
		tiny_string subdotpath =  subpath.substr_bytes(0,posdot);
		if (subdotpath.empty())
		{
			if (member)
				*member=asAtomHandler::invalidAtom;
			return nullptr;
		}
		DisplayObject* parent = AVM1GetClipFromPath(subdotpath,member);
		if (!parent)
		{
			if (member)
				*member=asAtomHandler::invalidAtom;
			return nullptr;
		}
		tiny_string localname = subpath.substr_bytes(posdot+1,subpath.numBytes()-posdot-1);
		return parent->AVM1GetClipFromPath(localname,member);
	}

	multiname objName(nullptr);
	objName.name_type=multiname::NAME_STRING;
	objName.name_s_id=getSystemState()->getUniqueStringId(subpath);
	objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	asAtom ret=asAtomHandler::invalidAtom;
	getVariableByMultiname(ret,objName,GET_VARIABLE_OPTION::NO_INCREF,getInstanceWorker());
	if (asAtomHandler::is<DisplayObject>(ret))
	{
		if (pos == tiny_string::npos)
			return asAtomHandler::as<DisplayObject>(ret);
		else
		{
			subpath = path.substr_bytes(pos+1,path.numBytes()-pos-1);
			return asAtomHandler::as<DisplayObject>(ret)->AVM1GetClipFromPath(subpath,member);
		}
	}
	else if (asAtomHandler::isValid(ret))
	{
		if (member)
			*member=ret;// we found a property/method of this clip
	}
	return nullptr;
}

void DisplayObject::AVM1SetVariable(tiny_string &name, asAtom v, bool setMember)
{
	bool caseSensitive = loadedFrom->version > 6;
	if (name.empty())
		return;
	if (name.startsWith("/"))
	{
		tiny_string newpath = name.substr_bytes(1,name.numBytes()-1);
		MovieClip* root = getRoot().getPtr();
		if (root)
			root->AVM1SetVariable(newpath,v);
		else
			LOG(LOG_ERROR,"AVM1: no root movie clip for name:"<<name<<" "<<this->toDebugString());
		return;
	}
	uint32_t pos = name.find(":");
	if (pos == tiny_string::npos)
	{
		ASATOM_INCREF(v); // ensure value is not destructed during binding
		auto nameID = getSystemState()->getUniqueStringId
		(
			name,
			// NOTE: Internal property keys are always case insensitive
			// (e.g. `_x`, `_y`, etc).
			caseSensitive && !name.startsWith("_")
		);
		auto it = avm1variables.find(nameID);
		if (it != avm1variables.end())
		{
			ASObject* o = asAtomHandler::getObject(it->second);
			if (o)
				o->removeStoredMember();
		}
		if (asAtomHandler::isUndefined(v))
			avm1variables.erase(nameID);
		else
		{
			ASATOM_ADDSTOREDMEMBER(v);
			avm1variables[nameID] = v;
		}
		if (setMember)
		{
			multiname objName(nullptr);
			objName.name_type=multiname::NAME_STRING;
			objName.name_s_id=nameID;
			ASObject* o = this;
			while (o && !o->hasPropertyByMultiname(objName,true,true,loadedFrom->getInstanceWorker()))
			{
				o=o->getprop_prototype();
			}
			ASATOM_INCREF(v);
			bool alreadyset;
			if (o)
				o->setVariableByMultiname(objName,v, CONST_ALLOWED,&alreadyset,loadedFrom->getInstanceWorker());
			else
				setVariableByMultiname(objName,v, CONST_ALLOWED,&alreadyset,loadedFrom->getInstanceWorker());
			if (alreadyset)
				ASATOM_DECREF(v);
		}
		AVM1UpdateVariableBindings(nameID,v);
		ASATOM_DECREF(v);
	}
	else if (pos == 0)
	{
		tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
		uint32_t nameId = getSystemState()->getUniqueStringId(localname, caseSensitive);
		auto it = avm1variables.find(nameId);
		if (it != avm1variables.end())
		{
			ASObject* o = asAtomHandler::getObject(it->second);
			if (o)
				o->removeStoredMember();
		}
		if (asAtomHandler::isUndefined(v))
			avm1variables.erase(nameId);
		else
		{
			ASATOM_ADDSTOREDMEMBER(v);
			avm1variables[nameId] = v;
		}
	}
	else
	{
		tiny_string path = name.substr_bytes(0,pos);
		DisplayObject* clip = AVM1GetClipFromPath(path);
		if (clip)
		{
			tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
			clip->AVM1SetVariable(localname,v);
		}
	}
}

void DisplayObject::AVM1SetVariableDirect(uint32_t nameId, asAtom v)
{
	// store as local, if it exists
	auto itl = avm1locals.find(nameId);
	if (itl != avm1locals.end())
	{
		ASATOM_ADDSTOREDMEMBER(v);
		ASATOM_REMOVESTOREDMEMBER(itl->second);
		itl->second = v;
		return;
	}
	auto it = avm1variables.find(nameId);
	if (it != avm1variables.end())
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->removeStoredMember();
	}
	if (asAtomHandler::isUndefined(v))
	{
		avm1variables.erase(nameId);
	}
	else
	{
		ASATOM_ADDSTOREDMEMBER(v);
		avm1variables[nameId] = v;
	}
}

asAtom DisplayObject::AVM1GetVariable(const tiny_string &name, bool checkrootvars)
{
	bool caseSensitive = loadedFrom->version > 6;
	uint32_t pos = name.find(":");
	if (pos == tiny_string::npos)
	{
		auto nameID = getSystemState()->getUniqueStringId(name, caseSensitive);
		if (loadedFrom->version > 4 && getSystemState()->avm1global)
		{
			// first check for class names
			asAtom ret=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=nameID;
			m.isAttribute = false;
			getSystemState()->avm1global->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
			if(!asAtomHandler::isInvalid(ret))
				return ret;
		}
		auto it = avm1variables.find(nameID);
		if (it != avm1variables.end())
		{
			ASATOM_INCREF(it->second);
			return it->second;
		}
	}
	else if (pos == 0)
	{
		tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
		return AVM1GetVariable(localname);
	}
	else
	{
		tiny_string path = name.substr_bytes(0,pos).lowercase();
		DisplayObject* clip = AVM1GetClipFromPath(path);
		if (clip)
		{
			tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
			return clip->AVM1GetVariable(localname);
		}
	}
	asAtom ret=asAtomHandler::invalidAtom;

	pos = name.find(".");
	if (pos != tiny_string::npos)
	{
		tiny_string path = name;
		DisplayObject* clip = AVM1GetClipFromPath(path);
		if (clip && clip != this)
		{
			clip->incRef();
			ret = asAtomHandler::fromObjectNoPrimitive(clip);
			return ret;
		}
		path = name.lowercase();
		clip = AVM1GetClipFromPath(path,&ret);
		if (asAtomHandler::isValid(ret))
		{
			ASATOM_INCREF(ret);
			return ret;
		}
		if (clip && clip != this)
		{
			clip->incRef();
			ret = asAtomHandler::fromObjectNoPrimitive(clip);
			return ret;
		}
	}

	if (loadedFrom->version > 4 && checkrootvars)
	{
		if (asAtomHandler::isInvalid(ret))// get Variable from root movie
			ret = AVM1getRoot()->AVM1GetVariable(name,false);
	}
	return ret;
}
void DisplayObject::AVM1UpdateVariableBindings(uint32_t nameID, asAtom& value)
{
	auto pair = variablebindings.equal_range(nameID);
	for (auto it = pair.first; it != pair.second; ++it)
	{
		// Make sure that the value isn't `free()`'d while updating the
		// bindings.
		ASATOM_INCREF(value);
		it->second->UpdateVariableBinding(value);
		ASATOM_DECREF(value);
	}
}

asAtom DisplayObject::getVariableBindingValue(const tiny_string &name)
{
	auto sys = getSystemState();
	auto wrk = getInstanceWorker();

	if (needsActionScript3())
		return asAtomHandler::invalidAtom;

	AVM1context fallback(this, sys);
	const auto& ctx =
	(
		!wrk->AVM1callStack.empty() ?
		*wrk->AVM1callStack.back() :
		fallback
	);
	this->incRef();
	auto pair = ctx.resolveVariablePath
	(
		asAtomHandler::fromObject(this),
		this,
		_MR(this),
		name
	);

	if (pair.first == nullptr)
		return asAtomHandler::invalidAtom;

	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = sys->getUniqueStringId
	(
		pair.second,
		wrk->AVM1isCaseSensitive()
	);

	asAtom ret = asAtomHandler::invalidAtom;
	pair.first->AVM1getVariableByMultiname
	(
		ret,
		m,
		GET_VARIABLE_OPTION::NONE,
		wrk,
		name.contains('/')
	);
	pair.first->decRef();
	return ret;
}
void DisplayObject::setVariableBinding(tiny_string &name, _NR<DisplayObject> obj)
{
	bool caseSensitive = loadedFrom->version > 6;
	uint32_t key = getSystemState()->getUniqueStringId(name, caseSensitive);
	if (obj)
	{
		obj->incRef();
		auto it = variablebindings.lower_bound(key);
		while (it != variablebindings.end() && it->first == key)
		{
			if (it->second == obj)
				return;
			it++;
		}
		variablebindings.insert(std::make_pair(key,obj));
	}
	else
	{
		auto it = variablebindings.find(key);
		if (it != variablebindings.end() && it->first == key)
			variablebindings.erase(it);
	}
}
void DisplayObject::AVM1SetFunction(const tiny_string& name, _NR<AVM1Function> obj)
{
	bool caseSensitive = getInstanceWorker()->AVM1callStack.back()->isCaseSensitive();
	uint32_t nameID = getSystemState()->getUniqueStringId(name, caseSensitive);

	auto it = avm1variables.find(nameID);
	if (it != avm1variables.end())
	{
		if (obj && asAtomHandler::isObject(it->second) && asAtomHandler::getObjectNoCheck(it->second) == obj.getPtr())
			return; // function is already set
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->removeStoredMember();
	}
	if (obj)
	{
		asAtom v = asAtomHandler::fromObjectNoPrimitive(obj.getPtr());
		obj->incRef();
		obj->addStoredMember();
		avm1variables[nameID] = v;

		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=nameID;
		obj->incRef();
		bool alreadyset = AVM1setVariableByMultiname(objName,v, CONST_ALLOWED,loadedFrom->getInstanceWorker());
		if (alreadyset)
			ASATOM_DECREF(v);
	}
	else
	{
		avm1variables.erase(nameID);
	}
}
AVM1Function* DisplayObject::AVM1GetFunction(uint32_t nameID)
{
	auto it = avm1variables.find(nameID);
	if (it != avm1variables.end() && asAtomHandler::is<AVM1Function>(it->second))
		return asAtomHandler::as<AVM1Function>(it->second);
	return nullptr;
}

DisplayObject* DisplayObject::AVM1getStage() const
{
	if (parent == nullptr)
		return const_cast<DisplayObject*>(this);

	if (parent->is<Loader>() || parent->is<Stage>())
		return parent;
	return parent->AVM1getStage();
}

DisplayObject* DisplayObject::AVM1getRoot()
{
	if (this->needsActionScript3())
		return getSystemState()->mainClip;
	if (parent && parent->needsActionScript3())
		return this;
	if (this->is<RootMovieClip>())
		return this;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=BUILTIN_STRINGS::STRING_LOCKROOT;
	m.isAttribute = false;
	asAtom l= asAtomHandler::undefinedAtom;
	getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
	bool lockroot = asAtomHandler::AVM1toBool(l,getInstanceWorker(),loadedFrom->version);
	if (!lockroot && parent)
		return parent->AVM1getRoot();
	return getSystemState()->mainClip;
}
