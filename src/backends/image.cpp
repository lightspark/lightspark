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

struct source_mgr : public jpeg_source_mgr
{
	source_mgr(uint8_t* _data, int _len) : data(_data), len(_len) {}
	uint8_t* data;
	int len;
};

struct istream_source_mgr : public jpeg_source_mgr
{
	istream_source_mgr(std::istream& str) : input(str), data(NULL), capacity(0) {}
	std::istream& input;
	char* data;
	int capacity;
};

static void init_source(j_decompress_ptr cinfo) {
	source_mgr*  src = static_cast<source_mgr*>(cinfo->src);
	src->next_input_byte = (const JOCTET*)src->data;
	src->bytes_in_buffer = src->len;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo) {
	return FALSE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	source_mgr*  src = static_cast<source_mgr*>(cinfo->src);
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
			src->input.seekg(num_bytes-src->bytes_in_buffer, std::ios_base::cur);
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

uint8_t* ImageDecoder::decodeJPEG(uint8_t* inData, int len, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	struct source_mgr src(inData,len);

	src.init_source = init_source;
	src.fill_input_buffer = fill_input_buffer;
	src.skip_input_data = skip_input_data;
	src.resync_to_restart = resync_to_restart;
	src.term_source = term_source;

	return decodeJPEGImpl(src, width, height, hasAlpha);
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

	uint8_t* res=decodeJPEGImpl(src, width, height, hasAlpha);

	delete[] src.data;
	return res;
}

uint8_t* ImageDecoder::decodeJPEGImpl(jpeg_source_mgr& src, uint32_t* width, uint32_t* height, bool* hasAlpha)
{
	struct jpeg_decompress_struct cinfo;
	struct error_mgr err;

	cinfo.err = jpeg_std_error(&err);
	err.error_exit = error_exit;

	if (setjmp(err.jmpBuf)) {
		return NULL;
	}

	jpeg_create_decompress(&cinfo);
	cinfo.src = &src;
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
		return NULL;
	}
	assert(cinfo.output_components == 3 || cinfo.output_components == 4);

	*hasAlpha = (cinfo.output_components == 4);

	int rowstride = cinfo.output_width * cinfo.output_components;
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
uint8_t* ImageDecoder::decodePNG(uint8_t* inData, int len, uint32_t* width, uint32_t* height)
{
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr)
	{
		LOG(LOG_ERROR,"Couldn't initialize png read struct");
		return NULL;
	}
	png_set_read_fn(pngPtr,(void*)inData, ReadPNGDataFromBuffer);

	return decodePNGImpl(pngPtr, width, height);
}

uint8_t* ImageDecoder::decodePNG(std::istream& str, uint32_t* width, uint32_t* height)
{
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr)
	{
		LOG(LOG_ERROR,"Couldn't initialize png read struct");
		return NULL;
	}
	png_set_read_fn(pngPtr,(void*)&str, ReadPNGDataFromStream);

	return decodePNGImpl(pngPtr, width, height);
}

uint8_t* ImageDecoder::decodePNGImpl(png_structp pngPtr, uint32_t* width, uint32_t* height)
{
	png_bytep* rowPtrs = NULL;
	uint8_t* outData = NULL;
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
	{
		LOG(LOG_ERROR,"Couldn't initialize png info struct");
		png_destroy_read_struct(&pngPtr, (png_infopp)0, (png_infopp)0);
		return NULL;
	}

	if (setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);
		if (rowPtrs != NULL) delete [] rowPtrs;
		if (outData != NULL) delete [] outData;

		LOG(LOG_ERROR,"error during reading of the png file");

		return NULL;
	}

	png_read_info(pngPtr, infoPtr);

	*width =  png_get_image_width(pngPtr, infoPtr);
	*height = png_get_image_height(pngPtr, infoPtr);

	//bits per CHANNEL! note: not per pixel!
	png_uint_32 bitdepth = png_get_bit_depth(pngPtr, infoPtr);
	//Number of channels
	png_uint_32 channels = png_get_channels(pngPtr, infoPtr);
	//Color type. (RGB, RGBA, Luminance, luminance alpha... palette... etc)
	png_uint_32 color_type = png_get_color_type(pngPtr, infoPtr);

	// Transform everything into 24 bit RGB
	switch (color_type)
	{
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(pngPtr);
			// png_set_palette_to_rgb wil convert into 32
			// bit, but we don't want the alpha
			png_set_strip_alpha(pngPtr);
			break;
		case PNG_COLOR_TYPE_GRAY:
			if (bitdepth < 8)
				png_set_gray_to_rgb(pngPtr);
			break;
	}

	if (bitdepth == 16)
	{
		png_set_strip_16(pngPtr);
	}

	if (channels > 3)
	{
		LOG(LOG_NOT_IMPLEMENTED, "Alpha channel not supported in PNG");
		png_set_strip_alpha(pngPtr);
	}

	// Update the infoPtr to reflect the transformations set
	// above. Read new values by calling png_get_* again.
	png_read_update_info(pngPtr, infoPtr);

	//bitdepth = png_get_bit_depth(pngPtr, infoPtr);
	//color_type = png_get_color_type(pngPtr, infoPtr);

	channels = png_get_channels(pngPtr, infoPtr);
	if (channels != 3)
	{
		// Should never get here because of the
		// transformations
		LOG(LOG_NOT_IMPLEMENTED, "Unexpected number of channels in PNG!");

		png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);

		return NULL;
	}

	const unsigned int stride = png_get_rowbytes(pngPtr, infoPtr);

	outData = new uint8_t[(*height) * stride];
	rowPtrs = new png_bytep[(*height)];
	for (size_t i = 0; i < (*height); i++)
	{
		rowPtrs[i] = (png_bytep)outData + i* stride;
	}

	png_read_image(pngPtr, rowPtrs);
	png_read_end(pngPtr, NULL);
	png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);
	delete[] (png_bytep)rowPtrs;

	return outData;
}
}
