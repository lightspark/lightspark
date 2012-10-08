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

class DisplayObject: public EventDispatcher, public IBitmapDrawable
{
friend class TokenContainer;
friend class GLRenderContext;
friend class AsyncDrawJob;
friend class Transform;
friend class ParseThread;
friend std::ostream& operator<<(std::ostream& s, const DisplayObject& r);
public:
	enum HIT_TYPE { GENERIC_HIT, DOUBLE_CLICK };
private:
	ASPROPERTY_GETTER_SETTER(_NR<AccessibilityProperties>,accessibilityProperties);
	static ATOMIC_INT32(instanceCount);
	MATRIX Matrix;
	number_t tx,ty;
	number_t rotation;
	number_t sx,sy;
	float alpha;
public:
	UI16_SWF Ratio;
	UI16_SWF ClipDepth;
private:
	/**
	  	The object we are masking, if any
	*/
	_NR<DisplayObject> maskOf;
	void becomeMaskOf(_NR<DisplayObject> m);
	void setMask(_NR<DisplayObject> m);
	_NR<DisplayObjectContainer> parent;
	/* cachedSurface may only be read/written from within the render thread
	 * It is the cached version of the object for fast draw on the Stage
	 */
	CachedSurface cachedSurface;
	/*
	 * Utility function to set internal MATRIX
	 * Also used by Transform
	 */
	void setMatrix(const MATRIX& m);
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
	float clippedAlpha() const;

	void defaultRender(RenderContext& ctxt) const;
	virtual bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		throw RunTimeException("DisplayObject::boundsRect: Derived class must implement this!");
	}
	virtual void renderImpl(RenderContext& ctxt) const
	{
		throw RunTimeException("DisplayObject::renderImpl: Derived class must implement this!");
	}
	virtual _NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type)
	{
		throw RunTimeException("DisplayObject::hitTestImpl: Derived class must implement this!");
	}

	void constructionComplete();
public:
	tiny_string name;
	_NR<DisplayObject> invalidateQueueNext;
	_NR<LoaderInfo> loaderInfo;
	ASPROPERTY_GETTER_SETTER(_NR<Array>,filters);
	CXFORMWITHALPHA ColorTransform;
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
	void Render(RenderContext& ctxt);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, const MATRIX& m) const;
	_NR<InteractiveObject> hitTest(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type);
	virtual void setOnStage(bool staged);
	bool isOnStage() const { return onStage; }
	bool isMask() const { return !maskOf.isNull(); }
	virtual _NR<RootMovieClip> getRoot();
	virtual _NR<Stage> getStage();
	void setLegacyMatrix(const MATRIX& m);
	virtual void advanceFrame() {}
	virtual void initFrame();
	Vector2f getLocalMousePos();
	Vector2f getXY();
	void setX(number_t x);
	void setY(number_t y);
	void setScaleX(number_t val);
	void setScaleY(number_t val);
	// Nominal width and heigt are the size before scaling and rotation
	number_t getNominalWidth();
	number_t getNominalHeight();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getVisible);
	ASFUNCTION(_setVisible);
	ASFUNCTION(_getStage);
	ASFUNCTION(_getX);
	ASFUNCTION(_setX);
	ASFUNCTION(_getY);
	ASFUNCTION(_setY);
	ASFUNCTION(_getMask);
	ASFUNCTION(_setMask);
	ASFUNCTION(_setAlpha);
	ASFUNCTION(_getAlpha);
	ASFUNCTION(_getScaleX);
	ASFUNCTION(_setScaleX);
	ASFUNCTION(_getScaleY);
	ASFUNCTION(_setScaleY);
	ASFUNCTION(_getLoaderInfo);
	ASFUNCTION(_getBounds);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
	ASFUNCTION(_getRotation);
	ASFUNCTION(_getName);
	ASFUNCTION(_setName);
	ASFUNCTION(_getParent);
	ASFUNCTION(_getRoot);
	ASFUNCTION(_getBlendMode);
	ASFUNCTION(_getScale9Grid);
	ASFUNCTION(_setRotation);
	ASFUNCTION(_getMouseX);
	ASFUNCTION(_getMouseY);
	ASFUNCTION(_getTransform);
	ASFUNCTION(localToGlobal);
	ASFUNCTION(globalToLocal);
};
};
#endif /* SCRIPTING_FLASH_DISPLAY_DISPLAYOBJECT_H */
