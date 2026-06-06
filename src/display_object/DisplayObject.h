/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include "backends/colortransformbase.h"
#include "backends/geometry.h"
#include "exceptions.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::display_object::DisplayObject`.

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
public:
	// Bit flags used by `DisplayObject`.
	enum class Flags : uint16_t
	{
		// Whether the `DisplayObject` has been removed from the display
		// list.
		//
		// This is needed in AVM1 to remove any queued actions from
		// removed `MovieClip`s.
		RemovedByAVM1 = 1 << 0,

		// Whether the `DisplayObject` is visible (i.e. the `_visible`
		// property).
		Visible = 1 << 1,

		// Whether the `DisplayObject` has cached `_[xy]scale`, and
		// `_rotation`.
		ScaleAndRotationCached = 1 << 2,

		// Whether the `DisplayObject` has been transformed by
		// ActionScript.
		//
		// When this flag is set, any changes from `RemoveObject` tags
		// are ignored.
		TransformedByActionScript = 1 << 3,
		
		// Whether the `DisplayObject` was placed into a container by
		// ActionScript.
		//
		// When this flag is set, any changes from `RemoveObject` tags
		// are ignored.
		PlacedByActionScript = 1 << 4,

		// Whether the `DisplayObject` was timeline created, i.e. created
		// by an SWF tag.
		//
		// When this flag is set, all attempts to change the
		// `DisplayObject`'s name from AVM2 throw an exception.
		CreatedByTimeline = 1 << 5,

		// Whether the `DisplayObject` is a root, i.e. the top most
		// `DisplayObject` of a loaded SWF, or `Bitmap`.
		//
		// This is used by `MovieClip.getBytesLoaded()` in AVM1, and
		// `DisplayObject.root` in AVM2.
		IsRoot = 1 << 6,

		// Whether the `DisplayObject` has `_lockroot` set, in which
		// case, it becomes the `_root` of itself, and any of it's
		// children.
		LockRoot = 1 << 7,

		// Whether the `DisplayObject` will be cached into a bitmap.
		CacheAsBitmap = 1 << 8,

		// Whether the `DisplayObject` has a scroll rect applied.
		HasScrollRect = 1 << 9,

		// Whether the `DisplayObject` has an explicit name.
		HasExplicitName = 1 << 10,

		// When this flag is set, the `DisplayObject`, and it's children
		// will skip the next call to `enterFrame()`.
		// This is true for `DisplayObject`s that are created from
		// ActionScript, which have been observed to lag behind ``
		// `DisplayObject`s that are timeline created (even if they're
		// both placed on the same frame).
		SkipFrame = 1 << 11,

		// Whether the `DisplayObject` has already called
		// `invalidateCachedBitmap()` for this frame.
		CacheInvalidated = 1 << 12,

		// Whether the AVM1 `DisplayObject` is pending removal (it'll be
		// removed on the next frame).
		AVM1PendingRemoval = 1 << 13,

		// Whether the `DisplayObject` has `matrix3D`.
		HasMatrix3D = 1 << 14,

		// Whether the `DisplayObject` was placed by the timeline on a
		// `MovieClip` before the clip had it's AVM2 object constructed
		// These kinds of `DisplayObject`s are only constructed by
		// `Sprite.constructChildren()`, which is usually when `super()`
		// is called in a `Sprite` subclass. However, if `super()` (and
		// therefore `Sprite.constructChildren()`) isn't called, the clip
		// won't be created/constructed. So we mark all clips that were
		// timeline placed on a load frame with this flag to ensure that
		// `MovieClip::initFrame()` doesn't construct them (they need to
		// be constructed manually by `Sprite.constructChildren()`).
		ManualFrameConstruction = 1 << 15,
	};
private:
	
	SystemState* sys;
	DisplayObject* parent;

	// The frame that this `DisplayObject` was placed on.
	uint16_t placeFrame;
	uint16_t depth;
	uint16_t clipDepth;
	uint16_t ratio;

	tiny_string name;

	// The transform of this `DisplayObject`.
	// (Split into separate members for easier access).
	MATRIX matrix;
	ColorTransform colorTransform;
	PerspectiveProjection perspectiveProjection;

	// The cached transform properties `_{[xy]scale,rotation}`.
	// These are very expensive to calculate, so they're calculated, and
	// cached when scripts request any of these properties.
	number_t rotation;
	Vector2f scale;
	number_t skew;

	// The sound transform of sounds playing from this `DisplayObject`.
	SoundTransform* soundTransform;

	// The `DisplayObject` that we're being masked by.
	DisplayObject* mask;

	// The `DisplayObject` that we're currently masking.
	DisplayObject* maskee;

	// The blend mode used when rendering this `DisplayObject`.
	//
	// NOTE: Values other than the default `BLENDMODE_NORMAL` will cause
	// the `DisplayObject` to act as if `cacheAsBitmap` was set.
	AS_BLENDMODE blendMode;

	// The opaque background colour of this `DisplayObject`.
	// The bounding box of the `DisplayObject` will be filled with the
	// supplied colour.
	//
	// NOTE: This also causes the `DisplayObject` to act as if
	// `cacheAsBitmap` was set.
	Optional<RGB> opaqueBackground;

	// Bit flags for various `DisplayObject` properites.
	Flags flags;

	// The internal scroll rect used for rendering, as well as methods
	// like `localToGlobal()`.
	// This is updated from `preRender()`.
	Optional<Rect<Twips>> scrollRect;

	// The next scroll rect, which'll be copied to `scrollRect` from
	// `preRender()`.
	// This is used by `DisplayObject.scrollRect`'s getter, which sees
	// any changes made to it immediately (without needing to wait for
	// the renderer).
	Rect<Twips> nextScrollRect;

	// The rectangle used for 9 slice scaling
	// (`DisplayObject.scale9grid`).
	Rect<Twips> scalingGrid;

	std::vector<FILTER> filters;

	/* cachedSurface may only be read/written from within the render thread
	 * It is the cached version of the object for fast draw on the Stage
	 */
	_R<CachedSurface> cachedSurface;
	bool needsTextureRecalculation;
	bool textureRecalculationSkippable;

	DisplayObject* invalidateQueueNext;
protected:
	mutable Mutex spinlock;

	virtual ~DisplayObject() {}
	RectF computeBoundsForTransformedRect(const RectF& rect, const MATRIX& m) const;

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
public:
	DisplayObject
	(
		SystemState* _sys,
		Optional<const tiny_string&> _name = {}
	);

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
	const RectF& getScalingGrid() const { return scalingGrid; }
	void setScalingGrid(const RectF& rect) { scalingGrid = rect; }
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
	const tiny_string& getName() const { return name; }
	void setName(const tiny_string& _name) { name = _name; }
	const ColorTransformBase& getColorTransform() const { return colorTransform; }
	void setColorTransform(const ColorTransformBase& ct) { colorTransform = ct; }
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
	Vector2f getNominalSize();
	number_t getNominalWidth() { return getNominalSize().x; }
	number_t getNominalHeight() { return getNominalSize().y; }
	DisplayObject* getMask() const { return mask; }
	DisplayObject* getClipMask() const { return clipMask; }
	bool inMask() const;
	bool belongsToMask() const;
	void addBroadcastEventListener();
	void removeBroadcastEventListener();
	bool hasBroadcastListeners() const { return broadcastEventListenerCount; }

	number_t getAlpha() const;
	void setAlpha(number_t alpha);
	number_t getWidth() const;
	void setWidth(number_t width);
	number_t getHeight() const;
	void setHeight(number_t height);
};

}
#endif /* DISPLAY_OBJECT_DISPLAYOBJECT_H */
