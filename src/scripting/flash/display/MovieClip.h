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

namespace lightspark
{

struct FrameLabel_data
{
	FrameLabel_data() : frame(0) {}
	FrameLabel_data(uint32_t _frame, tiny_string _name) : name(_name),frame(_frame){}
	tiny_string name;
	uint32_t frame;
};

class FrameLabel: public ASObject, public FrameLabel_data
{
public:
	FrameLabel(ASWorker* wrk,Class_base* c);
	FrameLabel(ASWorker* wrk,Class_base* c, const FrameLabel_data& data);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_getFrame);
	ASFUNCTION_ATOM(_getName);
};

struct Scene_data
{
	Scene_data() : startframe(0) {}
	//this vector is sorted with respect to frame
	std::vector<FrameLabel_data> labels;
	tiny_string name;
	uint32_t startframe;
	void addFrameLabel(uint32_t frame, const tiny_string& label);
};

class Scene: public ASObject, public Scene_data
{
	uint32_t numFrames;
public:
	Scene(ASWorker* wrk,Class_base* c);
	Scene(ASWorker* wrk,Class_base* c, const Scene_data& data, uint32_t _numFrames);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getLabels);
	ASFUNCTION_ATOM(_getName);
	ASFUNCTION_ATOM(_getNumFrames);
};

class Frame
{
friend class FrameContainer;
private:
	AVM1context avm1context;
public:
	inline AVM1context* getAVM1Context() { return &avm1context; }
	std::list<DisplayListTag*> blueprint;
	void execute(DisplayObjectContainer* displayList, bool inskipping, std::vector<_R<DisplayObject>>& removedFrameScripts);
	void AVM1executeActions(MovieClip* clip);
	/**
	 * destroyTags must be called only by the tag destructor, not by
	 * the objects that are instance of tags
	 */
	void destroyTags();
};

class FrameContainer
{
protected:
	/* This list is accessed by both the vm thread and the parsing thread,
	 * but the parsing thread only accesses frames.back(), while
	 * the vm thread only accesses the frames before that frame (until
	 * the parsing finished; then it can also access the last frame).
	 * To make that easier for the vm thread, the member framesLoaded keep
	 * track of how many frames the vm may access. Access to framesLoaded
	 * is guarded by a spinlock.
	 * For non-RootMovieClips, the parser fills the frames member before
	 * handing the object to the vm, so there is no issue here.
	 * RootMovieClips use the new_frame semaphore to wait
	 * for a finished frame from the parser.
	 * It cannot be implemented as std::vector, because then reallocation
	 * would break concurrent access.
	 */
	std::list<Frame> frames;
	std::vector<Scene_data> scenes;
	void addToFrame(DisplayListTag *r);
	void setFramesLoaded(uint32_t fl) { framesLoaded = fl; }
	FrameContainer();
	FrameContainer(const FrameContainer& f);
private:
	//No need for any lock, just make sure accesses are atomic
	ATOMIC_INT32(framesLoaded);
	AVM1context avm1context;
public:
	void addFrameLabel(uint32_t frame, const tiny_string& label);
	uint32_t getFramesLoaded() { return framesLoaded; }
	void setAvm1InitAction(AVM1InitActionTag* t);
	inline AVM1context* getAVM1Context() { return &avm1context; }
};

class MovieClip: public Sprite, public FrameContainer
{
friend class Stage;
private:
	uint32_t getCurrentScene() const;
	const Scene_data *getScene(const tiny_string &sceneName) const;
	uint32_t getFrameIdByNumber(uint32_t i, const tiny_string& sceneName) const;
	std::map<uint32_t,asAtom > frameScripts;
	std::vector<_R<DisplayObject>> removedFrameScripts;
	uint32_t fromDefineSpriteTag;
	uint32_t lastFrameScriptExecuted;
	uint32_t lastratio;
	bool inExecuteFramescript;
	bool inAVM1Attachment;
	bool isAVM1Loaded;
	bool AVM1EventScriptsAdded;
	void runGoto(bool newFrame);
protected:
	const CLIPACTIONS* actions;
	/* This is read from the SWF header. It's only purpose is for flash.display.MovieClip.totalFrames */
	uint32_t totalFrames_unreliable;
	ASPROPERTY_GETTER_SETTER(bool, enabled);
public:
	uint32_t getFrameIdByLabel(const tiny_string& l, const tiny_string& sceneName) const;
	void constructionComplete(bool _explicit = false) override;
	void beforeConstruction(bool _explicit = false) override;
	void afterConstruction(bool _explicit = false) override;
	RunState state;
	_NR<AVM1MovieClipLoader> avm1loader;
	Frame* getCurrentFrame();
	MovieClip(ASWorker* wrk,Class_base* c);
	MovieClip(ASWorker* wrk,Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID);
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
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(swapDepths);
	ASFUNCTION_ATOM(addFrameScript);
	ASFUNCTION_ATOM(stop);
	ASFUNCTION_ATOM(play);
	ASFUNCTION_ATOM(gotoAndStop);
	ASFUNCTION_ATOM(gotoAndPlay);
	ASFUNCTION_ATOM(prevFrame);
	ASFUNCTION_ATOM(nextFrame);
	ASFUNCTION_ATOM(_getCurrentFrame);
	ASFUNCTION_ATOM(_getCurrentFrameLabel);
	ASFUNCTION_ATOM(_getCurrentLabel);
	ASFUNCTION_ATOM(_getCurrentLabels);
	ASFUNCTION_ATOM(_getTotalFrames);
	ASFUNCTION_ATOM(_getFramesLoaded);
	ASFUNCTION_ATOM(_getScenes);
	ASFUNCTION_ATOM(_getCurrentScene);

	void enterFrame(bool implicit) override;
	void advanceFrame(bool implicit) override;
	void declareFrame(bool implicit) override;
	void initFrame() override;
	void executeFrameScript() override;
	void checkRatio(uint32_t ratio, bool inskipping) override;

	void afterLegacyInsert() override;
	void afterLegacyDelete(bool inskipping) override;

	void addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name);
	uint32_t getTagID() const override { return fromDefineSpriteTag; }
	void setupActions(const CLIPACTIONS& clipactions);

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
	void AVM1HandleConstruction();
	bool getAVM1Loaded() const { return isAVM1Loaded; }
	MovieClip* AVM1CloneSprite(asAtom target, uint32_t Depth, ASObject* initobj);

	ASFUNCTION_ATOM(AVM1AttachMovie);
	ASFUNCTION_ATOM(AVM1CreateEmptyMovieClip);
	ASFUNCTION_ATOM(AVM1RemoveMovieClip);
	ASFUNCTION_ATOM(AVM1DuplicateMovieClip);
	ASFUNCTION_ATOM(AVM1Clear);
	ASFUNCTION_ATOM(AVM1MoveTo);
	ASFUNCTION_ATOM(AVM1LineTo);
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
	ASFUNCTION_ATOM(AVM1CreateTextField);
	
	std::string toDebugString() const override;
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_MOVIECLIP_H */
