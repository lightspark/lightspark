/**************************************************************************
  Lightspark, a free flash player implementation

  Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "platforms/fastpaths.h"
#include <inttypes.h>

void lightspark::fastYUV420ChannelsToYUV0Buffer(uint8_t* y, uint8_t* u, uint8_t* v, uint8_t* out, uint32_t width, uint32_t height)
{
	for(uint32_t i=0;i<height;i++)
	{
		for(uint32_t j=0;j<width;j++)
		{
			uint32_t pixelCoordFull=i*width+j;
			uint32_t pixelCoordHalf=(i/2)*(width/2)+(j/2);
			out[pixelCoordFull*4+0]=y[pixelCoordFull];
			out[pixelCoordFull*4+1]=u[pixelCoordHalf];
			out[pixelCoordFull*4+2]=v[pixelCoordHalf];
			out[pixelCoordFull*4+3]=0xff;
		}
	}
}

