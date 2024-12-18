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
friend class ApplicationDomain;
private:
	bool parsingIsFailed;
	bool waitingforparser;
	RGB Background;
	int avm1level;

	/* those are private because you shouldn't call mainClip->*,
	 * but mainClip->getStage()->* instead.
	 */
	void initFrame() override;
	void advanceFrame(bool implicit) override;
	void executeFrameScript() override;
	ACQUIRE_RELEASE_FLAG(finishedLoading);
	
	unordered_map<uint32_t,AVM1InitActionTag*> avm1InitActionTags;
public:
	RootMovieClip(ASWorker* wrk,LoaderInfo* li, _NR<ApplicationDomain> appDomain, _NR<SecurityDomain> secDomain, Class_base* c);
	~RootMovieClip();
	void destroyTags();
	bool destruct() override;
	void finalize() override;
	bool hasFinishedLoading() override { return ACQUIRE_READ(finishedLoading); }
	bool isWaitingForParser() { return waitingforparser; }
	void constructionComplete(bool _explicit = false) override;
	void afterConstruction(bool _explicit = false) override;
	bool needsActionScript3() const override;
	ParseThread* parsethread;
	uint32_t fileLength;
	uint32_t executingFrameScriptCount;
	bool hasSymbolClass;
	bool hasMainClass;
	bool completionHandled;
	RGB getBackground();
	void setBackground(const RGB& bg);
	void labelCurrentFrame(const STRING& name);
	void commitFrame(bool another);
	void revertFrame();
	void parsingFailed();
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	void setOrigin(const tiny_string& u, const tiny_string& filename="") DLL_PUBLIC;
	URLInfo& getOrigin() DLL_PUBLIC;
	void setBaseURL(const tiny_string& url) DLL_PUBLIC;
	static RootMovieClip* getInstance(ASWorker* wrk, LoaderInfo* li, _R<ApplicationDomain> appDomain, _R<SecurityDomain> secDomain);
	/*
	 * The application domain for this clip
	 */
	_NR<ApplicationDomain> applicationDomain;
	/*
	 * The security domain for this clip
	 */
	_NR<SecurityDomain> securityDomain;
	//DisplayObject interface
	_NR<RootMovieClip> getRoot() override;
	_NR<Stage> getStage() override;
	void bindClass(const QName &classname, Class_inherit* cls);
	void setupAVM1RootMovie();
	void AVM1registerInitActionTag(uint32_t spriteID, AVM1InitActionTag* tag);
	void AVM1checkInitActions(MovieClip *sprite);
	int AVM1getLevel() const { return avm1level; }
	void AVM1setLevel(int level) { avm1level = level; }
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_ROOTMOVIECLIP_H */
