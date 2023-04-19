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
#include "backends/graphics.h"

namespace lightspark
{

class AccessibilityProperties;
class DisplayObjectContainer;
class LoaderInfo;
class RenderContext;
class Stage;
class Transform;
class Rectangle;
class KeyboardEvent;

class DisplayObject: public EventDispatcher, public IBitmapDrawable
{
friend class TokenContainer;
friend class GLRenderContext;
friend class AsyncDrawJob;
friend class Transform;
friend class ParseThread;
friend class Loader;
friend class TextField;
friend class Shape;
friend class Bitmap;
friend class CairoRenderer;
friend class Graphics;
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
	AS_BLENDMODE blendMode;
	// if true, this displayobject is the root object of a loaded file (swf or image)
	bool isLoadedRoot;
	bool ismask;
public:
	UI16_SWF Ratio;
	int ClipDepth;
	DisplayObject* avm1PrevDisplayObject;
	DisplayObject* avm1NextDisplayObject;
private:
	// the parent is not handled as a _NR<DisplayObjectContainer> because that will lead to circular dependencies in refcounting
	// and the parent can never be destructed
	DisplayObjectContainer* parent;
	// pointer to the parent this object was pointing to when an event is handled with this object as the dispatcher
	// this is used to keep track of refcounting, as the parent may change during handling the event
	std::map<Event*,DisplayObjectContainer*> eventparentmap;
	/* cachedSurface may only be read/written from within the render thread
	 * It is the cached version of the object for fast draw on the Stage
	 */
	CachedSurface cachedSurface;
	/*
	 * Utility function to set internal MATRIX
	 * Also used by Transform
	 */
	void setMatrix(_NR<Matrix> m);
	void setMatrix3D(_NR<Matrix3D> m);
	ACQUIRE_RELEASE_FLAG(constructed);
	bool useLegacyMatrix;
	bool needsTextureRecalculation;
	bool textureRecalculationSkippable;
	void gatherMaskIDrawables(std::vector<IDrawable::MaskData>& masks);
	std::map<uint32_t,asAtom> avm1variables;
	uint32_t avm1mouselistenercount;
	uint32_t avm1framelistenercount;
protected:
	_NR<Bitmap> cachedBitmap;
	_NR<Rectangle> scalingGrid;
	std::multimap<uint32_t,_NR<DisplayObject>> variablebindings;
	bool onStage;
	bool visible;
	~DisplayObject();
	/**
	  	The object that masks us, if any
	*/
	_NR<DisplayObject> mask;
	mutable Mutex spinlock;
	void computeBoundsForTransformedRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
			number_t& outXMin, number_t& outYMin, number_t& outWidth, number_t& outHeight,
			const MATRIX& m) const;
	/*
	 * Assume the lock is held and the matrix will not change
	 */
	void extractValuesFromMatrix();
	number_t computeWidth();
	number_t computeHeight();
	void geometryChanged();
	bool skipRender() const;

	bool defaultRender(RenderContext& ctxt);
	virtual bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
	{
		throw RunTimeException("DisplayObject::boundsRect: Derived class must implement this!");
	}
	virtual bool boundsRectWithoutChildren(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
	{
		return boundsRect(xmin, xmax, ymin, ymax);
	}
	bool boundsRectGlobal(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax);
	virtual bool renderImpl(RenderContext& ctxt)
	{
		throw RunTimeException("DisplayObject::renderImpl: Derived class must implement this!");
	}
	virtual _NR<DisplayObject> hitTestImpl(number_t x, number_t y, HIT_TYPE type,bool interactiveObjectsOnly)
	{
		throw RunTimeException("DisplayObject::hitTestImpl: Derived class must implement this!");
	}
	virtual void afterSetLegacyMatrix() {}
public:
	void setMask(_NR<DisplayObject> m);
	void setBlendMode(UI8 blendmode);
	AS_BLENDMODE getBlendMode() const { return blendMode; }
	void constructionComplete() override;
	void afterConstruction() override;
	void prepareDestruction()
	{
		destroyContents();
		setParent(nullptr);
		removeAVM1Listeners();
	}
	void applyFilters(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos,number_t scalex,number_t scaley);
	_NR<DisplayObject> invalidateQueueNext;
	_NR<LoaderInfo> loaderInfo;
	ASPROPERTY_GETTER_SETTER(_NR<Array>,filters);
	ASPROPERTY_GETTER_SETTER(_NR<Rectangle>,scrollRect);
	_NR<ColorTransform> colorTransform;
	// pointer to the ancestor of this DisplayObject that is cached as Bitmap
	DisplayObject* cachedAsBitmapOf;
	void invalidateCachedAsBitmapOf();
	void setNeedsTextureRecalculation(bool skippable=false);
	void resetNeedsTextureRecalculation() { needsTextureRecalculation=false; }
	bool getNeedsTextureRecalculation() const { return needsTextureRecalculation; }
	bool getTextureRecalculationSkippable() const { return textureRecalculationSkippable; }
	// this may differ from the main clip if this instance was generated from a loaded swf, not from the main clip
	RootMovieClip* loadedFrom;
	// this is reset after the drawjob is done to ensure a changed DisplayObject is only rendered once
	bool hasChanged;
	// this is set to true for DisplayObjects that are placed from a tag
	bool legacy;
	bool markedForLegacyDeletion;
	/**
	 * cacheAsBitmap is true also if any filter is used
	 */
	bool computeCacheAsBitmap(bool checksize=true);
	bool requestInvalidationForCacheAsBitmap(InvalidateQueue* q);
	void computeMasksAndMatrix(const DisplayObject *target, std::vector<IDrawable::MaskData>& masks, MATRIX& totalMatrix, bool includeRotation, bool &isMask, _NR<DisplayObject>& mask) const;
	ASPROPERTY_GETTER_SETTER(bool,cacheAsBitmap);
	IDrawable* getCachedBitmapDrawable(DisplayObject* target, const MATRIX& initialMatrix, _NR<DisplayObject>* pcachedBitmap);
	_NR<DisplayObject> getCachedBitmap() const { return cachedBitmap; }
	DisplayObjectContainer* getParent() const { return parent; }
	bool findParent(DisplayObject* d) const;
	void setParent(DisplayObjectContainer* p);
	void setScalingGrid();
	/*
	   Used to link DisplayObjects the invalidation queue
	*/
	DisplayObject(ASWorker* wrk, Class_base* c);
	virtual void markAsChanged();
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	MATRIX getMatrix(bool includeRotation = true) const;
	bool isConstructed() const override { return ACQUIRE_READ(constructed); }
	/**
	 * Generate a new IDrawable instance for this object
	 * @param target The topmost object in the hierarchy that is being drawn. Such object
	 * _must_ be on the parent chain of this
	 * @param initialMatrix A matrix that will be prepended to all transformations
	 */
	virtual IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap);
	virtual void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false);
	void updateCachedSurface(IDrawable* d);
	MATRIX getConcatenatedMatrix(bool includeRoot=false) const;
	void localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	void globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	float getConcatenatedAlpha() const;
	virtual float getScaleFactor() const
	{
		throw RunTimeException("DisplayObject::getScaleFactor");
	}
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk) override;
	virtual void removeAVM1Listeners();
	void AVM1registerPrototypeListeners();

	// used by MorphShapes and embedded video
	virtual void checkRatio(uint32_t ratio, bool inskipping) {}
	void onNewEvent(Event *ev) override;
	void afterHandleEvent(Event* ev) override;
	
	virtual void UpdateVariableBinding(asAtom v) {}
	
	tiny_string AVM1GetPath();
	virtual void afterLegacyInsert();
	virtual void afterLegacyDelete(DisplayObjectContainer* parent,bool inskipping) {}
	virtual uint32_t getTagID() const { return UINT32_MAX;}
	virtual void startDrawJob() {}
	virtual void endDrawJob() {}
	
	bool Render(RenderContext& ctxt,bool force=false);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, const MATRIX& m);
	_NR<DisplayObject> hitTest(number_t x, number_t y, HIT_TYPE type,bool interactiveObjectsOnly);
	virtual void setOnStage(bool staged, bool force, bool inskipping=false);
	bool isOnStage() const { return onStage; }
	bool isMask() const { return ismask; }
	// checks for visibility depending on parent visibility 
	bool isVisible() const;
	bool isLoadedRootObject() const { return isLoadedRoot; }
	float clippedAlpha() const;
	float getRotation() const { return rotation; }
	virtual _NR<RootMovieClip> getRoot();
	virtual _NR<Stage> getStage();
	void setLegacyMatrix(const MATRIX& m);
	void setFilters(const FILTERLIST& filterlist);
	virtual void advanceFrame(bool implicit) {}
	virtual void declareFrame(bool implicit) {}
	virtual void initFrame();
	virtual void executeFrameScript();
	virtual bool needsActionScript3() const;
	virtual void handleMouseCursor(bool rollover) {}
	virtual bool hasGraphics() const { return false; }
	Vector2f getLocalMousePos();
	Vector2f getXY();
	void setX(number_t x);
	void setY(number_t y);
	void setZ(number_t z);
	void setScaleX(number_t val);
	void setScaleY(number_t val);
	void setScaleZ(number_t val);
	inline void setVisible(bool v) { this->visible = v; }
	// Nominal width and heigt are the size before scaling and rotation
	number_t getNominalWidth();
	number_t getNominalHeight();
	DisplayObject* getMask() const { return mask.getPtr(); }
	static void sinit(Class_base* c);
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
	ASPROPERTY_GETTER_SETTER(uint32_t,name);
	ASFUNCTION_ATOM(_getParent);
	ASFUNCTION_ATOM(_getRoot);
	ASFUNCTION_ATOM(_getBlendMode);
	ASFUNCTION_ATOM(_setBlendMode);
	ASFUNCTION_ATOM(_getScale9Grid);
	ASFUNCTION_ATOM(_setScale9Grid);
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
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>, opaqueBackground);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>, metaData);
	
	ASFUNCTION_ATOM(AVM1_getScaleX);
	ASFUNCTION_ATOM(AVM1_setScaleX);
	ASFUNCTION_ATOM(AVM1_getScaleY);
	ASFUNCTION_ATOM(AVM1_setScaleY);
	ASFUNCTION_ATOM(AVM1_getParent);
	ASFUNCTION_ATOM(AVM1_getRoot);
	ASFUNCTION_ATOM(AVM1_getURL);
	ASFUNCTION_ATOM(AVM1_hitTest);
	ASFUNCTION_ATOM(AVM1_localToGlobal);
	ASFUNCTION_ATOM(AVM1_globalToLocal);
	ASFUNCTION_ATOM(AVM1_getBytesLoaded);
	ASFUNCTION_ATOM(AVM1_getBytesTotal);
	ASFUNCTION_ATOM(AVM1_getQuality);
	ASFUNCTION_ATOM(AVM1_setQuality);
	ASFUNCTION_ATOM(AVM1_getAlpha);
	ASFUNCTION_ATOM(AVM1_setAlpha);
	ASFUNCTION_ATOM(AVM1_getBounds);
	ASFUNCTION_ATOM(AVM1_swapDepths);
	ASFUNCTION_ATOM(AVM1_getDepth);
	static void AVM1SetupMethods(Class_base* c);
	DisplayObject* AVM1GetClipFromPath(tiny_string& path);
	void AVM1SetVariable(tiny_string& name, asAtom v, bool setMember=true);
	void AVM1SetVariableDirect(uint32_t nameId, asAtom v);
	asAtom AVM1GetVariable(const tiny_string &name, bool checkrootvars=true);
	void AVM1UpdateVariableBindings(uint32_t nameID, asAtom &value);
	asAtom getVariableBindingValue(const tiny_string &name) override;
	void setVariableBinding(tiny_string& name, _NR<DisplayObject> obj);
	void AVM1SetFunction(const tiny_string& name, _NR<AVM1Function> obj);
	AVM1Function *AVM1GetFunction(uint32_t nameID);
	virtual void AVM1AfterAdvance() {}
	void DrawToBitmap(BitmapData* bm, const MATRIX& initialMatrix, bool smoothing, bool forcachedbitmap, AS_BLENDMODE blendMode);
	std::string toDebugString() const override;
};
}
#endif /* SCRIPTING_FLASH_DISPLAY_DISPLAYOBJECT_H */
