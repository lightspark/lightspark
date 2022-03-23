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

#ifndef NETSTREAMINFO_H
#define NETSTREAMINFO_H

#include "asobject.h"

namespace lightspark
{

class NetStreamInfo: public ASObject
{
public:
	NetStreamInfo(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASPROPERTY_GETTER(number_t,audioBufferByteLength);
	ASPROPERTY_GETTER(number_t,audioBufferLength);
	ASPROPERTY_GETTER(number_t,audioByteCount);
	ASPROPERTY_GETTER(number_t,audioBytesPerSecond);
	ASPROPERTY_GETTER(number_t,audioLossRate);
	ASPROPERTY_GETTER(number_t,byteCount);
	ASPROPERTY_GETTER(number_t,currentBytesPerSecond);
	ASPROPERTY_GETTER(number_t,dataBufferByteLength);
	ASPROPERTY_GETTER(number_t,dataBufferLength);
	ASPROPERTY_GETTER(number_t,dataByteCount);
	ASPROPERTY_GETTER(number_t,dataBytesPerSecond);
	ASPROPERTY_GETTER(number_t,droppedFrames);
	ASPROPERTY_GETTER(bool,isLive);
	ASPROPERTY_GETTER(number_t,maxBytesPerSecond);
	ASPROPERTY_GETTER(_NR<ASObject>,metaData);
	ASPROPERTY_GETTER(number_t,playbackBytesPerSecond);
	ASPROPERTY_GETTER(tiny_string,resourceName);
	ASPROPERTY_GETTER(number_t,SRTT);
	ASPROPERTY_GETTER(tiny_string,uri);
	ASPROPERTY_GETTER(number_t,videoBufferByteLength);
	ASPROPERTY_GETTER(number_t,videoBufferLength);
	ASPROPERTY_GETTER(number_t,videoByteCount);
	ASPROPERTY_GETTER(number_t,videoBytesPerSecond);
	ASPROPERTY_GETTER(number_t,videoLossRate);
	ASPROPERTY_GETTER(_NR<ASObject>,xmpData);
};

}
#endif // NETSTREAMInfo_H
