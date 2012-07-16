/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Matthias Gehre (M.Gehre@gmx.de)

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
#ifndef BACKENDS_IMAGE_H 
#define BACKENDS_IMAGE_H 1

#include <cstdint>
#include <istream>

extern "C" {
#include <jpeglib.h>
#define PNG_SKIP_SETJMP_CHECK
#include <png.h>
}

namespace lightspark
{

class ImageDecoder
{
private:
	static uint8_t* decodeJPEGImpl(jpeg_source_mgr& src, uint32_t* width, uint32_t* height, bool* hasAlpha);
	static uint8_t* decodePNGImpl(png_structp pngPtr, uint32_t* width, uint32_t* height);
public:
	/*
	 * Returns a new[]'ed array of decompressed data and sets width, height and format
	 * Return NULL on error
	 */
	static uint8_t* decodeJPEG(uint8_t* inData, int len, uint32_t* width, uint32_t* height, bool* hasAlpha);
	static uint8_t* decodeJPEG(std::istream& str, uint32_t* width, uint32_t* height, bool* hasAlpha);
	static uint8_t* decodePNG(uint8_t* inData, int len, uint32_t* width, uint32_t* height);
	static uint8_t* decodePNG(std::istream& str, uint32_t* width, uint32_t* height);
};

}

#endif /* BACKENDS_IMAGE_H */
