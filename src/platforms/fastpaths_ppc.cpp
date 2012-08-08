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
#include <cinttypes>
#include <altivec.h>

void lightspark::fastYUV420ChannelsToYUV0Buffer(uint8_t* y, uint8_t* u, uint8_t* v, uint8_t* out, uint32_t width, uint32_t height)
{
	printf("HEy!");
	vector unsigned char y_vec;
	vector unsigned char u_vec;
	vector unsigned char v_vec;
	static vector unsigned char z_vec = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	vector unsigned char vv_vec;
	vector unsigned char uu_vec;
	vector unsigned char yv_vec;
	vector unsigned char u0_vec;

	unsigned char *p_y = y;
	unsigned char *p_u = u;
	unsigned char *p_v = v;
	unsigned char *p_out = out;

	unsigned int i, j;

	for(i=0; i<height; i++) {
		if (i!= 0 && i%2 == 0) { // Not clear why
			p_u-=width/2;
			p_v-=width/2;
		}
		for (j=0; j<width; j=j+16) {
			if (j%32 == 0) {
				u_vec = vec_ld(0, p_u); p_u += 16;
				v_vec = vec_ld(0, p_v); p_v += 16;

				vv_vec = vec_mergeh(v_vec, v_vec);
				uu_vec = vec_mergeh(u_vec, u_vec);
			} else {
				vv_vec = vec_mergel(v_vec, v_vec);
				uu_vec = vec_mergel(u_vec, u_vec);
			}
			y_vec = vec_ld(0, p_y); p_y += 16;

			// Do YVV and UU0 High part
			yv_vec = vec_mergeh(y_vec, vv_vec);
			u0_vec = vec_mergeh(uu_vec, z_vec);

			vec_st(vec_mergeh(yv_vec, u0_vec), 0, p_out);
			p_out += 16;
			vec_st(vec_mergel(yv_vec, u0_vec), 0, p_out);
			p_out += 16;

			// Do YVV and UU0 Low part
			yv_vec = vec_mergel(y_vec, vv_vec);
			u0_vec = vec_mergel(uu_vec, z_vec);

			vec_st(vec_mergeh(yv_vec, u0_vec), 0, p_out);
			p_out += 16;
			vec_st(vec_mergel(yv_vec, u0_vec), 0, p_out);
			p_out += 16;
		}
	}
}
