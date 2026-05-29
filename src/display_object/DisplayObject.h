/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_DISPLAYOBJECT_H
#define DISPLAY_OBJECT_DISPLAYOBJECT_H 1

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "backends/geometry.h"
#include "exceptions.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "tiny_string.h"

namespace lightspark
{

class BitmapContainer;
struct BitmapContainerRenderData;
class CachedSurface;
class DisplayObjectContainer;
class Filter;
class InvalidateQueue;
class LoaderInfo;
class Matrix3D;
template<typename T>
class Optional;
class RenderContext;
class RootMovieClip;
class SWFMovie;
class Stage;

class DisplayObject
{
private:
	SystemState* sys;
	DisplayObject* parent;

	// The frame that this clip was placed on.
	uint16_t placeFrame;
	uint16_t depth;
	uint16_t ratio;

	tiny_string name;

	MATRIX matrix;
	number_t tz;
	number_t rotation;
	Vector2f scale;
	number_t scaleZ;
	number_t skew;
	AS_BLENDMODE blendMode;

	// If `true`, this `DisplayObject` is the root of a loaded file
	// (either an SWF, or an image).
	bool isLoadedRoot;
	bool inInitFrame;
	bool filterListHasChanged;
	bool hasMatrix3D;
	// The number of `DisplayObject`s that use us as a mask.
	size_t maskCount;

	std::vector<FILTER> filters;
	number_t maxFilterBorder;

	/* cachedSurface may only be read/written from within the render thread
	 * It is the cached version of the object for fast draw on the Stage
	 */
	_R<CachedSurface> cachedSurface;
	bool useLegacyMatrix;
	bool needsTextureRecalculation;
	bool textureRecalculationSkippable;

	size_t broadcastEventListenerCount;
	RectF scalingGrid;
	RectF scrollRect;
	MATRIX currentRenderMatrix;

	// Whether this `DisplayObject` has been removed from the display
	// list.
	// This is needed in AVM1 to remove any queued actions from removed
	// `MovieClip`s.
	bool removedByAVM1;
	bool visible;
	bool transformedByActionScript;
	bool placedByActionScript;
	// If set, skip the next call to `enterFrame()`.
	bool skipFrame;
	// this is reset after the drawjob is done to ensure a changed DisplayObject is only rendered once
	bool hasChanged;
	// this is set to true for DisplayObjects that are placed from a tag
	bool legacy;
	bool markedForLegacyDeletion;
	bool cacheAsBitmap;
	bool lockRoot;

	/**
		The object that masks us, if any
	*/
	DisplayObject* mask;
	DisplayObject* clipMask;
	DisplayObject* invalidateQueueNext;
protected:
	mutable Mutex spinlock;

	virtual ~DisplayObject() {}
	RectF computeBoundsForTransformedRect(const RectF& rect, const MATRIX& m) const;

	/*
	 * Assume the lock is held and the matrix will not change
	 */
	void extractValuesFromMatrix();
	Vector2f computeSize();
	number_t computeWidth() { return computeSize().x; }
	number_t computeHeight() { return computeSize().y; }
	bool skipRender() const;

	bool defaultRender(RenderContext& ctxt);

	virtual DisplayObject* hitTestImpl
	(
		const Vector2f& globalPoint,
		const Vector2f& localPoint,
		HIT_TYPE type,
		bool interactiveObjectsOnly
	)
	{
		throw RunTimeException
		(
			"DisplayObject::hitTestImpl: "
			"Derived class must implement this!"
		);
	}

	virtual void afterSetLegacyMatrix() {}
public:
	DisplayObject();

	void geometryChanged();
	virtual Optional<RectF> boundsRect(bool visibleOnly)
	{
		throw RunTimeException
		(
			"DisplayObject::boundsRect: "
			"Derived class must implement this!"
		);
	}

	virtual Optional<RectF> boundsRectWithoutChildren(bool visibleOnly)
	{
		return boundsRect(visibleOnly);
	}

	Optional<RectF> boundsRectGlobal(bool fromCurrentRendering = true);
	void setMatrix(const MATRIX& m);
	void setMatrix3D(const Matrix3D* m);
	void updatedRect(); // scrollrect was changed
	void setMask(DisplayObject* m);
	void setClipMask(DisplayObject* m);
	void setBlendMode(const AS_BLENDMODE& _blendMode);
	AS_BLENDMODE getBlendMode() const { return blendMode; }
	static bool isShaderBlendMode(const AS_BLENDMODE& mode);
	void setNameOnParent();
	void applyFilters
	(
		BitmapContainer* target,
		BitmapContainer* source,
		const RECT& sourceRect,
		const Vector2f& pos,
		const Vector2f& scale
	);

	void setNeedsTextureRecalculation(bool skippable = false);
	void resetNeedsTextureRecalculation()
	{
		needsTextureRecalculation = false;
	}

	bool getNeedsTextureRecalculation() const
	{
		return needsTextureRecalculation;
	}

	bool getTextureRecalculationSkippable() const
	{
		return textureRecalculationSkippable;
	}

	_R<CachedSurface> getCachedSurface() const { return cachedSurface; }
	bool needsCacheAsBitmap() const;
	bool hasFilters() const;
	void requestInvalidationFilterParent(InvalidateQueue* q = nullptr);
	virtual void requestInvalidationIncludingChildren(InvalidateQueue* q);
	SystemState* getSys() const { return sys; }
	DisplayObject* getParent() const { return parent; }
	uint16_t getParentDepth() const;
	uint16_t findParentDepth(DisplayObject* d) const;
	DisplayObject* getAncestor(uint16_t depth) const;
	std::pair<DisplayObject*, uint16_t> findCommonAncestor
	(
		DisplayObject* obj,
		bool init = true
	) const;

	bool findParent(DisplayObject* d) const;
	void setParent(DisplayObject* p, bool forDestruction);
	void setScalingGrid();
	virtual void markAsChanged();
	MATRIX getMatrix(bool includeRotation = true) const;
	bool isInInitFrame() const { return inInitFrame; }
	/**
	 * Generate a new IDrawable instance for this object
	 * @param target The topmost object in the hierarchy that is being drawn. Such object
	 * _must_ be on the parent chain of this
	 * @param initialMatrix A matrix that will be prepended to all transformations
	 */
	virtual IDrawable* invalidate(bool smoothing);
	virtual void invalidateForRenderToBitmap
	(
		BitmapContainerRenderData* container,
		bool smoothing
	);

	virtual void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	);

	void updateCachedSurface(IDrawable* d);
	MATRIX getConcatenatedMatrix
	(
		bool includeRoot = false,
		bool fromCurrentRendering = true
	) const;

	Vector2f localToGlobal(const Vector2f& point, bool fromCurrentRendering = true) const;
	Vector2f globalToLocal(const Vector2f& point, bool fromCurrentRendering = true) const;
	number_t getConcatenatedAlpha() const;
	virtual number_t getScaleFactor() const;

	// used by MorphShapes and embedded video
	virtual void checkRatio(uint16_t ratio, bool inskipping) {}

	tiny_string getPath(bool dotNotation = true);
	virtual void afterLegacyInsert();
	virtual void afterLegacyDelete(bool inskipping) {}
	virtual uint32_t getTagID() const { return UINT32_MAX; }
	virtual LoaderInfo* getLoaderInfo() const { return nullptr; }
	virtual SWFMovie* getMovie() const { return nullptr; }

	Optional<RectF> getBounds(const MATRIX& m, bool visibleOnly = false);
	bool hitTestMask(const Vector2f& globalPoint, HIT_TYPE type);
	DisplayObject* hitTest
	(
		const Vector2f& globalPoint,
		const Vector2f& localPoint,
		HIT_TYPE type,
		bool interactiveObjectsOnly
	);

	void setAVM1Removed(bool removed) { removedByAVM1 = removed; }
	bool isAVM1Removed() const { return removedByAVM1; }
	bool isOnStage() const;
	bool isMask() const { return isMaskCount; }
	// checks for visibility depending on parent visibility
	bool isVisible() const;
	bool isLoadedRootObject() const { return isLoadedRoot; }
	bool isTransformedByActionScript() const
	{
		return transformedByActionScript;
	}

	bool isPlacedByActionScript() const
	{
		return placedByActionScript;
	}

	bool getSkipFrame() const { return skipFrame; }
	bool getHasChanged() const { return hasChanged; }
	bool isLegacy() const { return legacy; }
	bool isMarkedForLegacyDeletion() const
	{
		return markedForLegacyDeletion;
	}

	void getLockRoot() const { return lockRoot; }

	void setTransformedByActionScript(bool)
	{
		transformedByActionScript = flag;
	}

	void setPlacedByActionScript(bool flag)
	{
		placedByActionScript = flag;
	}

	void setSkipFrame(bool _skipFrame) { skipFrame = _skipFrame; }
	void setLegacy(bool _legacy) { legacy = _legacy; }
	void setMarkedForLegacyDeletion(bool flag)
	{
		markedForLegacyDeletion = flag;
	}


	void setCacheAsBitmap(bool _cacheAsBitmap);
	void setLockRoot(bool _lockRoot) { lockRoot = _lockRoot; }
	number_t clippedAlpha() const;
	number_t getRotation() const { return rotation; }
	uint16_t getDepth() { return depth; }
	uint16_t getClipDepth() const { return clipDepth; }
	number_t getMaxFilterBorder() const { return maxFilterBorder; }
	RootMovieClip* getAVM2Root() const;
	Stage* getAVM2Stage() const;
	DisplayObject& getAVM1Root(bool noLock = false) const;
	DisplayObject& getAVM1Stage() const;
	void setLegacyMatrix(const MATRIX& m);
	void setFilters(const FILTERLIST& filterList);
	void setFilter(const FILTER& filter);
	virtual void refreshSurfaceState();
	void setupSurfaceState(IDrawable* d);

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
	// Nominal width and heigt are the size before scaling and rotation
	number_t getNominalWidth();
	number_t getNominalHeight();
	DisplayObject* getMask() const { return mask; }
	DisplayObject* getClipMask() const { return clipMask; }
	bool inMask() const;
	bool belongsToMask() const;
	void addBroadcastEventListener();
	void removeBroadcastEventListener();
	bool hasBroadcastListeners() const { return broadcastEventListenerCount; }
};

}
#endif /* DISPLAY_OBJECT_DISPLAYOBJECT_H */
