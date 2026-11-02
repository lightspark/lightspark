/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "display_object/FrameContainer.h"
#include "display_object/Loader.h"
#include "display_object/LoaderInfo.h"
#include "display_object/RootMovieClip.h"
#include "parsing/tags.h"

using namespace lightspark;

RootMovieClip::RootMovieClip(ASWorker* wrk, LoaderInfo* li, _NR<ApplicationDomain> appDomain, _NR<SecurityDomain> secDomain, Class_base* c)
	:MovieClip(wrk,c)
	,parsingIsFailed(false)
	,waitingforparser(false)
	,firstframeavailable(false)
	,hasDefineSceneAndFrameLabelDataTag(false)
	,Background(0xFF,0xFF,0xFF)
	,avm1level(-1)
	,finishedLoading(false)
	,applicationDomain(appDomain)
	,securityDomain(secDomain)
RootMovieClip::RootMovieClip
(
	SystemState* sys,
	SWFMovie& _movie,
	LoaderInfo* _loaderInfo,
	Optional<const tiny_string&> name
) : MovieClip
(
	sys,
	_movie,
	new FrameContainer(),
	nullptr,
	_loaderInfo,
	name
),
parsingIsFailed(false),
waitingForParser(false),
firstFrameAvailable(false),
hasDefineSceneAndFrameLabelDataTag(false),
background(0xFFFFFF),
avm1level(-1),
finishedLoading(false),
parseThread(nullptr),
fileLength(0),
executingFrameScriptCount(0),
hasSymbolClass(false),
hasMainClass(false),
completionHandled(false),
applicationDomain(appDomain),
securityDomain(secDomain),
{
	avm1focusrect = asAtomHandler::falseAtom;
	objfreelist = nullptr; // ensure RootMovieClips aren't reused to avoid conflicts with "normal" MovieClips
	if (!applicationDomain.isNull())
		applicationDomain->setRefConstant();
	if (!securityDomain.isNull())
		securityDomain->setRefConstant();
	loadedFrom = applicationDomain.getPtr();
}

RootMovieClip::~RootMovieClip()
{
	if (frameContainer != nullptr)
		delete frameContainer;
}

void RootMovieClip::destroyTags()
{
	frameContainer->destroyTags();
}

void RootMovieClip::setParsingFailed()
{
	//The parsing is failed, we have no change to be ever valid
	parsingFailed = true;
}

void RootMovieClip::commitFrame(bool another)
{
	auto sys = getSys();
	frameContainer->setFramesLoaded(frameContainer->getFramesSize());

	if (another)
		frameContainer->addFrame();
	checkSound(frameContainer->getFramesSize());

	if (getFramesLoaded() != 1 || !sys->getFrameRate())
		return;

	/* now the frameRate is available and all SymbolClass tags have created their classes */

	// ensure construction is completed in vm thread
	getVm(sys)->addEvent(NullRef, _MR
	(
		new (sys->unaccountedMemory) FirstFrameAvailableEvent(this)
	));
}

void RootMovieClip::constructionComplete(bool _explicit, bool forInitAction)
{
	if (isConstructed())
		return;
	auto sys = getSys();
	bool isSWFRoot = this == sys->mainClip;
	bool isVMThread = sys->runSingleThreaded || isVmThread();
	if (isSWFRoot)
		this->avm1focusrect = asAtomHandler::trueAtom;
	if (!isVMThread && !getInstanceWorker()->isPrimordial)
	{
		getVm(sys)->prependEvent(NullRef, _MR
		(
			new (sys->unaccountedMemory) RootConstructedEvent
			(
				_MR(this),
				_explicit
			)
		));
		return;
	}

	sys->stage->AVM1AddDisplayObject(this);
	if (isSWFRoot)
	{
		MovieClip::constructionComplete(_explicit, forInitAction);
		return;
	}

	if (!isVMThread)
	{
		getVm(sys)->addBufferEvent(NullRef, _MR
		(
			new (sys->unaccountedMemory) RootConstructedEvent
			(
				_MR(this),
				_explicit
			)
		));
		return;
	}

	MovieClip::constructionComplete(_explicit, forInitAction);
	if (getInstanceWorker()->isPrimordial && this == getInstanceWorker()->rootClip.getPtr())
	{
		incRef();
		getInstanceWorker()->stage->_addChildAt(this, 0);
		this->setOnStage(true,true);
	}
}

void RootMovieClip::afterTimelineCreation()
{
	MovieClip::afterTimelineCreation();
	if (!isAS3())
		return;
	// ensure addedToStage event is executed after construction of root is complete
	getVm(getSys())->handleEvent(_MR(this), _MR(Class<Event>::getInstanceS
	(
		getInstanceWorker(),
		"addedToStage"
	)));
}

void RootMovieClip::revertFrame()
{
	auto frameSize = frameContainer->getFramesSize();
	auto framesLoaded = frameContainer->getFramesLoaded();
	assert(frameSize && framesLoaded == frameSize - 1);
	frameContainer->pop_frame();
}

/* This is run in vm's thread context */
void RootMovieClip::initFrame()
{
	if (waitingForParser)
		return;
	LOG_CALL
	(
		"Root:initFrame " << getFramesLoaded() << ' ' <<
		state.FP << ' ' <<
		state.stop_FP << ' ' <<
		state.next_FP << ' ' <<
		state.explicit_FP
	);
	/* We have to wait for at least one frame
	 * so our class get the right classdef. Else we will
	 * call the wrong constructor. */
	if (!firstFrameAvailable)
		return;

	MovieClip::initFrame();
}

/* This is run in vm's thread context */
void RootMovieClip::advanceFrame(bool implicit)
{
	/* We have to wait until enough frames are available */
	waitingForParser =
	(
		this == getSys()->mainClip &&
		!state.inEnterFrame
	) && (!firstFrameAvailable ||
	(
		state.next_FP >= getFramesLoaded() &&
		!hasFinishedLoading()
	));

	if (waitingForParser)
		return;

	if (!implicit || !isAS3() || !state.explicit_FP)
		MovieClip::advanceFrame(implicit);

	// ensure "complete" events are added _after_ the whole SystemState::tick() events are handled at least once
	if (!completionHandled)
	{
		if (loaderInfo != nullptr)
			loaderInfo->setComplete();
		completionHandled = true;
	}
}

void RootMovieClip::executeFrameScript()
{
	if (waitingForParser || !firstFrameAvailable)
		return;
	MovieClip::executeFrameScript();
}

void RootMovieClip::bindClass(const QName& classname, Class_inherit* cls)
{
	applicationDomain->bindClass(classname, cls);
}


void RootMovieClip::setupAVM1RootMovie()
{
	if (!needsActionScript3())
	{
		getSystemState()->stage->AVM1RootClipAdded();
		this->classdef = Class<AVM1MovieClip>::getRef(getSystemState()).getPtr();
		if (!getSystemState()->avm1global)
			getVm(getSystemState())->registerClassesAVM1();
		// it seems that the url parameters and flash vars are made available as properties of the root movie clip
		// I haven't found anything in the documentation but gnash also does this
		_NR<ASObject> params;
		if (this == getSystemState()->mainClip)
			params = getSystemState()->getParameters();
		else
			params = this->loaderInfo->parameters;
		if (params)
			params->copyValues(this,getInstanceWorker());
	}
}

void RootMovieClip::AVM1setLevel(int level)
{
	if (level < 0)
	{
		if (avm1level >= 0)
			getSystemState()->stage->AVM1removeLevelRoot(avm1level);
		avm1level = level;
	}
	else
	{
		getSystemState()->stage->AVM1SetLevelRoot(level,this);
		avm1level = level;
	}
}

void RootMovieClip::addToFrame(DisplayListTag* t)
{
	assert(frameContainer != nullptr);
	frameContainer->addToFrame(t);
}

void RootMovieClip::addFrameLabel
(
	size_t frame,
	const tiny_string& label
)
{
	assert(frameContainer != nullptr);
	frameContainer->addFrameLabel(frame, label);
}

void RootMovieClip::addScene
(
	size_t scene,
	size_t startFrame,
	const tiny_string& name
)
{
	assert(frameContainer != nullptr);
	frameContainer->addScene(scene, startFrame, name);
}

size_t RootMovieClip::getFramesLoaded() const
{
	assert(frameContainer != nullptr);
	return frameContainer->getFramesLoaded();
}
