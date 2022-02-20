#include "flashdisplay3dtextures.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/utils/ByteArray.h"
#define INITGUID
#include "3rdparty/jxrlib/jxrgluelib/JXRGlue.h"

namespace lightspark
{
bool decodejxr(uint8_t* bytes, uint32_t texlen, vector<uint8_t>& result, uint32_t width, uint32_t height, bool hasalpha)
{
	PKFactory* pFactory = nullptr;
	PKCodecFactory* pCodecFactory = nullptr;
	PKImageDecode* pDecoder = nullptr;
	PKFormatConverter* pConverter = nullptr;
	struct WMPStream* pDecodeStream = nullptr;
	PKRect rect;
	rect.X=0;
	rect.Y=0;
	rect.Width=width;
	rect.Height=height;
	bool success = true;
	if (success && !(success = (PKCreateFactory(&pFactory, PK_SDK_VERSION)>=0)))
		LOG(LOG_ERROR,"decodejxr:PKCreateFactory failed");
	if (success && !(success = (PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION)>=0)))
		LOG(LOG_ERROR,"decodejxr:PKCreateCodecFactory failed");
	if (success && !(success = (PKImageDecode_Create_WMP(&pDecoder)>=0)))
		LOG(LOG_ERROR,"decodejxr:PKImageDecode_Create_WMP failed");
	if (success && !(success = (pFactory->CreateStreamFromMemory(&pDecodeStream,bytes,texlen)>=0)))
		LOG(LOG_ERROR,"decodejxr:CreateStreamFromMemory failed");
	if (success && !(success = (pDecoder->Initialize(pDecoder,pDecodeStream)>=0)))
		LOG(LOG_ERROR,"decodejxr:Decoder.Initialize failed");
	if (success && !(success = (pCodecFactory->CreateFormatConverter(&pConverter)>=0)))
		LOG(LOG_ERROR,"decodejxr:CreateFormatConverter failed");
	if (success && !(success = (pConverter->Initialize(pConverter, pDecoder, nullptr, hasalpha ? GUID_PKPixelFormat32bppRGBA : GUID_PKPixelFormat24bppRGB)>=0)))
		LOG(LOG_ERROR,"decodejxr:Converter.Initialize failed");
	if (success && !(success = (pConverter->Copy(pConverter, &rect, result.data(), width*(hasalpha ? 4 : 3))>=0)))
		LOG(LOG_ERROR,"decodejxr:Converter.Copy failed");
	
	if (pConverter) pConverter->Release(&pConverter);
	if (pDecoder) pDecoder->Release(&pDecoder);
	if (pFactory) pFactory->Release(&pFactory);
	if (pCodecFactory) pCodecFactory->Release(&pCodecFactory);
	return success;
}
void TextureBase::parseAdobeTextureFormat(ByteArray *data,int32_t byteArrayOffset, bool forCubeTexture, bool& hasalpha)
{
	// https://www.adobe.com/devnet/archive/flashruntimes/articles/atf-file-format.html
	if (data->getLength()-byteArrayOffset < 20)
	{
		LOG(LOG_ERROR,"not enough bytes to read");
		throwError<RangeError>(kParamRangeError);
	}
	data->setPosition(byteArrayOffset);
	uint8_t b1, b2, b3;
	data->readByte(b1);
	data->readByte(b2);
	data->readByte(b3);
	if (b1 != 'A' || b2 != 'T' || b3 != 'F')
	{
		LOG(LOG_ERROR,"Texture.uploadCompressedTextureFromByteArray no ATF file");
		throwError<ArgumentError>(kInvalidArgumentError,"data");
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
	}
	if (data->getLength()-data->getPosition() < len)
	{
		LOG(LOG_ERROR,"not enough bytes to read");
		throwError<RangeError>(kParamRangeError);
	}
	uint32_t endposition = data->getPosition()+len;
	bool cubetexture = formatbyte&0x80;
	if (forCubeTexture != cubetexture)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray uploading a "<<(forCubeTexture ? "Texture" : "CubeTexture"));
		throwError<ArgumentError>(kInvalidArgumentError,"data");
	}
	int format = formatbyte&0x7f;
	data->readByte(b1);
	if (b1 > 12)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray invalid texture width:"<<int(b1));
		throwError<ArgumentError>(kInvalidArgumentError,"data");
	}
	data->readByte(b2);
	if (b2 > 12)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray invalid texture height:"<<int(b2));
		throwError<ArgumentError>(kInvalidArgumentError,"data");
	}
	data->readByte(b3);
	if (b3 > 13)
	{
		LOG(LOG_ERROR,"uploadCompressedTextureFromByteArray invalid texture count:"<<int(b3));
		throwError<ArgumentError>(kInvalidArgumentError,"data");
	}
	width = 1<<b1;
	height = 1<<b2;
	uint32_t texcount = b2 * (forCubeTexture ? 6 : 1);
	
	if (bitmaparray.size() < texcount)
		bitmaparray.resize(texcount);
	uint32_t tmpwidth = width;
	uint32_t tmpheight = width;
	uint8_t* maintexbytes=nullptr;
	uint32_t maintexlen=0;
	for (uint32_t i = 0; i < texcount; i++)
	{
		switch (format)
		{
			case 0:
			case 1:
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
				if (bitmaparray[i].size() < tmpwidth*tmpheight*(format == 0 ? 3 : 4))
					bitmaparray[i].resize(tmpwidth*tmpheight*(format == 0 ? 3 : 4));
				if (texlen == 0)
				{
					if (maintexbytes)
					{
						// fallback to main image if no data for mipmap image available
						decodejxr(maintexbytes,maintexlen,bitmaparray[i],tmpwidth,tmpheight,format == 1);
					}
				}
				else
				{
					uint8_t* texbytes = new uint8_t[texlen];
					data->readBytes(data->getPosition(),texlen,texbytes);
					if (decodejxr(texbytes,texlen,bitmaparray[i],tmpwidth,tmpheight,format == 1))
					{
						if (maintexbytes == nullptr)
						{
							maintexbytes = texbytes;
							maintexlen = texlen;
							texbytes = nullptr;
						}
						else
							delete[] texbytes;
					}
					else
					{
						if (maintexbytes)
						{
							// fallback to main image if decoding of mipmap image fails
							decodejxr(maintexbytes,maintexlen,bitmaparray[i],tmpwidth,tmpheight,format == 1);
						}
					}
				}
				hasalpha = (format == 1);
				data->setPosition(data->getPosition()+texlen);
				tmpwidth >>= 1;
				tmpheight >>= 1;
				break;
			}
			default:
				LOG(LOG_NOT_IMPLEMENTED,"uploadCompressedTextureFromByteArray format not yet supported:"<<hex<<format);
				break;
		}
	}
	data->setPosition(endposition);
	if (maintexbytes)
		delete[] maintexbytes;
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
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(TextureBase,dispose)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextureBase.dispose does nothing");
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
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(async,false);
	if (data.isNull())
		throwError<TypeError>(kNullArgumentError);
	if (async)
		LOG(LOG_NOT_IMPLEMENTED,"Texture.uploadCompressedTextureFromByteArray async loading");
	th->parseAdobeTextureFormat(data.getPtr(),byteArrayOffset,false,th->hasalpha);
	th->context->addAction(RENDER_LOADTEXTURE,th);
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromBitmapData)
{
	Texture* th = asAtomHandler::as<Texture>(obj);
	uint32_t miplevel;
	_NR<BitmapData> source;
	ARG_UNPACK_ATOM(source)(miplevel,0);
	if (source.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > max(th->width,th->height)))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width<<" "<<th->height);
		throwError<ArgumentError>(kInvalidArgumentError,"miplevel");
	}
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
	th->context->addAction(RENDER_LOADTEXTURE,th);
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromByteArray)
{
	Texture* th = asAtomHandler::as<Texture>(obj);
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(miplevel,0);
	if (data.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > max(th->width,th->height)))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width<<" "<<th->height);
		throwError<ArgumentError>(kInvalidArgumentError,"miplevel");
	}
	if (th->bitmaparray.size() <= miplevel)
		th->bitmaparray.resize(miplevel+1);
	uint32_t mipsize = (th->width>>miplevel)*(th->height>>miplevel)*4;
	th->bitmaparray[miplevel].resize(mipsize);
	uint32_t bytesneeded = (th->height>>miplevel) * (th->width>>miplevel)*4;
	if (byteArrayOffset + bytesneeded > data->getLength())
	{
		LOG(LOG_ERROR,"not enough bytes to read");
		throwError<RangeError>(kParamRangeError);
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
	th->context->addAction(RENDER_LOADTEXTURE,th);
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
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(async,false);
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromBitmapData)
{
	CubeTexture* th = asAtomHandler::as<CubeTexture>(obj);
	_NR<BitmapData> source;
	uint32_t side;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(source)(side)(miplevel,0);
	if (source.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > th->width))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width);
		throwError<ArgumentError>(kInvalidArgumentError,"miplevel");
	}
	if (side > 5)
	{
		LOG(LOG_ERROR,"invalid side:"<<side);
		throwError<ArgumentError>(kInvalidArgumentError,"side");
	}
	uint32_t bitmap_size = 1<<(th->max_miplevel-(miplevel+1));
	if ((source->getWidth() != source->getHeight())
		|| (uint32_t(source->getWidth()) != bitmap_size))
	{
		LOG(LOG_ERROR,"invalid bitmap:"<<source->getWidth()<<" "<<source->getHeight()<<" "<< th->max_miplevel <<" "<< miplevel);
		throwError<ArgumentError>(kInvalidArgumentError,"source");
	}

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
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"CubeTexture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	uint32_t side;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(side)(miplevel,0);
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
	ARG_UNPACK_ATOM(source);

	if (source.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (th->bitmaparray.size() == 0)
		th->bitmaparray.resize(1);
	uint32_t bytesneeded = th->width*th->height*4;
	th->bitmaparray[0].resize(bytesneeded);

	assert_and_throw(th->height == source->getHeight() && th->width == source->getWidth());
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
	th->context->addAction(RENDER_LOADTEXTURE,th);
}
ASFUNCTIONBODY_ATOM(RectangleTexture,uploadFromByteArray)
{
	RectangleTexture* th = asAtomHandler::as<RectangleTexture>(obj);
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	ARG_UNPACK_ATOM(data)(byteArrayOffset);
	if (data.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (th->bitmaparray.size() == 0)
		th->bitmaparray.resize(1);
	uint32_t bytesneeded = th->height*th->width*4;
	if (byteArrayOffset + bytesneeded > data->getLength())
	{
		LOG(LOG_ERROR,"not enough bytes to read");
		throwError<RangeError>(kParamRangeError);
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
	th->context->addAction(RENDER_LOADTEXTURE,th);
}

void VideoTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	REGISTER_GETTER(c,videoHeight);
	REGISTER_GETTER(c,videoWidth);
	c->setDeclaredMethodByQName("attachCamera","",Class<IFunction>::getFunction(c->getSystemState(),attachCamera),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",Class<IFunction>::getFunction(c->getSystemState(),attachNetStream),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(VideoTexture,videoHeight);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(VideoTexture,videoWidth);

ASFUNCTIONBODY_ATOM(VideoTexture,attachCamera)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoTexture.attachCamera does nothing");
//	_NR<Camera> theCamera;
//	ARG_UNPACK_ATOM(theCamera);
}
ASFUNCTIONBODY_ATOM(VideoTexture,attachNetStream)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoTexture.attachNetStream does nothing");
	_NR<NetStream> netStream;
	ARG_UNPACK_ATOM(netStream);
}

}
