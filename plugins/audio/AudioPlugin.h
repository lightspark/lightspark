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

#ifndef AUDIOPLUGIN_H
#define AUDIOPLUGIN_H

#include "../../compat.h"
#include "../../decoder.h"
#include "IAudioPlugin.h"

using namespace std;

/**********************
Abstract class for audio plugin implementation
***********************/
class AudioPlugin : public IAudioPlugin
{
/*  protected:
    class AudioStream; //Will be implemented per plugin
    const string audiobackend_name;
    PLUGIN_TYPES pluginType;
    vector<AudioStream*> streams;
    volatile bool contextReady;
    volatile bool noServer;
    bool stopped;
*/  public:
    AudioPlugin();
    const string get_audioBackend_name();
    bool get_serverStatus();
    PLUGIN_TYPES get_pluginType();
    const string get_pluginName();
    bool Is_ContextReady();
    bool Is_Stopped();
//    virtual uint32_t createStream(lightspark::AudioDecoder *decoder) = 0;
//    virtual void freeStream(uint32_t id) = 0;
//    virtual void fill(uint32_t id) = 0;
//    virtual void stop() = 0;
    bool isTimingAvailable() const;
//    virtual uint32_t getPlayedTime(uint32_t streamId) = 0;
//    virtual ~AudioPlugin() = 0;
};

#endif