/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010,2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "rtmputils.h"
#include "swf.h"

#ifdef ENABLE_RTMP
#include <librtmp/rtmp.h>
#endif

using namespace lightspark;
using namespace std;

RTMPDownloader::RTMPDownloader(const tiny_string& _url, const tiny_string& _stream):
	ThreadedDownloader(_url, false), //No caching to file allowed
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
	//Allocate and initialize the RTMP context
	RTMP* rtmpCtx=RTMP_Alloc();
	RTMP_Init(rtmpCtx);
	//Build the URL for the library
	tiny_string rtmpUrl=url;
	rtmpUrl+=" playpath=";
	rtmpUrl+=stream;
	rtmpUrl+=" swfUrl=";
	rtmpUrl+=sys->getOrigin().getURL();
	rtmpUrl+=" swfVfy=1";
	//Setup url needs a char*, not a const char*...
	int urlLen=rtmpUrl.len();
	char* urlBuf=new char[urlLen+1];
	strncpy(urlBuf,rtmpUrl.raw_buf(),urlLen);
	int ret=RTMP_SetupURL(rtmpCtx, urlBuf);
	cout << "Setup " << ret << endl;
	//TODO: add return if fails
	ret=RTMP_Connect(rtmpCtx, NULL);
	cout << "Connect " << ret << endl;
	ret=RTMP_ConnectStream(rtmpCtx, 0);
	cout << "ConnectStream " << ret << endl;
	//TODO: implement unsafe buffer concept
	char buf[4096];
	RTMP_SetBufferMS(rtmpCtx, 3600000);
	RTMP_UpdateBufferMS(rtmpCtx);
	while(1)
	{
		//TODO: avoid the copy in the temporary buffer
		ret=RTMP_Read(rtmpCtx,buf,4096);
		cout << "DL " << ret << endl;
		if(ret==0 || hasFailed() || aborting)
			break;
		append((uint8_t*)buf,ret);
	}
	RTMP_Close(rtmpCtx);
	RTMP_Free(rtmpCtx);
	delete[] urlBuf;
#else
	//ENABLE_RTMP not defined
	LOG(LOG_ERROR,_("NET: RTMP not enabled in this build. Downloader will always fail."));
	setFailed();
	return;
#endif
	//Notify the downloader no more data should be expected
	setFinished();
}
