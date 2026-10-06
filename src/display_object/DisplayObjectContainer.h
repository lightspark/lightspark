/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2023-2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_DISPLAYOBJECTCONTAINER_H
#define DISPLAY_OBJECT_DISPLAYOBJECTCONTAINER_H 1

#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "backends/geometry.h"
#include "swftypes.h"

// Based on Ruffle's `display_object::{TDisplayObject,Child}Container`.

namespace lightspark
{

class tiny_string;
class DisplayObject;
class MovieClip;
class Filter;
class InvalidateQueue;
template<typename T>
class Optional;
class RenderContext;
class SWFMovie;
class Stage;
enum VMCreator;

class DisplayObjectContainer
{
	template<typename T>
	using RefWrapper = std::reference_wrapper<T>;
	using DisplayObjectRef = RefWrapper<DisplayObject>;
private:
	bool mouseChildren;
	bool _isAS3;

	std::map<int32_t, DisplayObjectRef> mapDepthToChild;
	std::set<int32_t> childrenMarkedForDeletion;
	std::map<int32_t, DisplayObjectRef> mapFrameDepthToChildRemembered;
	bool contains(const DisplayObject& child) const;
	//void getObjectsFromPoint(const Vector2f& point, Array* ar);
	RectF boundsRectVisible;
	bool boundsRectVisibleDirty;
	void unmarkLegacyChild(DisplayObject& child);
	void setChildIndexImpl(DisplayObject& child, size_t index);
protected:
	std::vector<DisplayObjectRef> dynamicDisplayList;
	bool initializingFrame;
	void clearDisplayList();
	//The lock should only be taken when doing write operations
	//As the RenderThread only reads, it's safe to read without the lock
	mutable Mutex mutexDisplayList;
	virtual void resetToStart() {}
	bool tabChildren;
	void removeChildrenMarkedForDeletion();
	void stopAllMovieClipsImpl();
public:
	DisplayObjectContainer(const SWFMovie& movie);

	void constructionComplete(bool _explicit = false, bool forInitAction = false);
	DisplayObject* getLastFrameChildAtDepth(int32_t depth, uint32_t id);
	bool removeChildDeletionMark(int32_t depth);
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh = false);
	void requestInvalidationIncludingChildren(InvalidateQueue* q);
	IDrawable* invalidate(bool smoothing);
	void invalidateForRenderToBitmap(BitmapContainerRenderData* container, bool smoothing);

	void addChildAt(DisplayObject& child, size_t index, bool inskipping = false);
	bool fillTabStopsAutomatic
	(
		std::map<int32_t, DisplayObject&>& distanceMap,
		bool& hasTabIndices
	);

	void fillTabStopsByTabIndex(std::map<int32_t, DisplayObject&>& indexMap);
	void dumpDisplayList(size_t level = 0);
	static void handleAddedEvent
	(
		const DisplayObject& parent,
		DisplayObject& child,
		bool wasOnStage
	);

	static void handleAddedEventOnly(DisplayObject& child);
	static void handleAddedToStageEvent(DisplayObject& child);
	static void handleAddedToStageEventOnly(DisplayObject& child);

	void handleRemovedEvent(DisplayObject& child, bool keepOnStage);
	void handleRemovedFromStageEvent(DisplayObject& child);

	bool removeChild
	(
		DisplayObject& child,
		bool direct = false,
		bool inskipping = false,
		bool keepOnStage = false,
		bool removeName = true
	);

	void removeFromDisplayList(DisplayObject& child);
	void removeAllChildren(bool recursive);
	void removeAVM1Listeners();
	size_t getChildIndex(DisplayObject& child);
	void markAsChanged();
	void markBoundsRectDirty() { boundsRectVisibleDirty = true; }
	void markBoundsRectDirtyChildren();
	void cloneDisplayList(std::vector<DisplayObject&>& displayListCopy) const
	{
		displayListCopy = cloneDisplayList();
	}

	std::vector<DisplayObject&> cloneDisplayList() const;
	bool isEmpty() const { return dynamicDisplayList.empty(); }
	bool hasChildAt(int32_t depth) const;
	// this does not test if a DisplayObject exists at the provided depth
	const DisplayObject& getChildAt(int32_t depth) const;
	DisplayObject& getChildAt(int32_t depth);
	DisplayObject* tryGetChildAt(int32_t depth);
	bool hasChildByName(const tiny_string& name, bool caseSensitive) const;
	DisplayObject* getChildByName(const tiny_string& name, bool caseSensitive);
	void setupClipActionsAt(int32_t depth, const CLIPACTIONS& actions);
	void checkRatioForChildAt(int32_t depth, uint32_t ratio, bool inskipping);
	void checkColorTransformForChildAt(int32_t depth, const CXFORMWITHALPHA& ct);
	void removeChildName(DisplayObject& child);
	void deleteChildAt(int32_t depth, bool inskipping);
	void insertChildAt
	(
		int32_t depth,
		DisplayObject& obj,
		bool inskipping = false,
		bool fromTag = true
	);

	DisplayObject* findChildByTagID(uint32_t id);
	void transformChildAt(int32_t depth, const MATRIX& mat);
	uint32_t getMaxChildDepth();
	void checkClipDepth();
	void enterFrame(bool implicit);
	void advanceFrame(bool implicit);
	void declareFrame(bool implicit);
	void initFrame();
	void executeFrameScript();
	void afterTimelineCreation();
	void afterTimelineDeletion(bool inskipping);

	std::vector<DisplayObject&> getObjectsFromPoint
	(
		const Vector2Twips& point
	);

	void getObjectsFromPoint
	(
		const Vector2Twips& point,
		std::vector<DisplayObject&>& list
	);
};

}
#endif /* DISPLAY_OBJECT_DISPLAYOBJECTCONTAINER_H */
