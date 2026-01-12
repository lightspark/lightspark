/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DISPLAY_MOVIECLIP_H
#define SCRIPTING_FLASH_DISPLAY_MOVIECLIP_H 1

#include "scripting/flash/display/flashdisplay.h"
#include "scripting/avm1/scope.h"

namespace lightspark
{
class FrameContainer;

class MovieClip: public Sprite
{
friend class Stage;
friend class FrameContainer;
private:
	std::map<uint32_t,asAtom > frameScripts;
	std::vector<_R<DisplayObject>> removedFrameScripts;
	uint32_t fromDefineSpriteTag;
	uint32_t lastFrameScriptExecuted;
	uint32_t lastratio;
	bool inExecuteFramescript;
	bool inAVM1Attachment;
	bool isAVM1Loaded;
	bool forAVM1InitAction;
	bool hasAVM1LoadEvent;
	void runGoto(bool newFrame);
	AVM1context avm1context;
	std::map<uint32_t,AVM1context> AVM1FrameContexts;
protected:
	FrameContainer* framecontainer;
	const CLIPACTIONS* actions;
	uint32_t avm1clipeventlistenercount;
	/* This is read from the SWF header. It's only purpose is for flash.display.MovieClip.totalFrames */
	uint32_t totalFrames_unreliable;
	ASPROPERTY_GETTER_SETTER(bool, enabled);
public:
	void constructionComplete(bool _explicit = false, bool forInitAction = false) override;
	void beforeConstruction(bool _explicit = false) override;
	void afterConstruction(bool _explicit = false) override;
	RunState state;
	list<AVM1MovieClipLoader*> avm1loaderlist;
	uint32_t getFrameCount();
	FrameContainer* getFrameContainer() const { return framecontainer; }
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
	void AVM1ExecuteFrameActionsFromLabel(const tiny_string &label);
	void AVM1ExecuteFrameActions(uint32_t frame);
	void AVM1AddScriptEvents();
	void AVM1HandleConstruction(bool forInitAction);
	bool getAVM1Loaded() const { return isAVM1Loaded; }
	inline AVM1context* getAVM1Context() { return &avm1context; }
	AVM1context* AVM1getCurrentFrameContext();
	MovieClip* AVM1CloneSprite(asAtom target, uint32_t Depth, ASObject* initobj);

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

#endif /* SCRIPTING_FLASH_DISPLAY_MOVIECLIP_H */
