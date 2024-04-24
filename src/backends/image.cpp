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
#include <cstdio>
#include <cstring>

#include "logger.h"

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

#include <csetjmp>
#include "backends/image.h"

namespace lightspark
{

struct istream_source_mgr : public jpeg_source_mgr
{
	istream_source_mgr(std::istream& str) : input(str), data(nullptr), capacity(0) {}
	std::istream& input;
	char* data;
	int capacity;
};

static void init_source_nop(j_decompress_ptr cinfo)
{
	//do nothing
}

static boolean fill_input_buffer(j_decompress_ptr cinfo) {
	return FALSE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	jpeg_source_mgr *src = cinfo->src;
	src->next_input_byte = (const JOCTET*)((const char*)src->next_input_byte + num_bytes);
	src->bytes_in_buffer -= num_bytes;
}

static boolean resync_to_restart(j_decompress_ptr cinfo, int desired) {
	return TRUE;
}

static void term_source(j_decompress_ptr /*cinfo*/) {}

static void init_source_istream(j_decompress_ptr cinfo) {
	istream_source_mgr* src = static_cast<istream_source_mgr*>(cinfo->src);
	src->bytes_in_buffer=0;
}

static boolean fill_input_buffer_istream(j_decompress_ptr cinfo) {
	istream_source_mgr* src = static_cast<istream_source_mgr*>(cinfo->src);

	src->next_input_byte=(const JOCTET*)src->data;
	try
	{
		src->input.read(src->data, src->capacity);
	}
	catch(std::ios_base::failure& exc)
	{
		if(!src->input.eof())
			throw;
	}
	src->bytes_in_buffer=src->input.gcount();

	if(src->bytes_in_buffer==0)
	{
		// EOI marker
		src->data[0]=(char)0xff;
		src->data[1]=(char)0xd9;
		src->bytes_in_buffer=2;
	}

	return TRUE;
}

static void skip_input_data_istream(j_decompress_ptr cinfo, long num_bytes) {
	istream_source_mgr* src = static_cast<istream_source_mgr*>(cinfo->src);

	if(num_bytes<=0)
		return;

	if(num_bytes<=(long)src->bytes_in_buffer)
	{
		src->next_input_byte += num_bytes;
		src->bytes_in_buffer -= num_bytes;
	}
	else
	{
		try
		{
			int readcount = num_bytes-src->bytes_in_buffer;
			while (readcount)
			{
				int r = std::min(src->capacity,readcount);
				src->input.read(src->data,r);
				readcount-=r;
			}
		}
		catch(std::ios_base::failure& exc)
		{
			if(!src->input.eof())
				throw;
		}
		src->bytes_in_buffer=0;
	}
}

struct error_mgr : jpeg_error_mgr {
	jmp_buf jmpBuf;
};

void error_exit(j_common_ptr cinfo) {
	error_mgr* error = static_cast<error_mgr*>(cinfo->err);
	(*error->output_message) (cinfo);
	/* Let the memory manager delete any temp files before we die */
	jpeg_destroy(cinfo);
	longjmp(error->jmpBuf, -1);
}

uint8_t* ImageDecoder::decodeJPEG(uint8_t* inData, int len, const uint8_t* tablesData, int tablesLen, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	struct jpeg_source_mgr src;

	src.next_input_byte = (const JOCTET*)inData;
	src.bytes_in_buffer = len;
	src.init_source = init_source_nop;
	src.fill_input_buffer = fill_input_buffer;
	src.skip_input_data = skip_input_data;
	src.resync_to_restart = resync_to_restart;
	src.term_source = term_source;

	struct jpeg_source_mgr *tablesSrc;
	if (tablesData)
	{
		tablesSrc = new jpeg_source_mgr();

		tablesSrc->next_input_byte = (const JOCTET*)tablesData;
		tablesSrc->bytes_in_buffer = tablesLen;
		tablesSrc->init_source = init_source_nop;
		tablesSrc->fill_input_buffer = fill_input_buffer;
		tablesSrc->skip_input_data = skip_input_data;
		tablesSrc->resync_to_restart = resync_to_restart;
		tablesSrc->term_source = term_source;
	}
	else
	{
		tablesSrc = nullptr;
	}

	*width = 0;
	*height = 0;
	uint8_t* decoded = decodeJPEGImpl(&src, tablesSrc, width, height, hasAlpha);
	delete tablesSrc;
	return decoded;
}

uint8_t* ImageDecoder::decodeJPEG(std::istream& str, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	struct istream_source_mgr src(str);

	src.capacity=4096;
	src.data=new char[src.capacity];
	src.init_source = init_source_istream;
	src.fill_input_buffer = fill_input_buffer_istream;
	src.skip_input_data = skip_input_data_istream;
	src.resync_to_restart = jpeg_resync_to_restart;
	src.term_source = term_source;

	uint8_t* res=decodeJPEGImpl(&src, nullptr, width, height, hasAlpha);

	delete[] src.data;
	return res;
}

uint8_t* ImageDecoder::decodeJPEGImpl(jpeg_source_mgr *src, jpeg_source_mgr *headerTables, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	struct jpeg_decompress_struct cinfo;
	struct error_mgr err;

	cinfo.err = jpeg_std_error(&err);
	err.error_exit = error_exit;

	if (setjmp(err.jmpBuf)) {
		return nullptr;
	}

	jpeg_create_decompress(&cinfo);
	
	if (headerTables)
		cinfo.src = headerTables;
	else
		cinfo.src = src;

	//DefineBits tag may contain "abbreviated datastreams" (as
	//they are called in the libjpeg documentation), i.e. streams
	//with only compression tables and no image data. The first
	//jpeg_read_header accepts table-only streams.
	int headerStatus = jpeg_read_header(&cinfo, FALSE);

	if (headerTables)
	{
		// Must call init_source manually after switching src
		// Check this. Doesn't jpeg_read_header call
		// init_source anyway?
		cinfo.src = src;
		src->init_source(&cinfo);
	}

	//If the first jpeg_read_header got tables-only datastream,
	//a second call is needed to read the real image header.
	if (headerStatus == JPEG_HEADER_TABLES_ONLY) 
		jpeg_read_header(&cinfo, TRUE);

#ifdef JCS_EXTENSIONS
	//JCS_EXT_XRGB is a fast decoder that outputs alpha channel,
	//but is only available on libjpeg-turbo
	cinfo.out_color_space = JCS_EXT_XRGB;
	cinfo.output_components = 4;
#endif
	jpeg_start_decompress(&cinfo);

	*width = cinfo.output_width;
	*height = cinfo.output_height;
	if(cinfo.num_components != 3)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Only RGB JPEG's are supported");
		/* TODO: is this the right thing for aborting? */
		jpeg_abort_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return nullptr;
	}
	if (cinfo.output_components != 3 && cinfo.output_components != 4)
	{
		LOG(LOG_ERROR,"invalid number of output components for jpeg:"<<cinfo.output_components);
		jpeg_abort_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return nullptr;
	}

	*hasAlpha = (cinfo.output_components == 4);
	
	uint64_t rowstride = cinfo.output_width * cinfo.output_components;
	if(uint64_t(cinfo.output_height) * rowstride == 0 || uint64_t(cinfo.output_height) * rowstride >= UINT32_MAX)
	{
		LOG(LOG_ERROR,"invalid width/height for jpeg:"<<cinfo.output_width<<"/"<<cinfo.output_height);
		jpeg_abort_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return nullptr;
	}
	JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, rowstride, 1);

	uint8_t* outData = new uint8_t[cinfo.output_height * rowstride];

	/* read one scanline at a time */
	int y=0;
	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(&outData[y*rowstride], buffer[0], rowstride);
		y++;
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return outData;
}

/* PNG handling */

struct png_image_buffer
{
	uint8_t* data;
	int curpos;
};

static void ReadPNGDataFromStream(png_structp pngPtr, png_bytep data, png_size_t length)
{
	png_voidp a = png_get_io_ptr(pngPtr);
	((std::istream*)a)->read((char*)data, length);
}
static void ReadPNGDataFromBuffer(png_structp pngPtr, png_bytep data, png_size_t length)
{
	png_image_buffer* a = reinterpret_cast<png_image_buffer*>(png_get_io_ptr(pngPtr));

	memcpy(data,(void*)(a->data+a->curpos),length);
	a->curpos+= length;
}
uint8_t* ImageDecoder::decodePNG(uint8_t* inData, int len, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!pngPtr)
	{
		LOG(LOG_ERROR,"Couldn't initialize png read struct");
		return nullptr;
	}
	png_image_buffer b;
	b.data = inData;
	b.curpos = 0;
	png_set_read_fn(pngPtr,(void*)&b, ReadPNGDataFromBuffer);

	return decodePNGImpl(pngPtr, width, height,hasAlpha);
}

uint8_t* ImageDecoder::decodePNG(std::istream& str, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!pngPtr)
	{
		LOG(LOG_ERROR,"Couldn't initialize png read struct");
		return nullptr;
	}
	png_set_read_fn(pngPtr,(void*)&str, ReadPNGDataFromStream);

	return decodePNGImpl(pngPtr, width, height,hasAlpha);
}

uint8_t* ImageDecoder::decodePNGImpl(png_structp pngPtr, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	png_bytep* rowPtrs = nullptr;
	uint8_t* outData = nullptr;
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
	{
		LOG(LOG_ERROR,"Couldn't initialize png info struct");
		png_destroy_read_struct(&pngPtr, (png_infopp)0, (png_infopp)0);
		return nullptr;
	}

	if (setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);
		if (rowPtrs != nullptr) delete [] rowPtrs;
		if (outData != nullptr) delete [] outData;

		LOG(LOG_ERROR,"error during reading of the png file");

		return nullptr;
	}

	png_read_info(pngPtr, infoPtr);

	*width =  png_get_image_width(pngPtr, infoPtr);
	*height = png_get_image_height(pngPtr, infoPtr);

	//bits per CHANNEL! note: not per pixel!
	png_uint_32 bitdepth = png_get_bit_depth(pngPtr, infoPtr);
	//Color type. (RGB, RGBA, Luminance, luminance alpha... palette... etc)
	png_uint_32 color_type = png_get_color_type(pngPtr, infoPtr);

	// Transform everything into 24 bit RGB
	switch (color_type)
	{
		case PNG_COLOR_TYPE_PALETTE:
		{
			png_set_palette_to_rgb(pngPtr);
			
			png_bytep trans_alpha = nullptr;;
			int num_trans = 0;
			png_color_16p trans_color = nullptr;
			png_get_tRNS(pngPtr, infoPtr, &trans_alpha, &num_trans, &trans_color);
			*hasAlpha = trans_alpha != nullptr;
			break;
		}
		case PNG_COLOR_TYPE_GRAY:
			if (bitdepth < 8)
				png_set_gray_to_rgb(pngPtr);
			*hasAlpha = false;
			break;
		default:
			// libpng also returns ARGB32 for RGB images without alpha channel
			*hasAlpha = true;
			break;
	}

	if (bitdepth == 16)
	{
		png_set_strip_16(pngPtr);
	}

	// Update the infoPtr to reflect the transformations set
	// above. Read new values by calling png_get_* again.
	png_read_update_info(pngPtr, infoPtr);

	//bitdepth = png_get_bit_depth(pngPtr, infoPtr);
	//color_type = png_get_color_type(pngPtr, infoPtr);

	const unsigned int stride = png_get_rowbytes(pngPtr, infoPtr);

	outData = new uint8_t[(*height) * stride];
	rowPtrs = new png_bytep[(*height)];
	for (size_t i = 0; i < (*height); i++)
	{
		rowPtrs[i] = (png_bytep)outData + i* stride;
	}

	png_read_image(pngPtr, rowPtrs);
	png_read_end(pngPtr, nullptr);
	png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);
	delete[] (png_bytep)rowPtrs;

	return outData;
}

uint8_t* ImageDecoder::decodePalette(uint8_t* pixels, uint32_t width, uint32_t height, uint32_t stride, uint8_t* palette, unsigned int numColors, unsigned int paletteBPP)
{
	if (numColors == 0)
		return nullptr;

	assert(stride >= width);
	assert(paletteBPP==3 || paletteBPP==4);

	uint8_t* outData = new uint8_t[paletteBPP*width*height];
	for (size_t y=0; y<height; y++)
	{
		for (size_t x=0; x<width; x++)
		{
			size_t pixelPos = y*stride + x;
			uint8_t paletteIndex = pixels[pixelPos];
			if ((unsigned)paletteIndex >= numColors)
			{
				// Invalid palette index
				paletteIndex = 0;
			}

			uint8_t *dest = outData + paletteBPP*(y*width + x);
			if (paletteBPP == 4)
			{
				// convert color from rgba to argb;
				uint32_t color_argb = (uint32_t)(palette[paletteBPP*paletteIndex+3]) | palette[paletteBPP*paletteIndex] <<8| palette[paletteBPP*paletteIndex+1] <<16 | palette[paletteBPP*paletteIndex+2] <<24;
				memcpy(dest, &color_argb, paletteBPP);
			}
			else
				memcpy(dest, &palette[paletteBPP*paletteIndex], paletteBPP);
		}
	}

	return outData;
}

}
