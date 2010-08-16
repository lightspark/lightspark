/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef AUDIO_H
#define AUDIO_H

#ifdef ENABLE_SOUND

#include "compat.h"
#include "decoder.h"
#include <string.h>
#include <boost/filesystem.hpp>

#include "interfaces/audio/IAudioPlugin.h"

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
    vector<string *>audioplugins_list;
    string SelectedAudioPlugin;
    HMODULE hSelectedAudioPluginLib;
    IAudioPlugin *o_AudioPlugin;
    void get_audioplugins_list();
    void refresh_audioplugins_list();
    void load_audioplugin();
    void release_audioplugin();

  public:
    AudioManager();
    uint32_t createStreamPlugin(AudioDecoder *decoder);
    void freeStreamPlugin(uint32_t id);
    void fillPlugin(uint32_t id);
    void stopPlugin();
    bool isTimingAvailablePlugin() const;
    uint32_t getPlayedTimePlugin(uint32_t streamId);
    void select_audiobackend(string selected_backend);
    ~AudioManager();
};

};

#endif
#endif
