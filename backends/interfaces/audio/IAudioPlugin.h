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

class AudioStream
{
  protected:
	AudioStream(lightspark::AudioDecoder *dec = NULL, bool initPause = false);

  public:
	lightspark::AudioDecoder *decoder;
	bool pause;	//Indicates whether the stream is paused as a result of pauseStream being called
	virtual bool paused() = 0;	//Is the stream paused? (corked)
	virtual bool isValid() = 0; //Is the stream alive, fully working?
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
	std::string playbackDeviceName;
	std::string captureDeviceName;
	std::vector<std::string *> playbackDevicesList;
	std::vector<std::string *> captureDevicesList;
	std::list<AudioStream *> streams;
	typedef std::list<AudioStream *>::iterator stream_iterator;
	volatile bool contextReady;
	volatile bool noServer;
	bool stopped;
	IAudioPlugin ( std::string plugin_name, std::string backend_name, bool init_stopped = false );

public:
	enum DEVICE_TYPES { PLAYBACK, CAPTURE };
	virtual std::vector<std::string *> *get_devicesList ( DEVICE_TYPES desiredType );
	virtual void set_device ( std::string desiredDevice, DEVICE_TYPES desiredType ) = 0;
	virtual std::string get_device ( DEVICE_TYPES desiredType );
	virtual AudioStream *createStream ( lightspark::AudioDecoder *decoder ) = 0;
	virtual void freeStream ( AudioStream *audioStream ) = 0;
	virtual void pauseStream( AudioStream *audioStream ) = 0;	//Pause the stream (stops time from running, cork)
	virtual void resumeStream( AudioStream *audioStream ) = 0;	//Resume the stream (restart time, uncork)
	virtual bool isTimingAvailable() const = 0;
	virtual ~IAudioPlugin();
};

#endif

