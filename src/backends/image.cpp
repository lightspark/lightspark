/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011  Matthias Gehre (M.Gehre@gmx.de)

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
#include <stdio.h>
#include <string.h>

#include "logger.h"

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}

#include <setjmp.h>
#include "image.h"

namespace lightspark
{

struct source_mgr : public jpeg_source_mgr
{
	source_mgr(uint8_t* _data, int _len) : data(_data), len(_len) {}
	uint8_t* data;
	int len;
};

static void init_source(j_decompress_ptr cinfo) {
	source_mgr*  src = (source_mgr*)cinfo->src;
	src->next_input_byte = (const JOCTET*)src->data;
	src->bytes_in_buffer = src->len;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo) {
	return FALSE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	source_mgr*  src = (source_mgr*)cinfo->src;
	src->next_input_byte = (const JOCTET*)((const char*)src->next_input_byte + num_bytes);
	src->bytes_in_buffer -= num_bytes;
}

static boolean resync_to_restart(j_decompress_ptr cinfo, int desired) {
	return TRUE;
}

static void term_source(j_decompress_ptr /*cinfo*/) {}

struct error_mgr : jpeg_error_mgr {
	jmp_buf jmpBuf;
};

void error_exit(j_common_ptr cinfo) {
	error_mgr* error = (error_mgr*)cinfo->err;
	(*error->output_message) (cinfo);
	/* Let the memory manager delete any temp files before we die */
	jpeg_destroy(cinfo);
	longjmp(error->jmpBuf, -1);
}

uint8_t* ImageDecoder::decodeJPEG(uint8_t* inData, int len, uint32_t* width, uint32_t* height)
{
	struct jpeg_decompress_struct cinfo;
	struct error_mgr err;
	struct source_mgr src(inData,len);

	src.init_source = init_source;
	src.fill_input_buffer = fill_input_buffer;
	src.skip_input_data = skip_input_data;
	src.resync_to_restart = resync_to_restart;
	src.term_source = term_source;

	cinfo.err = jpeg_std_error(&err);
	err.error_exit = error_exit;

	if (setjmp(err.jmpBuf)) {
		return NULL;
	}

	jpeg_create_decompress(&cinfo);
	cinfo.src = &src;
	jpeg_read_header(&cinfo, TRUE);
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

}
