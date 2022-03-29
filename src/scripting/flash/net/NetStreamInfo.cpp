/**************************************************************************
    Lightspark, a free flash player implementation

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

#include "scripting/flash/net/NetStreamInfo.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

NetStreamInfo::NetStreamInfo(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
	,audioBufferByteLength(-1)
	,audioBufferLength(-1)
	,audioByteCount(-1)
	,audioBytesPerSecond(-1)
	,audioLossRate(-1)
	,byteCount(-1)
	,currentBytesPerSecond(-1)
	,dataBufferByteLength(-1)
	,dataBufferLength(-1)
	,dataByteCount(-1)
	,dataBytesPerSecond(-1)
	,droppedFrames(0)
	,isLive(false)
	,maxBytesPerSecond(-1)
	,metaData(NULL)
	,playbackBytesPerSecond(-1)
	,SRTT(-1)
	,videoBufferByteLength(-1)
	,videoBufferLength(-1)
	,videoByteCount(-1)
	,videoBytesPerSecond(-1)
	,videoLossRate(-1)
	,xmpData(NULL)
{
}
void NetStreamInfo::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL|CLASS_SEALED);
	REGISTER_GETTER(c,audioBufferByteLength);
	REGISTER_GETTER(c,audioBufferLength);
	REGISTER_GETTER(c,audioByteCount);
	REGISTER_GETTER(c,audioBytesPerSecond);
	REGISTER_GETTER(c,audioLossRate);
	REGISTER_GETTER(c,byteCount);
	REGISTER_GETTER(c,currentBytesPerSecond);
	REGISTER_GETTER(c,dataBufferByteLength);
	REGISTER_GETTER(c,dataBufferLength);
	REGISTER_GETTER(c,dataByteCount);
	REGISTER_GETTER(c,dataBytesPerSecond);
	REGISTER_GETTER(c,droppedFrames);
	REGISTER_GETTER(c,isLive);
	REGISTER_GETTER(c,maxBytesPerSecond);
	REGISTER_GETTER(c,metaData);
	REGISTER_GETTER(c,playbackBytesPerSecond);
	REGISTER_GETTER(c,resourceName);
	REGISTER_GETTER(c,SRTT);
	REGISTER_GETTER(c,uri);
	REGISTER_GETTER(c,videoBufferByteLength);
	REGISTER_GETTER(c,videoBufferLength);
	REGISTER_GETTER(c,videoByteCount);
	REGISTER_GETTER(c,videoBytesPerSecond);
	REGISTER_GETTER(c,videoLossRate);
	REGISTER_GETTER(c,xmpData);
}

ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,audioBufferByteLength);
ASFUNCTIONBODY_GETTER(NetStreamInfo,audioBufferLength);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,audioByteCount);
ASFUNCTIONBODY_GETTER(NetStreamInfo,audioBytesPerSecond);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,audioLossRate);
ASFUNCTIONBODY_GETTER(NetStreamInfo,byteCount);
ASFUNCTIONBODY_GETTER(NetStreamInfo,currentBytesPerSecond);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,dataBufferByteLength);
ASFUNCTIONBODY_GETTER(NetStreamInfo,dataBufferLength);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,dataByteCount);
ASFUNCTIONBODY_GETTER(NetStreamInfo,dataBytesPerSecond);
ASFUNCTIONBODY_GETTER(NetStreamInfo,droppedFrames);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,isLive);
ASFUNCTIONBODY_GETTER(NetStreamInfo,maxBytesPerSecond);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,metaData);
ASFUNCTIONBODY_GETTER(NetStreamInfo,playbackBytesPerSecond);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,resourceName);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,SRTT);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,uri);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,videoBufferByteLength);
ASFUNCTIONBODY_GETTER(NetStreamInfo,videoBufferLength);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,videoByteCount);
ASFUNCTIONBODY_GETTER(NetStreamInfo,videoBytesPerSecond);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,videoLossRate);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(NetStreamInfo,xmpData);

