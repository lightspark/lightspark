/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_CACHEDSURFACE_H
#define BACKENDS_CACHEDSURFACE_H 1

#include "forwards/scripting/flash/display/DisplayObject.h"
#include "compat.h"
#include <vector>
#include "smartrefs.h"
#include "swftypes.h"
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "memory_support.h"

#define FILTERDATA_MAXSIZE 256

namespace lightspark
{
class RenderContext;
class Array;

struct FilterData
{
	float gradientcolors[256*4];
	float filterdata[FILTERDATA_MAXSIZE];
};

class SurfaceState
{
public:
	SurfaceState(float _xoffset=0.0,float _yoffset=0.0, float _alpha=1.0, float _xscale=1.0, float _yscale=1.0,
				 const ColorTransformBase& _colortransform=ColorTransformBase(), const MATRIX& _matrix=MATRIX(), bool _ismask=false, bool _cacheAsBitmap=false,
				 AS_BLENDMODE _blendmode=BLENDMODE_NORMAL, SMOOTH_MODE _smoothing=SMOOTH_MODE::SMOOTH_ANTIALIAS, float _scaling=TWIPS_SCALING_FACTOR, bool _needsfilterrefresh=true, bool _needslayer=false);
	virtual ~SurfaceState();
	void reset();
	void setupChildrenList(std::vector < DisplayObject* >& dynamicDisplayList);
	float xOffset;
	float yOffset;
	float alpha;
	float xscale;
	float yscale;
	ColorTransformBase colortransform;
	MATRIX matrix;
	tokensVector tokens;
	std::vector<_R<CachedSurface>> childrenlist;
	_NR<CachedSurface> mask;
	_NR<CachedSurface> maskee;
	std::vector<FilterData> filters;
	RectF bounds;
	int depth;
	int clipdepth;
	number_t maxfilterborder;
	AS_BLENDMODE blendmode;
	SMOOTH_MODE smoothing;
	float scaling;
	RECT scrollRect;
	RECT scalingGrid;
	bool visible;
	bool allowAsMask;
	bool isMask;
	bool cacheAsBitmap;
	bool needsFilterRefresh;
	bool needsLayer;
	bool isYUV;
	bool renderWithNanoVG;
	bool hasOpaqueBackground;
	RGB opaqueBackground;
#ifndef _NDEBUG
	DisplayObject* src; // this points to the DisplayObject this surface belongs to
#endif
};

class CachedSurface: public RefCountable
{
private:
	SurfaceState* state;
	void renderImpl(SystemState* sys,RenderContext& ctxt);
	void defaultRender(RenderContext& ctxt);
public:
	CachedSurface():state(nullptr),tex(nullptr),isChunkOwner(true),isValid(false),isInitialized(false),wasUpdated(false),cachedFilterTextureID(UINT32_MAX)
	{
	}
	~CachedSurface();
	void SetState(SurfaceState* newstate)
	{
		if (state && state != newstate)
			delete state;
		state = newstate;
	}
	SurfaceState* getState() const
	{
		return state;
	}
	void Render(SystemState* sys, RenderContext& ctxt, const MATRIX* startmatrix=nullptr, RenderDisplayObjectToBitmapContainer* container=nullptr);
	RectF boundsRectWithRenderTransform(const MATRIX& matrix, const MATRIX& initialMatrix);
	void renderFilters(SystemState* sys, RenderContext& ctxt, uint32_t w, uint32_t h, const MATRIX& m);
	TextureChunk* tex;
	bool isChunkOwner;
	bool isValid;
	bool isInitialized;
	bool wasUpdated;
	uint32_t cachedFilterTextureID;
};

}
#endif /* BACKENDS_CACHEDSURFACE_H */
