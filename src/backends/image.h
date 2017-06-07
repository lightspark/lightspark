/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Matthias Gehre (M.Gehre@gmx.de)

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
	static uint8_t* decodeJPEGImpl(jpeg_source_mgr *src, jpeg_source_mgr* headerTables, uint32_t* width, uint32_t* height, bool* hasAlpha);
	static uint8_t* decodePNGImpl(png_structp pngPtr, uint32_t* width, uint32_t* height, bool *hasAlpha);
public:
	/*
	 * Returns a new[]'ed array of decompressed data and sets width, height and format
	 * Return NULL on error
	 */
	static uint8_t* decodeJPEG(uint8_t* inData, int len, const uint8_t* tablesData, int tablesLen,
				   uint32_t* width, uint32_t* height, bool* hasAlpha);
	static uint8_t* decodeJPEG(std::istream& str, uint32_t* width, uint32_t* height, bool* hasAlpha);
	static uint8_t* decodePNG(uint8_t* inData, int len, uint32_t* width, uint32_t* height, bool *hasAlpha);
	static uint8_t* decodePNG(std::istream& str, uint32_t* width, uint32_t* height, bool *hasAlpha);
	/* Convert paletted image into new[]'ed 24bit RGB image.
	 * pixels array contains indexes to the palette, 1 byte per
	 * index. Palette has numColors RGB(A) values, paletteBPP (==
	 * 3 or 4) bytes per color. The alpha channel (present if
	 * paletteBPP == 4) is ignored.
	 */
	static uint8_t* decodePalette(uint8_t* pixels, uint32_t width, uint32_t height, uint32_t stride, uint8_t* palette, unsigned int numColors, unsigned int paletteBPP);
};

}

#endif /* BACKENDS_IMAGE_H */
