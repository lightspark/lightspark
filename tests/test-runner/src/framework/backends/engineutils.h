/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef FRAMEWORK_BACKENDS_ENGINEUTILS_H
#define FRAMEWORK_BACKENDS_ENGINEUTILS_H 1

#include <cstdint>
#include <lightspark/platforms/engineutils.h>

#include "utils/filesystem_overloads.h"

using namespace lightspark;

namespace lightspark
{
	class ByteArray;
	class SystemState;
	class tiny_string;
};

struct TestRunner;

class TestRunnerEngineData : public EngineData
{
private:
	path_t basePath;
	TestRunner* runner;

	void notifyEventLoop() override;
protected:
	SDL_Window* createWidget(uint32_t w, uint32_t h) override { return nullptr; }
	void notifyTimer() override {}
public:
	TestRunnerEngineData
	(
		const path_t& _basePath,
		TestRunner* _runner
	) :
	basePath(_basePath),
	runner(_runner)
	{}

	bool isSizable() const override { return false; }
	void stopMainDownload() override {}
	void handleQuit() override {}
	uint32_t getWindowForGnash() override { return 0; }

	// TODO: Implement support for testing `SharedObject`/local storage.
	// local storage handling
	void setLocalStorageAllowedMarker(bool allowed) override {}
	bool getLocalStorageAllowedMarker() override { return false; }
	bool fillSharedObject(const tiny_string& name, ByteArray* data) override { return false; }
	bool flushSharedObject(const tiny_string& name, ByteArray* data) override { return false; }
	void removeSharedObject(const tiny_string& name) override {}

	void grabFocus() override {}
	void openPageInBrowser(const tiny_string& url, const tiny_string& window) override {}
	void setDisplayState(const tiny_string& displaystate, SystemState *sys) override {}
	bool inFullScreenMode() override { return false; }

	// context menu handling
	void openContextMenu() override {}
	void updateContextMenu(int newselecteditem) override {}
	void updateContextMenuFromMouse(uint32_t windowID, int mousey) override {}
	void renderContextMenu() override {}

	// file handling
	bool FileExists(SystemState* sys,const tiny_string& filename, bool isfullpath) override;
	bool FileIsHidden(SystemState* sys,const tiny_string& filename, bool isfullpath) override;
	bool FileIsDirectory(SystemState* sys,const tiny_string& filename, bool isfullpath) override;
	uint32_t FileSize(SystemState* sys,const tiny_string& filename, bool isfullpath) override;
	tiny_string FileFullPath(SystemState* sys,const tiny_string& filename) override;
	tiny_string FileBasename(SystemState* sys,const tiny_string& filename, bool isfullpath) override;
	tiny_string FileRead(SystemState* sys,const tiny_string& filename, bool isfullpath) override;
	void FileWrite(SystemState* sys,const tiny_string& filename, const tiny_string& data, bool isfullpath) override;
	uint8_t FileReadUnsignedByte(SystemState* sys,const tiny_string &filename, uint32_t startpos, bool isfullpath) override;
	void FileReadByteArray(SystemState* sys,const tiny_string &filename,ByteArray* res, uint32_t startpos, uint32_t length, bool isfullpath) override;
	void FileWriteByteArray(SystemState* sys,const tiny_string& filename, ByteArray* data,uint32_t startpos, uint32_t length, bool isfullpath) override;
	bool FileCreateDirectory(SystemState* sys,const tiny_string& filename, bool isfullpath) override;
	bool FilGetDirectoryListing(SystemState* sys, const tiny_string &filename, bool isfullpath, std::vector<tiny_string>& filelist) override;
	bool FilePathIsAbsolute(const tiny_string& filename) override;
	tiny_string FileGetApplicationStorageDir() override { return ""; }

	void setClipboardText(const std::string txt) override;
	bool getScreenData(SDL_DisplayMode* screen) override { return true; }
	double getScreenDPI() override;
	void setWindowPosition(int x, int y, uint32_t width, uint32_t height) override {}
	void setWindowPosition(const Vector2& pos, const Vector2& size) override {}
	void getWindowPosition(int* x, int* y) override
	{
		(void)x, (void)y;
		x = y = 0;
	}
	Vector2 getWindowPosition() override { return Vector2(); }

	// TODO: Implement audio support, for `soundComplete` events.
	int audio_StreamInit(AudioStream* s) override { return 0; }
	void audio_StreamPause(int channel, bool dopause) override {}
	void audio_StreamDeinit(int channel) override {}
	bool audio_ManagerInit() override { return false; }
	void audio_ManagerCloseMixer(AudioManager* manager) override {}
	bool audio_ManagerOpenMixer(AudioManager* manager) override { return false; }
	void audio_ManagerDeinit() override {}
	int audio_getSampleRate() override { return 44100; }
	bool audio_useFloatSampleFormat() override { return true; }
};

#endif /* FRAMEWORK_BACKENDS_ENGINEUTILS_H */
