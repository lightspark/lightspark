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

#include <algorithm>
#include <array>

#include "display_object/DisplayObject.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "gc/ptr.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

Optional<Rect<Twips>> DisplayObject::tryGetBounds(const MATRIX& m, bool visibleOnly)
{
	if (!isCreatedByTimeline())
		return {};

	return tryBoundsRect(visibleOnly).transform([&](const auto& rect)
	{
		auto ret = std::minmax
		({
			m.multiply2D(rect.tl()),
			m.multiply2D(rect.tr()),
			m.multiply2D(rect.bl()),
			m.multiply2D(rect.br())
		});
		return makeOptional(Rect<Twips> { ret.first, ret.second });
	});
}

Vector2f DisplayObject::getNominalSize()
{
	return boundsRect(false).size();
}

void DisplayObject::addBroadcastEventListener()
{
	if (!broadcastEventListenerCount++)
		sys->registerFrameListener(this);
}

void DisplayObject::removeBroadcastEventListener()
{
	assert(broadcastEventListenerCount);
	if (!--broadcastEventListenerCount)
		sys->unregisterFrameListener(this);
}

DisplayObject::DisplayObject
(
	SystemState _sys,
	Optional<const tiny_string&> _name
) :
sys(_sys),
parent(nullptr),
placeFrame(0),
depth(0),
clipDepth(0),
ratio(0),
name(*_name.orElse([&] { return _sys->getNextInstanceName() }),
rotation(0),
scale(1, 1),
skew(0),
masker(nullptr),
maskee(nullptr),
blendMode(BLENDMODE_NORMAL),
flags(Flags::Visible),
inInitFrame(false),
cachedSurface(new CachedSurface()),
needsTextureRecalculation(true),
textureRecalculationSkippable(false),
markedForLegacyDeletion(false),
broadcastEventListenerCount(0)
{
}

void DisplayObject::markAsChanged()
{
	setFlag(Flags::HasChanged, true);
	if (isOnStage())
		requestInvalidation(sys);
	else
		requestInvalidationFilterParent();
}

void DisplayObject::updatedRect()
{
	markAsChanged();
}

bool DisplayObject::needsCacheAsBitmap() const
{
	return
	(
		getCachedBitmapPreference() ||
		blendMode == BLENDMODE_MULTIPLY ||
		blendMode == BLENDMODE_ADD ||
		blendMode == BLENDMODE_SCREEN ||
		blendMode == BLENDMODE_DARKEN ||
		blendMode == BLENDMODE_DIFFERENCE ||
		blendMode == BLENDMODE_HARDLIGHT ||
		blendMode == BLENDMODE_LIGHTEN ||
		blendMode == BLENDMODE_OVERLAY ||
		blendMode == BLENDMODE_ERASE ||
		hasFilters()
	);
}

void DisplayObject::requestInvalidationFilterParent(InvalidateQueue* q)
{
	if (masker == this)
		return;

	auto needsRefresh = [&](DisplayObject* obj)
	{
		auto surface = obj->cachedSurface
		return surface->cachedFilterTextureID != UINT32_MAX ||
		(
			!surface->isInitialized &&
			(
				obj->hasFilters() ||
				obj->inMask() ||
				isShaderBlendMode(obj->blendMode)
			)
		) || !obj->scalingGrid.isValid();
	};

	if (needsRefresh(this))
	{
		if (cachedSurface->getState() != nullptr)
			cachedSurface->getState()->needsFilterRefresh = true;
		hasChanged = true;
		requestInvalidationIncludingChildren(q);
	}

	for (auto p = getParent(); p != nullptr; p = p->getParent())
	{
		if (needsRefresh(p))
		{
			p->requestInvalidationFilterParent(q);
			break;
		}
	}
}

void DisplayObject::requestInvalidationIncludingChildren(InvalidateQueue* q)
{
	setFlag(Flags::HasChanged, true);
	if (q == nullptr)
		return;

	q->addToInvalidateQueue(this);
	if (masker != nullptr)
		masker->requestInvalidationIncludingChildren(q);
}

void DisplayObject::setMatrix(const MATRIX& mtx)
{
	Locker locker(spinlock);
	if (matrix == mtx)
		return;

	matrix = mtx;
	setScaleAndRotationCached(false);
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setFilter(const FILTER& filter)
{
	filterListHasChanged = hasSameFilters(makeSpan({ filter }));
	filters = { filter };

	hasChanged = true;
	requestInvalidation(sys);
	updateFilterBorder();
}

void DisplayObject::refreshSurfaceState()
{
}

void DisplayObject::setupSurfaceState(IDrawable* d)
{
	SurfaceState* state = d->getState();
	assert(state != nullptr);
	#ifndef NDEBUG
	state->src = this; // keep track of the DisplayObject when debugging
	#endif
	if (masker != nullptr)
		state->mask = masker->getCachedSurface();
	else
		state->mask.reset();

	state->clipdepth = clipDepth;
	state->depth = depth;
	state->isMask = maskee != nullptr || clipDepth;
	state->visible = visible;

	state->alpha = getAlpha();
	state->allowAsMask = allowAsMask();
	state->maxfilterborder = getMaxFilterBorder();

	if (filterListHasChanged)
	{
		state->filters.clear();
		state->filters.reserve(filters.size() * 2);
		for (const auto& filter : filters)
		{
			FilterData fdata;
			filter.getRenderFilterGradient
			(
				fdata.gradientColors,
				fdata.gradientStops
			);

			uint32_t step = 0;
			for (size_t step = 0; fdata.filterData[0]; ++step)
			{
				filter.getRenderFilterArgs(step, fdata.filterdata);
				state->filters.push_back(fdata);
			}
		}
	}

	state->bounds = tryBoundsRectWithoutChildren(false).valueOr(state->bounds);
	state->scrollRect = scrollRect;

	if (is<RootMovieClip>())
	{
		state->renderWithNanoVG = true;
		state->opaqueBackground = as<RootMovieClip>()->getBackground();
		state->bounds = tryBoundsRect(false).valueOr(state->bounds);
	}
	else
		state->opaqueBackground = opaqueBackground;
	currentrendermatrix = state->matrix;
}

void DisplayObject::setMasker(DisplayObject* mask, bool removeOldLink)
{
	if (!removeOldLink)
	{
		masker = mask;
		return;
	}

	if (masker != nullptr)
		masker->setMaskee(nullptr, false);

	bool mustInvalidate = masker != mask ||
	(
		mask != nullptr &&
		mask->getHasChanged()
	);

	masker = mask;

	if (mustInvalidate)
		markAsChanged();
}

void DisplayObject::setMaskee(DisplayObject* mask, bool removeOldLink)
{
	if (!removeOldLink)
	{
		maskee = mask;
		return;
	}

	if (maskee != nullptr)
		maskee->setMasker(nullptr, false);

	bool mustInvalidate = maskee != mask ||
	(
		mask != nullptr &&
		mask->getHasChanged()
	);

	maskee = mask;

	if (mustInvalidate)
		markAsChanged();
}

void DisplayObject::setMask(DisplayObject* mask)
{
	setClipDepth(0);
	setMasker(mask);

	if (mask != nullptr)
	{
		mask->setClipDepth(0);
		mask->setMaskee(this);
	}
}

void DisplayObject::setBlendMode(const AS_BLENDMODE& _blendMode)
{
	blendMode =
	(
		_blendMode >= BLENDMODE_LAYER &&
		_blendMode <= BLENDMODE_HARDLIGHT
	) ? _blendMode : BLENDMODE_NORMAL;
}

bool DisplayObject::isShaderBlendMode(AS_BLENDMODE bl)
{
	// TODO add shader for other extended blendmodes
	return
	(
		bl == AS_BLENDMODE::BLENDMODE_OVERLAY
		bl == BLENDMODE_HARDLIGHT ||
		bl == BLENDMODE_DARKEN ||
		bl == BLENDMODE_LIGHTEN
	);
}

MATRIX DisplayObject::getConcatenatedMatrix(bool includeRoot, bool fromCurrentRendering) const
{
	auto _getMatrix = [&]
	{
		return
		(
			!fromCurrentRendering ?
			getMatrix() :
			currentRenderMatrix
		);
	};

	if (parent == nullptr || (!includeRoot && parent == sys->mainClip))
		return _getMatrix();
	return parent->getConcatenatedMatrix
	(
		includeRoot,
		fromCurrentRendering
	).multiplyMatrix(_getMatrix());
}

float DisplayObject::getConcatenatedAlpha() const
{
	return
	(
		parent != nullptr ?
		parent->getConcatenatedAlpha() * clippedAlpha() :
		clippedAlpha()
	);
}

float DisplayObject::getScaleFactor() const
{
	return 1.0;
}

tiny_string DisplayObject::getPath(bool dotNotation)
{
	if (parent == nullptr)
	{
		return
		(
			std::stringstream() <<
			"_level" << depth
		).str();
	}

	tiny_string path =
	(
		parent->getPath(dotNotation) +
		dotNotation ? '.' : '/'
	);

	if (!name.empty())
		path += name;
	return path;
}

void DisplayObject::afterTimelineCreation()
{
}

bool DisplayObject::skipRender() const
{
	return maskee == nullptr && clipDepth &&
	(
		!isVisible() ||
		!getAlpha()
	);
}

bool DisplayObject::defaultRender(RenderContext& ctxt)
{
	const auto& t = ctxt.transformStack().transform();
	const auto* surface = ctxt.getCachedSurface(this);
	/* surface is only modified from within the render thread
	 * so we need no locking here */
	bool canRender =
	(
		surface->isValid &&
		surface->isInitialized &&
		surface->tex != nullptr &&
		surface->tex->isValid() &&
		surface->tex->width > 0 &&
		surface->tex->height > 0
	)
	if (!canRender)
		return true;

	auto r = scalingGrid;
	if (!scalingGrid.isValid() && parent != nullptr)
		r = parent->scalingGrid;

	ctxt.lsglLoadIdentity();
	ColorTransformBase ct = t.colorTransform;
	MATRIX m = t.matrix;
	ctxt.renderTextured
	(
		*surface->tex,
		surface->getState()->alpha,
		RenderContext::RGB_MODE,
		ct,
		false,
		0,
		RGB(),
		surface->getState()->smoothing,
		m,
		r->getRect(),
		t.blendmode
	);
	return false;
}

Rect<Twips> DisplayObject::computeBoundsForTransformedRect
(
	const RectF& rect,
	const MATRIX& m
) const
{
	//As the transformation is arbitrary we have to check all the four vertices
	std::array<Vector2Twips, 4> coords =
	{
		m.multiply2D(rect.tl()),
		m.multiply2D(rect.bl()),
		m.multiply2D(rect.br()),
		m.multiply2D(rect.tr()),
	}
	//Now find out the minimum and maximum that represent the complete bounding rect
	Rect<Twips> ret { coords[3], coords[3] };
	for (size_t i = 0; i < coords.size() - 1; ++i)
		ret = ret._union(RectF { coords[i], coords[i] });
	return ret;
}

IDrawable* DisplayObject::invalidate(bool smoothing)
{
	//Not supposed to be called
	throw RunTimeException("DisplayObject::invalidate");
}

void DisplayObject::invalidateForRenderToBitmap
(
	BitmapContainerRenderData* container,
	bool smoothing
)
{
	if (masker != nullptr)
		masker->invalidateForRenderToBitmap(container, smoothing);
	IDrawable* d = invalidate(smoothing);
	if (d == nullptr)
		return;

	setupSurfaceState(d);
	#ifdef ENABLE_CAIRO
	if (getNeedsTextureRecalculation() || !d->isCachedSurfaceUsable(this))
	{
		AsyncDrawJob* j = new AsyncDrawJob(d, this);
		j->execute();
		j->threadAbort(); // avoid addUploadJob in jobFence()
		container->uploads.push_back(j);
		return;
	}
	#endif
	RefreshableSurface s;
	s.displayobject = this;
	s.drawable = d;
	container->surfacesToRefresh.push_back(s);
}

void DisplayObject::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	//Let's invalidate also the mask
	bool needsInvalidation = masker != nullptr && masker != this &&
	(
		mask->getHasChanged() ||
		forceTextureRefresh
	);

	if (needsInvalidation)
		masker->requestInvalidation(q, forceTextureRefresh);
}

void DisplayObject::updateCachedSurface(IDrawable* d)
{
	// this is called only from rendering thread, so no locking done here
	cachedSurface->SetState(d->getState());
	cachedSurface->isValid=true;
	cachedSurface->isInitialized=true;
	cachedSurface->wasUpdated=true;
}

MATRIX DisplayObject::localToGlobalMatrix(bool fromCurrentRendering) const
{
	MATRIX mtx;
	(void)scrollRect.andThen([&](const auto& rect)
	{
		mtx.translate(-rect.min);
		return makeOptional(rect);
	});

	return getConcatenatedMatrix(true, fromCurrentRendering) * mtx;
}

MATRIX DisplayObject::globalToLocalMatrix(bool fromCurrentRendering) const
{
	return localToGlobalMatrix(fromCurrentRendering).getInverted();
}

Vector2Twips DisplayObject::localToGlobal
(
	const Vector2Twips& point,
	bool fromRurrentRendering
) const
{
	return localToGlobalMatrix(fromCurrentRendering) * point;
}

Vector2Twips DisplayObject::globalToLocal
(
	const Vector2Twips& point,
	bool fromCurrentRendering = true
) const
{
	return globalToLocalMatrix(fromCurrentRendering) * point;
}

void DisplayObject::setAlpha(number_t alpha)
{
	/* The stored value is not clipped, _getAlpha will return the
	 * stored value even if it is outside the [0, 1] range. */
	if (colorTransform.aMult == alpha)
		return;

	setTransformedByActionScript(true);
	colorTransform.aMult = alpha;
	markAsChanged();
}

void DisplayObject::setScaleX(number_t val)
{
	if (scale.x == val)
		return;

	setTransformedByActionScript(true);
	cacheScaleAndRotation();
	scale.x = val;

	// NOTE: In to match Flash Player's behaviour, `scaleX`/`_xscale`
	// is allowed to return `NaN`, but we treat it as `0` for the
	// purposes of updating the matrix.
	auto value = val / 100.0;
	if (std::isnan(value))
		value = 0;

	// NOTE: In a similar fashion, `{,_}rotation` can return also `NaN`,
	// but we treat it as `0` when updating the matrix.
	auto _rotation = rotation * (M_PI / 180.0);
	if (std::isnan(_rotation))
		_rotation = 0;

	//Apply the difference
	auto cos = std::cos(_rotation);
	auto sin = std::sin(_rotation);
	matrix.a = cos * value;
	matrix.b = sin * value;

	setFlag(Flags::HasChanged, true);
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setScaleY(number_t val)
{
	if (scale.y == val)
		return;

	setTransformedByActionScript(true);
	cacheScaleAndRotation();
	scale.y = val;

	// NOTE: In to match Flash Player's behaviour, `scaleY`/`_yscale`
	// is allowed to return `NaN`, but we treat it as `0` for the
	// purposes of updating the matrix.
	auto value = val / 100.0;
	if (std::isnan(value))
		value = 0;

	// NOTE: In a similar fashion, `{,_}rotation` can return also `NaN`,
	// but we treat it as `0` when updating the matrix.
	auto _rotation = rotation * (M_PI / 180.0);
	if (std::isnan(_rotation))
		_rotation = 0;

	//Apply the difference
	auto cos = std::cos(_rotation);
	auto sin = std::sin(_rotation);
	matrix.c = -sin * value;
	matrix.d = cos * value;

	setFlag(Flags::HasChanged, true);
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setVisible(bool value)
{
	bool changed = isVisible() != value;
	setFlag(Flags::Visible, value);

	if (changed)
		markAsChanged();

	// NOTE: A `DisplayObject`s focus is dropped when it's made invisible.
	auto intrObj = as<InteractiveObject>();
	if (!value && intrObj != nullptr)
		intrObj->dropFocus();
}

void DisplayObject::setX(const Twips& x)
{
	if (matrix.tx == x)
		return;

	//Apply translation, it's trivial
	matrix.tx = x;

	setTransformedByActionScript(true);
	setFlag(Flags::HasChanged, true);
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setY(const Twips& y)
{
	if (matrix.ty == y)
		return;

	//Apply translation, it's trivial
	matrix.ty = y;

	setTransformedByActionScript(true);
	setFlag(Flags::HasChanged, true);
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setParent(DisplayObject* _parent)
{
	if (parent == _parent)
		return;

	bool hadParent = parent != nullptr;
	if (_parent != nullptr)
	{
		// mark old parent as dirty
		geometryChanged();
		sys->removeFromResetParentList(this);
		sys->stage->removeHiddenObject(this);
	}

	parent = _parent;
	setFlag(Flags::HasChanged, true);
	geometryChanged();

	if (isOnStage() && !sys->isShuttingDown())
		requestInvalidation(sys);

	if (!hadParent || parent != nullptr)
		return;

	auto intrObj = as<InteractiveObject>();
	if (intrObj != nullptr)
		intrObj->dropFocus();
	onParentRemoved();
}

Vector2Twips DisplayObject:getSize()
{
	return getBounds(matrix).size();
}

void DisplayObject::geometryChanged()
{
	auto container = as<DisplayObjectContainer>();
	if (container != nullptr)
		container->markBoundsRectDirtyChildren();

	if (parent != nullptr)
		parent->geometryChanged();
}

RootMovieClip* DisplayObject::getAVM2Root()
{
	return parent != nullptr ? parent->getAVM2Root() : nullptr;
}

Stage* DisplayObject::getAVM2Stage()
{
	return parent != nullptr ? parent->getAVM2Stage() : nullptr;
}

number_t DisplayObject::getWidth() const
{
	return getSize().x.toPx();
}

void DisplayObject::setWidth(number_t width)
{
	if (std::isnan(width))
		return;

	if (std::isinf(width) && isAS3())
		width = 0;

	auto size = boundsRect(false).size().toPx();
	auto aspectRatio = size.y / size.x;

	Vector2f targetScale;
	if (size.x != 0)
		targetScale = Vector2f(width, width) / size;

	auto prevScale = scale * 100;
	auto _rotation = rotation * (M_PI / 180.0);
	auto cos = std::abs(std::cos(_rotation));
	auto sin = std::abs(std::sin(_rotation));

	Vector2f newScale
	(
		// x.
		aspectRatio * (cos * targetScale.x + sin * targetScale.y) /
		((cos + aspectRatio * sin) * (aspectRatio * cos + sin)),
		// y.
		(sin * prevScale.x + aspectRatio * cos * prevScale.y) /
		(aspectRatio * cos + sin)
	);

	setScaleX(std::isfinite(newScale.x) ? newScale.x / 100.0 : 0);
	setScaleY(std::isfinite(newScale.y) ? newScale.y / 100.0 : 0);
}

number_t DisplayObject::getHeight() const
{
	return getSize().y.toPx();
}

void DisplayObject::setHeight(number_t height)
{
	if (std::isinf(width) && isAS3())
		width = 0;

	auto size = boundsRect(false).size().toPx();
	auto aspectRatio = size.y / size.x;

	Vector2f targetScale;
	if (size.y != 0)
		targetScale = Vector2f(height, height) / size;

	auto prevScale = scale * 100;
	auto _rotation = rotation * (M_PI / 180.0);
	auto cos = std::abs(std::cos(_rotation));
	auto sin = std::abs(std::sin(_rotation));

	Vector2f newScale
	(
		// x.
		(aspectRatio * cos * prevScale.x + sin * prevScale.y) /
		(aspectRatio * cos + sin),
		// y.
		aspectRatio * (sin * targetScale.x + cos * targetScale.y) /
		((cos + aspectRatio * sin) * (aspectRatio * cos + sin))
	);

	setScaleX(std::isfinite(newScale.x) ? newScale.x / 100.0 : 0);
	setScaleY(std::isfinite(newScale.y) ? newScale.y / 100.0 : 0);
}

void DisplayObject::setRatio(uint16_t _ratio, bool inskipping)
{
	ratio = _ratio;
	setFlag(Flags::HasChanged, true);
	checkRatio(_ratio, inskipping);
}

Vector2Twips DisplayObject::getLocalMousePos()
{
	auto mousePos = sys->getInputThread()->getMousePos();
	return localToGlobalMatrix() * mousePos.toPx();
}

bool DisplayObject::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	const bool skipInvis = flags & HitTestFlags::SkipInvisible;
	// Default to using the bounding box.
	return (!skipInvis || isVisible()) && hitTestBounds(globalPoint);
}

// Based on Ruffle's `TDisplayObject::on_construction_complete()`.
void DisplayObject::afterInitFrame()
{
	auto placedByAS = getPlacedByActionScript();
	fireAddedEvents();

	// Use the value of `placedByActionScript` from before we fired any
	// events, since said events might create new clips inside them,
	// causing `placedByActionScript` to be set.
	if (!placedByAS)
		setOnParentField();
}

void DisplayObject::fireAddedEvents()
{
	if (!isAS3() || getPlacedByActionScript())
		return;

	// NOTE: Children that're added to buttons by the timeline don't
	// fire events.
	if (parent != nullptr && parent->is<SimpleButton>())
		return;
	/*
	 * Legacy objects have their display list properties set on creation, but
	 * the related events must only be sent after the constructor is sent.
	 * This is from "Order of Operations".
	 */
	auto e = _MR(Class<Event>::getInstanceS(sys->worker, "added", true));
	ABCVm::publicHandleEvent(this, e);

	if (getAVM2Stage() != nullptr)
	{
		e = _MR(Class<Event>::getInstanceS(sys->worker, "addedToStage"));
		ABCVm::publicHandleEvent(this, e);
	}
}

void DisplayObject::setOnParentField()
{
	bool canSet =
	(
		hasExplicitName() &&
		parent != nullptr &&
		!parent->toASObject().isNull() &&
		!toASObject().isNull() &&
		!name.empty()
	)

	if (!canSet)
		return;

	auto parentObj = parent->toASObject();
	auto childObj = toASObject();
	auto wrk = childObj->getInstanceWorker();

	multiname objName(nullptr);
	objName.name_type = multiname::NAME_STRING;
	objName.name_s_id = name;
	objName.ns.emplace_back(sys, BUILTIN_STRINGS::EMPTY, NAMESPACE);
	asAtom val = asAtomHandler::invalidAtom;
	parentObj->getVariableByMultiname
	(
		val,
		objName,
		GET_VARIABLE_OPTION::DONT_CALL_GETTER,
		wrk
	);

	auto obj = asAtomHandler::tryAs<ASDisplayObject>(val);
	auto _obj = obj != nullptr ? obj->getBase() : nullptr;

	bool isDifferent = _obj != this &&
	(
		_obj == nullptr ||
		_obj->parent == nullptr ||
		_obj->depth >= depth
	);

	if (!isDifferent)
	{
		ASATOM_DECREF(obj);
		return;
	}

	if (asAtomHandler::isValid(obj))
	{
		ASATOM_DECREF(obj);
		parentObj->deleteVariableByMultiname(objName, wrk);
	}

	childObj->incRef();
	parentObj->setVariableByMultiname
	(
		objName,
		asAtomHandler::fromObject(childObj),
		CONST_NOT_ALLOWED,
		nullptr,
		wrk
	);
}

void DisplayObject::applyFilters
(
	BitmapContainer* target,
	BitmapContainer* source,
	const RECT& sourceRect,
	const Vector2f& pos,
	const Vector2f& scale
)
{
	for (const auto& filter : filters)
	{
		filter.applyFilter
		(
			target,
			source,
			sourceRect,
			pos,
			scale,
			this
		);
	}
}

void DisplayObject::setNeedsTextureRecalculation(bool skippable)
{
	textureRecalculationSkippable = skippable;
	needsTextureRecalculation = true;
	if (!cachedSurface->isChunkOwner)
		cachedSurface->tex = nullptr;
	cachedSurface->isChunkOwner = true;
}

string DisplayObject::toDebugString() const
{
	std::string res = EventDispatcher::toDebugString();
	res += "tag=";
	char buf[100];
	sprintf(buf,"%u %s pa=%p",getTagID(),legacy ? "legacy":"",getParent());
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

	auto _bounds = boundsRect(false);
	if (!_bounds.hasValue())
	{
		//No contents, nothing to do
		return nullptr;
	}

	MATRIX matrix = getMatrix();

	bool isMask=false;
	MATRIX m;
	m.scale(matrix.getScaleX(), matrix.getScaleY());
	auto bounds = computeBoundsForTransformedRect(*_bounds, m);
	auto size = bounds.size();

	if (isnan(size.x) || isnan(size.y))
	{
		// on stage with invalid concatenatedMatrix. Create a trash initial texture
		size = Vector2f(1, 1);
	}

	if (size >= Vector2f(8192, 8192) || (size.x * size.y) >= 16777216)
		return nullptr;

	if (size == Vector2f())
		return nullptr;

	resetNeedsTextureRecalculation();
	return new RefreshableDrawable
	(
		bounds.min.x,
		bounds.min.y,
		ceil(size.x),
		ceil(size.y),
		matrix.getScaleX(),
		matrix.getScaleY(),
		isMask,
		cacheAsBitmap,
		getScaleFactor(),
		getConcatenatedAlpha(),
		colorTransform,
		(
			smoothing ?
			SMOOTH_MODE::SMOOTH_ANTIALIAS :
			SMOOTH_MODE::SMOOTH_NONE
		),
		getBlendMode(),
		matrix
	);
}

bool DisplayObject::findParent(DisplayObject* d) const
{
	if (this == d)
		return true;
	if (parent == nullptr)
		return false;
	return parent->findParent(d);
}

int DisplayObject::getParentDepth() const
{
	int i;
	DisplayObject* p;
	for (i = 0, p = parent; p != nullptr; p = p->parent, ++i);
	return i;
}

int DisplayObject::findParentDepth(DisplayObject* d) const
{
	if (this != d)
	{
		int i;
		DisplayObject* p;
		for (i = 0, p = parent; p != nullptr && p != d; p = p->parent, ++i);
		return i;
	}
	return -1;
}

DisplayObject* DisplayObject::getAncestor(int depth) const
{
	if (depth < 0)
		return this;
	if (!depth)
		return parent;
	if (parent == nullptr)
		return nullptr;
	return parent->getAncestor(--depth);
}

DisplayObject* DisplayObject::findCommonAncestor(DisplayObject* d, int& depth, bool init) const
{
	const auto a = this;
	const auto b = d;
	if (init)
	{
		depth = 0;
		if ((a->getParentDepth() - b->getParentDepth()) < 0)
			std::swap(a, b);
	}

	if (a->parent == nullptr || b == nullptr)
		return a->parent == nullptr ? a : nullptr;
	if (b->findParent(a->parent))
		return a->parent;
	return a->parent->findCommonAncestor(b, ++depth, false);
}

// Compute the minimal, axis aligned bounding box in global
// coordinates
Optional<RectF> DisplayObject::boundsRectGlobal(bool fromCurrentRendering)
{
	return boundsRect(false).andThen([&](const auto& rect)
	{
		auto m = getConcatenatedMatrix(true, fromcurrentrendering);
		// check all four corners to get the bounding box taking rotation into account
		auto tmp = m.multiply2D(rect.tl());
		auto ret = rect._union(RectF { tmp, tmp });

		tmp = m.multiply2D(rect.tr());
		ret = rect._union(RectF { tmp, tmp });
		tmp = m.multiply2D(rect.bl());
		ret = rect._union(RectF { tmp, tmp });
		tmp = m.multiply2D(rect.br());
		ret = rect._union(RectF { tmp, tmp });
	});
}

DisplayObject* DisplayObject::getAVM1Stage() const
{
	if (parent == nullptr)
		return const_cast<DisplayObject*>(this);

	if (parent->is<Loader>() || parent->is<Stage>())
		return parent;
	return parent->getAVM1Stage();
}

DisplayObject* DisplayObject::getAVM1Root()
{
	if (needsActionScript3())
		return sys->mainClip;
	if (parent != nullptr && parent->needsActionScript3())
		return this;
	if (is<RootMovieClip>())
		return this;
	if (!lockRoot && parent != nullptr)
		return parent->getAVM1Root();
	return sys->mainClip;
}
