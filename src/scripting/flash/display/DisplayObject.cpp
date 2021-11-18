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
#include "backends/rendering.h"
#include "backends/input.h"
#include "scripting/argconv.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/filters/flashfilters.h"
#include <algorithm>

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

bool DisplayObject::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, const MATRIX& m) const
{
	if(!legacy && !isConstructed())
		return false;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
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

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	return ret?(xmax-xmin):0;
}

number_t DisplayObject::getNominalHeight()
{
	number_t xmin, xmax, ymin, ymax;

	if(!isConstructed())
		return 0;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	return ret?(ymax-ymin):0;
}

bool DisplayObject::Render(RenderContext& ctxt, bool force)
{
	if((!legacy && !isConstructed()) || (!force && skipRender()) || clippedAlpha()==0.0)
		return false;
	return renderImpl(ctxt);
}

DisplayObject::DisplayObject(Class_base* c):EventDispatcher(c),matrix(Class<Matrix>::getInstanceS(c->getSystemState())),tx(0),ty(0),rotation(0),
	sx(1),sy(1),alpha(1.0),blendMode(BLENDMODE_NORMAL),isLoadedRoot(false),ClipDepth(0),maskOf(),parent(nullptr),constructed(false),useLegacyMatrix(true),
	needsTextureRecalculation(true),textureRecalculationSkippable(false),onStage(false),
	visible(true),mask(),invalidateQueueNext(),loaderInfo(),loadedFrom(c->getSystemState()->mainClip),hasChanged(true),legacy(false),cacheAsBitmap(false),
	name(BUILTIN_STRINGS::EMPTY)
{
	subtype=SUBTYPE_DISPLAYOBJECT;
//	name = tiny_string("instance") + Integer::toString(ATOMIC_INCREMENT(instanceCount));
}

DisplayObject::~DisplayObject() {}

void DisplayObject::finalize()
{
	EventDispatcher::finalize();
	cachedBitmap.reset();
	cachedAsBitmapOf.reset();
	maskOf.reset();
	parent=nullptr;
	eventparentmap.clear();
	mask.reset();
	matrix.reset();
	loaderInfo.reset();
	colorTransform.reset();
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
	loadedFrom=getSystemState()->mainClip;
	hasChanged = true;
	needsTextureRecalculation=true;
	if (!cachedSurface.isChunkOwner)
		cachedSurface.tex=nullptr;
	cachedSurface.isChunkOwner=true;
	cachedSurface.isValid=false;
}

bool DisplayObject::destruct()
{
	// TODO make all DisplayObject derived classes reusable
	cachedBitmap.reset();
	cachedAsBitmapOf.reset();
	maskOf.reset();
	parent=nullptr;
	eventparentmap.clear();
	mask.reset();
	matrix.reset();
	loaderInfo.reset();
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
	colorTransform.reset();
	loadedFrom=getSystemState()->mainClip;
	hasChanged = true;
	needsTextureRecalculation=true;
	tx=0;
	ty=0;
	rotation=0;
	sx=1;
	sy=1;
	alpha=1.0;
	blendMode=BLENDMODE_NORMAL;
	isLoadedRoot=false;
	ClipDepth=0;
	constructed=false;
	useLegacyMatrix=true;
	onStage=false;
	visible=true;
	filters.reset();
	legacy=false;
	cacheAsBitmap=false;
	name=BUILTIN_STRINGS::EMPTY;
	for (auto it = avm1variables.begin(); it != avm1variables.end(); it++)
		ASATOM_DECREF(it->second);
	avm1variables.clear();
	variablebindings.clear();
	avm1functions.clear();
	if (!cachedSurface.isChunkOwner)
		cachedSurface.tex=nullptr;
	if (cachedSurface.tex)
		cachedSurface.tex->makeEmpty();
	cachedSurface.isChunkOwner=true;
	cachedSurface.isValid=false;
	return EventDispatcher::destruct();
}

void DisplayObject::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED);
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderInfo,0,Class<LoaderInfo>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleZ","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleZ,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleZ","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleZ),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_getX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_getY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(c->getSystemState(),_getZ,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(c->getSystemState(),_setZ),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(c->getSystemState(),_getVisible,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(c->getSystemState(),_setVisible),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(c->getSystemState(),_getRotation,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(c->getSystemState(),_setRotation),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,name,ASString);
	c->setDeclaredMethodByQName("parent","",Class<IFunction>::getFunction(c->getSystemState(),_getParent,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("root","",Class<IFunction>::getFunction(c->getSystemState(),_getRoot,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(c->getSystemState(),_getBlendMode,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(c->getSystemState(),_setBlendMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(c->getSystemState(),_getScale9Grid,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(c->getSystemState(),_setScale9Grid),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stage","",Class<IFunction>::getFunction(c->getSystemState(),_getStage,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(c->getSystemState(),_getMask,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(c->getSystemState(),_setMask),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(c->getSystemState(),_getAlpha,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(c->getSystemState(),_setAlpha),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",Class<IFunction>::getFunction(c->getSystemState(),_getBounds,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getRect","",Class<IFunction>::getFunction(c->getSystemState(),_getBounds,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseX","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseY","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",Class<IFunction>::getFunction(c->getSystemState(),localToGlobal,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("globalToLocal","",Class<IFunction>::getFunction(c->getSystemState(),globalToLocal,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTestObject","",Class<IFunction>::getFunction(c->getSystemState(),hitTestObject,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTestPoint","",Class<IFunction>::getFunction(c->getSystemState(),hitTestPoint,2,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transform","",Class<IFunction>::getFunction(c->getSystemState(),_getTransform,0,Class<Transform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("transform","",Class<IFunction>::getFunction(c->getSystemState(),_setTransform),SETTER_METHOD,true);
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

ASFUNCTIONBODY_GETTER_SETTER_STRINGID(DisplayObject,name);
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,accessibilityProperties);
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,scrollRect);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, rotationX);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, rotationY);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, opaqueBackground);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, metaData);

ASFUNCTIONBODY_ATOM(DisplayObject,_getter_filters)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
		throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object");
	if(argslen != 0)
		throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter");
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->filters.isNull())
		th->filters = _MR(Class<Array>::getInstanceSNoArgs(sys));
	th->filters->incRef();
	ret = asAtomHandler::fromObject(th->filters.getPtr());
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setter_filters)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
		throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object");
	if(argslen != 1)
		throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter");
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	th->filters =ArgumentConversionAtom<_NR<Array>>::toConcrete(sys,args[0],th->filters);
	th->requestInvalidation(sys,true);
}
bool DisplayObject::computeCacheAsBitmap() const
{
	return cacheAsBitmap || (!filters.isNull() && filters->size()!=0);
}

bool DisplayObject::requestInvalidationForCacheAsBitmap(InvalidateQueue* q)
{
	if (q->getCacheAsBitmapObject())
	{
		if (this->computeCacheAsBitmap() && this !=q->getCacheAsBitmapObject().getPtr())
		{
			this->incRef();
			q->addToInvalidateQueue(_MR(this));
			return true;
		}
	}
	else
	{
		if (!isMask() && cachedAsBitmapOf && cachedAsBitmapOf->computeCacheAsBitmap())
			return true;
	}
	return false;
}
ASFUNCTIONBODY_ATOM(DisplayObject,_getter_cacheAsBitmap)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
		throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object");
	if(argslen != 0)
		throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter");
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	ret = asAtomHandler::fromBool(th->computeCacheAsBitmap());
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setter_cacheAsBitmap)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
		throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object");
	if(argslen != 1)
		throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter");
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->filters.isNull() || th->filters->size()==0)
	{
		if (th->cacheAsBitmap != asAtomHandler::toInt(args[0]))
		{
			th->cacheAsBitmap = asAtomHandler::toInt(args[0]);
			th->requestInvalidation(sys,true);
		}
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getTransform)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	
	th->incRef();
	ret = asAtomHandler::fromObject(Class<Transform>::getInstanceS(sys,_MR(th)));
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setTransform)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<Transform> trans;
	ARG_UNPACK_ATOM(trans);
	if (!trans.isNull())
	{
		th->setMatrix(trans->owner->matrix);
		th->colorTransform = trans->owner->colorTransform;
	}
}

void DisplayObject::buildTraits(ASObject* o)
{
}

void DisplayObject::setMatrix(_NR<Matrix> m)
{
	bool mustInvalidate=false;
	if (m.isNull())
	{
		if (!matrix.isNull())
			mustInvalidate=true;
		matrix= NullRef;
	}
	else
	{
		Locker locker(spinlock);
		if (matrix.isNull())
			matrix= _MR(Class<Matrix>::getInstanceS(this->getSystemState()));
		if(matrix->matrix!=m->matrix)
		{
			matrix->matrix=m->matrix;
			extractValuesFromMatrix();
			mustInvalidate=true;
		}
	}
	if(mustInvalidate && onStage)
	{
		hasChanged=true;
		requestInvalidation(getSystemState());
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
			matrix= _MR(Class<Matrix>::getInstanceS(this->getSystemState()));
		if(m.getTranslateX() != tx ||
			m.getTranslateY() != ty ||
			m.getScaleX() != sx ||
			m.getScaleY() != sy ||
			m.getRotation() != rotation)
		{
			matrix->matrix=m;
			extractValuesFromMatrix();
			afterSetLegacyMatrix();
			mustInvalidate=true;
		}
	}
	if(mustInvalidate && onStage)
	{
		hasChanged=true;
		requestInvalidation(getSystemState());
	}
}

void DisplayObject::setFilters(const FILTERLIST& filterlist)
{
	if (filterlist.Filters.size())
	{
		if (filters.isNull())
			filters = _MR(Class<Array>::getInstanceSNoArgsNoFreelist(getSystemState()));
		filters->resize(0);
		auto it = filterlist.Filters.cbegin();
		while (it != filterlist.Filters.cend())
		{
			switch(it->FilterID)
			{
				case 0:
					filters->push(asAtomHandler::fromObject(Class<DropShadowFilter>::getInstanceS(getSystemState(),it->DropShadowFilter)));
					break;
				case 1:
					filters->push(asAtomHandler::fromObject(Class<BlurFilter>::getInstanceS(getSystemState(),it->BlurFilter)));
					break;
				case 2:
					filters->push(asAtomHandler::fromObject(Class<GlowFilter>::getInstanceS(getSystemState(),it->GlowFilter)));
					break;
				case 3:
					filters->push(asAtomHandler::fromObject(Class<BevelFilter>::getInstanceS(getSystemState(),it->BevelFilter)));
					break;
				case 4:
					filters->push(asAtomHandler::fromObject(Class<GradientGlowFilter>::getInstanceS(getSystemState(),it->GradientGlowFilter)));
					break;
				case 5:
					filters->push(asAtomHandler::fromObject(Class<ConvolutionFilter>::getInstanceS(getSystemState(),it->ConvolutionFilter)));
					break;
				case 6:
					filters->push(asAtomHandler::fromObject(Class<ColorMatrixFilter>::getInstanceS(getSystemState(),it->ColorMatrixFilter)));
					break;
				case 7:
					filters->push(asAtomHandler::fromObject(Class<GradientBevelFilter>::getInstanceS(getSystemState(),it->GradientBevelFilter)));
					break;
				default:
					LOG(LOG_ERROR,"Unsupported Filter Id " << (int)it->FilterID);
					break;
			}
			it++;
		}
		hasChanged=true;
		setNeedsTextureRecalculation();
	}
	else
	{
		if (!filters.isNull() && filters->size())
		{
			filters->resize(0);
			hasChanged=true;
			setNeedsTextureRecalculation();
		}
	}
	
}

void DisplayObject::becomeMaskOf(_NR<DisplayObject> m)
{
	maskOf=m;
}

void DisplayObject::setMask(_NR<DisplayObject> m)
{
	bool mustInvalidate=(mask!=m || (m && m->hasChanged)) && onStage;

	if(!mask.isNull())
	{
		//Remove previous mask
		mask->becomeMaskOf(NullRef);
	}

	mask=m;
	if(!mask.isNull())
	{
		//Use new mask
		this->incRef();
		mask->becomeMaskOf(_MR(this));
	}

	if(mustInvalidate && onStage)
	{
		hasChanged=true;
//		needsTextureRecalculation=true;
		requestInvalidation(getSystemState());
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
MATRIX DisplayObject::getConcatenatedMatrix() const
{
	if(!parent || parent == getSystemState()->mainClip)
		return getMatrix();
	else
		return parent->getConcatenatedMatrix().multiplyMatrix(getMatrix());
}

/* Return alpha value between 0 and 1. (The stored alpha value is not
 * necessary bounded.) */
float DisplayObject::clippedAlpha() const
{
	float a = alpha;
	if (!this->colorTransform.isNull())
	{
		if (alpha != 1.0 && this->colorTransform->alphaMultiplier != 1.0)
			a = alpha * this->colorTransform->alphaMultiplier /256.;
		if (this->colorTransform->alphaOffset != 0)
			a = alpha + this->colorTransform->alphaOffset /256.;
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

tiny_string DisplayObject::AVM1GetPath()
{
	tiny_string res;
	if (getParent())
		res = getParent()->AVM1GetPath();
	if (this->name != BUILTIN_STRINGS::EMPTY)
	{
		if (!res.empty())
			res += ".";
		res += getSystemState()->getStringFromUniqueId(this->name);
	}
	return res;
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
	matrix->matrix.rotate(-rotation*M_PI/180);
	//Deapply scaling
	matrix->matrix.scale(1.0/sx,1.0/sy);
}

bool DisplayObject::skipRender() const
{
	return !isMask() && !ClipDepth && (visible==false || clippedAlpha()==0.0);
}

bool DisplayObject::defaultRender(RenderContext& ctxt) const
{
	// TODO: use scrollRect
	const CachedSurface& surface=ctxt.getCachedSurface(this);
	/* surface is only modified from within the render thread
	 * so we need no locking here */
	if(!surface.isValid || !surface.tex || !surface.tex->isValid())
		return true;
	if (surface.tex->width == 0 || surface.tex->height == 0)
		return true;

	AS_BLENDMODE bl = this->blendMode;
	if (bl == BLENDMODE_NORMAL)
	{
		DisplayObject* obj = this->getParent();
		while (obj && bl == BLENDMODE_NORMAL)
		{
			bl = obj->getBlendMode();
			obj = obj->getParent();
		}
	}
	ctxt.setProperties(bl);
	ctxt.lsglLoadIdentity();
	if (surface.isMask)
		ctxt.currentMask=this;
	// ensure that the matching mask is rendered before rendering this DisplayObject
	if (ctxt.contextType == RenderContext::GL && surface.mask && surface.mask && ctxt.currentMask != surface.mask.getPtr())
		surface.mask->defaultRender(ctxt); 
	ctxt.renderTextured(*surface.tex, surface.xOffset,surface.yOffset,
			surface.tex->width, surface.tex->height,
			surface.alpha, RenderContext::RGB_MODE,
			surface.rotation,surface.xOffsetTransformed,surface.yOffsetTransformed,surface.widthTransformed,surface.heightTransformed,surface.xscale, surface.yscale,
			surface.redMultiplier, surface.greenMultiplier, surface.blueMultiplier, surface.alphaMultiplier,
			surface.redOffset, surface.greenOffset, surface.blueOffset, surface.alphaOffset,
			surface.isMask, !surface.mask.isNull(),0.0,RGB(),surface.smoothing,surface.matrix);
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

IDrawable* DisplayObject::invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	//Not supposed to be called
	throw RunTimeException("DisplayObject::invalidate");
}

void DisplayObject::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	//Let's invalidate also the mask
	if(!mask.isNull())
		mask->requestInvalidation(q);
}

void DisplayObject::updateCachedSurface(IDrawable *d)
{
	// this is called only from rendering thread, so no locking done here
	cachedSurface.xOffset=d->getXOffset();
	cachedSurface.yOffset=d->getYOffset();
	cachedSurface.xOffsetTransformed=d->getXOffsetTransformed();
	cachedSurface.yOffsetTransformed=d->getYOffsetTransformed();
	cachedSurface.widthTransformed=d->getWidthTransformed();
	cachedSurface.heightTransformed=d->getHeightTransformed();
	cachedSurface.alpha=d->getAlpha();
	cachedSurface.rotation=d->getRotation();
	cachedSurface.xscale=d->getXScale();
	cachedSurface.yscale=d->getYScale();
	cachedSurface.isMask=d->getIsMask();
	cachedSurface.mask=d->getMask();
	cachedSurface.smoothing=d->getSmoothing();
	cachedSurface.redMultiplier=d->getRedMultiplier();
	cachedSurface.greenMultiplier=d->getGreenMultiplier();
	cachedSurface.blueMultiplier=d->getBlueMultiplier();
	cachedSurface.alphaMultiplier=d->getAlphaMultiplier();
	cachedSurface.redOffset=d->getRedOffset();
	cachedSurface.greenOffset=d->getGreenOffset();
	cachedSurface.blueOffset=d->getBlueOffset();
	cachedSurface.alphaOffset=d->getAlphaOffset();
	cachedSurface.matrix=d->getMatrix();
	cachedSurface.isValid=true;
}
//TODO: Fix precision issues, Adobe seems to do the matrix mult with twips and rounds the results, 
//this way they have less pb with precision.
void DisplayObject::localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getMatrix().multiply2D(xin, yin, xout, yout);
	if(parent)
		parent->localToGlobal(xout, yout, xout, yout);
}
//TODO: Fix precision issues
void DisplayObject::globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getConcatenatedMatrix().getInverted().multiply2D(xin, yin, xout, yout);
}

void DisplayObject::setOnStage(bool staged, bool force)
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
			if (!this->cachedAsBitmapOf)
				requestInvalidation(getSystemState());
		}
		if(getVm(getSystemState())==nullptr)
			return;
		changed = true;
		invalidateCachedAsBitmapOf();
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
			// ensure that DisplayObject constructor is called if this was added by PlaceObjectTag
			// so that event listeners for "addedToStage" defined in constructor are added
			if (!this->getConstructIndicator() && this->legacy
					&& this->getClass()->hasConstructor() && getClass()->getConstructor()->getMethodInfo() && getClass()->getConstructor()->getMethodInfo()->numArgs()==0)
			{
				asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
				getClass()->handleConstruction(obj,nullptr,0,true);
			}

			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"addedToStage"));
			// the main clip is added to stage after the base class is constructed,
			// but there may be addedToStage event handlers added in the constructor of the derived class,
			// so we can't execute the event directly for the main clip
			if(isVmThread() && this != getSystemState()->mainClip)
				ABCVm::publicHandleEvent(this,e);
			else
			{
				this->incRef();
				getVm(getSystemState())->prependEvent(_MR(this),e);
			}
			if (this->is<MovieClip>())
				getSystemState()->stage->removeHiddenObject(this->as<MovieClip>());
		}
		else if(onStage==false)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"removedFromStage"));
			if(isVmThread())
				ABCVm::publicHandleEvent(this,e);
			else
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),e);
			}
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
	ARG_UNPACK_ATOM (val);
	if (!th->loadedFrom->usesActionScript3) // AVM1 uses alpha values from 0-100
		val /= 100.0;
	
	/* The stored value is not clipped, _getAlpha will return the
	 * stored value even if it is outside the [0, 1] range. */
	if(th->alpha != val)
	{
		th->alpha=val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(sys);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->loadedFrom->usesActionScript3)
		asAtomHandler::setNumber(ret,sys,th->alpha);
	else // AVM1 uses alpha values from 0-100
		asAtomHandler::setNumber(ret,sys,th->alpha*100.0);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMask)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if(th->mask.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->mask->incRef();
	ret = asAtomHandler::fromObject(th->mask.getPtr());
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
	asAtomHandler::setNumber(ret,sys,th->sx);
}

void DisplayObject::setScaleX(number_t val)
{
	if (std::isnan(val))
		return;
	ROUND_TO_TWIPS(val);
	//Apply the difference
	if(sx!=val)
	{
		sx=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
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
	asAtomHandler::setNumber(ret,sys,th->sy);
}

void DisplayObject::setScaleY(number_t val)
{
	if (std::isnan(val))
		return;
	ROUND_TO_TWIPS(val);
	//Apply the difference
	if(sy!=val)
	{
		sy=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
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
	asAtomHandler::setNumber(ret,sys,th->sz);
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
		if(onStage)
			requestInvalidation(getSystemState());
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
	asAtomHandler::setNumber(ret,sys,th->tx);
}

void DisplayObject::setX(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	if (std::isnan(val))
		return;
	ROUND_TO_TWIPS(val);
	//Apply translation, it's trivial
	if(tx!=val)
	{
		tx=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

void DisplayObject::setY(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	if (std::isnan(val))
		return;
	ROUND_TO_TWIPS(val);
	//Apply translation, it's trivial
	if(ty!=val)
	{
		ty=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
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
		if(onStage)
			requestInvalidation(getSystemState());
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
	asAtomHandler::setNumber(ret,sys,th->ty);
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
	asAtomHandler::setNumber(ret,sys,th->tz);
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

	if(asAtomHandler::is<Undefined>(args[0]) || asAtomHandler::is<Null>(args[0]))
	{
		ret =  asAtomHandler::fromObject(Class<Rectangle>::getInstanceS(sys));
		return;
	}
	if (!asAtomHandler::is<DisplayObject>(args[0]))
		LOG(LOG_ERROR,"DisplayObject.getBounds invalid type:"<<asAtomHandler::toDebugString(args[0]));
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
	DisplayObject* target=asAtomHandler::as<DisplayObject>(args[0]);
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

	Rectangle* res=Class<Rectangle>::getInstanceS(sys);
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
	if (r.isNull())
	{
		// if this DisplayObject is not yet added to the stage we just use the mainclip
		r = _MR(sys->mainClip);
		r->incRef();
	}
	if(r.isNull() || r->loaderInfo.isNull())
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	
	r->loaderInfo->incRef();
	ret = asAtomHandler::fromObject(r->loaderInfo.getPtr());
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
	//DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"DispalyObject.Scale9Grid");
	asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setScale9Grid)
{
	//DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"DispalyObject.Scale9Grid");
	asAtomHandler::setUndefined(ret);
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

	ret = asAtomHandler::fromString(sys,res);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setBlendMode)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	tiny_string val;
	ARG_UNPACK_ATOM(val);

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
}

ASFUNCTIONBODY_ATOM(DisplayObject,localToGlobal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t tempx, tempy;

	th->localToGlobal(pt->getX(), pt->getY(), tempx, tempy);

	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(sys,tempx, tempy));
}

ASFUNCTIONBODY_ATOM(DisplayObject,globalToLocal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t tempx, tempy;

	th->globalToLocal(pt->getX(), pt->getY(), tempx, tempy);

	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(sys,tempx, tempy));
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
		th->rotation=val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(sys);
	}
}

void DisplayObject::setParent(DisplayObjectContainer *p)
{
	Locker locker(spinlock);
	if(parent!=p)
	{
		if (p)
		{
			getSystemState()->removeFromResetParentList(this);
			if (p->computeCacheAsBitmap())
			{
				p->incRef();
				cachedAsBitmapOf = _MNR(p);
			}
			else
				cachedAsBitmapOf = p->cachedAsBitmapOf;
		}
		parent=p;
		hasChanged=true;
		if(onStage && !cachedAsBitmapOf && !getSystemState()->isShuttingDown())
			requestInvalidation(getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getParent)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if(!th->parent)
	{
		asAtomHandler::setUndefined(ret);
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
	asAtomHandler::setNumber(ret,sys,th->rotation);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setVisible)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	bool newval = asAtomHandler::Boolean_concrete(args[0]);
	if (newval != th->visible)
	{
		th->visible=newval;
		th->requestInvalidation(sys);
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

number_t DisplayObject::computeWidth()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix(false));

	return (ret)?(x2-x1):0;
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
	asAtomHandler::setNumber(ret,sys,th->computeWidth());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setWidth)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t newwidth=asAtomHandler::toNumber(args[0]);
	if (std::isnan(newwidth))
		return;
	ROUND_TO_TWIPS(newwidth);

	number_t xmin,xmax,y1,y2;
	if(!th->boundsRect(xmin,xmax,y1,y2))
		return;

	number_t width=xmax-xmin;
	if(width==0) //Cannot scale, nothing to do (See Reference)
		return;
	
	if(width*th->sx!=newwidth) //If the width is changing, calculate new scale
	{
		if(th->useLegacyMatrix)
			th->useLegacyMatrix=false;
		th->setScaleX(newwidth/width);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getHeight)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->computeHeight());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setHeight)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t newheight=asAtomHandler::toNumber(args[0]);
	if (std::isnan(newheight))
		return;
	ROUND_TO_TWIPS(newheight);

	number_t x1,x2,ymin,ymax;
	if(!th->boundsRect(x1,x2,ymin,ymax))
		return;

	number_t height=ymax-ymin;
	if(height==0) //Cannot scale, nothing to do (See Reference)
		return;

	if(height*th->sy!=newheight) //If the height is changing, calculate new scale
	{
		if(th->useLegacyMatrix)
			th->useLegacyMatrix=false;
		th->setScaleY(newheight/height);
	}
}

Vector2f DisplayObject::getLocalMousePos()
{
	return getConcatenatedMatrix().getInverted().multiply2D(getSystemState()->getInputThread()->getMousePos());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMouseX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->getLocalMousePos().x);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMouseY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->getLocalMousePos().y);
}

_NR<DisplayObject> DisplayObject::hitTest(_NR<DisplayObject> last, number_t x, number_t y, HIT_TYPE type,bool interactiveObjectsOnly)
{
	if((!(visible || type == GENERIC_HIT_INVISIBLE) || !isConstructed()) && !isMask())
		return NullRef;

	//First check if there is any mask on this object, if so the point must be inside the mask to go on
	if(!mask.isNull())
	{
		//First compute the global coordinates from the local ones
		//TODO: we may also pass the global coordinates to all the calls
		const MATRIX& thisMatrix = this->getConcatenatedMatrix();
		number_t globalX, globalY;
		thisMatrix.multiply2D(x,y,globalX,globalY);
		//Now compute the coordinates local to the mask
		const MATRIX& maskMatrix = mask->getConcatenatedMatrix();
		if(!maskMatrix.isInvertible())
		{
			//If the matrix is not invertible the mask as collapsed to zero size
			//If the mask is zero sized then the object is not visible
			return NullRef;
		}
		number_t maskX, maskY;
		maskMatrix.getInverted().multiply2D(globalX,globalY,maskX,maskY);
		if(mask->hitTest(last, maskX, maskY, type,false)==false)
			return NullRef;
	}

	return hitTestImpl(last, x,y, type,interactiveObjectsOnly);
}

/* Display objects have no children in general,
 * so we skip to calling the constructor, if necessary.
 * This is called in vm's thread context */
void DisplayObject::initFrame()
{
	if(!isConstructed() && getClass())
	{
		asAtom o = asAtomHandler::fromObject(this);
		getClass()->handleConstruction(o,NULL,0,true);

		/*
		 * Legacy objects have their display list properties set on creation, but
		 * the related events must only be sent after the constructor is sent.
		 * This is from "Order of Operations".
		 */
		if(parent)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"added"));
			ABCVm::publicHandleEvent(this,e);
		}
		if(onStage)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"addedToStage"));
			ABCVm::publicHandleEvent(this,e);
		}
	}
}

void DisplayObject::executeFrameScript()
{
}

bool DisplayObject::needsActionScript3() const
{
	return this->loadedFrom->usesActionScript3;
}

void DisplayObject::constructionComplete()
{
	RELEASE_WRITE(constructed,true);
}
void DisplayObject::afterConstruction()
{
	if(!loaderInfo.isNull())
	{
		this->incRef();
		loaderInfo->objectHasLoaded(_MR(this));
	}
//	hasChanged=true;
//	needsTextureRecalculation=true;
//	if(onStage)
//		requestInvalidation(getSystemState());
}

void DisplayObject::invalidateCachedAsBitmapOf()
{
	DisplayObject* c = cachedAsBitmapOf.getPtr();
	while (c)
	{
		c->hasChanged=true;
		c->setNeedsTextureRecalculation();
		if (!c->cachedAsBitmapOf)
		{
			c->requestInvalidation(c->getSystemState());
			break;
		}
		c = c->cachedAsBitmapOf.getPtr();
	}
}

void DisplayObject::setNeedsTextureRecalculation(bool skippable)
{
	textureRecalculationSkippable=skippable;
	needsTextureRecalculation=true;
	if (!cachedSurface.isChunkOwner)
		cachedSurface.tex=nullptr;
	cachedSurface.isChunkOwner=true;
}

void DisplayObject::gatherMaskIDrawables(std::vector<IDrawable::MaskData>& masks) const
{
	if(mask.isNull())
		return;

	//If the mask is hard we need the drawable for each child
	//If the mask is soft we need the rendered final result
	IDrawable::MASK_MODE maskMode = IDrawable::HARD_MASK;
	//For soft masking to work, both the object and the mask must be
	//cacheAsBitmap and the mask must be on the stage
	if(this->computeCacheAsBitmap() && mask->computeCacheAsBitmap() && mask->isOnStage())
		maskMode = IDrawable::SOFT_MASK;

	if(maskMode == IDrawable::HARD_MASK)
	{
		SoftwareInvalidateQueue queue(NullRef);
		mask->requestInvalidation(&queue);
		for(auto it=queue.queue.begin();it!=queue.queue.end();it++)
		{
			DisplayObject* target=(*it).getPtr();
			//Get the drawable from each of the added objects
			IDrawable* drawable=target->invalidate(nullptr, MATRIX(),false,nullptr,nullptr);
			if(drawable==nullptr)
				continue;
			masks.emplace_back(drawable, maskMode);
		}
	}
	else
	{
		IDrawable* drawable=nullptr;
		if(mask->is<DisplayObjectContainer>())
		{
			//HACK: use bitmap temporarily
			number_t xmin,xmax,ymin,ymax;
			bool ret=mask->getBounds(xmin,xmax,ymin,ymax,mask->getConcatenatedMatrix());
			if(ret==false)
				return;
			_R<BitmapData> data(Class<BitmapData>::getInstanceS(getSystemState(),xmax-xmin,ymax-ymin));
			//Forge a matrix. It must contain the right rotation and scaling while translation
			//only compensate for the xmin/ymin offset
			MATRIX m=mask->getConcatenatedMatrix();
			m.x0 -= xmin;
			m.y0 -= ymin;
			data->drawDisplayObject(mask.getPtr(), m,false,false);
			_R<Bitmap> bmp(Class<Bitmap>::getInstanceS(getSystemState(),data));

			//The created bitmap is already correctly scaled and rotated
			//Just apply the needed offset
			MATRIX m2(1,1,0,0,xmin,ymin);
			drawable=bmp->invalidate(nullptr, m2,false,nullptr,nullptr);
		}
		else
			drawable=mask->invalidate(nullptr, MATRIX(),false,nullptr,nullptr);

		if(drawable==nullptr)
			return;
		masks.emplace_back(drawable, maskMode);
	}
}

void DisplayObject::computeMasksAndMatrix(const DisplayObject* target, std::vector<IDrawable::MaskData>& masks, MATRIX& totalMatrix,bool includeRotation, bool &isMask, _NR<DisplayObject>&mask) const
{
	const DisplayObject* cur=this;
	bool gatherMasks = true;
	isMask = cur->ClipDepth || !cur->maskOf.isNull();
	mask = cur->mask;
	while(cur && cur!=target)
	{
		totalMatrix=cur->getMatrix(includeRotation).multiplyMatrix(totalMatrix);
		if(gatherMasks)
		{
			if (!cur->mask.isNull() && mask.isNull())
				mask=cur->mask;
			if (cur->ClipDepth || !cur->maskOf.isNull())
				isMask=true;
			if(!cur->maskOf.isNull())
			{
				//Stop gathering masks if any level of the hierarchy it's a mask
				masks.clear();
				masks.shrink_to_fit();
				gatherMasks=false;
			}
		}
		cur=cur->getParent();
	}
}
void DisplayObject::DrawToBitmap(BitmapData* bm,const MATRIX& initialMatrix,bool smoothing, bool forcachedbitmap)
{
	DisplayObjectContainer* origparent=this->parent;
	number_t origrotation = this->rotation;
	number_t origsx = this->sx;
	number_t origsy = this->sy;
	number_t origtx = this->tx;
	number_t origty = this->ty;
	// temporarily reset all position settings to avoid matrix manipulation from this DisplayObject
	this->parent = nullptr;
	this->rotation=0;
	this->sx=1;
	this->sy=1;
	this->tx=0;
	this->ty=0;
	bm->drawDisplayObject(this, initialMatrix,smoothing,forcachedbitmap);
	// reset position to original settings
	this->parent=origparent;
	this->rotation=origrotation;
	this->sx=origsx;
	this->sy=origsy;
	this->tx=origtx;
	this->ty=origty;
}
IDrawable* DisplayObject::getCachedBitmapDrawable(DisplayObject* target,const MATRIX& initialMatrix,_NR<DisplayObject>* pcachedBitmap)
{
	if (!computeCacheAsBitmap())
		return nullptr;
	number_t xmin,xmax,ymin,ymax;
	MATRIX m=getMatrix();
	bool ret=getBounds(xmin,xmax,ymin,ymax,m);
	if(ret==false || xmax-xmin >= 8192 || ymax-ymin >= 8192 || ((xmax-xmin)*(ymax-ymin)) >= 16777216)
		return nullptr;
	uint32_t maxfilterborder=0;
	if (filters)
	{
		// get maximum border in pixels around cached bitmap needed to properly apply filters
		for (uint32_t i = 0; i < filters->size(); i++)
		{
			asAtom f = asAtomHandler::invalidAtom;
			filters->at_nocheck(f,i);
			if (asAtomHandler::is<BitmapFilter>(f))
				maxfilterborder = max(maxfilterborder,asAtomHandler::as<BitmapFilter>(f)->getMaxFilterBorder());
		}
	}
	uint32_t w=ceil(xmax-xmin)+maxfilterborder*2;
	uint32_t h=ceil(ymax-ymin)+maxfilterborder*2;
	if (needsTextureRecalculation|| !cachedBitmap)
	{
		if (!cachedBitmap
				|| cachedBitmap->getBitmapSize().width != w
				|| cachedBitmap->getBitmapSize().height != h)
		{
			_R<BitmapData> data(Class<BitmapData>::getInstanceS(getSystemState(),w,h));
			data->incRef();
			cachedBitmap=_MR(Class<Bitmap>::getInstanceS(getSystemState(),data));
		}
		MATRIX m0=m;
		m0.translate(-(xmin-maxfilterborder) ,-(ymin-maxfilterborder));
		DrawToBitmap(cachedBitmap->bitmapData.getPtr(),m0,true,true);
		if (filters)
		{
			for (uint32_t i = 0; i < filters->size(); i++)
			{
				asAtom f = asAtomHandler::invalidAtom;
				filters->at_nocheck(f,i);
				if (asAtomHandler::is<BitmapFilter>(f))
					asAtomHandler::as<BitmapFilter>(f)->applyFilter(cachedBitmap->bitmapData->getBitmapContainer().getPtr(),nullptr,RECT(0,w,0,h),0,0);
			}
		}
		// apply colortransform for cached bitmap after the filters are applied
		ColorTransform* ct = colorTransform.getPtr();
		if (ct)
		{
			ct->applyTransformation(cachedBitmap->bitmapData->getBitmapContainer()->getData()
									,cachedBitmap->bitmapData->getBitmapContainer()->getWidth()*cachedBitmap->bitmapData->getBitmapContainer()->getHeight()*4);
		}
		
		cachedBitmap->resetNeedsTextureRecalculation();
		cachedBitmap->hasChanged=true;
		if (!this->cachedAsBitmapOf)
		{
			// force texture upload
			cachedBitmap->bitmapData->addUser(cachedBitmap.getPtr());
		}
	}
	if (pcachedBitmap)
		*pcachedBitmap = cachedBitmap;
	this->resetNeedsTextureRecalculation();
	this->hasChanged=false;
	MATRIX m1(1,1,0,0,xmin-maxfilterborder,ymin-maxfilterborder);
	return cachedBitmap->invalidateFromSource(target, initialMatrix,true,this->getParent(),m1,this);
}

bool DisplayObject::findParent(DisplayObject *d) const
{
	if (this == d)
		return true;
	if (!parent)
		return false;
	return parent->findParent(d);
}

// Compute the minimal, axis aligned bounding box in global
// coordinates
bool DisplayObject::boundsRectGlobal(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	number_t x1, x2, y1, y2;
	if (!boundsRect(x1, x2, y1, y2))
		return abstract_b(getSystemState(),false);

	localToGlobal(x1, y1, x1, y1);
	localToGlobal(x2, y2, x2, y2);

	if (!loadedFrom->usesActionScript3 && getRoot())
	{
		// it seems that in AVM1 adobe doesn't use the stage coordinates as reference point, instead it's the root object
		Vector2f rxy =getRoot()->getXY();
		x1-=rxy.x;
		x2-=rxy.x;
		y1-=rxy.y;
		y2-=rxy.y;
	}
	// Mapping to global may swap min and max values (for example,
	// rotation by 180 degrees)
	xmin = dmin(x1, x2);
	xmax = dmax(x1, x2);
	ymin = dmin(y1, y2);
	ymax = dmax(y1, y2);

	return true;
}

ASFUNCTIONBODY_ATOM(DisplayObject,hitTestObject)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<DisplayObject> another;
	ARG_UNPACK_ATOM(another);
	number_t xmin = 0, xmax, ymin = 0, ymax;
	if (!th->boundsRectGlobal(xmin, xmax, ymin, ymax))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	number_t xmin2, xmax2, ymin2, ymax2;
	if (!another->boundsRectGlobal(xmin2, xmax2, ymin2, ymax2))
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
	ARG_UNPACK_ATOM (x) (y) (shapeFlag, false);

	number_t xmin, xmax, ymin, ymax;
	if (!th->boundsRectGlobal(xmin, xmax, ymin, ymax))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	bool insideBoundingBox = (xmin <= x) && (x < xmax) && (ymin <= y) && (y < ymax);

	if (!shapeFlag)
	{
		asAtomHandler::setBool(ret,insideBoundingBox);
		return;
	}
	else
	{
		if (!insideBoundingBox)
		{
			asAtomHandler::setBool(ret,false);
			return;
		}

		number_t localX;
		number_t localY;
		th->globalToLocal(x, y, localX, localY);

		// Hmm, hitTest will also check the mask, is this the
		// right thing to do?
		th->incRef();
		_NR<DisplayObject> hit = th->hitTest(_MR(th), localX, localY,
						     HIT_TYPE::GENERIC_HIT_INVISIBLE,false);

		asAtomHandler::setBool(ret,!hit.isNull());
	}
}

multiname* DisplayObject::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset)
{
	multiname* res = EventDispatcher::setVariableByMultiname(name,o,allowConst,alreadyset);
	if (!needsActionScript3())
	{
		if (name.name_s_id == BUILTIN_STRINGS::STRING_ONENTERFRAME ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD)
		{
			if (asAtomHandler::isFunction(o))
			{
				this->incRef();
				getSystemState()->registerFrameListener(_MR(this));
				getSystemState()->stage->AVM1AddEventListener(this);
				setIsEnumerable(name, false);
			}
		}
		if (this->is<InteractiveObject>() && (
			name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEMOVE ||
			name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEDOWN ||
			name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEUP ||
			name.name_s_id == BUILTIN_STRINGS::STRING_ONPRESS ||
			name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEWHEEL ||
			name.name_s_id == BUILTIN_STRINGS::STRING_ONRELEASE))
		{
			this->as<InteractiveObject>()->setMouseEnabled(true);
			getSystemState()->stage->AVM1AddMouseListener(this);
			setIsEnumerable(name, false);
		}
	}
	return res;
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
		if (pr->hasPropertyByMultiname(name,true,false))
		{
			this->incRef();
			getSystemState()->registerFrameListener(_MR(this));
			getSystemState()->stage->AVM1AddEventListener(this);
		}
		name.name_s_id = BUILTIN_STRINGS::STRING_ONLOAD;
		if (pr->hasPropertyByMultiname(name,true,false))
		{
			this->incRef();
			getSystemState()->registerFrameListener(_MR(this));
			getSystemState()->stage->AVM1AddEventListener(this);
		}
		if (this->is<InteractiveObject>())
		{
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEMOVE;
			if (pr->hasPropertyByMultiname(name,true,false))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(this);
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEDOWN;
			if (pr->hasPropertyByMultiname(name,true,false))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(this);
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEUP;
			if (pr->hasPropertyByMultiname(name,true,false))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(this);
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONPRESS;
			if (pr->hasPropertyByMultiname(name,true,false))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(this);
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONMOUSEWHEEL;
			if (pr->hasPropertyByMultiname(name,true,false))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(this);
			}
			name.name_s_id = BUILTIN_STRINGS::STRING_ONRELEASE;
			if (pr->hasPropertyByMultiname(name,true,false))
			{
				this->as<InteractiveObject>()->setMouseEnabled(true);
				getSystemState()->stage->AVM1AddMouseListener(this);
			}
		}
		pr = pr->getprop_prototype();
	}
}
bool DisplayObject::deleteVariableByMultiname(const multiname& name)
{
	bool res = EventDispatcher::deleteVariableByMultiname(name);
	if (!this->loadedFrom->usesActionScript3)
	{
		if (name.name_s_id == BUILTIN_STRINGS::STRING_ONENTERFRAME ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD)
		{
			this->incRef();
			getSystemState()->unregisterFrameListener(_MR(this));
		}
		if (this->is<InteractiveObject>() && (
				name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEMOVE ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEDOWN ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEUP ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONPRESS ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEWHEEL ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONRELEASE))
		{
			this->as<InteractiveObject>()->setMouseEnabled(false);
			getSystemState()->stage->AVM1RemoveMouseListener(this);
		}
		tiny_string s = name.normalizedNameUnresolved(getSystemState()).lowercase();
		AVM1SetVariable(s,asAtomHandler::undefinedAtom,false);
	}
	return res;
}
void DisplayObject::removeAVM1Listeners()
{
	getSystemState()->stage->AVM1RemoveMouseListener(this);
	getSystemState()->stage->AVM1RemoveKeyboardListener(this);
	getSystemState()->stage->AVM1RemoveEventListener(this);
	this->incRef();
	getSystemState()->unregisterFrameListener(_MR(this));
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->sx*100.0);
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
	asAtomHandler::setNumber(ret,sys,th->sy*100.0);
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
	th->loadedFrom->incRef();
	ret = asAtomHandler::fromObject(th->loadedFrom);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getURL)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	ret = asAtomHandler::fromString(sys,th->loadedFrom->getOrigin().getURL());
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
					hitTestObject(ret,sys,obj,args,1);
				}
				else
				{
					tiny_string s = asAtomHandler::toString(args[0],sys);
					DisplayObject* path = asAtomHandler::as<DisplayObject>(obj)->AVM1GetClipFromPath(s);
					if (path)
					{
						asAtom pathobj = asAtomHandler::fromObject(path);
						hitTestObject(ret,sys,obj,&pathobj,1);
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
		hitTestPoint(ret,sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_localToGlobal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	
	ASObject* pt=asAtomHandler::toObject(args[0],sys);
	
	asAtom x = asAtomHandler::fromInt(0);
	asAtom y = asAtomHandler::fromInt(0);
	multiname mx(nullptr);
	mx.name_type=multiname::NAME_STRING;
	mx.name_s_id=sys->getUniqueStringId("x");
	mx.isAttribute = false;
	pt->getVariableByMultiname(x,mx);
	multiname my(nullptr);
	my.name_type=multiname::NAME_STRING;
	my.name_s_id=sys->getUniqueStringId("y");
	my.isAttribute = false;
	pt->getVariableByMultiname(y,my);

	number_t tempx, tempy;

	th->localToGlobal(asAtomHandler::toNumber(x), asAtomHandler::toNumber(y), tempx, tempy);
	asAtomHandler::setNumber(x,sys,tempx);
	asAtomHandler::setNumber(y,sys,tempy);
	pt->setVariableByMultiname(mx,x,CONST_ALLOWED);
	pt->setVariableByMultiname(my,y,CONST_ALLOWED);
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_globalToLocal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	
	ASObject* pt=asAtomHandler::toObject(args[0],sys);
	
	asAtom x = asAtomHandler::fromInt(0);
	asAtom y = asAtomHandler::fromInt(0);
	multiname mx(nullptr);
	mx.name_type=multiname::NAME_STRING;
	mx.name_s_id=sys->getUniqueStringId("x");
	mx.isAttribute = false;
	pt->getVariableByMultiname(x,mx);
	multiname my(nullptr);
	my.name_type=multiname::NAME_STRING;
	my.name_s_id=sys->getUniqueStringId("y");
	my.isAttribute = false;
	pt->getVariableByMultiname(y,my);

	number_t tempx, tempy;

	th->globalToLocal(asAtomHandler::toNumber(x), asAtomHandler::toNumber(y), tempx, tempy);
	asAtomHandler::setNumber(x,sys,tempx);
	asAtomHandler::setNumber(y,sys,tempy);
	pt->setVariableByMultiname(mx,x,CONST_ALLOWED);
	pt->setVariableByMultiname(my,y,CONST_ALLOWED);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBytesLoaded)
{
	if (sys->mainClip->loaderInfo)
	{
		asAtomHandler::setUInt(ret,sys,sys->mainClip->loaderInfo->getBytesLoaded());
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBytesTotal)
{
	if (sys->mainClip->loaderInfo)
	{
		asAtomHandler::setUInt(ret,sys,sys->mainClip->loaderInfo->getBytesTotal());
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getQuality)
{
	ret = asAtomHandler::fromString(sys,sys->stage->quality.uppercase());
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setQuality)
{
	if (argslen > 0)
		sys->stage->quality = asAtomHandler::toString(args[0],sys).uppercase();
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->alpha*100.0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t val;
	ARG_UNPACK_ATOM (val);
	val /= 100.0;
	if(th->alpha != val)
	{
		th->alpha=val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(sys);
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBounds)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);

	ASObject* o =  Class<ASObject>::getInstanceS(sys);
	ret = asAtomHandler::fromObject(o);
	DisplayObject* target= th;
	if(argslen>=1) // contrary to spec adobe allows getBounds with zero parameters
	{
		if (asAtomHandler::is<Undefined>(args[0]) || asAtomHandler::is<Null>(args[0]))
			return;
		if (!asAtomHandler::is<DisplayObject>(args[0]))
			LOG(LOG_ERROR,"DisplayObject.getBounds invalid type:"<<asAtomHandler::toDebugString(args[0]));
		assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
		target =asAtomHandler::as<DisplayObject>(args[0]);
	}
	
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
	th->getBounds(x1,x2,y1,y2, m);

	asAtom v=asAtomHandler::invalidAtom;
	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=sys->getUniqueStringId("xMin");
	v = asAtomHandler::fromNumber(sys,x1,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
	name.name_s_id=sys->getUniqueStringId("xMax");
	v = asAtomHandler::fromNumber(sys,x2,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
	name.name_s_id=sys->getUniqueStringId("yMin");
	v = asAtomHandler::fromNumber(sys,y1,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
	name.name_s_id=sys->getUniqueStringId("yMax");
	v = asAtomHandler::fromNumber(sys,y2,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
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
		DisplayObjectContainer::swapChildren(ret,sys,obj,newargs,2);
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

void DisplayObject::AVM1SetupMethods(Class_base* c)
{
	// setup all methods and properties available for MovieClips in AVM1
	
	c->destroyContents();
	c->borrowedVariables.destroyContents();
	c->setDeclaredMethodByQName("_x","",Class<IFunction>::getFunction(c->getSystemState(),_getX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_x","",Class<IFunction>::getFunction(c->getSystemState(),_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_y","",Class<IFunction>::getFunction(c->getSystemState(),_getY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_y","",Class<IFunction>::getFunction(c->getSystemState(),_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_visible","",Class<IFunction>::getFunction(c->getSystemState(),_getVisible),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_visible","",Class<IFunction>::getFunction(c->getSystemState(),_setVisible),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_xscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getScaleX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_xscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setScaleX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_yscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getScaleY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_yscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setScaleY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",Class<IFunction>::getFunction(c->getSystemState(),_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",Class<IFunction>::getFunction(c->getSystemState(),_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_parent","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getParent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_root","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getRoot),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_url","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTest","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_hitTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hittest","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_hitTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_localToGlobal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("globalToLocal","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_globalToLocal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getBytesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getBytesLoaded),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getBytesTotal","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getBytesTotal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("_xmouse","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_ymouse","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_quality","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getQuality),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_quality","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setQuality),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_alpha","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getAlpha),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_alpha","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setAlpha),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getBounds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapDepths","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_swapDepths),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDepth","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getDepth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMask","",Class<IFunction>::getFunction(c->getSystemState(),_setMask),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transform","",Class<IFunction>::getFunction(c->getSystemState(),_getTransform),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_rotation","",Class<IFunction>::getFunction(c->getSystemState(),_getRotation),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_rotation","",Class<IFunction>::getFunction(c->getSystemState(),_setRotation),SETTER_METHOD,true);
}
DisplayObject *DisplayObject::AVM1GetClipFromPath(tiny_string &path)
{
	if (path.empty() || path == "this")
		return this;
	if (path =="_root")
	{
		return loadedFrom;
	}
	if (path.startsWith("/"))
	{
		tiny_string newpath = path.substr_bytes(1,path.numBytes()-1);
		MovieClip* root = getRoot().getPtr();
		if (root)
			return root->AVM1GetClipFromPath(newpath);
		LOG(LOG_ERROR,"AVM1: no root movie clip for path:"<<path<<" "<<this->toDebugString());
		return nullptr;
	}
	if (path.startsWith("../"))
	{
		tiny_string newpath = path.substr_bytes(3,path.numBytes()-3);
		if (this->getParent() && this->getParent()->is<MovieClip>())
			return this->getParent()->as<MovieClip>()->AVM1GetClipFromPath(newpath);
		LOG(LOG_ERROR,"AVM1: no parent clip for path:"<<path<<" "<<this->toDebugString());
		return nullptr;
	}
	uint32_t pos = path.find("/");
	tiny_string subpath = (pos == tiny_string::npos) ? path : path.substr_bytes(0,pos);
	if (subpath.empty())
	{
		return nullptr;
	}
	// path "/stage" is mapped to the root movie (?) 
	if (this == getSystemState()->mainClip && subpath == "stage")
		return this;
	uint32_t posdot = subpath.find(".");
	if (posdot != tiny_string::npos)
	{
		tiny_string subdotpath =  subpath.substr_bytes(0,posdot);
		if (subdotpath.empty())
			return nullptr;
		DisplayObject* parent = AVM1GetClipFromPath(subdotpath);
		if (!parent)
			return nullptr;
		tiny_string localname = subpath.substr_bytes(posdot+1,subpath.numBytes()-posdot-1);
		return parent->AVM1GetClipFromPath(localname);
	}
	
	multiname objName(nullptr);
	objName.name_type=multiname::NAME_STRING;
	objName.name_s_id=getSystemState()->getUniqueStringId(subpath);
	objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	asAtom ret=asAtomHandler::invalidAtom;
	getVariableByMultiname(ret,objName);
	if (asAtomHandler::is<DisplayObject>(ret))
	{
		if (pos == tiny_string::npos)
			return asAtomHandler::as<DisplayObject>(ret);
		else
		{
			subpath = path.substr_bytes(pos+1,path.numBytes()-pos-1);
			return asAtomHandler::as<DisplayObject>(ret)->AVM1GetClipFromPath(subpath);
		}
	}
	return nullptr;
}

void DisplayObject::AVM1SetVariable(tiny_string &name, asAtom v, bool setMember)
{
	if (name.empty())
		return;
	if (name.startsWith("/"))
	{
		tiny_string newpath = name.substr_bytes(1,name.numBytes()-1);
		MovieClip* root = loadedFrom;
		if (root)
			root->AVM1SetVariable(newpath,v);
		else
			LOG(LOG_ERROR,"AVM1: no root movie clip for name:"<<name<<" "<<this->toDebugString());
		return;
	}
	uint32_t pos = name.find(":");
	if (pos == tiny_string::npos)
	{
		tiny_string localname = name.lowercase();
		uint32_t nameIdOriginal = getSystemState()->getUniqueStringId(name);
		uint32_t nameId = getSystemState()->getUniqueStringId(localname);
		auto it = avm1variables.find(nameId);
		if (it != avm1variables.end())
			ASATOM_DECREF(it->second);
		if (asAtomHandler::isUndefined(v))
			avm1variables.erase(nameId);
		else
		{
			ASATOM_INCREF(v);
			avm1variables[nameId] = v;
			if (asAtomHandler::is<AVM1Function>(v))
			{
				AVM1Function* f = asAtomHandler::as<AVM1Function>(v);
				f->incRef();
				avm1functions[nameIdOriginal] = _MR(f);
			}
		}
		if (setMember)
		{
			multiname objName(NULL);
			objName.name_type=multiname::NAME_STRING;
			objName.name_s_id=nameIdOriginal;
			setVariableByMultiname(objName,v, ASObject::CONST_ALLOWED);
		}
		AVM1UpdateVariableBindings(nameId,v);
	}
	else if (pos == 0)
	{
		tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1).lowercase();
		uint32_t nameId = getSystemState()->getUniqueStringId(localname);
		auto it = avm1variables.find(nameId);
		if (it != avm1variables.end())
			ASATOM_DECREF(it->second);
		if (asAtomHandler::isUndefined(v))
			avm1variables.erase(nameId);
		else
			avm1variables[nameId] = v;
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

asAtom DisplayObject::AVM1GetVariable(const tiny_string &name, bool checkrootvars)
{
	uint32_t pos = name.find(":");
	if (pos == tiny_string::npos)
	{
		if (loadedFrom->version > 4 && getSystemState()->avm1global)
		{
			// first check for class names
			asAtom ret=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=getSystemState()->getUniqueStringId(name);
			m.isAttribute = false;
			getSystemState()->avm1global->getVariableByMultiname(ret,m);
			if(!asAtomHandler::isInvalid(ret))
				return ret;
		}
		auto it = avm1variables.find(getSystemState()->getUniqueStringId(name.lowercase()));
		if (it != avm1variables.end())
			return it->second;
	}
	else if (pos == 0)
	{
		tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
		return AVM1GetVariable(localname.lowercase());
	}
	else
	{
		tiny_string path = name.substr_bytes(0,pos).lowercase();
		DisplayObject* clip = AVM1GetClipFromPath(path);
		if (clip)
		{
			tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
			return clip->AVM1GetVariable(localname.lowercase());
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
			ret = asAtomHandler::fromObjectNoPrimitive(clip);
			return ret;
		}
		path = name.lowercase();
		clip = AVM1GetClipFromPath(path);
		if (clip && clip != this)
		{
			ret = asAtomHandler::fromObjectNoPrimitive(clip);
			return ret;
		}
	}

	if (loadedFrom->version > 4 && checkrootvars)
	{
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=getSystemState()->getUniqueStringId(name);
		m.isAttribute = false;
		getVariableByMultiname(ret,m);
		if (asAtomHandler::isInvalid(ret))// get Variable from root movie
			loadedFrom->getVariableByMultiname(ret,m);
	}
	return ret;
}
void DisplayObject::AVM1UpdateVariableBindings(uint32_t nameID, asAtom& value)
{
	auto it = variablebindings.find(nameID);
	while (it != variablebindings.end() && it->first == nameID)
	{
		(*it).second->UpdateVariableBinding(value);
		it++;
	}
}
asAtom DisplayObject::getVariableBindingValue(const tiny_string &name)
{
	uint32_t pos = name.find(".");
	asAtom ret=asAtomHandler::invalidAtom;
	if (pos == tiny_string::npos)
	{
		ret = AVM1GetVariable(name);
	}
	else
	{
		tiny_string firstpart = name.substr_bytes(0,pos);
		asAtom obj = AVM1GetVariable(firstpart);
		if (asAtomHandler::isValid(obj))
		{
			tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
			ret = asAtomHandler::toObject(obj,getSystemState())->getVariableBindingValue(localname);
		}
	}
	return ret;
}
void DisplayObject::setVariableBinding(tiny_string &name, _NR<DisplayObject> obj)
{
	uint32_t key = getSystemState()->getUniqueStringId(name);
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
		while (it != variablebindings.end() && it->first == key)
		{
			if (it->second == obj)
			{
				variablebindings.erase(it);
				break;
			}
			it++;
		}
	}
}
void DisplayObject::AVM1SetFunction(uint32_t nameID, _NR<AVM1Function> obj)
{
	auto it = avm1variables.find(nameID);
	if (it != avm1variables.end())
	{
		if (obj && asAtomHandler::isObject(it->second) && asAtomHandler::getObjectNoCheck(it->second) == obj.getPtr())
			return; // function is already set
		ASATOM_DECREF(it->second);
	}
	if (obj)
	{
		obj->incRef();
		avm1functions[nameID] = obj;
		avm1variables[nameID] = asAtomHandler::fromObject(obj.getPtr());
	}
	else
	{
		avm1functions.erase(nameID);
		avm1variables.erase(nameID);
	}
}
AVM1Function* DisplayObject::AVM1GetFunction(uint32_t nameID)
{
	auto it = avm1functions.find(nameID);
	if (it != avm1functions.end())
		return it->second.getPtr();
	return nullptr;
}

