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

#include "forwards/backends/graphics.h"
#include "forwards/swftypes.h"
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
class KeyboardEvent;
class InvalidateQueue;
class CachedSurface;
struct RenderDisplayObjectToBitmapContainer;

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
	bool filterlistHasChanged;
	uint32_t ismaskCount; // number of DisplayObjects this is mask of
	
	number_t maxfilterborder;
public:
	UI16_SWF Ratio;
	int ClipDepth;
	// list of objects that are currently not on stage, but need to be handled in frame events
	DisplayObject* hiddenPrevDisplayObject;
	DisplayObject* hiddenNextDisplayObject;
	DisplayObject* avm1PrevDisplayObject;
	DisplayObject* avm1NextDisplayObject;
	std::map<uint32_t,asAtom > avm1locals;
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
	_R<CachedSurface> cachedSurface;
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
	std::map<uint32_t,asAtom> avm1variables;
	uint32_t avm1mouselistenercount;
	uint32_t avm1framelistenercount;
	void onSetScrollRect(_NR<Rectangle> oldValue);
protected:
	_NR<Rectangle> scalingGrid;
	MATRIX currentrendermatrix;
	std::multimap<uint32_t,_NR<DisplayObject>> variablebindings;
	bool onStage;
	bool visible;
	~DisplayObject();
	/**
	  	The object that masks us, if any
	*/
	DisplayObject* mask;
	DisplayObject* clipMask;
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
	virtual bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
	{
		throw RunTimeException("DisplayObject::boundsRect: Derived class must implement this!");
	}
	bool boundsRectGlobal(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool fromcurrentrendering=true);
	virtual _NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
	{
		throw RunTimeException("DisplayObject::hitTestImpl: Derived class must implement this!");
	}
	virtual void afterSetLegacyMatrix() {}
	bool skipCountCylicMemberReferences(garbagecollectorstate& gcstate);
public:
	bool boundsRectGlobal(RectF& rect, bool fromcurrentrendering=true);
	virtual bool boundsRectWithoutChildren(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
	{
		return boundsRect(xmin, xmax, ymin, ymax, visibleOnly);
	}
	virtual void fillGraphicsData(Vector* v, bool recursive) {}
	void updatedRect(); // scrollrect was changed
	void setMask(_NR<DisplayObject> m);
	void setClipMask(_NR<DisplayObject> m);
	void setBlendMode(UI8 blendmode);
	AS_BLENDMODE getBlendMode() const { return blendMode; }
	static bool isShaderBlendMode(AS_BLENDMODE bl);
	void constructionComplete(bool _explicit = false) override;
	void beforeConstruction(bool _explicit = false) override;
	void afterConstruction(bool _explicit = false) override;
	void prepareDestruction()
	{
		destroyContents();
		setParent(nullptr);
		removeAVM1Listeners();
	}
	void applyFilters(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley);
	_NR<DisplayObject> invalidateQueueNext;
	LoaderInfo* loaderInfo;
	ASPROPERTY_GETTER_SETTER(_NR<Array>,filters);
	ASPROPERTY_GETTER_SETTER(_NR<Rectangle>,scrollRect);
	_NR<ColorTransform> colorTransform;
	void setNeedsTextureRecalculation(bool skippable=false);
	void resetNeedsTextureRecalculation() { needsTextureRecalculation=false; }
	bool getNeedsTextureRecalculation() const { return needsTextureRecalculation; }
	bool getTextureRecalculationSkippable() const { return textureRecalculationSkippable; }
	// this may differ from the main clip if this instance was generated from a loaded swf, not from the main clip
	ApplicationDomain* loadedFrom;
	// this is reset after the drawjob is done to ensure a changed DisplayObject is only rendered once
	bool hasChanged;
	// this is set to true for DisplayObjects that are placed from a tag
	bool legacy;
	// The frame that this clip was placed on.
	unsigned int placeFrame;
	bool markedForLegacyDeletion;
	_R<CachedSurface>& getCachedSurface() { return cachedSurface; }
	bool needsCacheAsBitmap() const;
	bool hasFilters() const;
	void requestInvalidationFilterParent(InvalidateQueue* q=nullptr);
	virtual void requestInvalidationIncludingChildren(InvalidateQueue* q);
	ASPROPERTY_GETTER_SETTER(bool,cacheAsBitmap);
	IDrawable* getFilterDrawable(bool smoothing);
	DisplayObjectContainer* getParent() const { return parent; }
	int getParentDepth() const;
	int findParentDepth(DisplayObject* d) const;
	DisplayObjectContainer* getAncestor(int depth) const;
	DisplayObjectContainer* findCommonAncestor(DisplayObject* d, int& depth, bool init = true) const;
	DisplayObjectContainer* findCommonAncestor(DisplayObject* d) const { int dummy; return findCommonAncestor(d, dummy); }
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
	bool isConstructed() const override;
	/**
	 * Generate a new IDrawable instance for this object
	 * @param target The topmost object in the hierarchy that is being drawn. Such object
	 * _must_ be on the parent chain of this
	 * @param initialMatrix A matrix that will be prepended to all transformations
	 */
	virtual IDrawable* invalidate(bool smoothing);
	virtual void invalidateForRenderToBitmap(RenderDisplayObjectToBitmapContainer* container);
	virtual void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false);
	void updateCachedSurface(IDrawable* d);
	MATRIX getConcatenatedMatrix(bool includeRoot=false, bool fromcurrentrendering=true) const;
	Vector2f localToGlobal(const Vector2f& point, bool fromcurrentrendering=true) const;
	Vector2f globalToLocal(const Vector2f& point, bool fromcurrentrendering=true) const;
	void localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout, bool fromcurrentrendering=true) const;
	void globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout, bool fromcurrentrendering=true) const;
	float getConcatenatedAlpha() const;
	virtual float getScaleFactor() const;
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk) override;
	virtual void removeAVM1Listeners();
	void AVM1registerPrototypeListeners();

	// used by MorphShapes and embedded video
	virtual void checkRatio(uint32_t ratio, bool inskipping) {}
	void onNewEvent(Event *ev) override;
	void afterHandleEvent(Event* ev) override;
	
	void onSetName(uint32_t oldName);

	virtual void UpdateVariableBinding(asAtom v) {}
	
	tiny_string AVM1GetPath();
	virtual void afterLegacyInsert();
	virtual void afterLegacyDelete(bool inskipping) {}
	virtual uint32_t getTagID() const { return UINT32_MAX;}
	
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, const MATRIX& m, bool visibleOnly=false);
	_NR<DisplayObject> hitTest(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly);
	virtual void setOnStage(bool staged, bool force, bool inskipping=false);
	bool isOnStage() const { return onStage; }
	bool isMask() const { return ismaskCount; }
	// checks for visibility depending on parent visibility 
	bool isVisible() const;
	bool isLoadedRootObject() const { return isLoadedRoot; }
	float clippedAlpha() const;
	float getRotation() const { return rotation; }
	int getRawDepth();
	int getDepth();
	int getClipDepth() const;
	number_t getMaxFilterBorder() const { return maxfilterborder; }
	virtual _NR<RootMovieClip> getRoot();
	virtual _NR<Stage> getStage();
	void setLegacyMatrix(const MATRIX& m);
	void setFilters(const FILTERLIST& filterlist);
	virtual void refreshSurfaceState();
	void setupSurfaceState(IDrawable* d);

	bool placedByActionScript;
	// If set, skip the next call to enterFrame().
	bool skipFrame;
	virtual void enterFrame(bool implicit) {}
	virtual void advanceFrame(bool implicit) {}
	virtual void declareFrame(bool implicit) {}
	virtual void initFrame();
	virtual void executeFrameScript();
	virtual bool needsActionScript3() const;
	virtual void handleMouseCursor(bool rollover) {}
	virtual bool allowAsMask() const { return true; }
	Vector2f getLocalMousePos();
	Vector2f getXY();
	void setX(number_t x);
	void setY(number_t y);
	void setZ(number_t z);
	void setScaleX(number_t val);
	void setScaleY(number_t val);
	void setScaleZ(number_t val);
	void setVisible(bool v);
	void setLoaderInfo(LoaderInfo* li);
	// Nominal width and heigt are the size before scaling and rotation
	number_t getNominalWidth();
	number_t getNominalHeight();
	DisplayObject* getMask() const { return mask; }
	DisplayObject* getClipMask() const { return clipMask; }
	bool inMask() const;
	bool belongsToMask() const;
	
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
	ASPROPERTY_GETTER_SETTER(asAtom, opaqueBackground);
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
	ASFUNCTION_ATOM(AVM1_toString);
	static void AVM1SetupMethods(Class_base* c);
	DisplayObject* AVM1GetClipFromPath(tiny_string& path, lightspark::asAtom* member=nullptr);
	void AVM1SetVariable(tiny_string& name, asAtom v, bool setMember=true);
	void AVM1SetVariableDirect(uint32_t nameId, asAtom v);
	asAtom AVM1GetVariable(const tiny_string &name, bool checkrootvars=true);
	void AVM1UpdateVariableBindings(uint32_t nameID, asAtom &value);
	asAtom getVariableBindingValue(const tiny_string &name) override;
	void setVariableBinding(tiny_string& name, _NR<DisplayObject> obj);
	void AVM1SetFunction(const tiny_string& name, _NR<AVM1Function> obj);
	AVM1Function *AVM1GetFunction(uint32_t nameID);
	virtual void AVM1AfterAdvance() {}
	DisplayObject* AVM1getRoot();
	std::string toDebugString() const override;
};
}
#endif /* SCRIPTING_FLASH_DISPLAY_DISPLAYOBJECT_H */
