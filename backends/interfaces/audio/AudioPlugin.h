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

#include "../../../compat.h"
#include "../../decoder.h"
#include "IAudioPlugin.h"
#include <iostream>

using namespace std;

/**********************
Abstract class for audio plugin implementation
***********************/
class AudioPlugin : public IAudioPlugin
{
public:
        AudioPlugin ( PLUGIN_TYPES init_Type = AUDIO, string init_Name = "generic audio plugin",
                      string init_backend = "undefined", bool init_contextReady = false,
                      bool init_noServer = false, bool init_stopped = false );
        const string get_backendName();
        bool get_serverStatus();
        const PLUGIN_TYPES get_pluginType();
        const string get_pluginName();
        bool Is_ContextReady();
        bool Is_Stopped();
        bool isTimingAvailable() const;
        virtual ~AudioPlugin();
};

#endif
