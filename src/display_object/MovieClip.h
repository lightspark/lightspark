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

#ifndef DISPLAY_OBJECT_MOVIECLIP_H
#define DISPLAY_OBJECT_MOVIECLIP_H 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "backends/colortransformbase.h"
#include "backends/geometry.h"
#include "display_object/DisplayObject.h"
#include "display_object/DisplayObjectContainer.h"
#include "display_object/InteractiveObject.h"
#include "gc/ptr.h"
#include "exceptions.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/span.h"

// Based on Ruffle's `avm1::display_object::MovieClip`.

namespace lightspark
{

union asAtom;
class ASObject;
class AVM1Object;
class AVM1Value;
class CachedSurface;
class DictionaryTag;
class Filter;
class FrameContainer;
class IFunction;
class InvalidateQueue;
class LoaderInfo;
class Matrix3D;
template<typename T>
class Optional;
class RenderContext;
class SWFMovie;
class Stage;

class MovieClip :
public InteractiveObject,
public DisplayObjectContainer,
public TokenContainer
{
	friend class Stage;
	friend class FrameContainer;
	enum class ClipFlags : uint8_t
	{
		Initialized = 1 << 0,
		ExecutingFrameScript = 1 << 1,
		InAVM1Attachment = 1 << 2,
		ForAVM1InitAction = 1 << 3,
		RunningInitFrame = 1 << 4,
		HasAVM1LoadEvent = 1 << 5,
		PostTimelineCreated = 1 << 6,
	};
private:
	std::map<uint32_t, _R<IFunction>> frameScripts;
	std::vector<DisplayObject&> removedFrameScripts;
	std::vector<AVM1VarBinding> avm1VarBindings;
	DictionaryTag* tag;

	uint64_t tagStreamPos;

	DisplayObject* dropTarget;
	DisplayObject* hitArea;
	Graphics* graphics;

	uint32_t lastFrameScriptExecuted;
	uint32_t lastRatio;
	bool inExecuteFramescript;
	bool inAVM1Attachment;
	bool isAVM1Loaded;
	bool forAVM1InitAction;
	bool hasAVM1LoadEvent;

	bool buttonMode;
	bool avm2Enabled;
	bool avm2UseHandCursor;

	void runGoto(bool newFrame);
protected:
	FrameContainer* frameContainer;
	const CLIPACTIONS* actions;
public:
	void constructionComplete(bool _explicit = false, bool forInitAction = false) override;
	void beforeConstruction(bool _explicit = false) override;
	void afterConstruction(bool _explicit = false) override;
	RunState state;
	list<AVM1MovieClipLoader*> avm1loaderlist;
	uint32_t getFrameCount();
	FrameContainer* getFrameContainer() const { return frameContainer; }
	MovieClip(ASWorker* wrk,Class_base* c);
	MovieClip(ASWorker* wrk,Class_base* c, FrameContainer* f, uint32_t defineSpriteTagID);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void setPlaying();
	void setStopped();
	void gotoAnd(asAtom *args, const unsigned int argslen, bool stop);
	static void sinit(Class_base* c);
	/*
	 * returns true if all frames of this MovieClip are loaded
	 * this is overwritten in RootMovieClip, because that one is
	 * executed while loading
	 */
	virtual bool hasFinishedLoading() { return true; }

	void AVM1SetForInitAction() { forAVM1InitAction = true; }
	bool AVM1GetForInitAction() const { return forAVM1InitAction; }
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(swapDepths);
	ASFUNCTION_ATOM(addFrameScript);
	ASFUNCTION_ATOM(stop);
	ASFUNCTION_ATOM(play);
	ASFUNCTION_ATOM(gotoAndStop);
	ASFUNCTION_ATOM(gotoAndPlay);
	ASFUNCTION_ATOM(prevFrame);
	ASFUNCTION_ATOM(nextFrame);
	ASFUNCTION_ATOM(prevScene);
	ASFUNCTION_ATOM(nextScene);
	ASFUNCTION_ATOM(_getCurrentFrame);
	ASFUNCTION_ATOM(_getCurrentFrameLabel);
	ASFUNCTION_ATOM(_getCurrentLabel);
	ASFUNCTION_ATOM(_getCurrentLabels);
	ASFUNCTION_ATOM(_getTotalFrames);
	ASFUNCTION_ATOM(_getFramesLoaded);
	ASFUNCTION_ATOM(_getScenes);
	ASFUNCTION_ATOM(_getCurrentScene);
	ASFUNCTION_ATOM(_getIsPlaying);

	void enterFrame(bool implicit) override;
	void advanceFrame(bool implicit) override;
	void declareFrame(bool implicit) override;
	void initFrame() override;
	void executeFrameScript() override;
	void checkRatio(uint32_t ratio, bool inskipping) override;

	void afterLegacyInsert() override;
	void afterLegacyDelete(bool inskipping) override;

	uint32_t getTagID() const override { return fromDefineSpriteTag; }
	void setupActions(const CLIPACTIONS& clipactions);
	void updateVariableBindings();

	bool AVM1HandleKeyboardEvent(KeyboardEvent *e) override;
	bool AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e) override;
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;
	void AVM1AfterAdvance() override;

	void AVM1gotoFrameLabel(const tiny_string &label, bool stop, bool switchplaystate);
	void AVM1gotoFrame(int frame, bool stop, bool switchplaystate, bool advanceFrame=true);
	static void AVM1SetupMethods(Class_base* c);
	void AVM1ExecuteFrameActionsFromLabelDirect(const tiny_string &label);
	void AVM1ExecuteFrameActionsDirect(uint32_t frame);
	void AVM1AddScriptEvents();
	void AVM1HandleConstruction(bool forInitAction);
	bool getAVM1Loaded() const { return isAVM1Loaded; }
	inline AVM1context* getAVM1Context() { return &avm1context; }
	AVM1context* AVM1getCurrentFrameContext();
	MovieClip* AVM1CloneSprite(uint32_t targetNameID, int32_t Depth, ASObject* initobj);

	ASFUNCTION_ATOM(AVM1AttachMovie);
	ASFUNCTION_ATOM(AVM1CreateEmptyMovieClip);
	ASFUNCTION_ATOM(AVM1RemoveMovieClip);
	ASFUNCTION_ATOM(AVM1DuplicateMovieClip);
	ASFUNCTION_ATOM(AVM1Clear);
	ASFUNCTION_ATOM(AVM1MoveTo);
	ASFUNCTION_ATOM(AVM1LineTo);
	ASFUNCTION_ATOM(AVM1CurveTo);
	ASFUNCTION_ATOM(AVM1LineStyle);
	ASFUNCTION_ATOM(AVM1BeginFill);
	ASFUNCTION_ATOM(AVM1BeginGradientFill);
	ASFUNCTION_ATOM(AVM1EndFill);
	ASFUNCTION_ATOM(AVM1GetNextHighestDepth);
	ASFUNCTION_ATOM(AVM1AttachBitmap);
	ASFUNCTION_ATOM(AVM1getInstanceAtDepth);
	ASFUNCTION_ATOM(AVM1getSWFVersion);
	ASFUNCTION_ATOM(AVM1LoadMovie);
	ASFUNCTION_ATOM(AVM1UnloadMovie);
	ASFUNCTION_ATOM(AVM1LoadMovieNum);
	ASFUNCTION_ATOM(AVM1CreateTextField);

	std::string toDebugString() const override;
};

}
#endif /* DISPLAY_OBJECT_MOVIECLIP_H */
