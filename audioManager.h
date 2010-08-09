/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010 Alexandre Demers (papouta@hotmail.com)

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

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "compat.h"
#include "decoder.h"
#include <string.h>
#include <boost/filesystem.hpp>

#include "plugins/audio/IAudioPlugin.h"

using namespace std;
using namespace boost::filesystem;

//convenience typedef for the pointers to the 2 functions we expect to find in the plugin libraries
typedef IPlugin *(*PLUGIN_FACTORY)();
typedef void (*PLUGIN_CLEANUP)(IPlugin *);

namespace lightspark
{

class AudioManager
{
  private:
    class PluginInfo
    {
//      private:
      public:
	string plugin_name;		//plugin_name
	string audiobackend_name;	//audiobackend
	path plugin_path;		//full path to the plugin file
	bool enabled;			//should it be enabled (loaded)?
	HMODULE hAudioPlugin;		//if enabled, this is the handle to the loaded lib
//	PluginInfo *PreviousPluginLib;
//	PluginInfo *NextPluginLib;
//      public:
//	PluginInfo();
//	~PluginInfo();
    };
    HMODULE hSelectedAudioPluginLib;
    vector<PluginInfo> AudioPluginsList;
    PluginInfo *SelectedAudioPlugin;
    
    IAudioPlugin *o_AudioPlugin;
    void AddAudioPluginToList(string AudioBackend_name, string pathToPlugin);
    void FindAudioPlugins();
    void LoadAudioPlugin();

  public:
    AudioManager();
    uint32_t createStreamPlugin(AudioDecoder *decoder);
    void freeStreamPlugin(uint32_t id);
    void fillPlugin(uint32_t id);
    void stopPlugin();
    bool isTimingAvailablePlugin() const;
    uint32_t getPlayedTimePlugin(uint32_t streamId);
    void select_audiobackend();
    ~AudioManager();
};

};

#endif
