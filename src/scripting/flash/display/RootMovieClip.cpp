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
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/class.h"
#include "scripting/abc.h"



using namespace std;
using namespace lightspark;

RootMovieClip::RootMovieClip(ASWorker* wrk, _NR<LoaderInfo> li, _NR<ApplicationDomain> appDomain, _NR<SecurityDomain> secDomain, Class_base* c):
	MovieClip(wrk,c),
	parsingIsFailed(false),waitingforparser(false),Background(0xFF,0xFF,0xFF),frameRate(0),
	finishedLoading(false),applicationDomain(appDomain),securityDomain(secDomain)
{
	this->objfreelist=nullptr; // ensure RootMovieClips aren't reused to avoid conflicts with "normal" MovieClips
	subtype=SUBTYPE_ROOTMOVIECLIP;
	loaderInfo=li;
	parsethread=nullptr;
	hasSymbolClass=false;
	hasMainClass=false;
	usesActionScript3=false;
	completionHandled=false;
	executingFrameScriptCount=0;
}

RootMovieClip::~RootMovieClip()
{
	for(auto it=dictionary.begin();it!=dictionary.end();++it)
		delete it->second;
}

void RootMovieClip::destroyTags()
{
	for(auto it=frames.begin();it!=frames.end();++it)
		it->destroyTags();
}

void RootMovieClip::parsingFailed()
{
	//The parsing is failed, we have no change to be ever valid
	parsingIsFailed=true;
}

void RootMovieClip::setOrigin(const tiny_string& u, const tiny_string& filename)
{
	//We can use this origin to implement security measures.
	//Note that for plugins, this url is NOT the page url, but it is the swf file url.
	origin = URLInfo(u);
	//If this URL doesn't contain a filename, add the one passed as an argument (used in main.cpp)
	if(origin.getPathFile() == "" && filename != "")
	{
		tiny_string fileurl = g_path_is_absolute(filename.raw_buf()) ? g_filename_to_uri(filename.raw_buf(), nullptr,nullptr) : filename;
		origin = origin.goToURL(fileurl);
	}
	if(!loaderInfo.isNull())
	{
		loaderInfo->setURL(origin.getParsedURL(), false);
		loaderInfo->setLoaderURL(origin.getParsedURL());
	}
}

void RootMovieClip::setBaseURL(const tiny_string& url)
{
	//Set the URL to be used in resolving relative paths. For the
	//plugin this is either the value of base attribute in the
	//OBJECT or EMBED tag or, if the attribute is not provided,
	//the address of the hosting HTML page.
	baseURL = URLInfo(url);
}

const URLInfo& RootMovieClip::getBaseURL()
{
	//The plugin uses the address of the HTML page (baseURL) for
	//resolving relative paths. AIR and the standalone Lightspark
	//use the SWF location (origin).
	if(baseURL.isValid())
		return baseURL;
	else
		return origin;
}


RootMovieClip* RootMovieClip::getInstance(ASWorker* wrk,_NR<LoaderInfo> li, _R<ApplicationDomain> appDomain, _R<SecurityDomain> secDomain)
{
	Class_base* movieClipClass = Class<MovieClip>::getClass(getSys());
	RootMovieClip* ret=new (movieClipClass->memoryAccount) RootMovieClip(wrk,li, appDomain, secDomain, movieClipClass);
	ret->constructorCallComplete = true;
	ret->loadedFrom=ret;
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

void RootMovieClip::setFrameSize(const lightspark::RECT& f)
{
	frameSize=f;
}

lightspark::RECT RootMovieClip::getFrameSize() const
{
	return frameSize;
}

void RootMovieClip::setFrameRate(float f)
{
	if (frameRate != f)
	{
		frameRate=f;
		if (this == getSystemState()->mainClip )
			getSystemState()->setRenderRate(frameRate);
	}
}

float RootMovieClip::getFrameRate() const
{
	return frameRate;
}

void RootMovieClip::commitFrame(bool another)
{
	setFramesLoaded(frames.size());

	if(another)
		frames.push_back(Frame());
	checkSound(frames.size());

	if(getFramesLoaded()==1 && frameRate!=0)
	{
		SystemState* sys = getSys();
		if(this==sys->mainClip || !hasMainClass)
		{
			/* now the frameRate is available and all SymbolClass tags have created their classes */
			// in AS3 this is added to the stage after the construction of the main object is completed (if a main object exists)
			if (!usesActionScript3 || !hasMainClass)
			{
				while (!getVm(sys)->hasEverStarted()) // ensure that all builtin classes are defined
					sys->sleep_ms(10);
				constructionComplete();
				afterConstruction();
			}
		}
	}
}

void RootMovieClip::constructionComplete(bool _explicit)
{
	if(isConstructed())
		return;
	if (!isVmThread() && !getInstanceWorker()->isPrimordial)
	{
		this->incRef();
		getVm(getSystemState())->prependEvent(NullRef,_MR(new (getSystemState()->unaccountedMemory) RootConstructedEvent(_MR(this), _explicit)));
		return;
	}
	getSystemState()->stage->AVM1AddDisplayObject(this);
	if (this!=getSystemState()->mainClip)
	{
		if (!isVmThread())
		{
			this->incRef();
			getVm(getSystemState())->addBufferEvent(NullRef,_MR(new (getSystemState()->unaccountedMemory) RootConstructedEvent(_MR(this), _explicit)));
		}
		else
			MovieClip::constructionComplete(_explicit);
		return;
	}
	MovieClip::constructionComplete(_explicit);
	
	incRef();
	getSystemState()->stage->_addChildAt(this,0);
	this->setOnStage(true,true);
	getSystemState()->addTick(1000/frameRate,getSystemState());
}
void RootMovieClip::afterConstruction(bool _explicit)
{
	DisplayObject::afterConstruction(_explicit);
	if (this!=getSystemState()->mainClip)
		return;
	if (!needsActionScript3())
	{
		getSystemState()->stage->advanceFrame(true);
		initFrame();
	}
}
void RootMovieClip::revertFrame()
{
	//TODO: The next should be a regular assert
	assert_and_throw(frames.size() && getFramesLoaded()==(frames.size()-1));
	frames.pop_back();
}

RGB RootMovieClip::getBackground()
{
	return Background;
}

void RootMovieClip::setBackground(const RGB& bg)
{
	Background=bg;
}

/* called in parser's thread context */
void RootMovieClip::addToDictionary(DictionaryTag* r)
{
	Locker l(dictSpinlock);
	dictionary[r->getId()] = r;
}

/* called in vm's thread context */
DictionaryTag* RootMovieClip::dictionaryLookup(int id)
{
	Locker l(dictSpinlock);
	auto it = dictionary.find(id);
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,"No such Id on dictionary " << id << " for " << origin);
		//throw RunTimeException("Could not find an object on the dictionary");
		return nullptr;
	}
	return it->second;
}
DictionaryTag* RootMovieClip::dictionaryLookupByName(uint32_t nameID)
{
	Locker l(dictSpinlock);
	auto it = dictionary.begin();
	for(;it!=dictionary.end();++it)
	{
		if(it->second->nameID==nameID)
			return it->second;
	}
	// tag not found, also check case insensitive
	if(it==dictionary.end())
	{
		
		tiny_string namelower = getSystemState()->getStringFromUniqueId(nameID).lowercase();
		it = dictionary.begin();
		for(;it!=dictionary.end();++it)
		{
			if (it->second->nameID == UINT32_MAX)
				continue;
			tiny_string dictnamelower = getSystemState()->getStringFromUniqueId(it->second->nameID);
			dictnamelower = dictnamelower.lowercase();
			if(dictnamelower==namelower)
				return it->second;
		}
	}
	LOG(LOG_ERROR,"No such name on dictionary " << getSystemState()->getStringFromUniqueId(nameID) << " for " << origin);
	return nullptr;
}

void RootMovieClip::addToScalingGrids(const DefineScalingGridTag* r)
{
	Locker l(scalinggridsmutex);
	scalinggrids[r->CharacterId] = r->Splitter;
}

lightspark::RECT* RootMovieClip::ScalingGridsLookup(int id)
{
	Locker l(scalinggridsmutex);
	auto it = scalinggrids.find(id);
	if(it==scalinggrids.end())
		return nullptr;
	return &(*it).second;
}

void RootMovieClip::resizeCompleted()
{
	for(auto it=dictionary.begin();it!=dictionary.end();++it)
		it->second->resizeCompleted();
}

_NR<RootMovieClip> RootMovieClip::getRoot()
{
	this->incRef();
	return _MR(this);
}

_NR<Stage> RootMovieClip::getStage()
{
	if (this == getSystemState()->mainClip)
	{
		getSystemState()->stage->incRef();
		return _MR(getSystemState()->stage);
	}
	else if (getParent())
		return getParent()->getStage();
	else
		return NullRef;
}

/*ASObject* RootMovieClip::getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
{
	sem_wait(&mutex);
	ASObject* ret=ASObject::getVariableByQName(name, ns, owner);
	sem_post(&mutex);
	return ret;
}

void RootMovieClip::setVariableByMultiname(multiname& name, asAtom o)
{
	sem_wait(&mutex);
	ASObject::setVariableByMultiname(name,o);
	sem_post(&mutex);
}

void RootMovieClip::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
{
	sem_wait(&mutex);
	ASObject::setVariableByQName(name,ns,o);
	sem_post(&mutex);
}

void RootMovieClip::setVariableByString(const string& s, ASObject* o)
{
	abort();
	//TODO: ActionScript2 support
	string sub;
	int f=0;
	int l=0;
	ASObject* target=this;
	for(l;l<s.size();l++)
	{
		if(s[l]=='.')
		{
			sub=s.substr(f,l-f);
			ASObject* next_target;
			if(f==0 && sub=="__Packages")
			{
				next_target=&sys->cur_render_thread->vm.Global;
				owner=&sys->cur_render_thread->vm.Global;
			}
			else
				next_target=target->getVariableByQName(sub.c_str(),"",owner);

			f=l+1;
			if(!owner)
			{
				next_target=new Package;
				target->setVariableByQName(sub.c_str(),"",next_target);
			}
			target=next_target;
		}
	}
	sub=s.substr(f,l-f);
	target->setVariableByQName(sub.c_str(),"",o);
}*/

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
	if(this == getSystemState()->mainClip && (getFramesLoaded() == 0 || (state.next_FP>=(uint32_t)getFramesLoaded() && !hasFinishedLoading())))
	{
		waitingforparser=true;
		return;
	}
	waitingforparser=false;

	if (!implicit || !usesActionScript3 || !state.explicit_FP)
		MovieClip::advanceFrame(implicit);
	// ensure "complete" events are added _after_ the whole SystemState::tick() events are handled at least once
	if (!completionHandled)
	{
		if (!loaderInfo.isNull())
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
	applicationDomain.reset();
	securityDomain.reset();
	waitingforparser=false;
	parsethread=nullptr;
	return MovieClip::destruct();
}
void RootMovieClip::finalize()
{
	applicationDomain.reset();
	securityDomain.reset();
	parsethread=nullptr;
	MovieClip::finalize();
}

void RootMovieClip::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	MovieClip::prepareShutdown();
	if (applicationDomain)
		applicationDomain->prepareShutdown();
	if (securityDomain)
		securityDomain->prepareShutdown();
}

void RootMovieClip::addBinding(const tiny_string& name, DictionaryTag *tag)
{
	// This function will be called only be the parsing thread,
	// and will only access the last frame, so no locking needed.
	tag->bindingclassname = name;
	uint32_t pos = name.rfind(".");
	if (pos== tiny_string::npos)
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name),BUILTIN_STRINGS::EMPTY)] =  tag;
	else
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name.substr(pos+1,name.numChars()-(pos+1))),getSystemState()->getUniqueStringId(name.substr(0,pos)))] = tag;
}

void RootMovieClip::bindClass(const QName& classname, Class_inherit* cls)
{
	if (cls->isBinded() || classesToBeBound.empty())
		return;

	auto it=classesToBeBound.find(classname);
	if(it!=classesToBeBound.end())
	{
		cls->bindToTag(it->second);
		classesToBeBound.erase(it);
	}
}

void RootMovieClip::checkBinding(DictionaryTag *tag)
{
	if (tag->bindingclassname.empty())
		return;
	multiname clsname(nullptr);
	clsname.name_type=multiname::NAME_STRING;
	clsname.isAttribute = false;

	uint32_t pos = tag->bindingclassname.rfind(".");
	tiny_string ns;
	tiny_string nm;
	if (pos != tiny_string::npos)
	{
		nm = tag->bindingclassname.substr(pos+1,tag->bindingclassname.numBytes());
		ns = tag->bindingclassname.substr(0,pos);
		clsname.hasEmptyNS=false;
	}
	else
	{
		nm = tag->bindingclassname;
		ns = "";
	}
	clsname.name_s_id=getSystemState()->getUniqueStringId(nm);
	clsname.ns.push_back(nsNameAndKind(getSystemState(),ns,NAMESPACE));
	
	ASObject* typeObject = nullptr;
	auto i = applicationDomain->classesBeingDefined.cbegin();
	while (i != applicationDomain->classesBeingDefined.cend())
	{
		if(i->first->name_s_id == clsname.name_s_id && i->first->ns[0].nsRealId == clsname.ns[0].nsRealId)
		{
			typeObject = i->second;
			break;
		}
		i++;
	}
	if (typeObject == nullptr)
	{
		ASObject* target;
		asAtom o=asAtomHandler::invalidAtom;
		applicationDomain->getVariableAndTargetByMultiname(o,clsname,target,getInstanceWorker());
		if (asAtomHandler::isValid(o))
			typeObject=asAtomHandler::getObject(o);
	}
	if (typeObject != nullptr)
	{
		Class_inherit* cls = typeObject->as<Class_inherit>();
		if (cls)
		{
			ABCVm *vm = getVm(getSystemState());
			vm->buildClassAndBindTag(tag->bindingclassname.raw_buf(), tag,cls);
			tag->bindedTo=cls;
			tag->bindingclassname = "";
			cls->bindToTag(tag);
		}
	}
}

void RootMovieClip::registerEmbeddedFont(const tiny_string fontname, FontTag *tag)
{
	if (!fontname.empty())
	{
		auto it = embeddedfonts.find(fontname);
		if (it == embeddedfonts.end())
		{
			embeddedfonts[fontname] = tag;
			// it seems that adobe allows fontnames to be lowercased and stripped of spaces and numbers
			tiny_string tmp = fontname.lowercase();
			tiny_string fontnamenormalized;
			for (auto it = tmp.begin();it != tmp.end(); it++)
			{
				if (*it == ' ' || (*it >= '0' &&  *it <= '9'))
					continue;
				fontnamenormalized += *it;
			}
			embeddedfonts[fontnamenormalized] = tag;
		}
	}
	embeddedfontsByID[tag->getId()] = tag;
}

FontTag *RootMovieClip::getEmbeddedFont(const tiny_string fontname) const
{
	auto it = embeddedfonts.find(fontname);
	if (it != embeddedfonts.end())
		return it->second;
	it = embeddedfonts.find(fontname.lowercase());
	if (it != embeddedfonts.end())
		return it->second;
	return nullptr;
}
FontTag *RootMovieClip::getEmbeddedFontByID(uint32_t fontID) const
{
	auto it = embeddedfontsByID.find(fontID);
	if (it != embeddedfontsByID.end())
		return it->second;
	return nullptr;
}

void RootMovieClip::setupAVM1RootMovie()
{
	if (!usesActionScript3)
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

bool RootMovieClip::AVM1registerTagClass(const tiny_string &name, _NR<IFunction> theClassConstructor)
{
	uint32_t nameID = getSystemState()->getUniqueStringId(name);
	DictionaryTag* t = dictionaryLookupByName(nameID);
	if (!t)
	{
		LOG(LOG_ERROR,"registerClass:no tag found in dictionary for "<<name);
		return false;
	}
	if (theClassConstructor.isNull())
		avm1ClassConstructors.erase(t->getId());
	else
		avm1ClassConstructors.insert(make_pair(t->getId(),theClassConstructor));
	return true;
}

AVM1Function* RootMovieClip::AVM1getClassConstructor(uint32_t spriteID)
{
	auto it = avm1ClassConstructors.find(spriteID);
	if (it == avm1ClassConstructors.end())
		return nullptr;
	return it->second->is<AVM1Function>() ? it->second->as<AVM1Function>() : nullptr;
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
