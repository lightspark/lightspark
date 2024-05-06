/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef INTERFACES_BACKENDS_NETUTILS_H
#define INTERFACES_BACKENDS_NETUTILS_H 1

#include <cstdint>
#include "interfaces/threading.h"

namespace lightspark
{

class ILoadable
{
protected:
	~ILoadable(){}
public:
	virtual void setBytesTotal(uint32_t b) = 0;
	virtual void setBytesLoaded(uint32_t b) = 0;
};

class IDownloaderThreadListener
{
protected:
	virtual ~IDownloaderThreadListener() {}
public:
	virtual void threadFinished(IThreadJob*)=0;
};

}
#endif /* INTERFACES_BACKENDS_NETUTILS_H */
