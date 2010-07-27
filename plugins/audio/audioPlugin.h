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
#include "../iplugin.h"

namespace lightspark
{

/**********************
Abstract class for audio plugin implementation
***********************/
class AudioPlugin : IPlugin
{
  private:
	enum STREAM_STATUS { STREAM_STARTING=0, STREAM_READY=1, STREAM_DEAD=2 };
	volatile STREAM_STATUS streamStatus;
	AudioDecoder *decoder;
/*	virtual class AudioStream
	{
	public:
//		enum STREAM_STATUS { STREAM_STARTING=0, STREAM_READY=1, STREAM_DEAD=2 };
		pa_stream *stream;
//		AudioDecoder *decoder;
		AudioPlugin *manager;
//		volatile STREAM_STATUS streamStatus;
		virtual AudioStream(AudioPlugin *m){}
	};
*///	pa_threaded_mainloop *mainLoop;
//	pa_context *context;
//	std::vector<AudioStream*> streams;
	char *audiobackend_name;
//	virtual static void contextStatusCB(pa_context *context, AudioPlugin *th);
//	virtual static void streamStatusCB(pa_stream *stream, AudioStream *th);
//	virtual static void streamWriteCB(pa_stream *stream, size_t nbytes, AudioStream *th);
	virtual void contextStatusCB(AudioPlugin *th);
	virtual void streamStatusCB();
	virtual void streamWriteCB(size_t nbytes);
  public:
	virtual const char *Get_AudioBackend_name();
	virtual uint32_t createStream(AudioDecoder *decoder);
	virtual void freeStream(uint32_t id);
	virtual void fillAndSync(uint32_t id, uint32_t streamTime);
	virtual void stop();
};

};

#endif