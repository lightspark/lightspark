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

#include "AudioPlugin.h"

using namespace lightspark;

AudioPlugin::AudioPlugin()
//  : pluginType(AUDIO) //Why? Oh, why?
{
}

const string AudioPlugin::get_audioBackend_name()
{
  return audiobackend_name;
}

bool AudioPlugin::get_serverStatus()
{
  return noServer;
}

bool AudioPlugin::Is_ContextReady()
{
  return contextReady;
}

bool AudioPlugin::Is_Stopped()
{
  return stopped;
}

const string AudioPlugin::get_pluginName()
{
 return pluginName;
}

PLUGIN_TYPES AudioPlugin::get_pluginType()
{
  return pluginType;
}
