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

class AudioStream
{
public:
	virtual uint32_t getPlayedTime() = 0;
	virtual void fill() = 0;
	virtual ~AudioStream() {};
};

/**********************
Abstract class for audio plugin implementation
***********************/
class IAudioPlugin : public IPlugin
{
protected:
	string playbackDeviceName;
	string captureDeviceName;
	vector<string *> playbackDevicesList;
	vector<string *> captureDevicesList;
	vector<AudioStream *> streams;
	volatile bool contextReady;
	volatile bool noServer;
	bool stopped;
	IAudioPlugin ( string plugin_name, string backend_name, bool init_stopped = false );

public:
	enum DEVICE_TYPES { PLAYBACK, CAPTURE };
	virtual vector<string *> *get_devicesList ( DEVICE_TYPES desiredType );
	virtual void set_device ( string desiredDevice, DEVICE_TYPES desiredType ) = 0;
	virtual string get_device ( DEVICE_TYPES desiredType );
	virtual AudioStream *createStream ( lightspark::AudioDecoder *decoder ) = 0;
	virtual void freeStream ( AudioStream *audioStream ) = 0;
	virtual bool isTimingAvailable() const = 0;
	virtual ~IAudioPlugin();
};

#endif

