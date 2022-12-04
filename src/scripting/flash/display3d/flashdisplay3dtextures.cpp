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

namespace lightspark
{
enum LSJXRDATAFORMAT { RGB888,RGB8888,DXT5AlphaImageData,DXT5ImageData};
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
		case DXT5ImageData:
		{
			uint32_t n=3;
			uint32_t pos=my*16*2*w+mx*16*2;
			for (uint32_t i=0; i < 16*16*n && pos < imgdata->result->size(); i+=n)
			{
				int r = data[i];
				int g = data[i+1];
				int b = data[i+2];
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
bool decodelzma(ByteArray* data, std::vector<uint8_t>& result)
{
	uint32_t lzmadatalen;
	data->readUnsignedInt(lzmadatalen);
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
	if (reserved[3] != 0xff) // fourth byte is not 0xff, so it's ATF version "0"(?)
	{
		len = uint32_t(reserved[0]<<16) | uint32_t(reserved[1]<<8) | uint32_t(reserved[2]);
		formatbyte = reserved[3];
	}
	else
	{
		data->readByte(atfversion);
		data->readUnsignedInt(len);
		data->readByte(formatbyte);
		len--; // remove formatbyte from len;
	}
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
	
	uint32_t texcount = b3 * (forCubeTexture ? 6 : 1);
	if (bitmaparray.size() < texcount)
		bitmaparray.resize(texcount);
	uint32_t tmpwidth = width;
	uint32_t tmpheight = height;
	for (uint32_t level = 0; level < texcount; level++)
	{
		switch (format)
		{
			case 0x0://RGB888
			case 0x1://RGBA8888
			{
				uint32_t texlen;
				if (atfversion == 0)
				{
					data->readByte(b1);
					data->readByte(b2);
					data->readByte(b3);
					texlen = uint32_t(b1<<16) | uint32_t(b2<<8) | uint32_t(b3);
				}
				else
					data->readUnsignedInt(texlen);
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
				tmpwidth >>= 1;
				tmpheight >>= 1;
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
				bool hasalphadata=decodelzma(data,alphadata);
				
				// read JPEG-XR DXT5AlphaImageData
				std::vector<uint8_t> alphaimagedata;
				data->readUnsignedInt(tmp);
				if (hasalphadata)
				{
					alphaimagedata.resize(blocks*2); // 2 byte per 4x4 block
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,alphaimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT5AlphaImageData,1);
				}
				data->setPosition(data->getPosition()+tmp);
				
				// read LZMA DXT5Data
				std::vector<uint8_t> rgbdata;
				rgbdata.resize(blocks*4);// 4 byte per 4x4 block
				bool hasrgbdata=decodelzma(data,rgbdata);

				// read JPEG-XR DXT5ImageData
				std::vector<uint8_t> rgbimagedata;
				data->readUnsignedInt(tmp);
				if (hasrgbdata)
				{
					rgbimagedata.resize(blocks*4); // 4 byte per 4x4 block
					decodejxr(data->getBufferNoCheck()+data->getPosition(),tmp,rgbimagedata,max(uint32_t(1),tmpwidth/4),max(uint32_t(2),tmpheight/2),DXT5ImageData,2);
				}
				data->setPosition(data->getPosition()+tmp);

				if (hasalphadata && hasrgbdata)
				{
					bitmaparray[level].resize(tmpwidth*tmpheight);
					uint32_t pos = 0;
					for (uint32_t j=0; j < tmpwidth*tmpheight/16; j++)
					{
						bitmaparray[level][pos++]=alphaimagedata[j*2  ];
						bitmaparray[level][pos++]=alphaimagedata[j*2+1];
						bitmaparray[level][pos++]=alphadata[j*6  ];
						bitmaparray[level][pos++]=alphadata[j*6+1];
						bitmaparray[level][pos++]=alphadata[j*6+2];
						bitmaparray[level][pos++]=alphadata[j*6+3];
						bitmaparray[level][pos++]=alphadata[j*6+4];
						bitmaparray[level][pos++]=alphadata[j*6+5];
						bitmaparray[level][pos++]=rgbimagedata[j*4  ];
						bitmaparray[level][pos++]=rgbimagedata[j*4+1];
						bitmaparray[level][pos++]=rgbimagedata[j*4+2];
						bitmaparray[level][pos++]=rgbimagedata[j*4+3];
						bitmaparray[level][pos++]=rgbdata[j*4  ];
						bitmaparray[level][pos++]=rgbdata[j*4+1];
						bitmaparray[level][pos++]=rgbdata[j*4+2];
						bitmaparray[level][pos++]=rgbdata[j*4+3];
					}
				}
				else
					bitmaparray[level].clear();
					
				//skip all other formats
				for (uint32_t j = 0; j < 14; j++)
				{
					data->readUnsignedInt(tmp);
					data->setPosition(data->getPosition()+tmp);
				}
				tmpwidth >>= 1;
				tmpheight >>= 1;
				break;
			}
			default:
				LOG(LOG_NOT_IMPLEMENTED,"uploadCompressedTextureFromByteArray format not yet supported:"<<hex<<format);
				break;
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

void TextureBase::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
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
	c->setDeclaredMethodByQName("uploadCompressedTextureFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadCompressedTextureFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(Texture,uploadCompressedTextureFromByteArray)
{
	Texture* th = asAtomHandler::as<Texture>(obj);
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
	th->parseAdobeTextureFormat(data.getPtr(),byteArrayOffset,false);
	if (async)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Texture.uploadCompressedTextureFromByteArray async loading");
		th->incRef();
		getVm(wrk->getSystemState())->addEvent(_MR(th), _MR(Class<Event>::getInstanceS(wrk,"textureReady")));
	}
	renderaction action;
	action.action = RENDER_LOADTEXTURE;
	th->incRef();
	action.dataobject = _MR(th);
	action.udata1=UINT32_MAX;
	th->context->addAction(action);
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
	th->context->rendermutex.lock();
	if (th->bitmaparray.size() <= miplevel)
		th->bitmaparray.resize(miplevel+1);
	uint32_t mipsize = (th->width>>miplevel)*(th->height>>miplevel)*4;
	th->bitmaparray[miplevel].resize(mipsize);

	for (uint32_t i = 0; i < (th->height>>miplevel); i++)
	{
		for (uint32_t j = 0; j < (th->width>>miplevel); j++)
		{
			// It seems that flash expects the bitmaps to be premultiplied-alpha in shaders
			uint32_t* data = (uint32_t*)(&source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4+j*4]);
			uint8_t alpha = ((*data) >>24) & 0xff;
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4  ] = (uint8_t)((((*data)     ) & 0xff)*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+1] = (uint8_t)((((*data) >> 8) & 0xff)*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+2] = (uint8_t)((((*data) >>16) & 0xff)*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+3] = (uint8_t)((((*data) >>24) & 0xff)           );
		}
	}
	renderaction action;
	action.action = RENDER_LOADTEXTURE;
	th->incRef();
	action.dataobject = _MR(th);
	action.udata1=miplevel;
	th->context->addAction(action);
	th->context->rendermutex.unlock();
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
	th->context->rendermutex.lock();
	if (th->bitmaparray.size() <= miplevel)
		th->bitmaparray.resize(miplevel+1);
	uint32_t mipsize = (th->width>>miplevel)*(th->height>>miplevel)*4;
	th->bitmaparray[miplevel].resize(mipsize);
	uint32_t bytesneeded = (th->height>>miplevel) * (th->width>>miplevel)*4;
	if (byteArrayOffset + bytesneeded > data->getLength())
	{
		LOG(LOG_ERROR,"not enough bytes to read");
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	data->readBytes(byteArrayOffset,bytesneeded,th->bitmaparray[miplevel].data());
#ifdef ENABLE_GLES
	switch (th->format)
	{
		case TEXTUREFORMAT::BGRA:
		{
			for (uint32_t i = 0; i < th->bitmaparray[miplevel].size(); i+=4)
				std::swap(th->bitmaparray[miplevel][i],th->bitmaparray[miplevel][i+2]);
		}
		case TEXTUREFORMAT::BGRA_PACKED:
			LOG(LOG_NOT_IMPLEMENTED,"texture conversion from BGRA_PACKED to RGBA for opengles")
			break;
		case TEXTUREFORMAT::BGR_PACKED:
			LOG(LOG_NOT_IMPLEMENTED,"texture conversion from BGR_PACKED to RGB for opengles")
			break;
	}
#endif
	data->setPosition(byteArrayOffset+bytesneeded);
	renderaction action;
	action.action = RENDER_LOADTEXTURE;
	th->incRef();
	action.dataobject = _MR(th);
	action.udata1=miplevel;
	th->context->addAction(action);
	th->context->rendermutex.unlock();
}


void CubeTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadCompressedTextureFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadCompressedTextureFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadCompressedTextureFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"CubeTexture.uploadCompressedTextureFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	bool async;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset)(async,false));
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

	th->context->rendermutex.lock();
	uint32_t mipsize = (th->width>>miplevel)*(th->width>>miplevel)*4;
	th->bitmaparray[th->max_miplevel*side + miplevel].resize(mipsize);
	for (uint32_t i = 0; i < bitmap_size; i++)
	{
		for (uint32_t j = 0; j < bitmap_size; j++)
		{
			// It seems that flash expects the bitmaps to be premultiplied-alpha in shaders
			uint32_t* data = (uint32_t*)(&source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4+j*4]);
			uint8_t alpha = ((*data) >>24) & 0xff;
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4  ] = (uint8_t)((((*data) >>16) & 0xff)*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+1] = (uint8_t)((((*data) >> 8) & 0xff)*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+2] = (uint8_t)((((*data)     ) & 0xff)*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+3] = alpha;
		}
	}
	th->context->addAction(RENDER_LOADCUBETEXTURE,th);
	th->context->rendermutex.unlock();
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
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromBitmapData),NORMAL_METHOD,true);
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
	assert_and_throw(th->height == uint32_t(source->getHeight()) && th->width == uint32_t(source->getWidth()));
	th->context->rendermutex.lock();
	if (th->bitmaparray.size() == 0)
		th->bitmaparray.resize(1);
	uint32_t bytesneeded = th->width*th->height*4;
	th->bitmaparray[0].resize(bytesneeded);

	for (uint32_t i = 0; i < th->height; i++)
	{
		for (uint32_t j = 0; j < th->width; j++)
		{
			// It seems that flash expects the bitmaps to be premultiplied-alpha in shaders
			uint32_t* data = (uint32_t*)(&source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4+j*4]);
			uint8_t alpha = ((*data) >>24) & 0xff;
			th->bitmaparray[0][i*th->width*4 + j*4  ] = (uint8_t)((((*data)     ) & 0xff)*alpha /255);
			th->bitmaparray[0][i*th->width*4 + j*4+1] = (uint8_t)((((*data) >> 8) & 0xff)*alpha /255);
			th->bitmaparray[0][i*th->width*4 + j*4+2] = (uint8_t)((((*data) >>16) & 0xff)*alpha /255);
			th->bitmaparray[0][i*th->width*4 + j*4+3] = (uint8_t)((((*data) >>24) & 0xff)           );
		}
	}
	renderaction action;
	action.action = RENDER_LOADTEXTURE;
	th->incRef();
	action.dataobject = _MR(th);
	action.udata1=0;
	th->context->addAction(action);
	th->context->rendermutex.unlock();
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
	th->context->rendermutex.lock();
	if (th->bitmaparray.size() == 0)
		th->bitmaparray.resize(1);
	uint32_t bytesneeded = th->height*th->width*4;
	if (byteArrayOffset + bytesneeded > data->getLength())
	{
		LOG(LOG_ERROR,"not enough bytes to read");
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	th->bitmaparray[0].resize(bytesneeded);
	data->readBytes(byteArrayOffset,bytesneeded,th->bitmaparray[0].data());
#ifdef ENABLE_GLES
	switch (th->format)
	{
		case TEXTUREFORMAT::BGRA:
		{
			for (uint32_t i = 0; i < th->bitmaparray[0].size(); i+=4)
				std::swap(th->bitmaparray[0][i],th->bitmaparray[miplevel][i+2]);
		}
		case TEXTUREFORMAT::BGRA_PACKED:
			LOG(LOG_NOT_IMPLEMENTED,"texture conversion from BGRA_PACKED to RGBA for opengles")
			break;
		case TEXTUREFORMAT::BGR_PACKED:
			LOG(LOG_NOT_IMPLEMENTED,"texture conversion from BGR_PACKED to RGB for opengles")
			break;
	}
#endif
	data->setPosition(byteArrayOffset+bytesneeded);
	renderaction action;
	action.action = RENDER_LOADTEXTURE;
	th->incRef();
	action.dataobject = _MR(th);
	action.udata1=0;
	th->context->addAction(action);
	th->context->rendermutex.unlock();
}

void VideoTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	REGISTER_GETTER(c,videoHeight);
	REGISTER_GETTER(c,videoWidth);
	c->setDeclaredMethodByQName("attachCamera","",Class<IFunction>::getFunction(c->getSystemState(),attachCamera),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",Class<IFunction>::getFunction(c->getSystemState(),attachNetStream),NORMAL_METHOD,true);
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

}
