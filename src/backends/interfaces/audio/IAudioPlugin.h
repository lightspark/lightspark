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

#ifndef BACKENDS_INTERFACES_AUDIO_IAUDIOPLUGIN_H
#define BACKENDS_INTERFACES_AUDIO_IAUDIOPLUGIN_H 1

#include "compat.h"
#include "backends/decoder.h"
#include "backends/interfaces/IPlugin.h"
#include <iostream>

class AudioStream
{
  protected:
	AudioStream(lightspark::AudioDecoder *dec = NULL);

  public:
	lightspark::AudioDecoder *decoder;
	virtual bool ispaused() = 0;	//Is the stream paused? (corked)
	virtual bool isValid() = 0; //Is the stream alive, fully working?
	virtual void pause() = 0;
	virtual void resume() = 0;
	virtual uint32_t getPlayedTime() = 0;
	virtual ~AudioStream() {};
	virtual void setVolume(double volume)
		{LOG(LOG_NOT_IMPLEMENTED,"setVolume not implemented in plugin");}
};

/**********************
Abstract class for audio plugin implementation
***********************/
class IAudioPlugin : public IPlugin
{
protected:
	bool stopped;
	bool muteAllStreams;
	volatile bool contextReady;
	volatile bool noServer;
	std::string playbackDeviceName;
	std::string captureDeviceName;
	std::vector<std::string *> playbackDevicesList;
	std::vector<std::string *> captureDevicesList;
	std::list<AudioStream *> streams;
	typedef std::list<AudioStream *>::iterator stream_iterator;

	IAudioPlugin ( std::string plugin_name, std::string backend_name, bool init_stopped = false );

public:
	enum DEVICE_TYPES { PLAYBACK, CAPTURE };
	virtual std::vector<std::string *> *get_devicesList ( DEVICE_TYPES desiredType );
	virtual void set_device ( std::string desiredDevice, DEVICE_TYPES desiredType ) = 0;
	virtual std::string get_device ( DEVICE_TYPES desiredType );
	virtual AudioStream *createStream ( lightspark::AudioDecoder *decoder ) = 0;
	virtual bool isTimingAvailable() const = 0;

	virtual void muteAll() { muteAllStreams = true; }
	virtual void unmuteAll() { muteAllStreams = false; }
	virtual void toggleMuteAll() { muteAllStreams ? unmuteAll() : muteAll(); }
	virtual bool allMuted() { return muteAllStreams; }

	virtual ~IAudioPlugin();
};

#endif /* BACKENDS_INTERFACES_AUDIO_IAUDIOPLUGIN */

