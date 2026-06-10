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
#include <functional>
#include <utility>
#include <vector>

#include "backends/colortransformbase.h"
#include "backends/geometry.h"
#include "gc/ptr.h"
#include "exceptions.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::display_object::DisplayObject`.

namespace lightspark
{

union asAtom;
class AVM1Value;
class AVM1Object;
class ASObject;
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
		// `markAsChanged()` for this frame.
		HasChanged = 1 << 12,

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
	using Proj = PerspectiveProjection;

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
	Optional<Proj> perspectiveProjection;

	// The cached transform properties `_{[xy]scale,rotation}`.
	// These are very expensive to calculate, so they're calculated, and
	// cached when scripts request any of these properties.
	number_t rotation;
	Vector2f scale;
	number_t skew;

	// The sound transform of sounds playing from this `DisplayObject`.
	SoundTransform soundTransform;

	// The `DisplayObject` that we're being masked by.
	DisplayObject* masker;

	// The `DisplayObject` that we're currently masking.
	DisplayObject* maskee;

	// The blend mode used when rendering this `DisplayObject`.
	//
	// NOTE: Values other than the default `BLENDMODE_NORMAL` will cause
	// the `DisplayObject` to act as if `cacheAsBitmap` was set.
	AS_BLENDMODE blendMode;

	_NR<ASObject> metaData;

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
	bool markedForTimelineDeletion;

	void fireAddedEvents();
	void setOnParentField();
protected:
	size_t broadcastEventListenerCount;
	mutable Mutex spinlock;

	virtual ~DisplayObject() {}
	RectF computeBoundsForTransformedRect(const RectF& rect, const MATRIX& m) const;

	bool skipRender() const;

	bool defaultRender(RenderContext& ctxt);
public:
	DisplayObject
	(
		SystemState* _sys,
		Optional<const tiny_string&> _name = {}
	);

	bool hasFlag(const Flags& flag) const { return flags & flag; }
	void setFlag(const Flags& flag, bool val)
	{
		if (val)
			flags |= flag;
		else
			flags &= ~flag;
	}

	// Reset all properties that'd be adjusted by a movie load.
	void resetForMovieLoad();

	uint16_t getDepth() const { return depth; }
	void setDepth(uint16_t _depth) { depth = _depth; }
	uint16_t getPlaceFrame() const { return placeFrame; }
	void setPlaceFrame(uint16_t frame) { placeFrame = frame; }
	Transform getTransform(bool applyMatrix = true) const;
	const MATRIX& getMatrix() const { return matrix; }
	void setMatrix(const MATRIX& mtx);
	const ColorTransform& getColorTransform() const { return colorTransform; }
	void setColorTransform(const ColorTransform& ct) { colorTransform = ct; }
	Optional<const Proj&> getPerspectiveProjection() const
	{
		return perspectiveProjection.asRef();
	}

	void setPerspectiveProjection(const Optional<Proj>& proj);
	const Twips& getX() const { return matrix.tx; }
	void setX(const Twips& _x);
	const Twips& getY() const { return matrix.ty; }
	void setY(const Twips& _y);
	Vector2Twips getXY() const { return Vector2Twips(getX(), getY()); }

	// Caches the scale, and rotation factors of the `DisplayObject`, if
	// necessary.
	// Calculating these requires expensive trig ops, so we only do it
	// if `_[xy]scale`, or `_rotation` are accessed.
	void cacheScaleAndRotation();

	number_t getRotaion() const;
	void setRotation(number_t angle);
	number_t getScaleX() const
	void setScaleX(number_t val);
	number_t getScaleY() const;
	void setScaleY(number_t val);
	uint16_t getRatio() const { return ratio; }
	void setRatio(uint16_t _ratio bool inskipping);
	const tiny_string& getName() const { return name; }
	void setName(const tiny_string& _name) { name = _name; }
	const std::vector<Filter>& getFilters() const { return filters; }
	bool setFilters(const Span<Filter>& _filters);
	number_t getAlpha() const { return colorTransform.aMult; }
	void setAlpha(number_t alpha);
	Vector2Twips getSize();
	number_t getWidth() const;
	void setWidth(number_t width);
	number_t getHeight() const;
	void setHeight(number_t height);
	uint16_t getClipDepth() const { return clipDepth; }
	void setClipDepth(uint16_t _depth) { clipDepth = _depth; }
	SystemState* getSys() const { return sys; }
	DisplayObject* getParent() const { return parent; }
	void setParent(DisplayObject* _parent);

	// You should almost always use `setParent()` instead, since that
	// properly handles orphan clips/hidden objects.
	void setParentIgnoringOprhans(DisplayObject* _parent)
	{
		parent = _parent;
	}

	const SoundTransform& getSoundTransform() const
	{
		return soundTransform;
	}

	void setSoundTransform(const SoundTransform& xform)
	{
		soundTransform = xform;
	}

	AS_BLENDMODE getBlendMode() const { return blendMode; }
	void setBlendMode(const AS_BLENDMODE& _blendMode);
	Optional<const RGB&> getOpaqueBackground() const
	{
		return opaqueBackground.asRef();
	}

	void setOpaqueBackground(const Optional<RGB>& colour);
	DisplayObject* getMasker() const { return masker; }
	void setMasker(DisplayObject* mask, bool removeOldLink = true);
	DisplayObject* getMaskee() const { return maskee; }
	void setMaskee(DisplayObject* mask, bool removeOldLink = true);
	_NR<ASObject> getMetaData() const { return metaData; }
	void setMetaData(_NR<ASObject> obj) { metaData = obj; }

	// High level function for setting the mask. This sets both the
	// masker, and maskee.
	//
	// This is equivalent to setting the mask from AVM.
	void setMask(DisplayObject* mask);

	Optional<const Rect<Twips>&> getScrollRect() const
	{
		return scrollRect.asRef();
	}

	const Rect<Twips>& getNextScrollRect() const
	{
		return nextScrollRect;
	}

	void setNextScrollRect(const Rect<Twips>& rect);

	#define FLAG_GETTER_NAME_DECL(name, flag) bool name() const
	#define FLAG_GETTER_PREFIX_DECL(prefix, flag) \
		FLAG_GETTER_NAME_DECL(prefix##flag, flag)
	#define FLAG_GETTER_SUFFIX_DECL(suffix, flag) \
		FLAG_GETTER_NAME_DECL(get##suffix, flag)
	#define FLAG_GETTER_DECL(flag) FLAG_GETTER_PREFIX_DECL(get, flag)

	#define FLAG_SETTER_SUFFIX_DECL(suffix, flag) \
		void set##suffix(bool value)
	#define FLAG_SETTER_DECL(flag) FLAG_SETTER_SUFFIX_DECL(flag, flag)

	#define FLAG_GETTER_SETTER_NAME_DECL(name, flag) \
		FLAG_GETTER_NAME_DECL(name, flag); \
		FLAG_SETTER_DECL(flag)

	#define FLAG_GETTER_SETTER_PREFIX_DECL(prefix, flag) \
		FLAG_GETTER_SETTER_NAME_DECL(prefix##flag, flag)

	#define FLAG_GETTER_SETTER_SUFFIX_DECL(suffix, flag) \
		FLAG_GETTER_SUFFIX_DECL(suffix, flag); \
		FLAG_SETTER_SUFFIX_DECL(suffix, flag)

	#define FLAG_GETTER_SETTER_DECL(flag) \
		FLAG_GETTER_SETTER_PREFIX_DECL(get, flag)

	#define FLAG_GETTER_NAME(name, flag) \
		bool name() const { return hasFlag(Flags::flag); }
	#define FLAG_GETTER_PREFIX(prefix, flag) \
		FLAG_GETTER_NAME(prefix##flag, flag)
	#define FLAG_GETTER_SUFFIX(suffix, flag) \
		FLAG_GETTER_NAME(get##suffix, flag)
	#define FLAG_GETTER(flag) FLAG_GETTER_PREFIX(get, flag)

	#define FLAG_SETTER_SUFFIX(suffix, flag) \
		void set##suffix(bool value) { setFlag(Flags::flag, value); }
	#define FLAG_SETTER(flag) FLAG_SETTER_SUFFIX(flag, flag)

	#define FLAG_GETTER_SETTER_NAME(name, flag) \
		FLAG_GETTER_NAME(name, flag); \
		FLAG_SETTER(flag)

	#define FLAG_GETTER_SETTER_PREFIX(prefix, flag) \
		FLAG_GETTER_SETTER_NAME(prefix##flag, flag)

	#define FLAG_GETTER_SETTER_SUFFIX(suffix, flag) \
		FLAG_GETTER_SUFFIX(suffix, flag); \
		FLAG_SETTER_SUFFIX(suffix, flag)

	#define FLAG_GETTER_SETTER(flag) \
		FLAG_GETTER_SETTER_PREFIX(get, flag)

	FLAG_GETTER_SETTER_NAME(isAVM1Removed, RemovedByAVM1);
	FLAG_GETTER_SETTER_PREFIX(is, AVM1PendingRemoval);
	FLAG_GETTER_SETTER(SkipFrame);
	FLAG_GETTER_SETTER(ScaleAndRotationCached);
	FLAG_GETTER_PREFIX(is, Visible);
	FLAG_SETTER_DECL(Visible);
	FLAG_GETTER_SETTER_NAME(isRoot, IsRoot);
	FLAG_GETTER_SETTER(TransformedByActionScript);
	FLAG_GETTER_SETTER(PlacedByActionScript);
	FLAG_GETTER_SETTER(ManualFrameConstruction);
	FLAG_GETTER_SETTER(LockRoot);
	FLAG_GETTER_SUFFIX(CachedBitmapPreference, CacheAsBitmap);
	FLAG_SETTER_SUFFIX_DECL(CachedBitmapPreference, CacheAsBitmap);
	FLAG_GETTER_SETTER_PREFIX(is, CreatedByTimeline);
	FLAG_GETTER_SETTER(HasScrollRect);
	FLAG_GETTER_SETTER(HasExplicitName);
	FLAG_GETTER_SETTER(HasMatrix3D);

	#undef FLAG_GETTER_NAME_DECL
	#undef FLAG_GETTER_PREFIX_DECL
	#undef FLAG_GETTER_SUFFIX_DECL
	#undef FLAG_GETTER_DECL
	#undef FLAG_SETTER_SUFFIX_DECL
	#undef FLAG_SETTER_DECL
	#undef FLAG_GETTER_SETTER_NAME_DECL
	#undef FLAG_GETTER_SETTER_PREFIX_DECL
	#undef FLAG_GETTER_SETTER_SUFFIX_DECL
	#undef FLAG_GETTER_SETTER_DECL
	#undef FLAG_GETTER_NAME
	#undef FLAG_GETTER_PREFIX
	#undef FLAG_GETTER_SUFFIX
	#undef FLAG_GETTER
	#undef FLAG_SETTER_SUFFIX
	#undef FLAG_SETTER
	#undef FLAG_GETTER_SETTER_NAME
	#undef FLAG_GETTER_SETTER_PREFIX
	#undef FLAG_GETTER_SETTER_SUFFIX
	#undef FLAG_GETTER_SETTER

	virtual bool markAsChanged();
	void resetHasChanged() { setFlag(Flags::HasChanged, false); }
	void geometryChanged();

	_R<CachedSurface> getCachedSurface() const
	{
		return cachedSurface;
	}

	_R<CachedSurface> getCachedSurface()
	{
		return cachedSurface;
	}

	/**
	 * Generate a new IDrawable instance for this object
	 * @param target The topmost object in the hierarchy that is being drawn. Such object
	 * _must_ be on the parent chain of this
	 * @param initialMatrix A matrix that will be prepended to all transformations
	 */
	virtual IDrawable* invalidate(bool smoothing);
	virtual void invalidateForRenderToBitmap
	(
		BitmapContainerRenderData& container,
		bool smoothing
	);

	virtual void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	);

	void updateCachedSurface(IDrawable* d);
	virtual void refreshSurfaceState();
	void setupSurfaceState(IDrawable* d);

	bool isOnStage() const;

	RootMovieClip* getAVM2Root() const;
	Stage* getAVM2Stage() const;
	DisplayObject* getAVM2Parent() const;
	DisplayObject& getAVM1Root(bool noLock = false) const;
	DisplayObject& getAVM1Stage() const;
	DisplayObject* getAVM1Parent() const;
	void setLegacyMatrix(const MATRIX& m);
	void setFilter(const FILTER& filter);

	virtual Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly)
	{
		throw RunTimeException
		(
			"DisplayObject::tryBoundsRect: "
			"Derived class must implement this!"
		);
	}

	Rect<Twips> boundsRect(bool visibleOnly)
	{
		return tryBoundsRect(visibleOnly).valueOr(Rect<Twips> {});
	}

	virtual Optional<Rect<Twips>> tryBoundsRectWithoutChildren(bool visibleOnly)
	{
		return tryBoundsRect(visibleOnly);
	}

	Rect<Twips> boundsRectWithoutChildren(bool visibleOnly)
	{
		return tryBoundsRectWithoutChildren
		(
			visibleOnly
		).valueOr(Rect<Twips> {});
	}

	virtual Rect<Twips> boundsRectWithTransform(const MATRIX& mtx);
	Optional<Rect<Twips>> tryBoundsRectGlobal(bool fromCurrentRendering = true);
	Rect<Twips> boundsRectGlobal(bool fromCurrentRendering = true)
	{
		return tryBoundsRectGlobal
		(
			fromCurrentRendering
		).valueOr(Rect<Twips> {});
	}

	static bool isShaderBlendMode(const AS_BLENDMODE& mode);
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

	bool hasFilters() const { return !filters.empty(); }
	void requestInvalidationFilterParent(InvalidateQueue* q = nullptr);
	virtual void requestInvalidationIncludingChildren(InvalidateQueue* q);
	uint16_t getParentDepth() const;
	uint16_t findParentDepth(DisplayObject* d) const;
	DisplayObject* getAncestor(uint16_t depth) const;
	std::pair<DisplayObject*, uint16_t> findCommonAncestor
	(
		DisplayObject* obj,
		bool init = true
	) const;

	bool findParent(DisplayObject* d) const;
	const Rect<Twips>& getScalingGrid() const { return scalingGrid; }
	void setScalingGrid(const Rect<Twips>& rect) { scalingGrid = rect; }
	bool isInInitFrame() const { return inInitFrame; }
	MATRIX getConcatenatedMatrix
	(
		bool includeRoot = false,
		bool fromCurrentRendering = true
	) const;

	MATRIX localToGlobalMatrix(bool fromCurrentRendering = true) const;
	MATRIX globalToLocalMatrix(bool fromCurrentRendering = true) const;
	Vector2Twips localToGlobal
	(
		const Vector2Twips& point,
		bool fromCurrentRendering = true
	) const;

	Vector2Twips globalToLocal
	(
		const Vector2Twips& point,
		bool fromCurrentRendering = true
	) const;

	number_t getConcatenatedAlpha() const;
	virtual number_t getScaleFactor() const;

	// used by MorphShapes and embedded video
	virtual void checkRatio(uint16_t ratio, bool inskipping) {}

	tiny_string getPath(bool dotNotation = true);
	virtual void afterTimelineCreation() {}
	virtual void afterTimelineDeletion(bool inskipping) {}
	virtual uint32_t getTagID() const { return UINT32_MAX; }
	virtual LoaderInfo* getLoaderInfo() const { return nullptr; }
	virtual const SWFMovie& getMovie() const = 0;

	Optional<Rect<Twips>> tryGetBounds(const MATRIX& m, bool visibleOnly = false);
	Rect<Twips> getBounds(const MATRIX& m, bool visibleOnly = false)
	{
		return tryGetBounds(m, visibleOnly).valueOr(Rect<Twips> {});
	}

	virtual bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	);

	bool hitTestBounds(const Vector2Twips& point) const
	{
		return boundsRectGlobal(false).contains(point);
	}

	bool hitTestObject(const DisplayObject& obj) const
	{
		auto a = boundsRectGlobal(false);
		auto b = obj.boundsRectGlobal(false);
		return a.intersects(b);
	}

	bool isMarkedForTimelineDeletion() const
	{
		return markedForTimelineDeletion;
	}

	void setMarkedForTimelineDeletion(bool flag)
	{
		markedForTimelineDeletion = flag;
	}

	virtual void enterFrame(bool implicit) {}
	virtual void advanceFrame(bool implicit) {}
	virtual void declareFrame(bool implicit) {}
	virtual void initFrame() {}
	virtual void afterInitFrame();
	virtual void executeFrameScript() {}
	bool needsActionScript3() const { return getMovie().isAS3(); }
	bool isAS3() const { return needsActionScript3(); }
	virtual void handleMouseCursor(bool rollover) {}
	virtual bool allowAsMask() const { return true; }
	Vector2f getLocalMousePos();
	// Nominal width and heigt are the size before scaling and rotation
	Vector2f getNominalSize();
	number_t getNominalWidth() { return getNominalSize().x; }
	number_t getNominalHeight() { return getNominalSize().y; }
	void addBroadcastEventListener();
	void removeBroadcastEventListener();
	bool hasBroadcastListeners() const { return broadcastEventListenerCount; }

	void preRender();
	void avm1Unload();
	Span<const AVM1VarBinding> getAVM1VarBindings() const
	{
		return {};
	}

	Span<AVM1VarBinding> getAVM1VarBindings()
	{
		return {};
	}

	virtual _NGC<AVM1Object> tryToAVM1Object() const { return NullGc; }
	AVM1Value toAVM1ValueOrUndef() const;
	_GC<AVM1Object> toAVM1Object() const;

	virtual _NR<ASObject> toASObject() const { return NullRef; }
	asAtom toASAtomOrNull() const;
	uint8_t getSwfVersion() const { return getMovie().getVersion(); }

	// Get a named property from the AVM1 object.
	//
	// This is required because some `Boolean` properties in AVM1 can
	// actually hold any value.
	bool getAVM1BoolProp
	(
		const tiny_string& name,
		std::function<void(SystemState*)> _default
	) const;

	void setAVM1Prop(const tiny_string& name, const AVM1Value& value);

	template<typename T>
	bool is() const;
	template<typename T>
	T* as() const;
};

}
#endif /* DISPLAY_OBJECT_DISPLAYOBJECT_H */
