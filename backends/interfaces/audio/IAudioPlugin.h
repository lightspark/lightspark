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

#ifndef IAUDIOPLUGIN_H
#define IAUDIOPLUGIN_H

#include "../../../compat.h"
#include "../../decoder.h"
#include "../IPlugin.h"
#include <iostream>

using namespace std;

/**********************
Abstract class for audio plugin implementation
***********************/
class IAudioPlugin : public IPlugin
{
  protected:
    class AudioStream; //Will be implemented per plugin
    string audiobackend_name;
    vector<AudioStream*> streams;
    volatile bool contextReady;
    volatile bool noServer;
    bool stopped;
  public:
    IAudioPlugin(PLUGIN_TYPES init_Type = AUDIO, string init_Name = "undefined",
		 string init_audiobackend = "undefined", bool init_contextReady = false,
		 bool init_noServer = false, bool init_stopped = false);
    virtual bool Is_Connected() = 0;
    virtual const string get_audioBackend_name() = 0;
    virtual bool get_serverStatus() = 0;
    virtual PLUGIN_TYPES get_pluginType() = 0;
    virtual const string get_pluginName() = 0;
    virtual bool Is_ContextReady() = 0;
    virtual bool Is_Stopped() = 0;
    virtual uint32_t createStream(lightspark::AudioDecoder *decoder) = 0;
    virtual void freeStream(uint32_t id) = 0;
    virtual void fill(uint32_t id) = 0;
    virtual void stop() = 0;
    virtual bool isTimingAvailable() const = 0;
    /*
      Get the elapsed time in milliseconds for the stream streamId
    */
    virtual uint32_t getPlayedTime(uint32_t streamId) = 0;
    virtual ~IAudioPlugin() = 0;
};

#endif