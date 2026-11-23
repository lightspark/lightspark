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

#ifndef DISPLAY_OBJECT_ROOTMOVIECLIP_H
#define DISPLAY_OBJECT_ROOTMOVIECLIP_H 1

#include "display_object/MovieClip.h"
#include "swftypes.h"

namespace lightspark
{

class ParseThread;

class RootMovieClip : public MovieClip
{
	friend class ParseThread;
	friend class ApplicationDomain;
private:
	bool parsingFailed;
	bool waitingForParser;
	bool firstFrameAvailable;
	bool hasDefineSceneAndFrameLabelDataTag;
	RGB background;
	int avm1level;
	ACQUIRE_RELEASE_FLAG(finishedLoading);

	/* those are private because you shouldn't call mainClip->*,
	 * but mainClip->getStage()->* instead.
	 */
	void initFrame() override;
	void advanceFrame(bool implicit) override;
	void executeFrameScript() override;
public:
	ParseThread* parseThread;
	size_t fileLength;
	size_t executingFrameScriptCount;
	bool hasSymbolClass;
	bool hasMainClass;
	bool completionHandled;
	/*
	 * The application domain for this clip
	 */
	_NR<ApplicationDomain> applicationDomain;
	/*
	 * The security domain for this clip
	 */
	_NR<SecurityDomain> securityDomain;

	RootMovieClip
	(
		SystemState* sys,
		SWFMovie& _movie,
		LoaderInfo* _loaderInfo,
		Optional<const tiny_string&> name = {}
	);

	~RootMovieClip();
	void destroyTags();
	bool hasFinishedLoading() override { return ACQUIRE_READ(finishedLoading); }
	bool isWaitingForParser() const { return waitingForParser; }
	void setFirstFrameAvailable() { firstFrameAvailable = true; }
	void constructionComplete(bool _explicit = false, bool forInitAction = false) override;
	void afterConstruction(bool _explicit = false) override;
	void afterTimelineCreation() override;
	bool isFocusable(bool fromMouse) override { return false; }
	const RGB& getBackground() const { return background; }
	void setBackground(const RGB& bg) { background = bg; }
	void labelCurrentFrame(const STRING& name);
	void commitFrame(bool another);
	void revertFrame();
	void setParsingFailed();
	//DisplayObject interface
	void bindClass(const QName &classname, Class_inherit* cls);
	void setupAVM1RootMovie();
	int AVM1getLevel() const { return avm1level; }
	void AVM1setLevel(int level);
	bool hasScenes() const { return hasDefineSceneAndFrameLabelDataTag; }
	void setHasScenes() { hasDefineSceneAndFrameLabelDataTag = true; }
	void addToFrame(DisplayListTag* t);
	void addFrameLabel(size_t frame, const tiny_string& label);
	void addScene(size_t scene, size_t startFrame, const tiny_string& name);
	size_t getFramesLoaded() const;

};

}
#endif /* DISPLAY_OBJECT_ROOTMOVIECLIP_H */
