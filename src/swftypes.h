/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SWFTYPES_H
#define SWFTYPES_H 1

#include "compat.h"
#include <iostream>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <cairo.h>

#include "forwards/swftypes.h"
#include "forwards/scripting/flash/display/DisplayObject.h"
#include "forwards/scripting/flash/display/flashdisplay.h"
#include "forwards/scripting/flash/system/flashsystem.h"
#include "forwards/tiny_string.h"
#include "logger.h"
#include <cstdlib>
#include <cassert>
#include "exceptions.h"
#include "smartrefs.h"
#include "memory_support.h"

#ifdef BIG_ENDIAN
#include <algorithm>
#endif
#define BUILTIN_STRINGS_CHAR_MAX 0x10000 // strings 0-0xffff are one-char strings of the corresponding unicode char
namespace lightspark
{
enum BUILTIN_STRINGS { EMPTY=0, STRING_WILDCARD='*', ANY=BUILTIN_STRINGS_CHAR_MAX, VOID, PROTOTYPE, STRING_FUNCTION,STRING_AS3VECTOR,STRING_CLASS,STRING_AS3NS,STRING_NAMESPACENS,STRING_XML,STRING_TOSTRING,STRING_VALUEOF,STRING_LENGTH,STRING_CONSTRUCTOR
					   ,STRING_AVM1_TARGET,STRING_THIS,STRING_AVM1_ROOT,STRING_AVM1_PARENT,STRING_AVM1_GLOBAL,STRING_SUPER
					   ,STRING_ONENTERFRAME,STRING_ONMOUSEMOVE,STRING_ONMOUSEDOWN,STRING_ONMOUSEUP,STRING_ONPRESS,STRING_ONRELEASE,STRING_ONRELEASEOUTSIDE,STRING_ONMOUSEWHEEL, STRING_ONLOAD
					   ,STRING_OBJECT,STRING_UNDEFINED,STRING_BOOLEAN,STRING_NUMBER,STRING_STRING,STRING_FUNCTION_LOWERCASE,STRING_ONROLLOVER,STRING_ONROLLOUT
					   ,STRING_PROTO,STRING_TARGET,STRING_FLASH_EVENTS_IEVENTDISPATCHER,STRING_ADDEVENTLISTENER,STRING_REMOVEEVENTLISTENER,STRING_DISPATCHEVENT,STRING_HASEVENTLISTENER
					   ,STRING_ONCONNECT,STRING_ONDATA,STRING_ONCLOSE,STRING_ONSELECT
					   ,STRING_ADD,STRING_ALPHA,STRING_DARKEN,STRING_DIFFERENCE,STRING_ERASE,STRING_HARDLIGHT,STRING_INVERT,STRING_LAYER,STRING_LIGHTEN,STRING_MULTIPLY,STRING_OVERLAY,STRING_SCREEN,STRING_SUBTRACT
					   ,STRING_TEXT
					   ,LAST_BUILTIN_STRING };
enum BUILTIN_NAMESPACES { EMPTY_NS=0, AS3_NS };


enum SWFOBJECT_TYPE { T_OBJECT=0, T_INTEGER=1, T_NUMBER=2, T_FUNCTION=3, T_UNDEFINED=4, T_NULL=5, T_STRING=6, 
	T_INVALID=7, T_BOOLEAN=8, T_ARRAY=9, T_CLASS=10, T_QNAME=11, T_NAMESPACE=12, T_UINTEGER=13, T_PROXY=14, T_TEMPLATE=15};
// this is used to avoid calls to dynamic_cast when testing for some classes
enum CLASS_SUBTYPE { SUBTYPE_NOT_SET, SUBTYPE_PROXY, SUBTYPE_REGEXP, SUBTYPE_XML, SUBTYPE_XMLLIST,SUBTYPE_DATE, SUBTYPE_INHERIT, SUBTYPE_OBJECTCONSTRUCTOR,SUBTYPE_FUNCTIONOBJECT
					 ,SUBTYPE_GLOBAL,SUBTYPE_FUNCTION,SUBTYPE_SYNTHETICFUNCTION,SUBTYPE_EVENT,SUBTYPE_WAITABLE_EVENT, SUBTYPE_INTERACTIVE_OBJECT, SUBTYPE_STAGE
					 ,SUBTYPE_LOADERINFO,SUBTYPE_CONTEXTMENU,SUBTYPE_BITMAP,SUBTYPE_BITMAPDATA,SUBTYPE_SOUND,SUBTYPE_SOUNDTRANSFORM,SUBTYPE_SOUNDCHANNEL,SUBTYPE_PROGRESSEVENT
					 ,SUBTYPE_VECTOR,SUBTYPE_DISPLAYOBJECT,SUBTYPE_DISPLAYOBJECTCONTAINER,SUBTYPE_CONTEXTMENUBUILTINITEMS, SUBTYPE_SHAREDOBJECT,SUBTYPE_TEXTFIELD,SUBTYPE_TEXTFORMAT
					 ,SUBTYPE_KEYBOARD_EVENT,SUBTYPE_MOUSE_EVENT,SUBTYPE_ROOTMOVIECLIP,SUBTYPE_MATRIX,SUBTYPE_POINT,SUBTYPE_RECTANGLE,SUBTYPE_COLORTRANSFORM,SUBTYPE_BYTEARRAY
					 ,SUBTYPE_APPLICATIONDOMAIN,SUBTYPE_LOADERCONTEXT,SUBTYPE_SPRITE,SUBTYPE_MOVIECLIP,SUBTYPE_TEXTBLOCK,SUBTYPE_FONTDESCRIPTION,SUBTYPE_CONTENTELEMENT,SUBTYPE_ELEMENTFORMAT
					 ,SUBTYPE_TEXTELEMENT, SUBTYPE_ACTIVATIONOBJECT,SUBTYPE_TEXTLINE,SUBTYPE_STAGE3D,SUBTYPE_MATRIX3D,SUBTYPE_INDEXBUFFER3D,SUBTYPE_PROGRAM3D,SUBTYPE_VERTEXBUFFER3D
					 ,SUBTYPE_CONTEXT3D,SUBTYPE_TEXTUREBASE,SUBTYPE_TEXTURE,SUBTYPE_CUBETEXTURE,SUBTYPE_RECTANGLETEXTURE,SUBTYPE_VIDEOTEXTURE,SUBTYPE_VECTOR3D,SUBTYPE_NETSTREAM
					 ,SUBTYPE_WORKER,SUBTYPE_WORKERDOMAIN,SUBTYPE_MUTEX,SUBTYPE_AVM1FUNCTION,SUBTYPE_SAMPLEDATA_EVENT
					 ,SUBTYPE_BITMAPFILTER,SUBTYPE_GLOWFILTER,SUBTYPE_DROPSHADOWFILTER,SUBTYPE_GRADIENTGLOWFILTER,SUBTYPE_BEVELFILTER,SUBTYPE_COLORMATRIXFILTER,SUBTYPE_BLURFILTER,SUBTYPE_CONVOLUTIONFILTER,SUBTYPE_DISPLACEMENTFILTER,SUBTYPE_GRADIENTBEVELFILTER,SUBTYPE_SHADERFILTER
					 ,SUBTYPE_THROTTLE_EVENT,SUBTYPE_CONTEXTMENUEVENT,SUBTYPE_GAMEINPUTEVENT, SUBTYPE_GAMEINPUTDEVICE, SUBTYPE_VIDEO, SUBTYPE_MESSAGECHANNEL, SUBTYPE_CONDITION
					 ,SUBTYPE_FILE, SUBTYPE_FILEMODE, SUBTYPE_FILESTREAM, SUBTYPE_FILEREFERENCE, SUBTYPE_DATAGRAMSOCKET, SUBTYPE_NATIVEWINDOW,SUBTYPE_EXTENSIONCONTEXT,SUBTYPE_SIMPLEBUTTON,SUBTYPE_SHAPE,SUBTYPE_MORPHSHAPE
					 ,SUBTYPE_URLLOADER,SUBTYPE_URLREQUEST,SUBTYPE_DICTIONARY,SUBTYPE_TEXTLINEMETRICS,SUBTYPE_XMLNODE,SUBTYPE_XMLDOCUMENT,SUBTYPE_LOADER
					 ,SUBTYPE_TEXTJUSTIFIER,SUBTYPE_SPACEJUSTIFIER,SUBTYPE_EASTASIANJUSTIFIER
					 ,SUBTYPE_ERROR,SUBTYPE_SECURITYERROR,SUBTYPE_ARGUMENTERROR,SUBTYPE_DEFINITIONERROR,SUBTYPE_EVALERROR,SUBTYPE_RANGEERROR,SUBTYPE_REFERENCEERROR,SUBTYPE_SYNTAXERROR,SUBTYPE_TYPEERROR,SUBTYPE_URIERROR,SUBTYPE_VERIFYERROR,SUBTYPE_UNINITIALIZEDERROR
					 ,SUBTYPE_AVM1SOUND,SUBTYPE_LOCALCONNECTION,SUBTYPE_NATIVEWINDOWBOUNDSEVENT,SUBTYPE_AVM1MOVIECLIP,SUBTYPE_AVM1MOVIECLIPLOADER
					 ,SUBTYPE_GRAPHICSENDFILL,SUBTYPE_GRAPHICSSOLIDFILL,SUBTYPE_GRAPHICSPATH,SUBTYPE_AVM1MOVIE,SUBTYPE_URLVARIABLES
				   };
 
enum STACK_TYPE{STACK_NONE=0,STACK_OBJECT,STACK_INT,STACK_UINT,STACK_NUMBER,STACK_BOOLEAN};

enum AS_BLENDMODE { BLENDMODE_NORMAL=0, BLENDMODE_LAYER=2, BLENDMODE_MULTIPLY=3,BLENDMODE_SCREEN=4,BLENDMODE_LIGHTEN=5,BLENDMODE_DARKEN=6, BLENDMODE_DIFFERENCE=7,
					BLENDMODE_ADD=8, BLENDMODE_SUBTRACT=9,BLENDMODE_INVERT=10,BLENDMODE_ALPHA=11,BLENDMODE_ERASE=12,BLENDMODE_OVERLAY=13,BLENDMODE_HARDLIGHT=14
				  };
enum SMOOTH_MODE { SMOOTH_NONE=0, SMOOTH_SUBPIXEL=1, SMOOTH_ANTIALIAS=2 };

enum HIT_TYPE { GENERIC_HIT, // point is over the object
				GENERIC_HIT_INVISIBLE, // ...even if the object is invisible
				MOUSE_CLICK_HIT, // point over the object and mouseEnabled
				DOUBLE_CLICK_HIT // point over the object and doubleClickEnabled
};
enum LS_VIDEO_CODEC { H264=0, H263, VP6, VP6A, GIF };
// "Audio coding formats" from Chapter 11 in SWF documentation (except for LINEAR_PCM_FLOAT_PLATFORM_ENDIAN)
enum LS_AUDIO_CODEC { CODEC_NONE=-1, LINEAR_PCM_PLATFORM_ENDIAN=0, ADPCM=1, MP3=2, LINEAR_PCM_LE=3, NELLYMOSER16=4, NELLYMOSER8=5, NELLYMOSER=6, AAC=10, SPEEX=11, LINEAR_PCM_FLOAT_PLATFORM_ENDIAN = 100 };

enum OBJECT_ENCODING { AMF0=0, AMF3=3, DEFAULT=3 };

enum DEPTHSTENCIL_FUNCTION { DEPTHSTENCIL_ALWAYS, DEPTHSTENCIL_EQUAL, DEPTHSTENCIL_GREATER, DEPTHSTENCIL_GREATER_EQUAL, DEPTHSTENCIL_LESS, DEPTHSTENCIL_LESS_EQUAL, DEPTHSTENCIL_NEVER, DEPTHSTENCIL_NOT_EQUAL };
enum DEPTHSTENCIL_OP { DEPTHSTENCIL_KEEP, DEPTHSTENCIL_ZERO, DEPTHSTENCIL_REPLACE, DEPTHSTENCIL_INCR, DEPTHSTENCIL_INCR_WRAP, DEPTHSTENCIL_DECR, DEPTHSTENCIL_DECR_WRAP, DEPTHSTENCIL_INVERT };
enum TRIANGLE_FACE { FACE_BACK, FACE_FRONT, FACE_FRONT_AND_BACK, FACE_NONE };
enum BLEND_FACTOR { BLEND_ONE,BLEND_ZERO,BLEND_SRC_ALPHA,BLEND_SRC_COLOR,BLEND_DST_ALPHA,BLEND_DST_COLOR,BLEND_ONE_MINUS_SRC_ALPHA,BLEND_ONE_MINUS_SRC_COLOR,BLEND_ONE_MINUS_DST_ALPHA,BLEND_ONE_MINUS_DST_COLOR };
enum VERTEXBUFFER_FORMAT { BYTES_4=0, FLOAT_1, FLOAT_2, FLOAT_3, FLOAT_4 };
enum CLEARMASK { COLOR = 0x1, DEPTH = 0x2, STENCIL = 0x4 };
enum TEXTUREFORMAT { BGRA, BGRA_PACKED, BGR_PACKED, COMPRESSED, COMPRESSED_ALPHA, RGBA_HALF_FLOAT,BGR };
enum TEXTUREFORMAT_COMPRESSED { UNCOMPRESSED, DXT5, DXT1 };
enum SAMPLEPOSITION { SAMPLEPOS_STANDARD=0,SAMPLEPOS_BLEND=1,SAMPLEPOS_FILTER=2,SAMPLEPOS_FILTER_DST=3 };

// Enum used during early binding in abc_optimizer.cpp
enum EARLY_BIND_STATUS { NOT_BINDED=0, CANNOT_BIND=1, BINDED };

inline std::ostream& operator<<(std::ostream& s, const STACK_TYPE& st)
{
	switch(st)
	{
	case STACK_NONE:
		s << "none";
		break;
	case STACK_OBJECT:
		s << "object";
		break;
	case STACK_INT:
		s << "int";
		break;
	case STACK_UINT:
		s << "uint";
		break;
	case STACK_NUMBER:
		s << "number";
		break;
	case STACK_BOOLEAN:
		s << "boolean";
		break;
	default:
		assert(false);
	}
	return s;
}

enum TRISTATE { TFALSE=0, TTRUE, TUNDEFINED };

enum FILE_TYPE { FT_UNKNOWN=0, FT_SWF, FT_COMPRESSED_SWF, FT_LZMA_COMPRESSED_SWF, FT_PNG, FT_JPEG, FT_GIF };

typedef double number_t;

class Type;
class SystemState;
class ASObject;
class ASString;
class ABCContext;
class URLInfo;
class AVM1Function;
class AVM1context;
struct namespace_info;

struct multiname;
class QName
{
public:
	uint32_t nsStringId;
	uint32_t nameId;
	QName(uint32_t _nameId, uint32_t _nsId):nsStringId(_nsId),nameId(_nameId){}
	bool operator<(const QName& r) const
	{
		if(nsStringId==r.nsStringId)
			return nameId<r.nameId;
		else
			return nsStringId<r.nsStringId;
	}
	bool operator==(const QName& r) const
	{
		return nameId==r.nameId && nsStringId==r.nsStringId;
	}
	tiny_string getQualifiedName(SystemState* sys, bool forDescribeType = false) const;
	operator multiname() const;
};

class UI8 
{
friend std::istream& operator>>(std::istream& s, UI8& v);
private:
	uint8_t val;
public:
	UI8():val(0){}
	UI8(uint8_t v):val(v){}
	operator uint8_t() const { return val; }
};

class UI16_SWF
{
friend std::istream& operator>>(std::istream& s, UI16_SWF& v);
protected:
	uint16_t val;
public:
	UI16_SWF():val(0){}
	UI16_SWF(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
};

class UI16_FLV
{
friend std::istream& operator>>(std::istream& s, UI16_FLV& v);
protected:
	uint16_t val;
public:
	UI16_FLV():val(0){}
	UI16_FLV(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
};

class SI16_SWF
{
friend std::istream& operator>>(std::istream& s, SI16_SWF& v);
protected:
	int16_t val;
public:
	SI16_SWF():val(0){}
	SI16_SWF(int16_t v):val(v){}
	operator int16_t() const { return val; }
};

class SI16_FLV
{
friend std::istream& operator>>(std::istream& s, SI16_FLV& v);
protected:
	int16_t val;
public:
	SI16_FLV():val(0){}
	SI16_FLV(int16_t v):val(v){}
	operator int16_t(){ return val; }
};

class UI24_SWF
{
friend std::istream& operator>>(std::istream& s, UI24_SWF& v);
protected:
	uint32_t val;
public:
	UI24_SWF():val(0){}
	operator uint32_t() const { return val; }
};

class UI24_FLV
{
friend std::istream& operator>>(std::istream& s, UI24_FLV& v);
protected:
	uint32_t val;
public:
	UI24_FLV():val(0){}
	operator uint32_t() const { return val; }
};

class SI24_SWF
{
friend std::istream& operator>>(std::istream& s, SI24_SWF& v);
protected:
	int32_t val;
public:
	SI24_SWF():val(0){}
	operator int32_t() const { return val; }
};

class SI24_FLV
{
friend std::istream& operator>>(std::istream& s, SI24_FLV& v);
protected:
	int32_t val;
public:
	SI24_FLV():val(0){}
	operator int32_t() const { return val; }
};

class UI32_SWF
{
friend std::istream& operator>>(std::istream& s, UI32_SWF& v);
protected:
	uint32_t val;
public:
	UI32_SWF():val(0){}
	UI32_SWF(uint32_t v):val(v){}
	operator uint32_t() const{ return val; }
};

class UI32_FLV
{
friend std::istream& operator>>(std::istream& s, UI32_FLV& v);
protected:
	uint32_t val;
public:
	UI32_FLV():val(0){}
	UI32_FLV(uint32_t v):val(v){}
	operator uint32_t() const{ return val; }
};

class u32
{
friend std::istream& operator>>(std::istream& in, u32& v);
private:
	uint32_t val;
public:
	inline operator uint32_t() const{return val;}
};

class STRING
{
friend std::ostream& operator<<(std::ostream& s, const STRING& r);
friend std::istream& operator>>(std::istream& stream, STRING& v);
friend class ASString;
private:
	std::string String;
public:
	STRING():String(){}
	STRING(const char* s):String(s)
	{
	}
	STRING(const char* s, int len):String(s,len)
	{
	}
	bool operator==(const STRING& s) const
	{
		if(String.size()!=s.String.size())
			return false;
		for(uint32_t i=0;i<String.size();i++)
		{
			if(String[i]!=s.String[i])
				return false;
		}
		return true;
	}
	/*STRING operator+(const STRING& s)
	{
		STRING ret(*this);
		for(unsigned int i=0;i<s.String.size();i++)
			ret.String.push_back(s.String[i]);
		return ret;
	}*/
	bool isNull() const
	{
		return !String.size();
	}
	operator const std::string&() const
	{
		return String;
	}
	operator const tiny_string() const
	{
		return tiny_string(String);
	}
	operator const char*() const
	{
		return String.c_str();
	}
	int size()
	{
		return String.size();
	}
};

//Numbers taken from AVM2 specs
enum NS_KIND { NAMESPACE=0x08, PACKAGE_NAMESPACE=0x16, PACKAGE_INTERNAL_NAMESPACE=0x17, PROTECTED_NAMESPACE=0x18, 
			EXPLICIT_NAMESPACE=0x19, STATIC_PROTECTED_NAMESPACE=0x1A, PRIVATE_NAMESPACE=0x05 };
class RootMovieClip;
struct nsNameAndKindImpl
{
	uint32_t nameId;
	NS_KIND kind;
	uint32_t baseId;
	// this is null for all namespaces except kind PROTECTED_NAMESPACE, to ensure they are treated as different namespaces when declared in different swf files
	ApplicationDomain* applicationDomain;
	nsNameAndKindImpl(uint32_t _nameId, NS_KIND _kind, ApplicationDomain* appDomain=nullptr,uint32_t b=-1);
	bool operator<(const nsNameAndKindImpl& r) const
	{
		if(applicationDomain != r.applicationDomain)
			return applicationDomain<r.applicationDomain;
		if(kind==r.kind)
			return nameId < r.nameId;
		else
			return kind < r.kind;
	}
	bool operator>(const nsNameAndKindImpl& r) const
	{
		if(applicationDomain != r.applicationDomain)
			return applicationDomain>r.applicationDomain;
		if(kind==r.kind)
			return nameId > r.nameId;
		else
			return kind > r.kind;
	}
};

struct nsNameAndKind
{
	uint32_t nsId;
	uint32_t nsRealId;
	uint32_t nsNameId;
	NS_KIND kind;
	nsNameAndKind():nsId(0),nsRealId(0),nsNameId(BUILTIN_STRINGS::EMPTY),kind(NAMESPACE) {}
	nsNameAndKind(SystemState *sys, const tiny_string& _name, NS_KIND _kind);
	nsNameAndKind(SystemState* sys,const char* _name, NS_KIND _kind);
	nsNameAndKind(SystemState* sys, uint32_t _nameId, NS_KIND _kind);
	nsNameAndKind(SystemState* sys, uint32_t _nameId, NS_KIND _kind, ApplicationDomain* appDomain);
	nsNameAndKind(ABCContext * c, uint32_t nsContextIndex);
	/*
	 * Special constructor for protected namespace, which have
	 * different representationId
	 */
	nsNameAndKind(SystemState* sys, uint32_t _nameId, uint32_t _baseId, NS_KIND _kind, ApplicationDomain* appDomain);
	/*
	 * Special version to create the empty bultin namespace
	 */
	nsNameAndKind(uint32_t id):nsId(id),nsRealId(id)
	{
		assert(nsId==0);
	}
	inline bool operator<(const nsNameAndKind& r) const
	{
		return nsId < r.nsId;
	}
	inline bool operator>(const nsNameAndKind& r) const
	{
		return nsId > r.nsId;
	}
	inline bool operator==(const nsNameAndKind& r) const
	{
		return nsId==r.nsId;
	}
	inline bool hasEmptyName() const
	{
		return nsId==0;
	}
	inline bool hasBuiltinName() const
	{
		return nsNameId==STRING_AS3NS;
	}
};

struct multiname: public memory_reporter
{
	uint32_t name_s_id;
	union
	{
		int32_t name_i;
		uint32_t name_ui;
		number_t name_d;
		ASObject* name_o;
	};
	std::vector<nsNameAndKind, reporter_allocator<nsNameAndKind>> ns;
	Type* cachedType;
	std::vector<multiname*> templateinstancenames;
	enum NAME_TYPE {NAME_STRING,NAME_INT,NAME_UINT,NAME_NUMBER,NAME_OBJECT};
	NAME_TYPE name_type:3;
	bool isAttribute:1;
	bool isStatic:1;
	bool hasEmptyNS:1;
	bool hasBuiltinNS:1;
	bool hasGlobalNS:1;
	bool isInteger:1;
	multiname(MemoryAccount* m):name_s_id(UINT32_MAX),name_o(nullptr),ns(reporter_allocator<nsNameAndKind>(m)),cachedType(nullptr),name_type(NAME_OBJECT),isAttribute(false),isStatic(true),hasEmptyNS(true),hasBuiltinNS(false),hasGlobalNS(true),isInteger(false)
	{
	}
	
	/*
		Returns a string name whatever is the name type
	*/
	const tiny_string normalizedName(SystemState *sys) const;
	/*
	 * 	Return a string id whatever is the name type
	 */
	uint32_t normalizedNameId(SystemState *sys) const;
	/*
		Returns a string name whatever is the name type, but does not resolve NAME_OBJECT names
		this should be used for exception or debug messages to avoid calling 
		overridden toString property of the object
	*/
	const tiny_string normalizedNameUnresolved(SystemState *sys) const;

	const tiny_string qualifiedString(SystemState *sys, bool forDescribeType=false) const;
	/* sets name_type, name_s/name_d based on the object n */
	void setName(union asAtom &n, ASWorker* w);
	void resetNameIfObject();
	inline bool isQName() const { return ns.size() == 1; }
	bool toUInt(SystemState *sys, uint32_t& out, bool acceptStringFractions=false, bool* isNumber=nullptr, bool forAVM1=false) const;
	inline bool isEmpty() const { return name_type == NAME_OBJECT && name_o == nullptr;}
};

class FLOAT 
{
friend std::istream& operator>>(std::istream& s, FLOAT& v);
private:
	float val;
public:
	FLOAT():val(0){}
	FLOAT(float v):val(v){}
	operator float(){ return val; }
	inline void read(const uint8_t* data)
	{
		union float_reader
		{
			uint32_t dump;
			float value;
		};
		float_reader dummy;
		dummy.dump = *(uint32_t*)data;
		dummy.dump=GINT32_FROM_LE(dummy.dump);
		val=dummy.value;
	}
};

class DOUBLE 
{
friend std::istream& operator>>(std::istream& s, DOUBLE& v);
private:
	double val;
public:
	DOUBLE():val(0){}
	DOUBLE(double v):val(v){}
	operator double(){ return val; }
	void read(const uint8_t* data)
	{
		union double_reader
		{
			uint64_t dump;
			double value;
		};
		double_reader dummy;
		// "Wacky format" is 45670123. Thanks to Gnash for reversing :-)
		uint8_t* p = (uint8_t*)&dummy.dump;
		*p++ = data[4];
		*p++ = data[5];
		*p++ = data[6];
		*p++ = data[7];
		*p++ = data[0];
		*p++ = data[1];
		*p++ = data[2];
		*p++ = data[3];
		dummy.dump=GINT64_FROM_LE(dummy.dump);
		val=dummy.value;
	}
};

class FIXED
{
friend std::istream& operator>>(std::istream& s, FIXED& v);
protected:
	int32_t val;
public:
	FIXED():val(0){}
	FIXED(int32_t v):val(v){}
	operator number_t() const{ return number_t(val)/65536.0; }
};

class FIXED8
{
friend std::istream& operator>>(std::istream& s, FIXED8& v);
protected:
	int16_t val;
public:
	FIXED8():val(0){}
	FIXED8(int16_t v):val(v){}
	operator number_t() const{ return number_t(val)/256.0; }
};

class RECORDHEADER
{
friend std::istream& operator>>(std::istream& s, RECORDHEADER& v);
private:
	UI32_SWF Length;
	UI16_SWF CodeAndLen;
public:
	unsigned int getLength() const
	{
		if((CodeAndLen&0x3f)==0x3f)
			return Length;
		else
			return CodeAndLen&0x3f;
	}
	unsigned int getHeaderSize() const
	{
		if((CodeAndLen&0x3f)==0x3f)
			return sizeof(uint16_t) + sizeof(uint32_t);
		else
			return sizeof(uint16_t);
	}
	unsigned int getTagType() const
	{
		return CodeAndLen>>6;
	}
};

class RGB
{
public:
	RGB(){}
	RGB(int r,int g, int b):Red(r),Green(g),Blue(b){}
	RGB(uint32_t color):Red((color>>16)&0xFF),Green((color>>8)&0xFF),Blue(color&0xFF){}
	//Parses a color from hex triplet string #RRGGBB
	RGB(const tiny_string& colorstr);
	UI8 Red;
	UI8 Green;
	UI8 Blue;
	uint32_t toUInt() const { return Blue + (Green<<8) + (Red<<16); }
	//Return a string representation in #RRGGBB format
	tiny_string toString() const;
};

class RGBA
{
public:
	RGBA():Red(0),Green(0),Blue(0),Alpha(255){}
	RGBA(int r, int g, int b, int a):Red(r),Green(g),Blue(b),Alpha(a){}
	RGBA(uint32_t color, int a):Red((color>>16)&0xFF),Green((color>>8)&0xFF),Blue(color&0xFF),Alpha(a){}
	UI8 Red;
	UI8 Green;
	UI8 Blue;
	UI8 Alpha;
	RGBA& operator=(const RGB& r)
	{
		Red=r.Red;
		Green=r.Green;
		Blue=r.Blue;
		Alpha=255;
		return *this;
	}
	bool operator==(const RGBA& r) const
	{
		return Red==r.Red && Green==r.Green && Blue==r.Blue && Alpha==r.Alpha;
	}
	float rf() const
	{
		float ret=Red;
		ret/=255;
		return ret;
	}
	float gf() const
	{
		float ret=Green;
		ret/=255;
		return ret;
	}
	float bf() const
	{
		float ret=Blue;
		ret/=255;
		return ret;
	}
	float af() const
	{
		float ret=Alpha;
		ret/=255;
		return ret;
	}
};

typedef UI8 LANGCODE;

std::istream& operator>>(std::istream& s, RGB& v);

inline std::istream& operator>>(std::istream& s, UI8& v)
{
	s.read((char*)&v.val,1);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI16_SWF& v)
{
	s.read((char*)&v.val,2);
	v.val=GINT16_FROM_LE(v.val);
	return s;
}

inline std::istream & operator>>(std::istream &s, SI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=GINT16_FROM_BE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_SWF& v)
{
	s.read((char*)&v.val,2);
	v.val=GUINT16_FROM_LE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=GUINT16_FROM_BE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI24_SWF& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=LittleEndianToUnsignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI24_FLV& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=BigEndianToUnsignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI24_SWF& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=LittleEndianToSignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI24_FLV& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=BigEndianToSignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32_SWF& v)
{
	s.read((char*)&v.val,4);
	v.val=GUINT32_FROM_LE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32_FLV& v)
{
	s.read((char*)&v.val,4);
	v.val=GUINT32_FROM_BE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& in, u32& v)
{
	int i=0;
	v.val=0;
	uint8_t t;
	do
	{
		in.read((char*)&t,1);
		//No more than 5 bytes should be read
		if(i==28)
		{
			//Only the first 4 bits should be used to reach 32 bits
			if((t&0xf0))
				LOG(LOG_ERROR,"Error in u32");
			uint8_t t2=(t&0xf);
			v.val|=(t2<<i);
			break;
		}
		else
		{
			uint8_t t2=(t&0x7f);
			v.val|=(t2<<i);
			i+=7;
		}
	}
	while(t&0x80);
	return in;
}

inline std::istream& operator>>(std::istream& s, FLOAT& v)
{
	union float_reader
	{
		uint32_t dump;
		float value;
	};
	float_reader dummy;
	s.read((char*)&dummy.dump,4);
	dummy.dump=GINT32_FROM_LE(dummy.dump);
	v.val=dummy.value;
	return s;
}

inline std::istream& operator>>(std::istream& s, DOUBLE& v)
{
	union double_reader
	{
		uint64_t dump;
		double value;
	};
	double_reader dummy;
	// "Wacky format" is 45670123. Thanks to Gnash for reversing :-)
	s.read(((char*)&dummy.dump)+4,4);
	s.read(((char*)&dummy.dump),4);
	dummy.dump=GINT64_FROM_LE(dummy.dump);
	v.val=dummy.value;
	return s;
}

inline std::istream& operator>>(std::istream& s, FIXED& v)
{
	s.read((char*)&v.val,4);
	v.val=GINT32_FROM_LE(v.val);
	return s;
}
inline std::istream& operator>>(std::istream& s, FIXED8& v)
{
	s.read((char*)&v.val,2);
	v.val=GINT16_FROM_LE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, RECORDHEADER& v)
{
	s >> v.CodeAndLen;
	if((v.CodeAndLen&0x3f)==0x3f)
		s >> v.Length;
	return s;
}

class BitStream
{
public:
	std::istream& f;
	unsigned char buffer;
	unsigned char pos;
public:
	BitStream(std::istream& in):f(in),buffer(0),pos(0){}
	unsigned int readBits(unsigned int num)
	{
		unsigned int ret=0;
		while(num)
		{
			if(!pos)
			{
				pos=8;
				f.read((char*)&buffer,1);
			}
			ret<<=1;
			ret|=(buffer>>(pos-1))&1;
			pos--;
			num--;
		}
		return ret;
	}
	// discards 'num' bits (padding)
	void discard(unsigned int num)
	{
		readBits(num);
	}
};

class FB
{
	int32_t buf;
public:
	FB():buf(0){}
	FB(int s,BitStream& stream)
	{
		if(s>32)
			LOG(LOG_ERROR,"Fixed point bit field wider than 32 bit not supported");
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator float() const
	{
		if(buf>=0)
		{
			int32_t b=buf;
			return float(b)/65536.0f;
		}
		else
		{
			int32_t b=-buf;
			return -(float(b)/65536.0f);
		}
		//return (buf>>16)+(buf&0xffff)/65536.0f;
	}
};

class UB
{
	uint32_t buf;
public:
	UB():buf(0) {}
	UB(int s,BitStream& stream)
	{
/*		if(s%8)
			buf=new uint8_t[s/8+1];
		else
			buf=new uint8_t[s/8];
		int i=0;
		while(!s)
		{
			buf[i]=stream.readBits(imin(s,8));
			s-=imin(s,8);
			i++;
		}*/
		if(s>32)
			LOG(LOG_ERROR,"Unsigned bit field wider than 32 bit not supported");
		buf=stream.readBits(s);
	}
	operator int() const
	{
		return buf;
	}
};

class SB
{
	int32_t buf;
public:
	SB():buf(0) {}
	SB(int s,BitStream& stream)
	{
		if(s>32)
			LOG(LOG_ERROR,"Signed bit field wider than 32 bit not supported");
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator int() const
	{
		return buf;
	}
};

struct RectF;
class RECT
{
	friend std::ostream& operator<<(std::ostream& s, const RECT& r);
	friend std::istream& operator>>(std::istream& stream, RECT& v);
public:
	int32_t Xmin;
	int32_t Xmax;
	int32_t Ymin;
	int32_t Ymax;
public:
	RECT();
	RECT(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax);
	RECT(const RECT& r):Xmin(r.Xmin),Xmax(r.Xmax),Ymin(r.Ymin),Ymax(r.Ymax) {}
	RECT& operator=(const RECT& r)
	{
		Xmin=r.Xmin;
		Xmax=r.Xmax;
		Ymin=r.Ymin;
		Ymax=r.Ymax;
		return *this;
	}
	bool operator==(const RECT& r) const;
	operator RectF() const;
};

template<class T> class Vector2Tmpl;
typedef Vector2Tmpl<int32_t> Vector2;
typedef Vector2Tmpl<double> Vector2f;

class MATRIX: public cairo_matrix_t
{
	friend std::istream& operator>>(std::istream& stream, MATRIX& v);
	friend std::ostream& operator<<(std::ostream& s, const MATRIX& r);
public:
	MATRIX(number_t sx=1, number_t sy=1, number_t sk0=0, number_t sk1=0, number_t tx=0, number_t ty=0);
	void get4DMatrix(float matrix[16]) const;
	void multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	Vector2f multiply2D(const Vector2f& in) const;
	Vector2 multiply2D(const Vector2& in) const;
	MATRIX multiplyMatrix(const MATRIX& r) const;
	bool operator!=(const MATRIX& r) const;
	bool operator==(const MATRIX& r) const;
	MATRIX getInverted() const;
	bool isInvertible() const;
	number_t getTranslateX() const
	{
		return x0;
	}
	number_t getTranslateY() const
	{
		return y0;
	}
	number_t getScaleX() const;
	number_t getScaleY() const;
	number_t getRotation() const;
	/*
	 * Implement flash style premultiply matrix operators
	 */
	void rotate(number_t angle)
	{
		cairo_matrix_t tmp;
		cairo_matrix_init_rotate(&tmp,angle);
		cairo_matrix_multiply(this,this,&tmp);
	}
	void scale(number_t sx, number_t sy)
	{
		cairo_matrix_t tmp;
		cairo_matrix_init_scale(&tmp,sx,sy);
		cairo_matrix_multiply(this,this,&tmp);
	}
	void translate(number_t dx, number_t dy)
	{
		cairo_matrix_t tmp;
		cairo_matrix_init_translate(&tmp,dx,dy);
		cairo_matrix_multiply(this,this,&tmp);
	}
};

class GRADRECORD
{
	friend std::istream& operator>>(std::istream& s, GRADRECORD& v);
public:
	GRADRECORD(uint8_t v):version(v){}
	RGBA Color;
	uint8_t version;
	UI8 Ratio;
	bool operator<(const GRADRECORD& g) const
	{
		return Ratio<g.Ratio;
	}
	bool operator==(const GRADRECORD& g) const
	{
		return Color == g.Color && version == g.version && Ratio == g.Ratio;
	}
};

class GRADIENT
{
	friend std::istream& operator>>(std::istream& s, GRADIENT& v);
public:
	GRADIENT(uint8_t v, bool _isFocal):isFocal(_isFocal),SpreadMode(0),InterpolationMode(0),version(v) {}
	std::vector<GRADRECORD> GradientRecords;
	FIXED8 FocalPoint;
	bool isFocal:1;
	uint8_t SpreadMode:2;
	uint8_t InterpolationMode:2;
	uint8_t version;
	bool operator==(const GRADIENT& g) const
	{
		return version == g.version 
				&& SpreadMode == g.SpreadMode
				&& InterpolationMode == g.InterpolationMode
			    && FocalPoint == g.FocalPoint
				&& GradientRecords == g.GradientRecords;
	}
};

class FILLSTYLEARRAY;
class MORPHFILLSTYLE;

enum FILL_STYLE_TYPE { SOLID_FILL=0x00, LINEAR_GRADIENT=0x10, RADIAL_GRADIENT=0x12, FOCAL_RADIAL_GRADIENT=0x13, REPEATING_BITMAP=0x40,
			CLIPPED_BITMAP=0x41, NON_SMOOTHED_REPEATING_BITMAP=0x42, NON_SMOOTHED_CLIPPED_BITMAP=0x43};

class BitmapContainer;

class FILLSTYLE
{
public:
	FILLSTYLE(uint8_t v);
	FILLSTYLE(const FILLSTYLE& r);
	FILLSTYLE& operator=(const FILLSTYLE& r);
	bool operator==(const FILLSTYLE& r) const;
	virtual ~FILLSTYLE();
	MATRIX Matrix;
	GRADIENT Gradient;
	_NR<BitmapContainer> bitmap;
	RECT ShapeBounds;
	RGBA Color;
	FILL_STYLE_TYPE FillStyleType;
	uint8_t version;
};

class MORPHFILLSTYLE:public FILLSTYLE
{
public:
	MORPHFILLSTYLE():FILLSTYLE(1){}
	MATRIX StartGradientMatrix;
	MATRIX EndGradientMatrix;
	MATRIX StartBitmapMatrix;
	MATRIX EndBitmapMatrix;
	std::vector<UI8> StartRatios;
	std::vector<UI8> EndRatios;
	std::vector<RGBA> StartColors;
	std::vector<RGBA> EndColors;
	RGBA StartColor;
	RGBA EndColor;
	
	// FOCAL_RADIAL_GRADIENT
	int SpreadMode;
	int InterpolationMode;
	FIXED8 StartFocalPoint;
	FIXED8 EndFocalPoint;
	std::map<uint16_t,FILLSTYLE> fillstylecache;

	~MORPHFILLSTYLE(){}
};

class LINESTYLE
{
public:
	LINESTYLE(uint8_t v):version(v){}
	RGBA Color;
	UI16_SWF Width;
	uint8_t version;
};

class LINESTYLE2
{
public:
	LINESTYLE2(uint8_t v):StartCapStyle(0),JointStyle(0),EndCapStyle(0),HasFillFlag(false),NoHScaleFlag(false),NoVScaleFlag(false),PixelHintingFlag(false),NoClose(false),FillType(v),version(v){}
	LINESTYLE2(const LINESTYLE2& r);
	LINESTYLE2& operator=(const LINESTYLE2& r);
	bool operator==(const LINESTYLE2& r) const;
	virtual ~LINESTYLE2();
	uint8_t StartCapStyle;
	uint8_t JointStyle;
	uint8_t EndCapStyle;
	bool HasFillFlag:1;
	bool NoHScaleFlag:1;
	bool NoVScaleFlag:1;
	bool PixelHintingFlag:1;
	bool NoClose:1;
	UI16_SWF Width;
	UI16_SWF MiterLimitFactor;
	RGBA Color;
	FILLSTYLE FillType;
	uint8_t version;
};

class MORPHLINESTYLE
{
public:
	UI16_SWF StartWidth;
	UI16_SWF EndWidth;
	RGBA StartColor;
	RGBA EndColor;
};

class MORPHLINESTYLE2: public MORPHLINESTYLE
{
public:
	uint8_t StartCapStyle;
	MORPHFILLSTYLE FillType;
	uint8_t JoinStyle;
	bool HasFillFlag:1;
	bool NoHScaleFlag:1;
	bool NoVScaleFlag:1;
	bool PixelHintingFlag:1;
	bool NoClose:1;
	uint8_t EndCapStyle;
	UI16_SWF MiterLimitFactor;
	std::map<uint16_t,LINESTYLE2> linestylecache;
};

class LINESTYLEARRAY
{
public:
	LINESTYLEARRAY(uint8_t v):version(v){}
	void appendStyles(const LINESTYLEARRAY& r);
	std::list<LINESTYLE> LineStyles;
	std::list<LINESTYLE2> LineStyles2;
	uint8_t version;
};

class MORPHLINESTYLEARRAY
{
public:
	MORPHLINESTYLEARRAY(uint8_t v):version(v){}
	std::list<MORPHLINESTYLE> LineStyles;
	std::list<MORPHLINESTYLE2> LineStyles2;
	const uint8_t version;
};

class FILLSTYLEARRAY
{
public:
	FILLSTYLEARRAY(uint8_t v):version(v){}
	void appendStyles(const FILLSTYLEARRAY& r);
	std::list<FILLSTYLE> FillStyles;
	uint8_t version;
};

class MORPHFILLSTYLEARRAY
{
public:
	std::list<MORPHFILLSTYLE> FillStyles;
};

class SHAPE;
class SHAPEWITHSTYLE;

class SHAPERECORD
{
public:
	union
	{
		int32_t AnchorDeltaX;
		struct
		{
			uint16_t FillStyle1;
			uint16_t FillStyle0;
		};
	};
	union
	{
		int32_t AnchorDeltaY:18;
		uint16_t LineStyle;
	};
	int32_t DeltaX:18;
	int32_t DeltaY:18;
	bool TypeFlag:1;
	bool StateNewStyles:1;
	bool StateLineStyle:1;
	bool StateFillStyle1:1;
	bool StateFillStyle0:1;
	bool StateMoveTo:1;
	bool StraightFlag:1;
	SHAPERECORD(SHAPE* parent,BitStream& bs);
};

class TEXTRECORD;

class GLYPHENTRY
{
public:
	UB GlyphIndex;
	SB GlyphAdvance;
	GLYPHENTRY(TEXTRECORD* p,BitStream& bs);
};

class DefineTextTag;

class TEXTRECORD
{
public:
	std::vector <GLYPHENTRY> GlyphEntries;
	DefineTextTag* parent;
	RGBA TextColor;
	SI16_SWF XOffset;
	SI16_SWF YOffset;
	UI16_SWF TextHeight;
	UI16_SWF FontID;
	bool StyleFlagsHasColor:1;
	bool StyleFlagsHasYOffset:1;
	bool StyleFlagsHasXOffset:1;
	bool empty:1;
	TEXTRECORD(DefineTextTag* p):parent(p){}
};
class CharacterRenderer;
class SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPE& v);
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	SHAPE(uint8_t v=0,bool _forfont=false):fillOffset(0),lineOffset(0),version(v),forfont(_forfont){}
	virtual ~SHAPE();
	uint8_t NumFillBits:4;
	uint8_t NumLineBits:4;
	uint16_t fillOffset;
	uint16_t lineOffset;
	uint8_t version:3; /* version of the DefineShape tag, 0 if
			  * DefineFont or other tag */
	bool forfont:1;
	std::vector<SHAPERECORD> ShapeRecords;
};

class SHAPEWITHSTYLE : public SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	SHAPEWITHSTYLE(uint8_t v):SHAPE(v),FillStyles(v),LineStyles(v){}
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
};

class CXFORMWITHALPHA
{
	friend std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
private:
	int16_t RedMultTerm;
	int16_t GreenMultTerm;
	int16_t BlueMultTerm;
	int16_t AlphaMultTerm;
	int16_t RedAddTerm;
	int16_t GreenAddTerm;
	int16_t BlueAddTerm;
	int16_t AlphaAddTerm;
public:
	void getParameters(number_t& redMultiplier, 
			   number_t& greenMultiplier, 
			   number_t& blueMultiplier,
			   number_t& alphaMultiplier,
			   number_t& redOffset,
			   number_t& greenOffset,
			   number_t& blueOffset,
			   number_t& alphaOffset) const;
	float transformedAlpha(float alpha) const;
	bool isIdentity() const;
};

class DROPSHADOWFILTER
{
public:
	DROPSHADOWFILTER():Passes(0){}
	RGBA DropShadowColor;
	FIXED BlurX;
	FIXED BlurY;
	FIXED Angle;
	FIXED Distance;
	FIXED8 Strength;
	bool InnerShadow:1;
	bool Knockout:1;
	bool CompositeSource:1;
	uint8_t Passes:4;
};

class BLURFILTER
{
public:
	BLURFILTER():Passes(0){}
	FIXED BlurX;
	FIXED BlurY;
	uint8_t Passes:4;
};

class GLOWFILTER
{
public:
	GLOWFILTER():Passes(0){}
	RGBA GlowColor;
	FIXED BlurX;
	FIXED BlurY;
	FIXED8 Strength;
	bool InnerGlow:1;
	bool Knockout:1;
	bool CompositeSource:1;
	uint8_t Passes:4;
};

class BEVELFILTER
{
public:
	BEVELFILTER():Passes(0){}
	RGBA ShadowColor;
	RGBA HighlightColor;
	FIXED BlurX;
	FIXED BlurY;
	FIXED Angle;
	FIXED Distance;
	FIXED8 Strength;
	bool InnerShadow:1;
	bool Knockout:1;
	bool CompositeSource:1;
	bool OnTop:1;
	uint8_t Passes:4;
};

class GRADIENTGLOWFILTER
{
public:
	GRADIENTGLOWFILTER():GradientColors(nullptr),GradientRatio(nullptr),Passes(0){}
	~GRADIENTGLOWFILTER()
	{
		if (GradientColors)
			delete[] GradientColors;
		if(GradientRatio)
			delete[] GradientRatio;
	}
	RGBA* GradientColors;
	UI8* GradientRatio;
	UI8 NumColors;
	FIXED BlurX;
	FIXED BlurY;
	FIXED Angle;
	FIXED Distance;
	FIXED8 Strength;
	bool InnerGlow:1;
	bool Knockout:1;
	bool CompositeSource:1;
	bool OnTop:1;
	uint8_t Passes:4;
};

class CONVOLUTIONFILTER
{
public:
	CONVOLUTIONFILTER():Matrix(nullptr){}
	~CONVOLUTIONFILTER()
	{
		if (Matrix)
			delete[] Matrix;
	}
	FLOAT Divisor;
	FLOAT Bias;
	FLOAT* Matrix;
	RGBA DefaultColor;
	UI8 MatrixX;
	UI8 MatrixY;
	bool Clamp:1;
	bool PreserveAlpha:1;
};

class COLORMATRIXFILTER
{
public:
	COLORMATRIXFILTER(){}
	FLOAT Matrix[20];
};

class GRADIENTBEVELFILTER
{
public:
	GRADIENTBEVELFILTER():GradientColors(nullptr),GradientRatio(nullptr),Passes(0){}
	~GRADIENTBEVELFILTER()
	{
		if (GradientColors)
			delete[] GradientColors;
		if(GradientRatio)
			delete[] GradientRatio;
	}
	RGBA* GradientColors;
	UI8* GradientRatio;
	UI8 NumColors;
	FIXED BlurX;
	FIXED BlurY;
	FIXED Angle;
	FIXED Distance;
	FIXED8 Strength;
	bool InnerShadow:1;
	bool Knockout:1;
	bool CompositeSource:1;
	bool OnTop:1;
	uint8_t Passes:4;
};

class FILTER
{
public:
	FILTER() {}
	~FILTER() {}
	FILTER(const FILTER& f);
	FILTER& operator=(const FILTER& f);
	enum FILTER_ID { FILTER_DROPSHADOW = 0, FILTER_BLUR = 1, FILTER_GLOW = 2, FILTER_BEVEL = 3, FILTER_GRADIENTGLOW = 4, FILTER_CONVOLUTION = 5, FILTER_COLORMATRIX = 6, FILTER_GRADIENTBEVEL = 7 };
	UI8 FilterID;
	union
	{
		DROPSHADOWFILTER DropShadowFilter;
		BLURFILTER BlurFilter;
		GLOWFILTER GlowFilter;
		BEVELFILTER BevelFilter;
		GRADIENTGLOWFILTER GradientGlowFilter;
		CONVOLUTIONFILTER ConvolutionFilter;
		COLORMATRIXFILTER ColorMatrixFilter;
		GRADIENTBEVELFILTER GradientBevelFilter;
	};
};

class FILTERLIST
{
public:
	FILTERLIST():Filters(nullptr){}
	FILTERLIST(const FILTERLIST& r)
	{
		NumberOfFilters=r.NumberOfFilters;
		if (r.Filters)
		{
			Filters = new FILTER[NumberOfFilters];
			for (uint32_t i=0; i < NumberOfFilters; i++)
				Filters[i]=r.Filters[i];
		}
		else
			Filters=nullptr;
	}
	~FILTERLIST()
	{
		if (Filters)
			delete[] Filters;
	}
	UI8 NumberOfFilters;
	FILTER* Filters;
};

class BUTTONRECORD
{
public:
	BUTTONRECORD(uint8_t v):buttonVersion(v){}
	uint8_t buttonVersion:2;
	bool ButtonHasBlendMode:1;
	bool ButtonHasFilterList:1;
	bool ButtonStateHitTest:1;
	bool ButtonStateDown:1;
	bool ButtonStateOver:1;
	bool ButtonStateUp:1;
	MATRIX PlaceMatrix;
	FILTERLIST FilterList;
	CXFORMWITHALPHA	ColorTransform;
	UI16_SWF CharacterID;
	UI16_SWF PlaceDepth;
	UI8 BlendMode;

	bool isNull() const
	{
		return !(ButtonHasBlendMode | ButtonHasFilterList | ButtonStateHitTest | ButtonStateDown | ButtonStateOver | ButtonStateUp);
	}
};

class CLIPEVENTFLAGS
{
private:
	uint32_t swfversion;
public:
	CLIPEVENTFLAGS(uint32_t v):swfversion(v) {}
	bool ClipEventKeyUp:1;
	bool ClipEventKeyDown:1;
	bool ClipEventMouseUp:1;
	bool ClipEventMouseDown:1;
	bool ClipEventMouseMove:1;
	bool ClipEventUnload:1;
	bool ClipEventEnterFrame:1;
	bool ClipEventLoad:1;
	bool ClipEventDragOver:1;
	bool ClipEventRollOut:1;
	bool ClipEventRollOver:1;
	bool ClipEventReleaseOutside:1;
	bool ClipEventRelease:1;
	bool ClipEventPress:1;
	bool ClipEventInitialize:1;
	bool ClipEventData:1;
	bool ClipEventConstruct:1;
	bool ClipEventKeyPress:1;
	bool ClipEventDragOut:1;
	bool isNull();
	uint32_t getSWFVersion() const { return swfversion; }
};

class AdditionalDataTag;
class ACTIONRECORD;
class CLIPACTIONRECORD
{
public:
	CLIPACTIONRECORD(uint32_t v, uint32_t _dataskipbytes,AdditionalDataTag* _datatag):EventFlags(v),startactionpos(0),dataskipbytes(_dataskipbytes),datatag( _datatag) {}
	CLIPEVENTFLAGS EventFlags;
	UI32_SWF ActionRecordSize;
	UI8 KeyCode;
	std::vector<uint8_t> actions;
	bool isLast();
	uint32_t startactionpos;
	uint32_t dataskipbytes;
	AdditionalDataTag* datatag;
};
class CLIPACTIONS
{
public:
	CLIPACTIONS(uint32_t v, AdditionalDataTag* _datatag=nullptr):AllEventFlags(v),dataskipbytes(0),datatag(_datatag) {}
	std::vector<CLIPACTIONRECORD> ClipActionRecords;
	CLIPEVENTFLAGS AllEventFlags;
	uint32_t dataskipbytes;
	AdditionalDataTag* datatag;
};

class SOUNDENVELOPE
{
public:
	UI32_SWF Pos44;
	UI16_SWF LeftLevel;
	UI16_SWF RightLevel;
};

class SOUNDINFO
{
public:
	bool SyncStop:1;
	bool SyncNoMultiple:1;
	UI32_SWF InPoint;
	UI32_SWF OutPoint;
	UI16_SWF LoopCount;
	std::vector<SOUNDENVELOPE> SoundEnvelope;
};

class RunState
{
public:
	int last_FP;
	unsigned int FP;
	unsigned int next_FP;
	bool stop_FP;
	bool explicit_FP;
	bool inEnterFrame;
	bool gotoQueued;
	bool creatingframe;
	bool frameadvanced;
	RunState();
	inline void reset()
	{
		last_FP = -1;
		FP = 0;
		next_FP = 0;
		stop_FP = false;
		explicit_FP = false;
		inEnterFrame = false;
		gotoQueued = false;
		creatingframe = false;
		frameadvanced = false;
	}
};
class Activation_object;
class ACTIONRECORD
{
public:
	static void PushStack(std::stack<asAtom>& stack,const asAtom& a);
	static asAtom PopStack(std::stack<asAtom>& stack);
	static asAtom PeekStack(std::stack<asAtom>& stack);
	static void executeActions(DisplayObject* clip, AVM1context* context, const std::vector<uint8_t> &actionlist, uint32_t startactionpos, std::map<uint32_t, asAtom> &scopevariables, bool fromInitAction = false, asAtom *result = nullptr, asAtom* obj = nullptr, asAtom *args = nullptr, uint32_t num_args=0, const std::vector<uint32_t>& paramnames=std::vector<uint32_t>(), const std::vector<uint8_t>& paramregisternumbers=std::vector<uint8_t>(),
			bool preloadParent=false, bool preloadRoot=false, bool suppressSuper=true, bool preloadSuper=false, bool suppressArguments=false, bool preloadArguments=false, bool suppressThis=true, bool preloadThis=false, bool preloadGlobal=false, AVM1Function *caller = nullptr, AVM1Function *callee = nullptr, Activation_object *actobj=nullptr, asAtom* superobj=nullptr);
};
class BUTTONCONDACTION
{
friend std::istream& operator>>(std::istream& s, BUTTONCONDACTION& v);
public:
	BUTTONCONDACTION():CondActionSize(0)
	  ,CondIdleToOverDown(false),CondOutDownToIdle(false),CondOutDownToOverDown(false),CondOverDownToOutDown(false)
	  ,CondOverDownToOverUp(false),CondOverUpToOverDown(false),CondOverUpToIdle(false),CondIdleToOverUp(false),CondOverDownToIdle(false),startactionpos(0)
	{}
	UI16_SWF CondActionSize;
	bool CondIdleToOverDown:1;
	bool CondOutDownToIdle:1;
	bool CondOutDownToOverDown:1;
	bool CondOverDownToOutDown:1;
	bool CondOverDownToOverUp:1;
	bool CondOverUpToOverDown:1;
	bool CondOverUpToIdle:1;
	bool CondIdleToOverUp:1;
	bool CondOverDownToIdle:1;
	uint32_t CondKeyPress;
	uint32_t startactionpos;
	std::vector<uint8_t> actions;
};
class ASWorker;
ASObject* abstract_i(ASWorker* wrk, int32_t i);
ASObject* abstract_ui(ASWorker* wrk, uint32_t i);
ASObject* abstract_d(ASWorker* wrk, number_t i);
ASObject* abstract_d_constant(ASWorker* wrk, number_t i);
ASObject* abstract_di(ASWorker* wrk, int64_t i);
ASObject* abstract_s(ASWorker* wrk);
ASObject* abstract_s(ASWorker* wrk, const char* s, uint32_t len);
ASObject* abstract_s(ASWorker* wrk, const char* s, int numbytes, int numchars, bool issinglebyte, bool hasNull, bool isInteger);
ASObject* abstract_s(ASWorker* wrk, const char* s);
ASObject* abstract_s(ASWorker* wrk, const tiny_string& s);
ASObject* abstract_s(ASWorker* wrk, uint32_t stringId);
ASObject* abstract_null(SystemState *sys);
ASObject* abstract_undefined(SystemState *sys);
ASObject* abstract_b(SystemState *sys, bool b);

void stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns);

inline double twipsToPixels(double twips) { return twips/20.0; }

std::ostream& operator<<(std::ostream& s, const RECT& r);
std::ostream& operator<<(std::ostream& s, const RGB& r);
std::ostream& operator<<(std::ostream& s, const RGBA& r);
std::ostream& operator<<(std::ostream& s, const STRING& r);
std::ostream& operator<<(std::ostream& s, const nsNameAndKind& r);
std::ostream& operator<<(std::ostream& s, const multiname& r);
std::ostream& operator<<(std::ostream& s, const tiny_string& r) DLL_PUBLIC;
std::ostream& operator<<(std::ostream& s, const QName& r);
std::ostream& operator<<(std::ostream& s, const MATRIX& r);
std::ostream& operator<<(std::ostream& s, const URLInfo& u);
std::ostream& operator<<(std::ostream& s, const DisplayObject& r);

std::istream& operator>>(std::istream& s, RECT& v);
std::istream& operator>>(std::istream& s, CLIPEVENTFLAGS& v);
std::istream& operator>>(std::istream& s, CLIPACTIONRECORD& v);
std::istream& operator>>(std::istream& s, CLIPACTIONS& v);
std::istream& operator>>(std::istream& s, RGB& v);
std::istream& operator>>(std::istream& s, RGBA& v);
std::istream& operator>>(std::istream& s, GRADRECORD& v);
std::istream& operator>>(std::istream& s, GRADIENT& v);
std::istream& operator>>(std::istream& s, FOCALGRADIENT& v);
std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLE& v);
std::istream& operator>>(std::istream& stream, LINESTYLE2& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLE& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLE2& v);
std::istream& operator>>(std::istream& stream, FILLSTYLE& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPERECORD& v);
std::istream& operator>>(std::istream& stream, TEXTRECORD& v);
std::istream& operator>>(std::istream& stream, MATRIX& v);
std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
std::istream& operator>>(std::istream& stream, GLYPHENTRY& v);
std::istream& operator>>(std::istream& stream, STRING& v);
std::istream& operator>>(std::istream& stream, BUTTONRECORD& v);
std::istream& operator>>(std::istream& stream, RECORDHEADER& v);
std::istream& operator>>(std::istream& stream, FILTERLIST& v);
std::istream& operator>>(std::istream& stream, FILTER& v);
std::istream& operator>>(std::istream& stream, DROPSHADOWFILTER& v);
std::istream& operator>>(std::istream& stream, BLURFILTER& v);
std::istream& operator>>(std::istream& stream, GLOWFILTER& v);
std::istream& operator>>(std::istream& stream, BEVELFILTER& v);
std::istream& operator>>(std::istream& stream, GRADIENTGLOWFILTER& v);
std::istream& operator>>(std::istream& stream, CONVOLUTIONFILTER& v);
std::istream& operator>>(std::istream& stream, COLORMATRIXFILTER& v);
std::istream& operator>>(std::istream& stream, GRADIENTBEVELFILTER& v);
std::istream& operator>>(std::istream& stream, SOUNDINFO& v);
std::istream& operator>>(std::istream& stream, ACTIONRECORD& v);
std::istream& operator>>(std::istream& stream, BUTTONCONDACTION& v);
}
#endif /* SWFTYPES_H */
