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

#ifndef SCRIPTING_FLASH_DISPLAY_ROOTMOVIECLIP_H
#define SCRIPTING_FLASH_DISPLAY_ROOTMOVIECLIP_H 1

#include "parsing/tags.h"
#include "scripting/flash/display/MovieClip.h"


namespace lightspark
{

class ParseThread;

class RootMovieClip: public MovieClip
{
friend class ParseThread;
protected:
	URLInfo origin;
private:
	bool parsingIsFailed;
	bool waitingforparser;
	RGB Background;
	Mutex dictSpinlock;
	std::unordered_map < uint32_t, DictionaryTag* > dictionary;
	Mutex scalinggridsmutex;
	std::unordered_map < uint32_t, RECT > scalinggrids;
	std::map < QName, DictionaryTag* > classesToBeBound;
	std::map < tiny_string,FontTag* > embeddedfonts;
	std::map < uint32_t,FontTag* > embeddedfontsByID;

	//frameSize and frameRate are valid only after the header has been parsed
	RECT frameSize;
	float frameRate;
	URLInfo baseURL;
	/* those are private because you shouldn't call mainClip->*,
	 * but mainClip->getStage()->* instead.
	 */
	void initFrame() override;
	void advanceFrame(bool implicit) override;
	void executeFrameScript() override;
	ACQUIRE_RELEASE_FLAG(finishedLoading);
	
	unordered_map<uint32_t,_NR<IFunction>> avm1ClassConstructors;
	unordered_map<uint32_t,AVM1InitActionTag*> avm1InitActionTags;
public:
	RootMovieClip(ASWorker* wrk,_NR<LoaderInfo> li, _NR<ApplicationDomain> appDomain, _NR<SecurityDomain> secDomain, Class_base* c);
	~RootMovieClip();
	void destroyTags();
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool hasFinishedLoading() override { return ACQUIRE_READ(finishedLoading); }
	bool isWaitingForParser() { return waitingforparser; }
	void constructionComplete(bool _explicit = false) override;
	void afterConstruction(bool _explicit = false) override;
	bool needsActionScript3() const override { return this->usesActionScript3;}
	ParseThread* parsethread;
	uint32_t version;
	uint32_t fileLength;
	uint32_t executingFrameScriptCount;
	bool hasSymbolClass;
	bool hasMainClass;
	bool usesActionScript3;
	bool completionHandled;
	RGB getBackground();
	void setBackground(const RGB& bg);
	void setFrameSize(const RECT& f);
	RECT getFrameSize() const;
	float getFrameRate() const;
	void setFrameRate(float f);
	void addToDictionary(DictionaryTag* r);
	DictionaryTag* dictionaryLookup(int id);
	DictionaryTag* dictionaryLookupByName(uint32_t nameID);
	void addToScalingGrids(const DefineScalingGridTag* r);
	RECT* ScalingGridsLookup(int id);
	void resizeCompleted();
	void labelCurrentFrame(const STRING& name);
	void commitFrame(bool another);
	void revertFrame();
	void parsingFailed();
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	void DLL_PUBLIC setOrigin(const tiny_string& u, const tiny_string& filename="");
	URLInfo& getOrigin() { return origin; }
	void DLL_PUBLIC setBaseURL(const tiny_string& url);
	const URLInfo& getBaseURL();
	static RootMovieClip* getInstance(ASWorker* wrk, _NR<LoaderInfo> li, _R<ApplicationDomain> appDomain, _R<SecurityDomain> secDomain);
	/*
	 * The application domain for this clip
	 */
	_NR<ApplicationDomain> applicationDomain;
	/*
	 * The security domain for this clip
	 */
	_NR<SecurityDomain> securityDomain;
	//map of all classed defined in the swf. They own one reference to each class/template
	//key is the stringID of the class name (without namespace)
	std::multimap<uint32_t, Class_base*> customClasses;
	/*
	 * Support for class aliases in AMF3 serialization
	 */
	std::map<tiny_string, _R<Class_base> > aliasMap;
	std::map<QName,std::unordered_set<uint32_t>*> customclassoverriddenmethods;
	std::map<QName, Template_base*> templates;
	//DisplayObject interface
	_NR<RootMovieClip> getRoot() override;
	_NR<Stage> getStage() override;
	void addBinding(const tiny_string& name, DictionaryTag *tag);
	void bindClass(const QName &classname, Class_inherit* cls);
	void checkBinding(DictionaryTag* tag);
	void registerEmbeddedFont(const tiny_string fontname, FontTag *tag);
	FontTag* getEmbeddedFont(const tiny_string fontname) const;
	FontTag* getEmbeddedFontByID(uint32_t fontID) const;
	void setupAVM1RootMovie();
	// map AVM1 class constructors to named tags
	bool AVM1registerTagClass(const tiny_string& name, _NR<IFunction> theClassConstructor);
	AVM1Function* AVM1getClassConstructor(uint32_t spriteID);
	void AVM1registerInitActionTag(uint32_t spriteID, AVM1InitActionTag* tag);
	void AVM1checkInitActions(MovieClip *sprite);
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_ROOTMOVIECLIP_H */
