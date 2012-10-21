/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
	if(!isConstructed())
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

void DisplayObject::Render(RenderContext& ctxt)
{
	if(!isConstructed() || skipRender())
		return;

	renderImpl(ctxt);
}

DisplayObject::DisplayObject(Class_base* c):EventDispatcher(c),tx(0),ty(0),rotation(0),
	sx(1),sy(1),alpha(1.0),maskOf(),parent(),constructed(false),useLegacyMatrix(true),onStage(false),
	visible(true),mask(),invalidateQueueNext(),loaderInfo(),filters(Class<Array>::getInstanceS()),cacheAsBitmap(false)
{
	name = tiny_string("instance") + Integer::toString(ATOMIC_INCREMENT(instanceCount));
}

DisplayObject::~DisplayObject() {}

void DisplayObject::finalize()
{
	EventDispatcher::finalize();
	maskOf.reset();
	parent.reset();
	mask.reset();
	loaderInfo.reset();
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
}

void DisplayObject::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(_getLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(_getScaleX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(_setScaleX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(_getScaleY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(_setScaleY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_getX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_getY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(_getVisible),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(_setVisible),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(_getRotation),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(_setRotation),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(_getName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(_setName),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("parent","",Class<IFunction>::getFunction(_getParent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("root","",Class<IFunction>::getFunction(_getRoot),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(_getBlendMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(_getScale9Grid),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stage","",Class<IFunction>::getFunction(_getStage),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(_getMask),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(_setMask),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(_getAlpha),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(_setAlpha),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",Class<IFunction>::getFunction(_getBounds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getRect","",Class<IFunction>::getFunction(_getBounds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseX","",Class<IFunction>::getFunction(_getMouseX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseY","",Class<IFunction>::getFunction(_getMouseY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",Class<IFunction>::getFunction(localToGlobal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("globalToLocal","",Class<IFunction>::getFunction(globalToLocal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transform","",Class<IFunction>::getFunction(_getTransform),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,accessibilityProperties);
	REGISTER_GETTER_SETTER(c,cacheAsBitmap);
	REGISTER_GETTER_SETTER(c,filters);

	c->addImplementedInterface(InterfaceClass<IBitmapDrawable>::getClass());
	IBitmapDrawable::linkTraits(c);
}

ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,accessibilityProperties);
//TODO: Use a callback for the cacheAsBitmap getter, since it should use computeCacheAsBitmap
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,cacheAsBitmap);
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,filters);

bool DisplayObject::computeCacheAsBitmap() const
{
	return cacheAsBitmap || (!filters.isNull() && filters->size()!=0);
}

ASFUNCTIONBODY(DisplayObject,_getTransform)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	//The tested behaviour is that every time ::transform is accessed
	//a new object is generated

	LOG(LOG_NOT_IMPLEMENTED, "DisplayObject::transform is a stub and does not reflect the real display state");
	th->incRef();
	return Class<Transform>::getInstanceS(_MR(th));
}

void DisplayObject::buildTraits(ASObject* o)
{
}

void DisplayObject::setMatrix(const lightspark::MATRIX& m)
{
	bool mustInvalidate=false;
	{
		SpinlockLocker locker(spinlock);
		if(Matrix!=m)
		{
			Matrix=m;
			extractValuesFromMatrix();
			mustInvalidate=true;
		}
	}
	if(mustInvalidate && onStage)
		requestInvalidation(getSys());
}

void DisplayObject::setLegacyMatrix(const lightspark::MATRIX& m)
{
	if(useLegacyMatrix)
		setMatrix(m);
}

void DisplayObject::becomeMaskOf(_NR<DisplayObject> m)
{
	maskOf=m;
}

void DisplayObject::setMask(_NR<DisplayObject> m)
{
	bool mustInvalidate=(mask!=m) && onStage;

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
		requestInvalidation(getSys());
}

MATRIX DisplayObject::getConcatenatedMatrix() const
{
	if(parent.isNull())
		return getMatrix();
	else
		return parent->getConcatenatedMatrix().multiplyMatrix(getMatrix());
}

/* Return alpha value between 0 and 1. (The stored alpha value is not
 * necessary bounded.) */
float DisplayObject::clippedAlpha() const
{
	return dmin(dmax(alpha, 0.), 1.);
}

float DisplayObject::getConcatenatedAlpha() const
{
	if(parent.isNull())
		return clippedAlpha();
	else
		return parent->getConcatenatedAlpha()*clippedAlpha();
}

MATRIX DisplayObject::getMatrix() const
{
	SpinlockLocker locker(spinlock);
	//Start from the residual matrix and construct the whole one
	MATRIX ret=Matrix;
	ret.scale(sx,sy);
	ret.rotate(rotation*M_PI/180);
	ret.translate(tx,ty);
	return ret;
}

void DisplayObject::extractValuesFromMatrix()
{
	//Extract the base components from the matrix and leave in
	//it only the residual components
	tx=Matrix.getTranslateX();
	ty=Matrix.getTranslateY();
	sx=Matrix.getScaleX();
	sy=Matrix.getScaleY();
	rotation=Matrix.getRotation();
	//Deapply translation
	Matrix.translate(-tx,-ty);
	//Deapply rotation
	Matrix.rotate(-rotation*M_PI/180);
	//Deapply scaling
	Matrix.scale(1.0/sx,1.0/sy);
}

bool DisplayObject::skipRender() const
{
	return visible==false || clippedAlpha()==0.0;
}

void DisplayObject::defaultRender(RenderContext& ctxt) const
{
	const CachedSurface& surface=ctxt.getCachedSurface(this);
	/* surface is only modified from within the render thread
	 * so we need no locking here */
	if(!surface.tex.isValid())
		return;

	ctxt.lsglLoadIdentity();
	ctxt.renderTextured(surface.tex, surface.xOffset, surface.yOffset,
			surface.tex.width, surface.tex.height,
			surface.alpha, RenderContext::RGB_MODE);
}

void DisplayObject::computeBoundsForTransformedRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
		int32_t& outXMin, int32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight,
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
	outWidth=ceil(maxx-minx);
	outHeight=ceil(maxy-miny);
}

IDrawable* DisplayObject::invalidate(DisplayObject* target, const MATRIX& initialMatrix)
{
	//Not supposed to be called
	throw RunTimeException("DisplayObject::invalidate");
}

void DisplayObject::requestInvalidation(InvalidateQueue* q)
{
	//Let's invalidate also the mask
	if(!mask.isNull())
		mask->requestInvalidation(q);
}
//TODO: Fix precision issues, Adobe seems to do the matrix mult with twips and rounds the results, 
//this way they have less pb with precision.
void DisplayObject::localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getMatrix().multiply2D(xin, yin, xout, yout);
	if(!parent.isNull())
		parent->localToGlobal(xout, yout, xout, yout);
}
//TODO: Fix precision issues
void DisplayObject::globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getConcatenatedMatrix().getInverted().multiply2D(xin, yin, xout, yout);
}

void DisplayObject::setOnStage(bool staged)
{
	//TODO: When removing from stage released the cachedTex
	if(staged!=onStage)
	{
		//Our stage condition changed, send event
		onStage=staged;
		if(staged==true)
			requestInvalidation(getSys());
		if(getVm()==NULL)
			return;
		/*NOTE: By tests we can assert that added/addedToStage is dispatched
		  immediately when addChild is called. On the other hand setOnStage may
		  be also called outside of the VM thread (for example by Loader::execute)
		  so we have to check isVmThread and act accordingly. If in the future
		  asynchronous uses of setOnStage are removed the code can be simplified
		  by removing the !isVmThread case.
		*/
		if(onStage==true)
		{
			this->incRef();
			_R<Event> e=_MR(Class<Event>::getInstanceS("addedToStage"));
			if(isVmThread())
				ABCVm::publicHandleEvent(_MR(this),e);
			else
				getVm()->addEvent(_MR(this),e);
		}
		else if(onStage==false)
		{
			this->incRef();
			_R<Event> e=_MR(Class<Event>::getInstanceS("removedFromStage"));
			if(isVmThread())
				ABCVm::publicHandleEvent(_MR(this),e);
			else
				getVm()->addEvent(_MR(this),e);
		}
	}
}

ASFUNCTIONBODY(DisplayObject,_setAlpha)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t val;
	ARG_UNPACK (val);
	/* The stored value is not clipped, _getAlpha will return the
	 * stored value even if it is outside the [0, 1] range. */
	th->alpha=val;
	if(th->onStage)
		th->requestInvalidation(getSys());
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getAlpha)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->alpha);
}

ASFUNCTIONBODY(DisplayObject,_getMask)
{
	DisplayObject* th=Class<DisplayObject>::cast(obj);
	if(th->mask.isNull())
		return getSys()->getNullRef();

	th->mask->incRef();
	return th->mask.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_setMask)
{
	DisplayObject* th=Class<DisplayObject>::cast(obj);
	assert_and_throw(argslen==1);
	if(args[0] && args[0]->getClass() && args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()))
	{
		//We received a valid mask object
		DisplayObject* newMask=Class<DisplayObject>::cast(args[0]);
		newMask->incRef();
		th->setMask(_MR(newMask));
	}
	else
		th->setMask(NullRef);

	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getScaleX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->sx);
}

void DisplayObject::setScaleX(number_t val)
{
	//Apply the difference
	if(sx!=val)
	{
		sx=val;
		if(onStage)
			requestInvalidation(getSys());
	}
}

ASFUNCTIONBODY(DisplayObject,_setScaleX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleX(val);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getScaleY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->sy);
}

void DisplayObject::setScaleY(number_t val)
{
	//Apply the difference
	if(sy!=val)
	{
		sy=val;
		if(onStage)
			requestInvalidation(getSys());
	}
}

ASFUNCTIONBODY(DisplayObject,_setScaleY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleY(val);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->tx);
}

void DisplayObject::setX(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	//Apply translation, it's trivial
	if(tx!=val)
	{
		tx=val;
		if(onStage)
			requestInvalidation(getSys());
	}
}

void DisplayObject::setY(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	//Apply translation, it's trivial
	if(ty!=val)
	{
		ty=val;
		if(onStage)
			requestInvalidation(getSys());
	}
}

ASFUNCTIONBODY(DisplayObject,_setX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	th->setX(val);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->ty);
}

ASFUNCTIONBODY(DisplayObject,_setY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	th->setY(val);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getBounds)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);

	if(args[0]->is<Undefined>())
		return Class<Rectangle>::getInstanceS();

	assert_and_throw(args[0]->is<DisplayObject>());
	DisplayObject* target=Class<DisplayObject>::cast(args[0]);
	//Compute the transformation matrix
	MATRIX m;
	DisplayObject* cur=th;
	while(cur!=NULL && cur!=target)
	{
		m = cur->getMatrix().multiplyMatrix(m);
		cur=cur->parent.getPtr();
	}
	if(cur==NULL)
	{
		//We crawled all the parent chain without finding the target
		//The target is unrelated, compute it's transformation matrix
		const MATRIX& targetMatrix=target->getConcatenatedMatrix();
		//If it's not invertible just use the previous computed one
		if(targetMatrix.isInvertible())
			m = targetMatrix.getInverted().multiplyMatrix(m);
	}

	Rectangle* ret=Class<Rectangle>::getInstanceS();
	number_t x1,x2,y1,y2;
	bool r=th->getBounds(x1,x2,y1,y2, m);
	if(r)
	{
		//Bounds are in the form [XY]{min,max}
		//convert it to rect (x,y,width,height) representation
		ret->x=x1;
		ret->width=x2-x1;
		ret->y=y1;
		ret->height=y2-y1;
	}
	else
	{
		ret->x=0;
		ret->width=0;
		ret->y=0;
		ret->height=0;
	}
	return ret;
}

ASFUNCTIONBODY(DisplayObject,_constructor)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	EventDispatcher::_constructor(obj,NULL,0);

	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getLoaderInfo)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);

	/* According to tests returning root.loaderInfo is the correct
	 * behaviour, even though the documentation states that only
	 * the main class should have non-null loaderInfo. */
	_NR<RootMovieClip> r=th->getRoot();
	if(r.isNull() || r->loaderInfo.isNull())
		return getSys()->getUndefinedRef();
	
	r->loaderInfo->incRef();
	return r->loaderInfo.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_getStage)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	_NR<Stage> ret=th->getStage();
	if(ret.isNull())
		return getSys()->getNullRef();

	ret->incRef();
	return ret.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_getScale9Grid)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj);
	return getSys()->getUndefinedRef();
}

ASFUNCTIONBODY(DisplayObject,_getBlendMode)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj);
	return getSys()->getUndefinedRef();
}

ASFUNCTIONBODY(DisplayObject,localToGlobal)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=static_cast<Point*>(args[0]);

	number_t tempx, tempy;

	th->localToGlobal(pt->getX(), pt->getY(), tempx, tempy);

	return Class<Point>::getInstanceS(tempx, tempy);
}

ASFUNCTIONBODY(DisplayObject,globalToLocal)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=static_cast<Point*>(args[0]);

	number_t tempx, tempy;

	th->globalToLocal(pt->getX(), pt->getY(), tempx, tempy);

	return Class<Point>::getInstanceS(tempx, tempy);
}

ASFUNCTIONBODY(DisplayObject,_setRotation)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	//Apply the difference
	if(th->rotation!=val)
	{
		th->rotation=val;
		if(th->onStage)
			th->requestInvalidation(getSys());
	}
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_setName)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	th->name=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getName)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return Class<ASString>::getInstanceS(th->name);
}

void DisplayObject::setParent(_NR<DisplayObjectContainer> p)
{
	if(parent!=p)
	{
		parent=p;
		if(onStage)
			requestInvalidation(getSys());
	}
}

ASFUNCTIONBODY(DisplayObject,_getParent)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->parent.isNull())
		return getSys()->getUndefinedRef();

	th->parent->incRef();
	return th->parent.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_getRoot)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	_NR<RootMovieClip> ret=th->getRoot();
	if(ret.isNull())
		return getSys()->getUndefinedRef();

	ret->incRef();
	return ret.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_getRotation)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->rotation);
}

ASFUNCTIONBODY(DisplayObject,_setVisible)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	th->visible=Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getVisible)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_b(th->visible);
}

number_t DisplayObject::computeHeight()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix());

	return (ret)?(y2-y1):0;
}

number_t DisplayObject::computeWidth()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix());

	return (ret)?(x2-x1):0;
}

_NR<RootMovieClip> DisplayObject::getRoot()
{
	if(parent.isNull())
		return NullRef;

	return parent->getRoot();
}

_NR<Stage> DisplayObject::getStage()
{
	if(parent.isNull())
		return NullRef;

	return parent->getStage();
}

ASFUNCTIONBODY(DisplayObject,_getWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->computeWidth());
}

ASFUNCTIONBODY(DisplayObject,_setWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t newwidth=args[0]->toNumber();

	number_t xmin,xmax,y1,y2;
	if(!th->boundsRect(xmin,xmax,y1,y2))
		return NULL;

	number_t width=xmax-xmin;
	if(width==0) //Cannot scale, nothing to do (See Reference)
		return NULL;
	
	if(width*th->sx!=newwidth) //If the width is changing, calculate new scale
	{
		if(th->useLegacyMatrix)
			th->useLegacyMatrix=false;
		th->setScaleX(newwidth/width);
	}
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->computeHeight());
}

ASFUNCTIONBODY(DisplayObject,_setHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t newheight=args[0]->toNumber();

	number_t x1,x2,ymin,ymax;
	if(!th->boundsRect(x1,x2,ymin,ymax))
		return NULL;

	number_t height=ymax-ymin;
	if(height==0) //Cannot scale, nothing to do (See Reference)
		return NULL;
	
	if(height*th->sy!=newheight) //If the height is changing, calculate new scale
	{
		if(th->useLegacyMatrix)
			th->useLegacyMatrix=false;
		th->setScaleY(newheight/height);
	}
	return NULL;
}

Vector2f DisplayObject::getLocalMousePos()
{
	return getConcatenatedMatrix().getInverted().multiply2D(getSys()->getInputThread()->getMousePos());
}

ASFUNCTIONBODY(DisplayObject,_getMouseX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->getLocalMousePos().x);
}

ASFUNCTIONBODY(DisplayObject,_getMouseY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->getLocalMousePos().y);
}

_NR<InteractiveObject> DisplayObject::hitTest(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type)
{
	if(!visible || !isConstructed())
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
		if(mask->hitTest(last, maskX, maskY, type)==false)
			return NullRef;
	}

	_NR<InteractiveObject> ret = hitTestImpl(last, x,y, type);
	return ret;
}

/* Display objects have no children in general,
 * so we skip to calling the constructor, if necessary.
 * This is called in vm's thread context */
void DisplayObject::initFrame()
{
	if(!isConstructed() && getClass())
	{
		getClass()->handleConstruction(this,NULL,0,true);

		/*
		 * Legacy objects have their display list properties set on creation, but
		 * the related events must only be sent after the constructor is sent.
		 * This is from "Order of Operations".
		 */
		if(!parent.isNull())
		{
			this->incRef();
			_R<Event> e=_MR(Class<Event>::getInstanceS("added"));
			ABCVm::publicHandleEvent(_MR(this),e);
		}
		if(onStage)
		{
			this->incRef();
			_R<Event> e=_MR(Class<Event>::getInstanceS("addedToStage"));
			ABCVm::publicHandleEvent(_MR(this),e);
		}
	}
}

void DisplayObject::constructionComplete()
{
	RELEASE_WRITE(constructed,true);
	if(!loaderInfo.isNull())
	{
		this->incRef();
		loaderInfo->objectHasLoaded(_MR(this));
	}
	if(onStage)
		requestInvalidation(getSys());
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
		SoftwareInvalidateQueue queue;
		mask->requestInvalidation(&queue);
		for(auto it=queue.queue.begin();it!=queue.queue.end();it++)
		{
			DisplayObject* target=(*it).getPtr();
			//Get the drawable from each of the added objects
			IDrawable* drawable=target->invalidate(NULL, MATRIX());
			if(drawable==NULL)
				continue;
			masks.emplace_back(drawable, maskMode);
		}
	}
	else
	{
		IDrawable* drawable=NULL;
		if(mask->is<DisplayObjectContainer>())
		{
			//HACK: use bitmap temporarily
			number_t xmin,xmax,ymin,ymax;
			bool ret=mask->getBounds(xmin,xmax,ymin,ymax,mask->getConcatenatedMatrix());
			if(ret==false)
				return;
			_R<BitmapData> data(Class<BitmapData>::getInstanceS(xmax-xmin,ymax-ymin));
			//Forge a matrix. It must contain the right rotation and scaling while translation
			//only compensate for the xmin/ymin offset
			MATRIX m=mask->getConcatenatedMatrix();
			m.x0 -= xmin;
			m.y0 -= ymin;
			data->drawDisplayObject(mask.getPtr(), m);
			_R<Bitmap> bmp(Class<Bitmap>::getInstanceS(data));

			//The created bitmap is already correctly scaled and rotated
			//Just apply the needed offset
			MATRIX m2(1,1,0,0,xmin,ymin);
			drawable=bmp->invalidate(NULL, m2);
		}
		else
			drawable=mask->invalidate(NULL, MATRIX());

		if(drawable==NULL)
			return;
		masks.emplace_back(drawable, maskMode);
	}
}

void DisplayObject::computeMasksAndMatrix(DisplayObject* target, std::vector<IDrawable::MaskData>& masks, MATRIX& totalMatrix) const
{
	const DisplayObject* cur=this;
	bool gatherMasks = true;
	while(cur!=target)
	{
		totalMatrix=cur->getMatrix().multiplyMatrix(totalMatrix);
		//Get an IDrawable for all the hierarchy of each mask.
		if(gatherMasks)
		{
			if(cur->maskOf.isNull())
				cur->gatherMaskIDrawables(masks);
			else
			{
				//Stop gathering masks if any level of the hierarchy it's a mask
				masks.clear();
				masks.shrink_to_fit();
				gatherMasks=false;
			}
		}
		cur=cur->getParent().getPtr();
	}
}
