/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "backends/rtmputils.h"
#include "logger.h"
#include "swf.h"
#include "backends/streamcache.h"

#ifdef ENABLE_RTMP
#include <librtmp/rtmp.h>
#endif

using namespace lightspark;
using namespace std;

RTMPDownloader::RTMPDownloader(const tiny_string& _url, _R<StreamCache> _cache,
			       const tiny_string& _stream, ILoadable* o):
	ThreadedDownloader(_url, _cache, o),
	stream(_stream)
{
}

void RTMPDownloader::threadAbort()
{
	Downloader::stop();
}

void RTMPDownloader::execute()
{
#ifdef ENABLE_RTMP
	bool downloadFailed=true;
	//Allocate and initialize the RTMP context
	RTMP* rtmpCtx=RTMP_Alloc();
	RTMP_Init(rtmpCtx);
	//Build the URL for the library
	tiny_string rtmpUrl=url;
	rtmpUrl+=" playpath=";
	rtmpUrl+=stream;
	rtmpUrl+=" swfVfy=";
	rtmpUrl+=getSys()->mainClip->getOrigin().getURL();
	rtmpUrl+=" tcUrl=";
	rtmpUrl+=url;
	//Setup url needs a char*, not a const char*...
	int urlLen=rtmpUrl.numBytes();
	char* urlBuf=new char[urlLen+1];
	strncpy(urlBuf,rtmpUrl.raw_buf(),urlLen+1);
	int ret=RTMP_SetupURL(rtmpCtx, urlBuf);
	LOG(LOG_TRACE, "RTMP_SetupURL " << rtmpUrl << " " << ret);
	if(!ret)
	{
		setFailed();
		goto cleanup;
	}
	ret=RTMP_Connect(rtmpCtx, NULL);
	LOG(LOG_TRACE, "Connect_Connect " << ret);
	if(!ret)
	{
		setFailed();
		goto cleanup;
	}
	ret=RTMP_ConnectStream(rtmpCtx, 0);
	LOG(LOG_TRACE, "RTMP_ConnectStream " << ret);
	if(!ret)
	{
		setFailed();
		goto cleanup;
	}
	//TODO: implement unsafe buffer concept
	char buf[4096];
	RTMP_SetBufferMS(rtmpCtx, 3600000);
	RTMP_UpdateBufferMS(rtmpCtx);
	// looping conditions copied from rtmpdump sources
	do
	{
		//TODO: avoid the copy in the temporary buffer
		ret=RTMP_Read(rtmpCtx,buf,4096);
		if(ret>0)
			append((uint8_t*)buf,ret);
	}
	while(ret >= 0 && !threadAborting && !hasFailed() && RTMP_IsConnected(rtmpCtx) && !RTMP_IsTimedout(rtmpCtx));

	// Apparently negative ret indicates error only if
	// m_read.status isn't a success code, see rtmpdump sources
	downloadFailed=ret < 0 && rtmpCtx->m_read.status != RTMP_READ_COMPLETE;
	if(downloadFailed || RTMP_IsTimedout(rtmpCtx))
		setFailed();

cleanup:
	RTMP_Close(rtmpCtx);
	RTMP_Free(rtmpCtx);
	delete[] urlBuf;
#else
	//ENABLE_RTMP not defined
	LOG(LOG_ERROR,"NET: RTMP not enabled in this build. Downloader will always fail.");
	setFailed();
	return;
#endif
	//Notify the downloader no more data should be expected. Do
	//this only if setFailed() hasn't been called above.
	if(!hasFinished())
		setFinished();
}
