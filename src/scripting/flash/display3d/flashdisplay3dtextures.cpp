#include "flashdisplay3dtextures.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "abc.h"
#include <lzma.h>
#include "3rdparty/jpegxr/jpegxr.h" // jpeg-xr decoding library taken from https://github.com/adobe/dds2atf/

using namespace lightspark;

enum LSJXRDATAFORMAT { RGB888,RGB8888,DXT5AlphaImageData,DXT5ImageData,DXT1ImageData};
struct lsjxrdata
{
	LSJXRDATAFORMAT dataformat;
	vector<uint8_t>* result;
};

void jpegxrcallback(jxr_image_t image, int mx, int my, int* data)
{
	lsjxrdata* imgdata =(lsjxrdata*)jxr_get_user_data(image);
	int32_t w = jxr_get_IMAGE_WIDTH(image);
	switch (imgdata->dataformat)
	{
		case DXT5AlphaImageData:
		{
			uint32_t pos=my*16*w+mx*16;
			for (uint32_t i=0; i < 16*16 && pos < imgdata->result->size(); i++)
			{
				(*imgdata->result)[pos++] = data[i]&0xff;
				if ((i+1)%(16)==0)
					pos += w-16;
			}
			break;
		}
		case DXT1ImageData:
		case DXT5ImageData:
		{
			uint32_t n=3;
			uint32_t pos=my*16*2*w+mx*16*2;
			for (uint32_t i=0; i < 16*16*n && pos < imgdata->result->size(); i+=n)
			{
				uint32_t r = data[i+2]&0x1f;
				uint32_t g = data[i+1]&0x3f;
				uint32_t b = data[i]&0x1f;
				// the r,g,b values are already computed to 5-6-5 format
				// convert to 2 byte rgb565 with the lower byte first
				(*imgdata->result)[pos++] = ((g<<5)&0xe0) | b;
				(*imgdata->result)[pos++] = ((r<<3)&0xf8) | ((g>>3)&0x07);
				if ((i+n)%(16*n)==0)
					pos += (w-16)*2;
			}
			break;
		}
		case RGB888:
		{
			uint32_t n=3;
			uint32_t pos=my*16*3*w+mx*16*3;
			for (uint32_t i=0; i < 16*16*n && pos < imgdata->result->size(); i+=n)
			{
				int r = data[i];
				int g = data[i+1];
				int b = data[i+2];
				(*imgdata->result)[pos++] = b;
				(*imgdata->result)[pos++] = g;
				(*imgdata->result)[pos++] = r;
				if ((i+n)%(16*n)==0)
					pos += (w-16)*3;
			}
			break;
		}
		case RGB8888:
		{
			uint32_t n=4;
			uint32_t pos=my*16*4*w+mx*16*4;
			for (uint32_t i=0; i < 16*16*n && pos < imgdata->result->size(); i+=n)
			{
				int r = data[i];
				int g = data[i+1];
				int b = data[i+2];
				int a = data[i+3];
				(*imgdata->result)[pos++] = b;
				(*imgdata->result)[pos++] = g;
				(*imgdata->result)[pos++] = r;
				(*imgdata->result)[pos++] = a;
				if ((i+n)%(16*n)==0)
					pos += (w-16)*4;
			}
			break;
		}
		default:
			break;
	}
}
bool decodejxr(uint8_t* bytes, uint32_t texlen, vector<uint8_t>& result, uint32_t width, uint32_t height, LSJXRDATAFORMAT format, uint32_t bpp)
{
	jxr_container_t container = jxr_create_container();
	int rc;
	if ((rc =jxr_read_image_container(container,bytes,texlen)) < 0)
	{
		LOG(LOG_ERROR,"decodejxr: couldn't create container:"<<rc);
		return false;
	}
	if ((rc = jxrc_image_count(container)) < 1)
	{
		LOG(LOG_ERROR,"decodejxr: invalid image count:"<<rc);
		return false;
	}
	uint32_t pos = jxrc_image_offset(container, 0);
	uint32_t len = jxrc_image_bytecount(container, 0);
	jxr_image_t image = jxr_create_input();
	result.resize(width*height*bpp);
	jxr_set_block_output(image,jpegxrcallback);
	lsjxrdata imgdata;
	imgdata.result = &result;
	imgdata.dataformat = format;
	jxr_set_user_data(image, (void*)&imgdata);
	jxrc_t_pixelFormat pixel_format;
	switch (format)
	{
		case RGB888:
			pixel_format = JXRC_FMT_24bppRGB;
			break;
		case RGB8888:
			pixel_format = JXRC_FMT_32bppBGRA;
			break;
		case DXT5AlphaImageData:
			pixel_format = JXRC_FMT_8bppGray;
			break;
		case DXT1ImageData:
		case DXT5ImageData:
			pixel_format = JXRC_FMT_16bppBGR565;
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"decodejxr: format not implemented:"<<format);
			jxr_destroy(image);
			jxr_destroy_container(container);
			return false;
	}

	jxr_set_container_parameters(image,pixel_format,width,height,0,0,0,0);
	if ((rc = jxr_read_image_bitstream(image,bytes+pos,len))< 0)
	{
		LOG(LOG_ERROR,"decodejxr: failed to decode image:"<<rc);
		jxr_destroy(image);
		jxr_destroy_container(container);
		return false;
	}
	jxr_destroy(image);
	jxr_destroy_container(container);
	return true;
}
uint32_t readTexLen(ByteArray* data, uint8_t atfversion)
{
	uint32_t texlen;
	if (atfversion == 0) // it seems that contrary to specs in atf version 0 all lengths are only 3 bytes
	{
		uint8_t b1;
		uint8_t b2;
		uint8_t b3;
		data->readByte(b1);
		data->readByte(b2);
		data->readByte(b3);
		texlen = uint32_t(b1<<16) | uint32_t(b2<<8) | uint32_t(b3);
	}
	else
		data->readUnsignedInt(texlen);
	return texlen;
}
bool decodelzma(ByteArray* data, uint8_t atfversion, std::vector<uint8_t>& result)
{
	uint32_t lzmadatalen = readTexLen(data,atfversion);
	if (lzmadatalen)
	{
		lzma_stream strm = LZMA_STREAM_INIT;
		lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX);
		if (ret != LZMA_OK)
		{
			LOG(LOG_ERROR,"Failed to initialize lzma decoder in parseAdobeTextureFormat");
			return false;
		}
		uint8_t* inbuffer = new uint8_t[lzmadatalen+sizeof(int64_t)];
		memcpy(inbuffer,(const uint8_t*)(data->getBufferNoCheck()+data->getPosition()),5);
		// insert length into lzma buffer to match liblzma format (see liblzma_filter constructor)
		for (unsigned int j=0; j<sizeof(int64_t); j++)
			inbuffer[5 + j] = 0xFF;
		memcpy(inbuffer+5+sizeof(int64_t),(const uint8_t*)(data->getBufferNoCheck()+data->getPosition()+5),lzmadatalen-5);
		
		strm.next_in = inbuffer;
		strm.avail_in = lzmadatalen+sizeof(int64_t);
		strm.avail_out = result.size();
		strm.next_out = (uint8_t *)result.data();
		while (strm.avail_in!=0)
		{
			do
			{
				lzma_ret ret=lzma_code(&strm, LZMA_RUN);
				
				if(ret==LZMA_STREAM_END || strm.avail_in==0)
					break;
				else if(ret!=LZMA_OK)
				{
					LOG(LOG_ERROR,"lzma decoder error:"<<ret);
					break;
				}
			}
			while(strm.avail_out!=0);
		}
		lzma_end(&strm);
		data->setPosition(data->getPosition()+lzmadatalen);
	}
	return lzmadatalen;
}
void TextureBase::fillFromDXT1(bool hasrgbdata, uint32_t level, uint32_t w, uint32_t h, std::vector<uint8_t>& rgbdata, std::vector<uint8_t>& rgbimagedata)
{
	if (hasrgbdata && w*h>=16)
	{
		bitmaparray[level].resize(w*h/2);
		uint32_t pos = 0;
		// adobe stores the dxt image data in a very weird way:
		// the first entry is in the upper half of the jpeg-xr image, the second entry is at the corresponding position in the lower half
		uint32_t lowerhalf = (w*h/16/2);
		for (uint32_t j=0; j < w*h/16; j++)
		{
			bitmaparray[level][pos++]=rgbimagedata[j*2  ];
			bitmaparray[level][pos++]=rgbimagedata[j*2+1];
			bitmaparray[level][pos++]=rgbimagedata[j*2+lowerhalf*4];
			bitmaparray[level][pos++]=rgbimagedata[j*2+1+lowerhalf*4];
			bitmaparray[level][pos++]=rgbdata[j*4  ];
			bitmaparray[level][pos++]=rgbdata[j*4+1];
			bitmaparray[level][pos++]=rgbdata[j*4+2];
			bitmaparray[level][pos++]=rgbdata[j*4+3];
		}
	}
	else if (rgbimagedata.empty() || rgbdata.empty())
		bitmaparray[level].clear();
	else
		bitmaparray[level].resize(8);
}
void TextureBase::fillFromDXT5(bool hasalphadata, bool hasrgbdata, uint32_t level, uint32_t w, uint32_t h, std::vector<uint8_t>& alphadata, std::vector<uint8_t>& alphaimagedata, std::vector<uint8_t>& rgbdata, std::vector<uint8_t>& rgbimagedata)
{
	if (hasalphadata && hasrgbdata && w*h>=16)
	{
		bitmaparray[level].resize(w*h);
		uint32_t pos = 0;
		// adobe stores the dxt image data in a very weird way:
		// the first entry is in the upper half of the jpeg-xr image, the second entry is at the corresponding position in the lower half
		uint32_t lowerhalf = (w*h/16/2);
		for (uint32_t j=0; j < w*h/16; j++)
		{
			bitmaparray[level][pos++]=alphaimagedata[j];
			bitmaparray[level][pos++]=alphaimagedata[j+lowerhalf*2];;
			bitmaparray[level][pos++]=alphadata[j*6  ];
			bitmaparray[level][pos++]=alphadata[j*6+1];
			bitmaparray[level][pos++]=alphadata[j*6+2];
			bitmaparray[level][pos++]=alphadata[j*6+3];
			bitmaparray[level][pos++]=alphadata[j*6+4];
			bitmaparray[level][pos++]=alphadata[j*6+5];
			bitmaparray[level][pos++]=rgbimagedata[j*2  ];
			bitmaparray[level][pos++]=rgbimagedata[j*2+1];
			bitmaparray[level][pos++]=rgbimagedata[j*2+lowerhalf*4];
			bitmaparray[level][pos++]=rgbimagedata[j*2+1+lowerhalf*4];
			bitmaparray[level][pos++]=rgbdata[j*4  ];
			bitmaparray[level][pos++]=rgbdata[j*4+1];
			bitmaparray[level][pos++]=rgbdata[j*4+2];
			bitmaparray[level][pos++]=rgbdata[j*4+3];
		}
	}
	else if (rgbimagedata.empty() || rgbdata.empty())
		bitmaparray[level].clear();
	else
		bitmaparray[level].resize(16);
}
void TextureBase::parseAdobeTextureFormat(ByteArray *data, int32_t byteArrayOffset, bool forCubeTexture)
{
	// https://www.adobe.com/devnet/archive/flashruntimes/articles/atf-file-format.html
	if (data->getLength()-byteArrayOffset < 20)
	{
		LOG(LOG_ERROR,"not enough bytes to read");
		createError<RangeError>(getInstanceWorker(),kParamRangeError);
		return;
	}
	uint8_t oldpos = data->getPosition();
	data->setPosition(byteArrayOffset);
	uint8_t b1, b2, b3;
	data->readByte(b1);
	data->readByte(b2);
	data->readByte(b3);
	if (b1 != 'A' || b2 != 'T' || b3 != 'F')
	{
		LOG(LOG_ERROR,"Texture.uploadCompressedTextureFromByteArray no ATF file");
		createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"data");
		return;
	}
	uint8_t atfversion=0;
	uint8_t formatbyte;
	uint8_t reserved[4];
	data->readByte(reserved[0]);
	data->readByte(reserved[1]);
	data->readByte(reserved[2]);
	data->readByte(reserved[3]);
	uint32_t len;
	uint32_t numtextures=UINT32_MAX;
	if (reserved[3] != 0xff) // fourth byte is not 0xff, so it's ATF version "0"(?)
	{
		len = uint32_t(reserved[0]<<16) | uint32_t(reserved[1]<<8) | uint32_t(reserved[2]);
		formatbyte = reserved[3];
	}
	else
	{
		if (reserved[2]) // LSB indicates if mipmaps are available, other 7 bits set number of mipmaps filled
			numtextures = ((reserved[2] & 1) == 1) ? 1 : (reserved[2] >> 1)&0x7f;
		data->readByte(atfversion);
		data->readUnsignedInt(len);
		data->readByte(formatbyte);
	}
	len--; // remove formatbyte from len;
	if (data->getLength()-data->getPosition() < len)
	{
		LOG(LOG_ERROR,"not enough bytes to read:"<<len<<" "<<data->getPosition()<<"/"<<data->getLength()<<" "<<byteArrayOffset);
		createError<RangeError>(getInstanceWorker(),kParamRangeError);
		return;
	}
	bool cubetexture = formatbyte&0x80;
	if (forCubeTexture != cubetexture)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray uploading a "<<(forCubeTexture ? "Texture" : "CubeTexture"));
		createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"data");
		return;
	}
	int format = formatbyte&0x7f;
	data->readByte(b1);
	if (b1 > 12)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray invalid texture width:"<<int(b1));
		createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"data");
		return;
	}
	data->readByte(b2);
	if (b2 > 12)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray invalid texture height:"<<int(b2));
		createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"data");
		return;
	}
	data->readByte(b3);
	if (b3 > 13)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray invalid texture count:"<<int(b3));
		createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"data");
		return;
	}
	width = 1<<b1;
	height = 1<<b2;
	maxmiplevel = (numtextures == UINT32_MAX ? b3  : numtextures);
	uint32_t texcount = maxmiplevel * (forCubeTexture ? 6 : 1);
	if (bitmaparray.size() < texcount)
		bitmaparray.resize(texcount);
	uint32_t tmpwidth = width;
	uint32_t tmpheight = height;
	for (uint32_t i = 0; i < texcount; i++)
	{
		uint32_t level = i;
		if (forCubeTexture) // cube texture negative/positive sides are swapped in atf 
			level = (i/b3)%2 ? i-b3 : i+b3;
		switch (format)
		{
			case 0x0://RGB888
			case 0x1://RGBA8888
			{
				uint32_t texlen = readTexLen(data,atfversion);
				if (bitmaparray[level].size() < tmpwidth*tmpheight*(format == 0 ? 3 : 4))
					bitmaparray[level].resize(tmpwidth*tmpheight*(format == 0 ? 3 : 4));
				if (texlen != 0)
				{
					uint8_t* texbytes = new uint8_t[texlen];
					data->readBytes(data->getPosition(),texlen,texbytes);
					decodejxr(texbytes,texlen,bitmaparray[level],tmpwidth,tmpheight,format == 1 ? RGB8888 : RGB888, format == 1 ? 4 : 3);
					delete[] texbytes;
				}
				if (format == 0)
					this->format=TEXTUREFORMAT::BGR;
				data->setPosition(data->getPosition()+texlen);
				break;
			}
			case 0x2://compressed
			{
				if (this->format != TEXTUREFORMAT::COMPRESSED)
				{
					createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"Texture format mismatch");
					return;
				}
				this->compressedformat = TEXTUREFORMAT_COMPRESSED::DXT1;
				uint32_t blocks = max(uint32_t(1),tmpwidth/4)*max(uint32_t(1),tmpheight/4);
				uint32_t tmp;
				
				// read LZMA DXT1Data
				std::vector<uint8_t> rgbdata;
				rgbdata.resize(blocks*4); // 4 byte per 4x4 block
				bool hasrgbdata=decodelzma(data,atfversion,rgbdata);
				
				// read JPEG-XR DXT1ImageData
				std::vector<uint8_t> rgbimagedata;
				tmp = readTexLen(data,atfversion);
				if (hasrgbdata)
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,rgbimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT1ImageData,2);
				data->setPosition(data->getPosition()+tmp);
				fillFromDXT1(hasrgbdata,level,tmpwidth,tmpheight,rgbdata,rgbimagedata);
				
				//skip all other formats
				for (uint32_t j = 0; j < (atfversion>=3 ? 9 : 6); j++)
				{
					tmp = readTexLen(data,atfversion);
					data->setPosition(data->getPosition()+tmp);
				}
				break;
			}
			case 0x3://raw compressed
			{
				if (this->format != TEXTUREFORMAT::COMPRESSED)
				{
					createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"Texture format mismatch");
					return;
				}
				this->compressedformat = TEXTUREFORMAT_COMPRESSED::DXT1;
				
				// read raw DXT1ImageData
				uint32_t texlen = readTexLen(data,atfversion);
				bitmaparray[level].resize(texlen);
				data->readBytes(data->getPosition(),texlen,bitmaparray[level].data());
				data->setPosition(data->getPosition()+texlen);
				
				//skip all other formats
				for (uint32_t j = 0; j < (atfversion>=3 ? 3 : 2); j++)
				{
					texlen = readTexLen(data,atfversion);
					data->setPosition(data->getPosition()+texlen);
				}
				break;
			}
			case 0x4://compressedalpha
			{
				if (this->format != TEXTUREFORMAT::COMPRESSED_ALPHA)
				{
					createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"Texture format mismatch");
					return;
				}
				this->compressedformat = TEXTUREFORMAT_COMPRESSED::DXT5;
				uint32_t blocks = max(uint32_t(1),tmpwidth/4)*max(uint32_t(1),tmpheight/4);
				uint32_t tmp;
				
				// read LZMA DXT5AlphaData
				std::vector<uint8_t> alphadata;
				alphadata.resize(blocks*6); // 6 byte per 4x4 block
				bool hasalphadata=decodelzma(data,atfversion,alphadata);
				
				// read JPEG-XR DXT5AlphaImageData
				std::vector<uint8_t> alphaimagedata;
				data->readUnsignedInt(tmp);
				if (hasalphadata)
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,alphaimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT5AlphaImageData,1);
				data->setPosition(data->getPosition()+tmp);
				
				// read LZMA DXT5Data
				std::vector<uint8_t> rgbdata;
				rgbdata.resize(blocks*4);// 4 byte per 4x4 block
				bool hasrgbdata=decodelzma(data,atfversion,rgbdata);
				
				// read JPEG-XR DXT5ImageData
				std::vector<uint8_t> rgbimagedata;
				data->readUnsignedInt(tmp);
				if (hasrgbdata)
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,rgbimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT5ImageData,2);
				data->setPosition(data->getPosition()+tmp);
				
				fillFromDXT5(hasalphadata,hasrgbdata,level,tmpwidth,tmpheight,alphadata,alphaimagedata,rgbdata,rgbimagedata);
				
				//skip all other formats
				for (uint32_t j = 0; j < (atfversion>=3 ? 12 : 6); j++)
				{
					tmp = readTexLen(data,atfversion);
					data->setPosition(data->getPosition()+tmp);
				}
				break;
			}
			case 0x5://raw compressed alpha
			{
				if (this->format != TEXTUREFORMAT::COMPRESSED_ALPHA)
				{
					createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"Texture format mismatch");
					return;
				}
				this->compressedformat = TEXTUREFORMAT_COMPRESSED::DXT5;
				
				// read raw DXT1ImageData
				uint32_t texlen = readTexLen(data,atfversion);
				bitmaparray[level].resize(texlen);
				data->readBytes(data->getPosition(),texlen,bitmaparray[level].data());
				data->setPosition(data->getPosition()+texlen);
				
				//skip all other formats
				for (uint32_t j = 0; j < (atfversion>=3 ? 3 : 2); j++)
				{
					texlen = readTexLen(data,atfversion);
					data->setPosition(data->getPosition()+texlen);
				}
				break;
			}
			case 0xc://compressed lossy
			{
				if (this->format != TEXTUREFORMAT::COMPRESSED)
				{
					createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"Texture format mismatch");
					return;
				}
				this->compressedformat = TEXTUREFORMAT_COMPRESSED::DXT1;
				uint32_t blocks = max(uint32_t(1),tmpwidth/4)*max(uint32_t(1),tmpheight/4);
				uint32_t tmp;

				// read LZMA DXT1Data
				std::vector<uint8_t> rgbdata;
				rgbdata.resize(blocks*4); // 4 byte per 4x4 block
				bool hasrgbdata=decodelzma(data,atfversion,rgbdata);

				// read JPEG-XR DXT1ImageData
				std::vector<uint8_t> rgbimagedata;
				data->readUnsignedInt(tmp);
				if (hasrgbdata)
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,rgbimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT1ImageData,2);
				data->setPosition(data->getPosition()+tmp);
				fillFromDXT1(hasrgbdata,level,tmpwidth,tmpheight,rgbdata,rgbimagedata);
					
				//skip all other formats
				for (uint32_t j = 0; j < (atfversion>=3 ? 10 : 6); j++)
				{
					tmp = readTexLen(data,atfversion);
					data->setPosition(data->getPosition()+tmp);
				}
				break;
			}
			case 0xd://compressed lossy with alpha
			{
				if (this->format != TEXTUREFORMAT::COMPRESSED_ALPHA)
				{
					createError<ArgumentError>(getInstanceWorker(),kInvalidArgumentError,"Texture format mismatch");
					return;
				}
				this->compressedformat = TEXTUREFORMAT_COMPRESSED::DXT5;
				uint32_t blocks = max(uint32_t(1),tmpwidth/4)*max(uint32_t(1),tmpheight/4);
				uint32_t tmp;

				// read LZMA DXT5AlphaData
				std::vector<uint8_t> alphadata;
				alphadata.resize(blocks*6); // 6 byte per 4x4 block
				bool hasalphadata=decodelzma(data,atfversion,alphadata);
				
				// read JPEG-XR DXT5AlphaImageData
				std::vector<uint8_t> alphaimagedata;
				data->readUnsignedInt(tmp);
				if (hasalphadata)
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,alphaimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT5AlphaImageData,1);
				data->setPosition(data->getPosition()+tmp);
				
				// read LZMA DXT5Data
				std::vector<uint8_t> rgbdata;
				rgbdata.resize(blocks*4);// 4 byte per 4x4 block
				bool hasrgbdata=decodelzma(data,atfversion,rgbdata);

				// read JPEG-XR DXT5ImageData
				std::vector<uint8_t> rgbimagedata;
				data->readUnsignedInt(tmp);
				if (hasrgbdata)
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,rgbimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT5ImageData,2);
				data->setPosition(data->getPosition()+tmp);
				
				fillFromDXT5(hasalphadata,hasrgbdata,level,tmpwidth,tmpheight,alphadata,alphaimagedata,rgbdata,rgbimagedata);

				//skip all other formats
				for (uint32_t j = 0; j < (atfversion>=3 ? 13 : 6); j++)
				{
					tmp = readTexLen(data,atfversion);
					data->setPosition(data->getPosition()+tmp);
				}
				break;
			}
			default:
				LOG(LOG_NOT_IMPLEMENTED,"uploadCompressedTextureFromByteArray format not yet supported:"<<hex<<format);
				break;
		}
		if (!forCubeTexture || (level+1)%6==0)
		{
			tmpwidth >>= 1;
			tmpheight >>= 1;
		}
	}
	data->setPosition(oldpos);
}

void TextureBase::setFormat(const tiny_string& f)
{
	if (f == "bgra")
		format = TEXTUREFORMAT::BGRA;
	else if (f == "bgraPacked4444")
		format = TEXTUREFORMAT::BGRA_PACKED;
	else if (f == "bgrPacked565")
		format = TEXTUREFORMAT::BGR_PACKED;
	else if (f == "compressed")
		format = TEXTUREFORMAT::COMPRESSED;
	else if (f == "compressedAlpha")
		format = TEXTUREFORMAT::COMPRESSED_ALPHA;
	else if (f == "rgbaHalfFloat")
		format = TEXTUREFORMAT::RGBA_HALF_FLOAT;
}
uint32_t TextureBase::getBytesNeeded(uint32_t miplevel)
{
	uint32_t numpixels = max(width>>miplevel,1U)*max(height>>miplevel,1U);
	switch (format)
	{
		case TEXTUREFORMAT::BGRA:
			return numpixels*4;
		case TEXTUREFORMAT::BGRA_PACKED:
		case TEXTUREFORMAT::BGR_PACKED:
			return numpixels*2;
		case TEXTUREFORMAT::COMPRESSED:
		case TEXTUREFORMAT::COMPRESSED_ALPHA:
			LOG(LOG_NOT_IMPLEMENTED,"TextureBase::getBytesNeeded for compressed formats");
			break;
		case TEXTUREFORMAT::RGBA_HALF_FLOAT:
			LOG(LOG_NOT_IMPLEMENTED,"TextureBase::getBytesNeeded for RGBA_HALF_FLOAT");
			break;
		default:
			break;
	}
	return numpixels*4;
}

void TextureBase::uploadFromBitmapDataIntern(BitmapData* source, uint32_t miplevel, uint32_t side, uint32_t max_miplevel)
{
	context->rendermutex.lock();
	format = TEXTUREFORMAT::BGRA;
	if (bitmaparray.size() <= miplevel)
		bitmaparray.resize(miplevel+1);
	if (maxmiplevel < miplevel)
		maxmiplevel = miplevel;
	uint32_t mipsize = getBytesNeeded(miplevel);
	uint32_t bitmappos = max_miplevel*side + miplevel;
	bitmaparray[bitmappos].resize(mipsize);
	uint32_t sourcewidth = source->getBitmapContainer()->getWidth();
	uint32_t sourceheight = source->getBitmapContainer()->getHeight();
	memset(bitmaparray[bitmappos].data(),0,mipsize);
	for (uint32_t i = 0; i < (height>>miplevel) && i < sourceheight; i++)
	{
		for (uint32_t j = 0; j < (width>>miplevel) && j < sourcewidth; j++)
		{
			// It seems that flash expects the bitmaps to be premultiplied-alpha in shaders
			uint32_t* data = (uint32_t*)(&source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4+j*4]);
			uint8_t alpha = ((*data) >>24) & 0xff;
			bitmaparray[bitmappos][i*(width>>miplevel)*4 + j*4  ] = (uint8_t)((((*data)     ) & 0xff)*alpha /255);
			bitmaparray[bitmappos][i*(width>>miplevel)*4 + j*4+1] = (uint8_t)((((*data) >> 8) & 0xff)*alpha /255);
			bitmaparray[bitmappos][i*(width>>miplevel)*4 + j*4+2] = (uint8_t)((((*data) >>16) & 0xff)*alpha /255);
			bitmaparray[bitmappos][i*(width>>miplevel)*4 + j*4+3] = (uint8_t)((((*data) >>24) & 0xff)           );
		}
	}
	renderaction action;
	action.action = this->is<CubeTexture>() ? RENDER_LOADCUBETEXTURE : RENDER_LOADTEXTURE;
	incRef();
	action.dataobject = _MR(this);
	action.udata1=miplevel;
	action.udata2=side;
	context->addAction(action);
	context->rendermutex.unlock();
}

void TextureBase::uploadFromByteArrayIntern(ByteArray* source, uint32_t offset, uint32_t miplevel)
{
	context->rendermutex.lock();
	if (bitmaparray.size() <= miplevel)
		bitmaparray.resize(miplevel+1);
	if (maxmiplevel < miplevel)
		maxmiplevel = miplevel;
	uint32_t mipsize = getBytesNeeded(miplevel);
	bitmaparray[miplevel].resize(mipsize);
	uint32_t bytesneeded = getBytesNeeded(miplevel);
	if (offset + bytesneeded > source->getLength())
	{
		LOG(LOG_ERROR,"TextureBase uploadFromByteArrayIntern: not enough bytes to read");
		createError<RangeError>(getInstanceWorker(),kParamRangeError);
		return;
	}
	source->readBytes(offset,bytesneeded,bitmaparray[miplevel].data());
#ifdef ENABLE_GLES
	switch (format)
	{
		case TEXTUREFORMAT::BGRA:
		{
			for (uint32_t i = 0; i < bitmaparray[miplevel].size(); i+=4)
				std::swap(bitmaparray[miplevel][i],bitmaparray[miplevel][i+2]);
		}
		case TEXTUREFORMAT::BGRA_PACKED:
			LOG(LOG_NOT_IMPLEMENTED,"texture conversion from BGRA_PACKED to RGBA for opengles")
			break;
		case TEXTUREFORMAT::BGR_PACKED:
			LOG(LOG_NOT_IMPLEMENTED,"texture conversion from BGR_PACKED to RGB for opengles")
			break;
	}
#endif
	source->setPosition(offset+bytesneeded);
	renderaction action;
	action.action = RENDER_LOADTEXTURE;
	incRef();
	action.dataobject = _MR(this);
	action.udata1=miplevel;
	context->addAction(action);
	context->rendermutex.unlock();
}
 
void TextureBase::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,true);
}

bool TextureBase::destruct()
{
	renderaction action;
	action.action =RENDER_ACTION::RENDER_DELETETEXTURE;
	action.udata1 = textureID;
	context->addAction(action);
	textureID=UINT32_MAX;
	width=0;
	height=0;
	async=false;
	format=BGRA;
	compressedformat=UNCOMPRESSED;
	context=nullptr;
	return EventDispatcher::destruct();
}
ASFUNCTIONBODY_ATOM(TextureBase,dispose)
{
	TextureBase* th = asAtomHandler::as<TextureBase>(obj);
	renderaction action;
	action.action =RENDER_ACTION::RENDER_DELETETEXTURE;
	action.udata1 = th->textureID;
	th->context->addAction(action);
	th->textureID=UINT32_MAX;
}

void Texture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadCompressedTextureFromByteArray","",c->getSystemState()->getBuiltinFunction(uploadCompressedTextureFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",c->getSystemState()->getBuiltinFunction(uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",c->getSystemState()->getBuiltinFunction(uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(Texture,uploadCompressedTextureFromByteArray)
{
	Texture* th = asAtomHandler::as<Texture>(obj);
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset)(th->async,false));
	if (data.isNull())
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}
	th->context->rendermutex.lock();
	th->parseAdobeTextureFormat(data.getPtr(),byteArrayOffset,false);
	renderaction action;
	action.action = RENDER_LOADTEXTURE;
	if (th->async)
	{
		th->context->addTextureToUpload(th);
	}
	else
	{
		th->incRef();
		action.dataobject = _MR(th);
		action.udata1=UINT32_MAX;
		th->context->addAction(action);
	}
	th->context->rendermutex.unlock();
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromBitmapData)
{
	Texture* th = asAtomHandler::as<Texture>(obj);
	uint32_t miplevel;
	_NR<BitmapData> source;
	ARG_CHECK(ARG_UNPACK(source)(miplevel,0));
	if (source.isNull())
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > max(th->width,th->height)))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width<<" "<<th->height);
		createError<ArgumentError>(wrk,kInvalidArgumentError,"miplevel");
		return;
	}
	th->uploadFromBitmapDataIntern(source.getPtr(),miplevel);
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromByteArray)
{
	Texture* th = asAtomHandler::as<Texture>(obj);
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	uint32_t miplevel;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset)(miplevel,0));
	if (data.isNull())
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > max(th->width,th->height)))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width<<" "<<th->height);
		createError<ArgumentError>(wrk,kInvalidArgumentError,"miplevel");
		return;
	}
	th->uploadFromByteArrayIntern(data.getPtr(),byteArrayOffset,miplevel);
}


void CubeTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadCompressedTextureFromByteArray","",c->getSystemState()->getBuiltinFunction(uploadCompressedTextureFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",c->getSystemState()->getBuiltinFunction(uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",c->getSystemState()->getBuiltinFunction(uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadCompressedTextureFromByteArray)
{
	CubeTexture* th = asAtomHandler::as<CubeTexture>(obj);
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	bool async;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset)(async,false));
	if (data.isNull())
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}
	th->context->rendermutex.lock();
	th->parseAdobeTextureFormat(data.getPtr(),byteArrayOffset,true);
	renderaction action;
	action.action = RENDER_LOADCUBETEXTURE;
	if (th->async)
	{
		th->context->addTextureToUpload(th);
	}
	else
	{
		th->incRef();
		action.dataobject = _MR(th);
		action.udata1=UINT32_MAX;
		action.udata2=UINT32_MAX;
		th->context->addAction(action);
	}
	th->context->rendermutex.unlock();
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromBitmapData)
{
	CubeTexture* th = asAtomHandler::as<CubeTexture>(obj);
	_NR<BitmapData> source;
	uint32_t side;
	uint32_t miplevel;
	ARG_CHECK(ARG_UNPACK(source)(side)(miplevel,0));
	if (source.isNull())
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > th->width))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width);
		createError<ArgumentError>(wrk,kInvalidArgumentError,"miplevel");
		return;
	}
	if (side > 5)
	{
		LOG(LOG_ERROR,"invalid side:"<<side);
		createError<ArgumentError>(wrk,kInvalidArgumentError,"side");
		return;
	}
	uint32_t bitmap_size = 1<<(th->max_miplevel-(miplevel+1));
	if ((source->getWidth() != source->getHeight())
		|| (uint32_t(source->getWidth()) != bitmap_size))
	{
		LOG(LOG_ERROR,"invalid bitmap:"<<source->getWidth()<<" "<<source->getHeight()<<" "<< th->max_miplevel <<" "<< miplevel);
		createError<ArgumentError>(wrk,kInvalidArgumentError,"source");
		return;
	}
	th->uploadFromBitmapDataIntern(source.getPtr(),miplevel,side,th->max_miplevel);
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"CubeTexture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	uint32_t side;
	uint32_t miplevel;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset)(side)(miplevel,0));
}

void RectangleTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadFromByteArray","",c->getSystemState()->getBuiltinFunction(uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",c->getSystemState()->getBuiltinFunction(uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(RectangleTexture,uploadFromBitmapData)
{
	RectangleTexture* th = asAtomHandler::as<RectangleTexture>(obj);
	_NR<BitmapData> source;
	ARG_CHECK(ARG_UNPACK(source));

	if (source.isNull())
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}
	if (th->format != TEXTUREFORMAT::BGRA)
		LOG(LOG_NOT_IMPLEMENTED,"RectangleTexture.uploadFromBitmapData always uses format BGRA:"<<th->format);
	th->uploadFromBitmapDataIntern(source.getPtr(),0);
}
ASFUNCTIONBODY_ATOM(RectangleTexture,uploadFromByteArray)
{
	RectangleTexture* th = asAtomHandler::as<RectangleTexture>(obj);
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset));
	if (data.isNull())
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}
	th->uploadFromByteArrayIntern(data.getPtr(),byteArrayOffset,0);
}

void VideoTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	REGISTER_GETTER(c,videoHeight);
	REGISTER_GETTER(c,videoWidth);
	c->setDeclaredMethodByQName("attachCamera","",c->getSystemState()->getBuiltinFunction(attachCamera),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",c->getSystemState()->getBuiltinFunction(attachNetStream),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(VideoTexture,videoHeight)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(VideoTexture,videoWidth)

ASFUNCTIONBODY_ATOM(VideoTexture,attachCamera)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoTexture.attachCamera does nothing");
//	_NR<Camera> theCamera;
//	ARG_CHECK(ARG_UNPACK(theCamera));
}
ASFUNCTIONBODY_ATOM(VideoTexture,attachNetStream)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoTexture.attachNetStream does nothing");
	_NR<NetStream> netStream;
	ARG_CHECK(ARG_UNPACK(netStream));
}
