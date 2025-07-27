/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/display/FrameContainer.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Number.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/class.h"
#include "scripting/abc.h"
#include "parsing/tags.h"



using namespace std;
using namespace lightspark;

RootMovieClip::RootMovieClip(ASWorker* wrk, LoaderInfo* li, _NR<ApplicationDomain> appDomain, _NR<SecurityDomain> secDomain, Class_base* c):
	MovieClip(wrk,c),
	parsingIsFailed(false),waitingforparser(false),hasDefineSceneAndFrameLabelDataTag(false),
	Background(0xFF,0xFF,0xFF),avm1level(-1),
	finishedLoading(false),applicationDomain(appDomain),securityDomain(secDomain)
{
	framecontainer = new FrameContainer();
	this->avm1focusrect=asAtomHandler::falseAtom;
	this->objfreelist=nullptr; // ensure RootMovieClips aren't reused to avoid conflicts with "normal" MovieClips
	subtype=SUBTYPE_ROOTMOVIECLIP;
	setLoaderInfo(li);
	if (applicationDomain)
		applicationDomain->setRefConstant();
	if (securityDomain)
		securityDomain->setRefConstant();
	loadedFrom = applicationDomain.getPtr();
	parsethread=nullptr;
	hasSymbolClass=false;
	hasMainClass=false;
	completionHandled=false;
	executingFrameScriptCount=0;
}

RootMovieClip::~RootMovieClip()
{
}

void RootMovieClip::destroyTags()
{
	framecontainer->destroyTags();
}

void RootMovieClip::parsingFailed()
{
	//The parsing is failed, we have no change to be ever valid
	parsingIsFailed=true;
}

void RootMovieClip::setOrigin(const tiny_string& u, const tiny_string& filename)
{
	applicationDomain->setOrigin(u,filename);
	if(loaderInfo)
	{
		loaderInfo->setURL(getOrigin().getParsedURL(), false);
		loaderInfo->setLoaderURL(getOrigin().getParsedURL());
	}
}

URLInfo& RootMovieClip::getOrigin() 
{
	return applicationDomain->getOrigin();
}

void RootMovieClip::setBaseURL(const tiny_string& url)
{
	applicationDomain->setBaseURL(url);
}

RootMovieClip* RootMovieClip::getInstance(ASWorker* wrk, LoaderInfo* li, _R<ApplicationDomain> appDomain, _R<SecurityDomain> secDomain)
{
	Class_base* movieClipClass = Class<MovieClip>::getClass(getSys());
	RootMovieClip* ret=new (movieClipClass->memoryAccount) RootMovieClip(wrk,li, appDomain, secDomain, movieClipClass);
	ret->loadedFrom=appDomain.getPtr();
	return ret;
}


bool RootMovieClip::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	// at least for hittest the bounds of the root are computed by the bounds of its contents
	return MovieClip::boundsRect(xmin, xmax, ymin, ymax,visibleOnly);
//	RECT f=getFrameSize();
//	xmin=f.Xmin/20;
//	ymin=f.Ymin/20;
//	xmax=f.Xmax/20;
//	ymax=f.Ymax/20;
//	return true;
}

void RootMovieClip::commitFrame(bool another)
{
	framecontainer->setFramesLoaded(framecontainer->getFramesSize());

	if(another)
		framecontainer->addFrame();
	checkSound(framecontainer->getFramesSize());

	if(getFramesLoaded()==1 && applicationDomain->getFrameRate()!=0)
	{
		SystemState* sys = getSys();
		if(this==sys->mainClip || !hasMainClass)
		{
			/* now the frameRate is available and all SymbolClass tags have created their classes */
			// in AS3 this is added to the stage after the construction of the main object is completed (if a main object exists)
			if (!needsActionScript3() || !hasMainClass)
			{
				while (!getVm(sys)->hasEverStarted()) // ensure that all builtin classes are defined
					sys->sleep_ms(10);
				constructionComplete();
				afterConstruction();
			}
		}
	}
}

void RootMovieClip::constructionComplete(bool _explicit, bool forInitAction)
{
	if(isConstructed())
		return;
	if (this==getSystemState()->mainClip)
		this->avm1focusrect = asAtomHandler::trueAtom;
	if (!getSystemState()->runSingleThreaded && !isVmThread() && !getInstanceWorker()->isPrimordial)
	{
		this->incRef();
		getVm(getSystemState())->prependEvent(NullRef,_MR(new (getSystemState()->unaccountedMemory) RootConstructedEvent(_MR(this), _explicit)));
		return;
	}
	getSystemState()->stage->AVM1AddDisplayObject(this);
	if (this!=getSystemState()->mainClip)
	{
		if (!getSystemState()->runSingleThreaded && !isVmThread())
		{
			this->incRef();
			getVm(getSystemState())->addBufferEvent(NullRef,_MR(new (getSystemState()->unaccountedMemory) RootConstructedEvent(_MR(this), _explicit)));
		}
		else
		{
			MovieClip::constructionComplete(_explicit,forInitAction);
			if (this==getInstanceWorker()->rootClip.getPtr())
			{
				incRef();
				getInstanceWorker()->stage->_addChildAt(this,0);
				this->setOnStage(true,true);
			}
			if (needsActionScript3())
			{
				tiny_string s = "root";
				s += Number::toString(DisplayObject::getDepth());
				this->name = getSystemState()->getUniqueStringId("root1");
				this->hasDefaultName=true;
			}
		}
		return;
	}
	if (!getParent())
	{
		// add to stage before continuing construction to make sure stage is available from AS code
		incRef();
		getSystemState()->stage->insertLegacyChildAt(-16384, this, false, false);
	}
	if (needsActionScript3())
	{
		this->name = getSystemState()->getUniqueStringId("root1");
		this->hasDefaultName=true;
	}

	MovieClip::constructionComplete(_explicit,forInitAction);

	this->setOnStage(true,true);
}
void RootMovieClip::afterConstruction(bool _explicit)
{
	DisplayObject::afterConstruction(_explicit);
	// ensure addedToStage event is executed after construction of root is complete
	_R<Event> e=_MR(Class<Event>::getInstanceS(getInstanceWorker(),"addedToStage"));
	if(isVmThread())
	{
		ABCVm::publicHandleEvent(this,e);
		getSystemState()->runInnerGotoFrame(this);
	}
	else
	{
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),e);
	}
	if (this==getSystemState()->mainClip)
		getSystemState()->addFrameTick(getSystemState());
}

bool RootMovieClip::needsActionScript3() const
{
	assert(getSystemState()->isShuttingDown() || applicationDomain || getInDestruction() || deletedingarbagecollection || gcNext || gcPrev);
	return !this->applicationDomain || this->applicationDomain->usesActionScript3;
}
void RootMovieClip::revertFrame()
{
	assert(framecontainer->getFramesSize() && framecontainer->getFramesLoaded()==(framecontainer->getFramesSize()-1));
	framecontainer->pop_frame();
}

RGB RootMovieClip::getBackground()
{
	return Background;
}

void RootMovieClip::setBackground(const RGB& bg)
{
	Background=bg;
}

_NR<RootMovieClip> RootMovieClip::getRoot()
{
	this->incRef();
	return _MR(this);
}

_NR<Stage> RootMovieClip::getStage()
{
	if (this == getInstanceWorker()->rootClip.getPtr())
	{
		getInstanceWorker()->stage->incRef();
		return _MR(getInstanceWorker()->stage);
	}
	else if (getParent())
		return getParent()->getStage();
	else
		return NullRef;
}

/* This is run in vm's thread context */
void RootMovieClip::initFrame()
{
	if (waitingforparser)
		return;
	LOG_CALL("Root:initFrame " << getFramesLoaded() << " " << state.FP<<" "<<state.stop_FP<<" "<<state.next_FP<<" "<<state.explicit_FP);
	/* We have to wait for at least one frame
	 * so our class get the right classdef. Else we will
	 * call the wrong constructor. */
	if(getFramesLoaded() == 0)
		return;

	MovieClip::initFrame();
}

/* This is run in vm's thread context */
void RootMovieClip::advanceFrame(bool implicit)
{
	/* We have to wait until enough frames are available */
	if(this == getSystemState()->mainClip && !state.inEnterFrame && (getFramesLoaded() == 0 || (state.next_FP>=(uint32_t)getFramesLoaded() && !hasFinishedLoading())))
	{
		waitingforparser=true;
		return;
	}
	waitingforparser=false;

	if (!implicit || !needsActionScript3() || !state.explicit_FP)
		MovieClip::advanceFrame(implicit);
	// ensure "complete" events are added _after_ the whole SystemState::tick() events are handled at least once
	if (!completionHandled)
	{
		if (loaderInfo)
			loaderInfo->setComplete();
		completionHandled=true;
	}
}

void RootMovieClip::executeFrameScript()
{
	if (waitingforparser)
		return;
	MovieClip::executeFrameScript();
}

bool RootMovieClip::destruct()
{
	if (framecontainer)
		delete framecontainer;
	framecontainer = new FrameContainer();
	framecontainer->addFrame();
	applicationDomain.reset();
	securityDomain.reset();
	waitingforparser=false;
	avm1focusrect=asAtomHandler::falseAtom;
	parsethread=nullptr;
	AVM1setLevel(-1);
	return MovieClip::destruct();
}
void RootMovieClip::finalize()
{
	if (framecontainer)
		delete framecontainer;
	framecontainer=nullptr;
	applicationDomain.reset();
	securityDomain.reset();
	parsethread=nullptr;
	MovieClip::finalize();
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

void RootMovieClip::AVM1registerInitActionTag(uint32_t spriteID, AVM1InitActionTag *tag)
{
	avm1InitActionTags[spriteID] = tag;
}
void RootMovieClip::AVM1checkInitActions(MovieClip* sprite)
{
	if (!sprite)
		return;
	auto it = avm1InitActionTags.find(sprite->getTagID());
	if (it != avm1InitActionTags.end())
	{
		AVM1InitActionTag* t = it->second;
		// a new instance of the sprite may be constructed during code execution, so we remove it from the initactionlist before executing the code to ensure it's only executed once
		avm1InitActionTags.erase(it);
		t->executeDirect(sprite);
		delete t;
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
	assert(framecontainer);
	framecontainer->addToFrame(t);
}
void RootMovieClip::addFrameLabel(uint32_t frame, const tiny_string& label)
{
	assert(framecontainer);
	framecontainer->addFrameLabel(frame,label);
}
void RootMovieClip::addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name)
{
	assert(framecontainer);
	framecontainer->addScene(sceneNo,startframe,name);
}
uint32_t RootMovieClip::getFramesLoaded()
{
	assert(framecontainer);
	return framecontainer->getFramesLoaded();
}
