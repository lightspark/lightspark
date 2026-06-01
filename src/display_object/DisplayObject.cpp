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

#include <algorithm>
#include <array>

#include "display_object/DisplayObject.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "gc/ptr.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

Vector2f DisplayObject::getXY()
{
	auto mtx = getMatrix();
	return Vector2f
	(
		mtx.getTranslateX(),
		mtx.getTranslateY()
	) / TWIPS_FACTOR;
}

Optional<RectF> DisplayObject::getBounds(const MATRIX& m, bool visibleOnly)
{
	if (!legacy)
		return {};

	return boundsRect(visibleOnly).transform([&](const auto& rect)
	{
		auto ret = std::minmax
		({
			m.multiply2D(rect.tl()),
			m.multiply2D(rect.tr()),
			m.multiply2D(rect.bl()),
			m.multiply2D(rect.br())
		});
		return makeOptional(RectF { ret.first, ret.second });
	});
}

Vector2f DisplayObject::getNominalSize()
{
	return boundsRect(false).transformOr(Vector2f(), [&](const auto& rect)
	{
		return rect.size() / TWIPS_FACTOR;
	});
}

bool DisplayObject::inMask() const
{
	if (mask != nullptr || getClipDepth())
		return true;
	if (parent != nullptr)
		return parent->inMask();
	return false;
}

bool DisplayObject::belongsToMask() const
{
	if (maskCount)
		return true;
	if (parent != nullptr)
		return parent->belongsToMask();
	return false;
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
tz(0),
rotation(0),
scale(1, 1),
scaleZ(1),
blendMode(BLENDMODE_NORMAL),
isLoadedRoot(false),
inInitFrame(false),
filterlistHasChanged(false),
hasMatrix3D(false),
maskCount(0),
maxFilterBorder(0),
cachedSurface(new CachedSurface()),
useLegacyMatrix(true),
needsTextureRecalculation(true),
textureRecalculationSkippable(false),
broadcastEventListenerCount(0),
removedByAVM1(false),
visible(true),
transformedByActionScript(false),
placedByActionScript(false),
skipFrame(false),
hasChanged(true),
legacy(false),
markedForLegacyDeletion(false),
cacheAsBitmap(false),
lockRoot(false),
mask(nullptr),
clipMask(nullptr),
invalidateQueueNext(nullptr) {}

void DisplayObject::markAsChanged()
{
	hasChanged = true;
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
		cacheAsBitmap ||
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

bool DisplayObject::hasFilters() const
{
	return !filters.empty();
}

void DisplayObject::requestInvalidationFilterParent(InvalidateQueue* q)
{
	if (mask == this)
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
				isShaderBlendMode(obj->getBlendMode())
			)
		) || !obj->scalingGrid.isNull();
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
	hasChanged = true;
	if (q == nullptr)
		return;

	q->addToInvalidateQueue(this);
	if (mask != nullptr)
		mask->requestInvalidationIncludingChildren(q);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_getter_cacheAsBitmap)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	ret = asAtomHandler::fromBool(th->cacheAsBitmap);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setter_cacheAsBitmap)
{
	if(!asAtomHandler::is<DisplayObject>(obj))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Function applied to wrong object");
		return;
	}
	if(argslen != 1)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if (th->cacheAsBitmap != asAtomHandler::toInt(args[0]))
	{
		th->hasChanged=true;
		th->cacheAsBitmap = asAtomHandler::toInt(args[0]);
		th->requestInvalidation(wrk->getSystemState());
	}
}

void DisplayObject::setMatrix(const MATRIX& m)
{
	Locker locker(spinlock);
	if (getMatrix() == m)
		return;

	matrix = m;
	extractValuesFromMatrix();
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setMatrix3D(const Matrix3D* m)
{
	if (m == nullptr)
		return;

	Locker locker(spinlock);
	hasMatrix3D=true;
	float rawdata[16];
	m->getRawDataAsFloat(rawdata);
	matrix.x0 = rawdata[12] * TWIPS_FACTOR;
	matrix.y0 = rawdata[13] * TWIPS_FACTOR;
	tz = rawdata[14];
	scale = Vector2f(rawdata[0], rawdata[5]);
	scaleZ = rawdata[10];
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"Not all values of Matrix3D are handled in DisplayObject"
	);
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setLegacyMatrix(const MATRIX& m)
{
	if (!useLegacyMatrix)
		return;

	Locker locker(spinlock);
	bool isDifferent =
	(
		m.getTranslateX() != matrix.x0 ||
		m.getTranslateY() != matrix.y0 ||
		m.getScaleX() != scale.x ||
		m.getScaleY() != scale.y ||
		m.getRotation() != rotation
	);

	if (!isDifferent)
		return;

	matrix = m;
	extractValuesFromMatrix();
	geometryChanged();
	afterSetLegacyMatrix();
	markAsChanged();
}

void DisplayObject::setFilters(const FILTERLIST& filterlist)
{
	auto span = Span(filterlist.Filters, filterlist.NumberOfFilters);
	filterListHasChanged = hasSameFilters(span);
	filters = { span.begin(), span.end() };

	hasChanged = true;
	setNeedsTextureRecalculation();
	requestInvalidation(sys);
	updateFilterBorder();
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
	if (mask != nullptr)
		state->mask = mask->getCachedSurface();
	else
		state->mask.reset();

	state->clipdepth = getClipDepth();
	state->depth = getDepth();
	state->isMask = maskCount || getClipDepth();
	state->visible = visible;

	state->alpha = clippedAlpha();
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

	state->bounds = boundsRectWithoutChildren(false).valueOr(state->bounds);
	state->scrollRect = scrollRect;

	if (is<RootMovieClip>())
	{
		state->renderWithNanoVG = true;
		state->opaqueBackground = as<RootMovieClip>()->getBackground();
		state->bounds = boundsRect(false).valueOr(state->bounds);
	}
	else
		state->opaqueBackground = opaqueBackground();
	currentrendermatrix = state->matrix;
}

void DisplayObject::setMask(DisplayObject* m)
{
	bool mustInvalidate = mask != m ||
	(
		m != nullptr &&
		m->hasChanged
	);

	if (mask != nullptr)
	{
		//Remove previous mask
		assert(mask->maskCount--);
		mask = nullptr;
	}

	mask = m;
	if (mask != nullptr)
	{
		//Use new mask
		mask->ismaskCount++;
	}

	if (mustInvalidate)
		markAsChanged();
}

void DisplayObject::setClipMask(DisplayObject* m)
{
	clipMask = m;
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

MATRIX DisplayObject::getConcatenatedMatrix(bool includeRoot, bool fromcurrentrendering) const
{
	auto _getMatrix = [&]
	{
		return
		(
			!fromcurrentrendering ?
			getMatrix() :
			currentrendermatrix
		);
	};

	if (parent == nullptr || (!includeRoot && parent == sys->mainClip))
		return _getMatrix();
	return parent->getConcatenatedMatrix
	(
		includeRoot,
		fromcurrentrendering
	).multiplyMatrix(_getMatrix());
}

/* Return alpha value between 0 and 1. (The stored alpha value is not
 * necessary bounded.) */
float DisplayObject::clippedAlpha() const
{
	auto a = this->colorTransform.alphaMultiplier;
	if (colorTransform.alphaOffset != 0)
		a += this->colorTransform.alphaOffset / 256.0;
	return dclamp(a, 0, 1);
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

void DisplayObject::afterLegacyInsert()
{
}

MATRIX DisplayObject::getMatrix(bool includeRotation) const
{
	Locker locker(spinlock);
	//Start from the residual matrix and construct the whole one
	MATRIX ret = matrix;
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
	scale = Vector2f
	(
		matrix.getScaleX(),
		matrix.getScaleY()
	);
	rotation = matrix.getRotation();
	//Deapply translation
	matrix.translate(-matrix.x0, matrix.y0);
	//Deapply rotation
	matrix.rotate(-rotation * M_PI / 180.0);
	//Deapply scaling
	matrix.scale(1 / scale.x, 1 / scale.y);
}

bool DisplayObject::skipRender() const
{
	return !isMask() && !ClipDepth && (visible==false || clippedAlpha()==0.0);
}

bool DisplayObject::defaultRender(RenderContext& ctxt)
{
	const Transform2D& t = ctxt.transformStack().transform();
	const CachedSurface* surface=ctxt.getCachedSurface(this);
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
	m.x0 /= TWIPS_FACTOR;
	m.y0 /= TWIPS_FACTOR;
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

RectF DisplayObject::computeBoundsForTransformedRect
(
	const RectF& rect,
	const MATRIX& m
) const
{
	//As the transformation is arbitrary we have to check all the four vertices
	std::array<Vector2f, 4> coords =
	{
		m.multiply2D(rect.tl()),
		m.multiply2D(rect.bl()),
		m.multiply2D(rect.br()),
		m.multiply2D(rect.tr()),
	}
	//Now find out the minimum and maximum that represent the complete bounding rect
	RectF ret { coords[3], coords[3] };
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
	if (mask != nullptr)
		mask->invalidateForRenderToBitmap(container, smoothing);
	IDrawable* d = this->invalidate(smoothing);
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
	bool needsInvalidation = mask != nullptr && mask != this &&
	(
		mask->hasChanged ||
		forceTextureRefresh
	);
	if (needsInvalidation)
		mask->requestInvalidation(q, forceTextureRefresh);
}

void DisplayObject::updateCachedSurface(IDrawable* d)
{
	// this is called only from rendering thread, so no locking done here
	cachedSurface->SetState(d->getState());
	cachedSurface->isValid=true;
	cachedSurface->isInitialized=true;
	cachedSurface->wasUpdated=true;
}

//TODO: Fix precision issues, Adobe seems to do the matrix mult with twips and rounds the results,
//this way they have less pb with precision.
Vector2f DisplayObject::localToGlobal(const Vector2f& point, bool fromcurrentrendering) const
{
	return getConcatenatedMatrix
	(
		true,
		fromcurrentrendering
	).multiply2D(point);
}

//TODO: Fix precision issues
Vector2f DisplayObject::globalToLocal(const Vector2f& point, bool fromcurrentrendering) const
{
	return getConcatenatedMatrix
	(
		true,
		fromcurrentrendering
	).getInverted().multiply2D(point);
}

void DisplayObject::setOnStage(bool staged, bool force,bool inskipping)
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
			requestInvalidation(getSystemState());
		}
		if(getVm(getSystemState())==nullptr)
			return;
		changed = true;
		this->requestInvalidationFilterParent();
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
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"addedToStage"));
			// root clips are added to stage after the builtin MovieClip is constructed, but before the constructor call is completed.
			// So the EventListeners for "addedToStage" may not be registered yet and we can't execute the event directly
			if (!this->is<RootMovieClip>())
			{
				if(isVmThread())
					ABCVm::publicHandleEvent(this,e);
				else
				{
					this->incRef();
					getVm(getSystemState())->addEvent(_MR(this),e);
				}
			}
		}
		else if(onStage==false)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"removedFromStage"));
			if(isVmThread())
				ABCVm::publicHandleEvent(this,e);
			else
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),e);
			}
			if (this->is<InteractiveObject>() && getSystemState()->getEngineData())
				getSystemState()->getEngineData()->InteractiveObjectRemovedFromStage();
			getSystemState()->stage->AVM1RemoveDisplayObject(this);
		}
	}
}

bool DisplayObject::isVisible() const
{
	return visible && parent != nullptr && parent->isVisible();
}

void DisplayObject::setScaleX(number_t val)
{
	if (std::isnan(val) || scale.x == val)
		return;

	//Apply the difference
	scale.x = val;
	hasChanged = true;
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setScaleY(number_t val)
{
	if (std::isnan(val) || scale.y == val)
		return;

	//Apply the difference
	scale.y = val;
	hasChanged = true;
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setScaleZ(number_t val)
{
	if (std::isnan(val) || scaleZ == val)
		return;

	//Apply the difference
	scaleZ = val;
	hasChanged = true;
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setVisible(bool v)
{
	visible = v;
	if (onStage)
		requestInvalidation(sys);
	else
		requestInvalidationFilterParent();
}

void DisplayObject::setX(number_t val)
{
	//Stop using the legacy matrix
	if (useLegacyMatrix)
		useLegacyMatrix = false;

	if (std::isnan(val) && !needsActionScript3())
		return;
	else if (std::isnan(val))
		val = 0;

	if (matrix.x0 == val * 20)
		return;

	//Apply translation, it's trivial
	matrix.x0 = val * 20;
	hasChanged = true;
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setY(number_t val)
{
	//Stop using the legacy matrix
	if (useLegacyMatrix)
		useLegacyMatrix = false;

	if (std::isnan(val) && !needsActionScript3())
		return;
	else if (std::isnan(val))
		val = 0;

	if (matrix.y0 == val * 20)
		return;

	//Apply translation, it's trivial
	matrix.y0 = val * 20;
	hasChanged = true;
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setZ(number_t val)
{
	LOG(LOG_NOT_IMPLEMENTED,"setting DisplayObject.z has no effect");

	//Stop using the legacy matrix
	if (useLegacyMatrix)
		useLegacyMatrix = false;

	if (std::isnan(val) && !needsActionScript3())
		return;
	else if (std::isnan(val))
		val = 0;

	if (tz == val * 20)
		return;

	//Apply translation, it's trivial
	tz = val * 20;
	hasChanged = true;
	geometryChanged();
	markAsChanged();
}

void DisplayObject::setParent(DisplayObjectContainer *p, bool fordestruction)
{
	Locker locker(spinlock);
	if(parent!=p)
	{
		if (parent)
		{
			if (fordestruction)
				parent->removeChildName(this);
			parent->removeStoredMember();
		}
		if (p)
		{
			// mark old parent as dirty
			geometryChanged();
			getSystemState()->removeFromResetParentList(this);
			getSystemState()->stage->removeHiddenObject(this);
			p->incRef();
			p->addStoredMember();
		}
		parent=p;
		hasChanged=true;
		geometryChanged();
		if(onStage && (!fordestruction && !getSystemState()->isShuttingDown()))
			requestInvalidation(getSystemState());
	}
}

void DisplayObject::setScalingGrid()
{
	RECT* r = loadedFrom->ScalingGridsLookup(this->getTagID());
	if (r)
	{
		this->scalingGrid = _MR(Class<Rectangle>::getInstanceS(getInstanceWorker()));
		this->scalingGrid->x=-r->Xmin/TWIPS_FACTOR;
		this->scalingGrid->y=-r->Ymin/TWIPS_FACTOR;
		this->scalingGrid->width=(r->Xmax+r->Xmin)/TWIPS_FACTOR;
		this->scalingGrid->height=(r->Ymax+r->Ymin)/TWIPS_FACTOR;
	}
}

number_t DisplayObject::computeHeight()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix(false));

	return (ret)?(y2-y1):0;
}

void DisplayObject::geometryChanged()
{
	if (this->is<DisplayObjectContainer>())
	{
		this->as<DisplayObjectContainer>()->markBoundsRectDirtyChildren();
	}
	DisplayObjectContainer* p = this->getParent();
	while (p)
	{
		p->markBoundsRectDirty();
		p=p->getParent();
	}
}

number_t DisplayObject::computeWidth()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix(false));

	return (ret)?(x2-x1):0;
}

int DisplayObject::getRawDepth()
{
	return (parent != nullptr) ? parent->findLegacyChildDepth(this) : 0;
}

int DisplayObject::getDepth()
{
	return getRawDepth() + 16384;
}

int DisplayObject::getClipDepth() const
{
	return ClipDepth ? ClipDepth + 16384 : 0;
}

RootMovieClip* DisplayObject::getRoot()
{
	if(!parent)
		return nullptr;

	return parent->getRoot();
}

_NR<Stage> DisplayObject::getStage()
{
	if(!parent)
		return NullRef;

	return parent->getStage();
}

Vector2f DisplayObject::getLocalMousePos()
{
	return getConcatenatedMatrix().getInverted().multiply2D
	(
		sys->getInputThread()->getMousePos() *
		TWIPS_FACTOR
	) / TWIPS_FACTOR;
}

bool DisplayObject::hitTestMask(const Vector2f& globalPoint, HIT_TYPE type)
{
	const auto* mask = mask != nullptr ? mask : clipMask;
	if (mask == nullptr)
		return true;
	//Compute the coordinates local to the mask
	const MATRIX& maskMatrix = mask->getConcatenatedMatrix(false, false);
	if (!maskMatrix.isInvertible())
	{
		//If the matrix is not invertible the mask as collapsed to zero size
		//If the mask is zero sized then the object is not visible
		return false;
	}

	const auto maskPoint = maskMatrix.getInverted().multiply2D(globalPoint);
	return mask->hitTest
	(
		globalPoint,
		maskPoint,
		GENERIC_HIT_INVISIBLE,
		false
	) != nullptr;
}

DisplayObject* DisplayObject::hitTest
(
	const Vector2f& globalPoint,
	const Vector2f& localPoint,
	HIT_TYPE type,
	bool interactiveObjectsOnly
)
{
	bool canTest =
	(
		visible ||
		type == GENERIC_HIT_INVISIBLE
	) && isMask() && getClipDepth();
	if (canTest)
		return nullptr;

	//First check if there is any mask on this object, if so the point must be inside the mask to go on
	switch (type)
	{
		case GENERIC_HIT_EXCLUDE_CHILDREN:
			break;
		case GENERIC_HIT_INVISIBLE:
			if (mask != nullptr && !hitTestMask(globalPoint, type))
				return nullptr;
			break;
		default:
			if (!hitTestMask(globalPoint, type))
				return nullptr;
			break;
	}
	return hitTestImpl
	(
		globalPoint,
		localPoint,
		type,
		interactiveObjectsOnly
	);
}

/* Display objects have no children in general,
 * so we skip to calling the constructor, if necessary.
 * This is called in vm's thread context */
void DisplayObject::initFrame()
{
	if(!inInitFrame && !isConstructed() && getClass() && needsActionScript3())
	{
		inInitFrame=true;
		handleConstruction();
		if (this->is<RootMovieClip>())
		{
			// events are handled in RootMovieClip::afterConstruction
			inInitFrame=false;
			return;
		}
		/*
		 * Legacy objects have their display list properties set on creation, but
		 * the related events must only be sent after the constructor is sent.
		 * This is from "Order of Operations".
		 */
		if(parent)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"added",true));
			ABCVm::publicHandleEvent(this,e);
		}
		if(onStage)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"addedToStage"));
			ABCVm::publicHandleEvent(this,e);
		}
		inInitFrame=false;
	}
}

void DisplayObject::executeFrameScript()
{
}

bool DisplayObject::needsActionScript3() const
{
	return !this->loadedFrom || this->loadedFrom->usesActionScript3;
}

void DisplayObject::constructionComplete(bool _explicit, bool forInitAction)
{
	RELEASE_WRITE(constructed,true);
	if (getParent())
		setNameOnParent();
	if (!placedByActionScript && needsActionScript3() && getParent() != nullptr)
	{
		_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"added",true));
		if (isVmThread())
			ABCVm::publicHandleEvent(this, e);
		else
		{
			incRef();
			getVm(getSystemState())->addEvent(_MR(this), e);
		}
		if (isOnStage())
			setOnStage(true, true);
	}
}
void DisplayObject::setNameOnParent()
{
	if( this->name != BUILTIN_STRINGS::EMPTY
		&& !this->hasDefaultName
		&& this->name != UINT32_MAX
	)
	{
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=this->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		asAtom obj = asAtomHandler::invalidAtom;
		getParent()->getVariableByMultiname(obj,objName,GET_VARIABLE_OPTION::DONT_CALL_GETTER,getInstanceWorker());
		if (asAtomHandler::getObject(obj) != this
				&& (!asAtomHandler::is<DisplayObject>(obj)
				|| !asAtomHandler::as<DisplayObject>(obj)->getParent()
				|| asAtomHandler::as<DisplayObject>(obj)->getDepth() >= this->getDepth()))
		{
			if (asAtomHandler::isValid(obj))
			{
				ASATOM_DECREF(obj);
				getParent()->deleteVariableByMultiname(objName,loadedFrom->getInstanceWorker());
			}
			this->incRef();
			asAtom v = asAtomHandler::fromObject(this);
			getParent()->setVariableByMultiname(objName,v,CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
		}
		else
			ASATOM_DECREF(obj);
	}
}
void DisplayObject::beforeConstruction(bool _explicit)
{
	skipFrame |= needsActionScript3() && _explicit;
	placedByActionScript |= _explicit;
	if (needsActionScript3() && getParent() == nullptr && this != getSystemState()->mainClip)
		getSystemState()->stage->addHiddenObject(this);
}
void DisplayObject::afterConstruction(bool _explicit)
{
//	hasChanged=true;
//	needsTextureRecalculation=true;
//	if(onStage)
//		requestInvalidation(getSystemState());
}

void DisplayObject::applyFilters(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley)
{
	if (filters)
	{
		for (uint32_t i = 0; i < filters->size(); i++)
		{
			asAtom f = asAtomHandler::invalidAtom;
			filters->at_nocheck(f,i);
			if (asAtomHandler::is<BitmapFilter>(f))
				asAtomHandler::as<BitmapFilter>(f)->applyFilter(target, source, sourceRect, xpos, ypos, scalex, scaley, this);
		}
	}
}

void DisplayObject::setNeedsTextureRecalculation(bool skippable)
{
	textureRecalculationSkippable=skippable;
	needsTextureRecalculation=true;
	if (!cachedSurface->isChunkOwner)
		cachedSurface->tex=nullptr;
	cachedSurface->isChunkOwner=true;
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
	number_t bxmin,bxmax,bymin,bymax;
	if(!boundsRect(bxmin,bxmax,bymin,bymax,false))
	{
		//No contents, nothing to do
		return nullptr;
	}
	MATRIX matrix = getMatrix();

	bool isMask=false;
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);

	if (isnan(width) || isnan(height))
	{
		// on stage with invalid concatenatedMatrix. Create a trash initial texture
		width = 1;
		height = 1;
	}
	if (width >= 8192 || height >= 8192 || (width * height) >= 16777216)
		return nullptr;

	if(width==0 || height==0)
		return nullptr;
	ColorTransformBase ct = this->colorTransform;
	this->resetNeedsTextureRecalculation();
	return new RefreshableDrawable(x, y, ceil(width), ceil(height)
				, matrix.getScaleX(), matrix.getScaleY()
				, isMask, cacheAsBitmap
				, this->getScaleFactor(), getConcatenatedAlpha()
				, ct, smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS:SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
}

bool DisplayObject::findParent(DisplayObject *d) const
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
	DisplayObjectContainer* p;
	for (i = 0, p = parent; p != nullptr; p = p->parent, ++i);
	return i;
}

int DisplayObject::findParentDepth(DisplayObject* d) const
{
	if (this != d)
	{
		int i;
		DisplayObjectContainer* p;
		for (i = 0, p = parent; p != nullptr && p != d; p = p->parent, ++i);
		return i;
	}
	return -1;
}

DisplayObjectContainer* DisplayObject::getAncestor(int depth) const
{
	if (depth < 0)
		return (DisplayObjectContainer*)this;
	if (!depth)
		return parent;
	if (parent == nullptr)
		return nullptr;
	return parent->getAncestor(--depth);
}

DisplayObjectContainer* DisplayObject::findCommonAncestor(DisplayObject* d, int& depth, bool init) const
{
	const DisplayObject* a = this;
	const DisplayObject* b = d;
	if (init)
	{
		depth = 0;
		if ((a->getParentDepth() - b->getParentDepth()) < 0)
			std::swap(a, b);
	}
	if (a->parent == nullptr || b == nullptr)
		return a->parent == nullptr ? (DisplayObjectContainer*)a : nullptr;
	if (b->findParent(a->parent))
		return a->parent;
	return a->parent->findCommonAncestor((DisplayObject*)b, ++depth, false);
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
	return parent->AVM1getStage();
}

DisplayObject* DisplayObject::getAVM1Root()
{
	if (this->needsActionScript3())
		return getSystemState()->mainClip;
	if (parent && parent->needsActionScript3())
		return this;
	if (this->is<RootMovieClip>())
		return this;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=BUILTIN_STRINGS::STRING_LOCKROOT;
	m.isAttribute = false;
	asAtom l= asAtomHandler::undefinedAtom;
	getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
	bool lockroot = asAtomHandler::AVM1toBool(l,getInstanceWorker(),loadedFrom->version);
	if (!lockroot && parent)
		return parent->AVM1getRoot();
	return getSystemState()->mainClip;
}
