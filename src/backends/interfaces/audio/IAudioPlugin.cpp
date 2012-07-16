/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include "backends/interfaces/audio/IAudioPlugin.h"
#include <iostream>

using namespace std;


IAudioPlugin::IAudioPlugin ( string plugin_name, string backend_name, bool init_stopped ):
	IPlugin(AUDIO, plugin_name, backend_name), stopped(init_stopped), muteAllStreams(false)
{

}

string IAudioPlugin::get_device ( DEVICE_TYPES desiredType )
{
	if ( PLAYBACK )
	{
		return playbackDeviceName;
	}
	else if ( CAPTURE )
	{
		return captureDeviceName;
	}
	else
	{
		return NULL;
	}
}

vector< string* > *IAudioPlugin::get_devicesList ( DEVICE_TYPES desiredType )
{
	if ( desiredType == PLAYBACK )
	{
		return &playbackDevicesList;
	}
	else if ( desiredType == CAPTURE )
	{
		return &captureDevicesList;
	}
	else
	{
		return NULL;
	}
}

IAudioPlugin::~IAudioPlugin()
{

}


AudioStream::AudioStream ( lightspark::AudioDecoder* dec ):
	decoder(dec)
{

}
