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

#ifndef SCRIPTING_FLASH_DISPLAY_DISPLAYOBJECT_H
#define SCRIPTING_FLASH_DISPLAY_DISPLAYOBJECT_H 1

#include "smartrefs.h"
#include "scripting/flash/display/IBitmapDrawable.h"
#include "asobject.h"
#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class AccessibilityProperties;
class DisplayObjectContainer;
class LoaderInfo;
class RenderContext;
class Stage;
class Transform;
class Rectangle;

class DisplayObject: public EventDispatcher, public IBitmapDrawable
{
friend class TokenContainer;
friend class GLRenderContext;
friend class AsyncDrawJob;
friend class Transform;
friend class ParseThread;
friend class Loader;
friend std::ostream& operator<<(std::ostream& s, const DisplayObject& r);
public:
	enum HIT_TYPE { GENERIC_HIT, // point is over the object
                        GENERIC_HIT_INVISIBLE, // ...even if the object is invisible
			MOUSE_CLICK, // point over the object and mouseEnabled
			DOUBLE_CLICK // point over the object and doubleClickEnabled
		      };
private:
	ASPROPERTY_GETTER_SETTER(_NR<AccessibilityProperties>,accessibilityProperties);
	static ATOMIC_INT32(instanceCount);
	_NR<Matrix> matrix;
	number_t tx,ty,tz;
	number_t rotation;
	number_t sx,sy,sz;
	float alpha;
	tiny_string blendMode;
	// if true, this displayobject is the root object of a loaded file (swf or image)
	bool isLoadedRoot;
public:
	UI16_SWF Ratio;
	UI16_SWF ClipDepth;
private:
	/**
	  	The object we are masking, if any
	*/
	_NR<DisplayObject> maskOf;
	void becomeMaskOf(_NR<DisplayObject> m);
	_NR<DisplayObjectContainer> parent;
	/* cachedSurface may only be read/written from within the render thread
	 * It is the cached version of the object for fast draw on the Stage
	 */
	CachedSurface cachedSurface;
	/*
	 * Utility function to set internal MATRIX
	 * Also used by Transform
	 */
	void setMatrix(_NR<Matrix> m);
	ACQUIRE_RELEASE_FLAG(constructed);
	bool useLegacyMatrix;
	void gatherMaskIDrawables(std::vector<IDrawable::MaskData>& masks) const;
protected:
	bool onStage;
	bool visible;
	~DisplayObject();
	/**
	  	The object that masks us, if any
	*/
	_NR<DisplayObject> mask;
	mutable Spinlock spinlock;
	void computeBoundsForTransformedRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
			int32_t& outXMin, int32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight,
			const MATRIX& m) const;
	/*
	 * Assume the lock is held and the matrix will not change
	 */
	void extractValuesFromMatrix();
	number_t computeWidth();
	number_t computeHeight();
	bool skipRender() const;

	void defaultRender(RenderContext& ctxt) const;
	virtual bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		throw RunTimeException("DisplayObject::boundsRect: Derived class must implement this!");
	}
	bool boundsRectGlobal(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	virtual void renderImpl(RenderContext& ctxt) const
	{
		throw RunTimeException("DisplayObject::renderImpl: Derived class must implement this!");
	}
	virtual _NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, HIT_TYPE type)
	{
		throw RunTimeException("DisplayObject::hitTestImpl: Derived class must implement this!");
	}

public:
	void setMask(_NR<DisplayObject> m);
	void constructionComplete();
	tiny_string name;
	_NR<DisplayObject> invalidateQueueNext;
	_NR<LoaderInfo> loaderInfo;
	ASPROPERTY_GETTER_SETTER(_NR<Array>,filters);
	ASPROPERTY_GETTER_SETTER(_NR<Rectangle>,scrollRect);
	_NR<ColorTransform> colorTransform;
	// this is reset after the drawjob is done to ensure a changed DisplayObject is only rendered once
	bool hasChanged;
	/**
	 * cacheAsBitmap is true also if any filter is used
	 */
	bool computeCacheAsBitmap() const;
	void computeMasksAndMatrix(DisplayObject* target, std::vector<IDrawable::MaskData>& masks,MATRIX& totalMatrix) const;
	ASPROPERTY_GETTER_SETTER(bool,cacheAsBitmap);
	_NR<DisplayObjectContainer> getParent() const { return parent; }
	void setParent(_NR<DisplayObjectContainer> p);
	/*
	   Used to link DisplayObjects the invalidation queue
	*/
	DisplayObject(Class_base* c);
	void finalize();
	MATRIX getMatrix() const;
	bool isConstructed() const { return ACQUIRE_READ(constructed); }
	/**
	 * Generate a new IDrawable instance for this object
	 * @param target The topmost object in the hierarchy that is being drawn. Such object
	 * _must_ be on the parent chain of this
	 * @param initialMatrix A matrix that will be prepended to all transformations
	 */
	virtual IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix);
	virtual void requestInvalidation(InvalidateQueue* q);
	MATRIX getConcatenatedMatrix() const;
	void localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	void globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	float getConcatenatedAlpha() const;
	virtual float getScaleFactor() const
	{
		throw RunTimeException("DisplayObject::getScaleFactor");
	}
	// used by MorphShapes
	virtual void checkRatio(uint32_t ratio) {}
	void Render(RenderContext& ctxt);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, const MATRIX& m) const;
	_NR<DisplayObject> hitTest(_NR<DisplayObject> last, number_t x, number_t y, HIT_TYPE type);
	virtual void setOnStage(bool staged);
	bool isOnStage() const { return onStage; }
	bool isMask() const { return !maskOf.isNull(); }
	bool isVisible() const { return visible; }
	bool isLoadedRootObject() const { return isLoadedRoot; }
	float clippedAlpha() const;
	virtual _NR<RootMovieClip> getRoot();
	virtual _NR<Stage> getStage();
	void setLegacyMatrix(const MATRIX& m);
	virtual void advanceFrame() {}
	virtual void initFrame();
	Vector2f getLocalMousePos();
	Vector2f getXY();
	void setX(number_t x);
	void setY(number_t y);
	void setZ(number_t z);
	void setScaleX(number_t val);
	void setScaleY(number_t val);
	void setScaleZ(number_t val);
	// Nominal width and heigt are the size before scaling and rotation
	number_t getNominalWidth();
	number_t getNominalHeight();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_getVisible);
	ASFUNCTION_ATOM(_setVisible);
	ASFUNCTION_ATOM(_getStage);
	ASFUNCTION_ATOM(_getX);
	ASFUNCTION_ATOM(_setX);
	ASFUNCTION_ATOM(_getY);
	ASFUNCTION_ATOM(_setY);
	ASFUNCTION_ATOM(_getZ);
	ASFUNCTION_ATOM(_setZ);
	ASFUNCTION_ATOM(_getMask);
	ASFUNCTION_ATOM(_setMask);
	ASFUNCTION_ATOM(_setAlpha);
	ASFUNCTION_ATOM(_getAlpha);
	ASFUNCTION_ATOM(_getScaleX);
	ASFUNCTION_ATOM(_setScaleX);
	ASFUNCTION_ATOM(_getScaleY);
	ASFUNCTION_ATOM(_setScaleY);
	ASFUNCTION_ATOM(_getScaleZ);
	ASFUNCTION_ATOM(_setScaleZ);
	ASFUNCTION_ATOM(_getLoaderInfo);
	ASFUNCTION_ATOM(_getBounds);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_setWidth);
	ASFUNCTION_ATOM(_getHeight);
	ASFUNCTION_ATOM(_setHeight);
	ASFUNCTION_ATOM(_getRotation);
	ASFUNCTION_ATOM(_getName);
	ASFUNCTION_ATOM(_setName);
	ASFUNCTION_ATOM(_getParent);
	ASFUNCTION_ATOM(_getRoot);
	ASFUNCTION_ATOM(_getBlendMode);
	ASFUNCTION_ATOM(_setBlendMode);
	ASFUNCTION_ATOM(_getScale9Grid);
	ASFUNCTION_ATOM(_setRotation);
	ASFUNCTION_ATOM(_getMouseX);
	ASFUNCTION_ATOM(_getMouseY);
	ASFUNCTION_ATOM(_getTransform);
	ASFUNCTION_ATOM(_setTransform);
	ASFUNCTION_ATOM(localToGlobal);
	ASFUNCTION_ATOM(globalToLocal);
	ASFUNCTION_ATOM(hitTestObject);
	ASFUNCTION_ATOM(hitTestPoint);
	ASPROPERTY_GETTER_SETTER(number_t, rotationX);
	ASPROPERTY_GETTER_SETTER(number_t, rotationY);
};
}
#endif /* SCRIPTING_FLASH_DISPLAY_DISPLAYOBJECT_H */
