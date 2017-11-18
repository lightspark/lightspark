#include "flashdisplay3dtextures.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"

namespace lightspark
{

void TextureBase::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(TextureBase,dispose)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextureBase.dispose does nothing");
	return asAtom::invalidAtom;
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
	LOG(LOG_NOT_IMPLEMENTED,"Texture.uploadCompressedTextureFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	bool async;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(async,false);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromBitmapData)
{
	Texture* th = obj.as<Texture>();
	uint32_t miplevel;
	_NR<BitmapData> source;
	ARG_UNPACK_ATOM(source)(miplevel,0);
	if (source.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (miplevel > 0 && (1<<(miplevel-1) > max(th->width,th->height)))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width<<" "<<th->height);
		throwError<ArgumentError>(kInvalidArgumentError,"miplevel");
	}
	if (th->bitmaparray.size() <= miplevel)
		th->bitmaparray.resize(miplevel+4);
	th->bitmaparray[miplevel] = source->getBitmapContainer();
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"Texture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(miplevel,0);
	return asAtom::invalidAtom;
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
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromBitmapData)
{
	LOG(LOG_NOT_IMPLEMENTED,"CubeTexture.uploadFromBitmapData does nothing");
	_NR<BitmapData> source;
	uint32_t side;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(source)(side)(miplevel,0);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"CubeTexture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	uint32_t side;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(side)(miplevel,0);
	return asAtom::invalidAtom;
}

void RectangleTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(RectangleTexture,uploadFromBitmapData)
{
	LOG(LOG_NOT_IMPLEMENTED,"RectangleTexture.uploadFromBitmapData does nothing");
	_NR<BitmapData> source;
	ARG_UNPACK_ATOM(source);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(RectangleTexture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"RectangleTexture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	ARG_UNPACK_ATOM(data)(byteArrayOffset);
	return asAtom::invalidAtom;
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
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(VideoTexture,attachNetStream)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoTexture.attachNetStream does nothing");
	_NR<NetStream> netStream;
	ARG_UNPACK_ATOM(netStream);
	return asAtom::invalidAtom;
}

}
