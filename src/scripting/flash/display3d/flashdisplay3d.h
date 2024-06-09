/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2017 Ludger Krämer <dbluelle@onlinehome.de>

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
#ifndef FLASHDISPLAY3D_H
#define FLASHDISPLAY3D_H

#include "compat.h"

#include "swftypes.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/display3d/flashdisplay3dtextures.h"
#include <map>

enum RegisterType {
	ATTRIBUTE = 0,
	CONSTANT = 1,
	TEMPORARY = 2,
	OUTPUT = 3,
	VARYING = 4,
	SAMPLER = 5
};
enum RegisterUsage {
	UNUSED,
	VECTOR_4,
	MATRIX_4_4,
	SAMPLER_2D,
	SAMPLER_2D_ALPHA,
	SAMPLER_CUBE,
	SAMPLER_CUBE_ALPHA,
	VECTOR_4_ARRAY
};

struct SamplerRegister
{
	int32_t b; // lod bias
	int32_t d; // dimension 0=2d 1=cube
	int32_t f; // Filter (0=nearest,1=linear) (4 bits)
	int32_t m; // Mipmap (0=disable,1=nearest, 2=linear)
	int32_t n; // number
	bool isVertexProgram;
	int32_t s; // special flags bit
	int32_t t; // texture format (0=none, dxt1=1, dxt5=2)
	RegisterType type;
	int32_t w; // wrap (0=clamp 1=repeat_wrap_s 2=repeat_wrap_t 3=repeat)
	uint32_t program_sampler_id;

	static SamplerRegister parse (uint64_t v, bool isVertexProgram);
	lightspark::tiny_string toGLSL ();
};
struct RegisterMapEntry
{
	uint32_t program_register_id;
	lightspark::tiny_string name;
	uint32_t number;
	RegisterType type;
	RegisterUsage usage;
	RegisterMapEntry():program_register_id(UINT32_MAX) {}
	RegisterMapEntry(const RegisterMapEntry& r)
	{
		program_register_id = r.program_register_id;
		name = r.name.raw_buf();
		number = r.number;
		type = r.type;
		usage = r.usage;
	}
	RegisterMapEntry& operator=(const RegisterMapEntry& r)
	{
		program_register_id = r.program_register_id;
		name = r.name.raw_buf();
		number = r.number;
		type = r.type;
		usage = r.usage;
		return *this;
	}
	
};

#define CONTEXT3D_SAMPLER_COUNT 8
#define CONTEXT3D_ATTRIBUTE_COUNT 8
#define CONTEXT3D_PROGRAM_REGISTERS 128

namespace lightspark
{
class RenderContext;
class VertexBuffer3D;
class Program3D;
class Stage3D;
class EngineData;

enum RENDER_ACTION { RENDER_CLEAR,RENDER_CONFIGUREBACKBUFFER,RENDER_RENDERTOBACKBUFFER,RENDER_TOTEXTURE,
					 RENDER_SETPROGRAM,RENDER_UPLOADPROGRAM,RENDER_DELETEPROGRAM,
					 RENDER_SETVERTEXBUFFER,RENDER_DRAWTRIANGLES,RENDER_DELETEBUFFER,
					 RENDER_SETPROGRAMCONSTANTS_FROM_MATRIX,RENDER_SETPROGRAMCONSTANTS_FROM_VECTOR,RENDER_SETTEXTUREAT,
					 RENDER_SETBLENDFACTORS,RENDER_SETDEPTHTEST,RENDER_SETCULLING,RENDER_GENERATETEXTURE,RENDER_LOADTEXTURE,RENDER_LOADCUBETEXTURE,
					 RENDER_SETSCISSORRECTANGLE, RENDER_SETCOLORMASK, RENDER_SETSAMPLERSTATE, RENDER_DELETETEXTURE,
					 RENDER_CREATEINDEXBUFFER,RENDER_UPLOADINDEXBUFFER,RENDER_DELETEINDEXBUFFER,
					 RENDER_CREATEVERTEXBUFFER,RENDER_UPLOADVERTEXBUFFER,RENDER_DELETEVERTEXBUFFER,
				     RENDER_SETSTENCILREFERENCEVALUE,RENDER_SETSTENCILACTIONS};
struct renderaction
{
	RENDER_ACTION action;
	uint32_t udata1;
	uint32_t udata2;
	uint32_t udata3;
	float fdata[CONTEXT3D_PROGRAM_REGISTERS*4];
	_NR<ASObject> dataobject;
	renderaction():udata1(0),udata2(0),udata3(0),fdata{0} {}
};
struct constantregister
{
	float data[4];
};
struct attribregister
{
	uint32_t bufferID;
	uint32_t data32PerVertex;
	uint32_t offset;
	VERTEXBUFFER_FORMAT format;
	attribregister():bufferID(UINT32_MAX) {}
};

class Context3D: public EventDispatcher
{
friend class Stage3D;
private:
	std::vector<renderaction> actions[2];
	std::vector<_NR<TextureBase>> texturestoupload;
	constantregister vertexConstants[CONTEXT3D_PROGRAM_REGISTERS];
	constantregister fragmentConstants[CONTEXT3D_PROGRAM_REGISTERS];
	attribregister attribs[CONTEXT3D_ATTRIBUTE_COUNT];
	uint32_t samplers[CONTEXT3D_SAMPLER_COUNT];
	float vcposdata[4] = { 1.0,1.0,1.0,1.0 };
	int currentactionvector;

	uint32_t backframebufferIDcurrent;
	uint32_t backframebuffer[2];
	uint32_t backframebufferID[2];
	uint32_t backDepthRenderBuffer[2];
	uint32_t backStencilRenderBuffer[2];

	uint32_t textureframebuffer;
	uint32_t depthRenderBuffer;
	uint32_t stencilRenderBuffer;

	Program3D* currentprogram;
	uint32_t currenttextureid;
	bool renderingToTexture;
	bool enableDepthAndStencilBackbuffer;
	bool enableDepthAndStencilTextureBuffer;
	bool swapbuffers;
	TRIANGLE_FACE currentcullface;
	DEPTHSTENCIL_FUNCTION currentdepthfunction;
	DEPTHSTENCIL_FUNCTION currentstencilfunction;
	uint32_t currentstencilref;
	uint32_t currentstencilmask;
	void handleRenderAction(EngineData *engineData, renderaction &action);
	void setRegisters(EngineData *engineData, std::vector<RegisterMapEntry> &registermap, constantregister *constants, bool isVertex);
	void setAttribs(EngineData* engineData, std::vector<RegisterMapEntry> &attributes);
	void resetAttribs(EngineData* engineData, std::vector<RegisterMapEntry> &attributes);
	void setSamplers(EngineData* engineData);
	void setPositionScale(EngineData *engineData);
	unordered_set<Program3D*> programlist;
	unordered_set<TextureBase*> texturelist;
	unordered_set<IndexBuffer3D*> indexbufferlist;
	unordered_set<VertexBuffer3D*> vectorbufferlist;
	Stage3D* stage3D;
	void disposeintern();
	void configureBackBufferIntern(bool enableDepthAndStencil, uint32_t width, uint32_t height, int index);
protected:
	bool renderImpl(RenderContext &ctxt);
	void loadTexture(TextureBase* tex, uint32_t level);
	void loadCubeTexture(CubeTexture* tex, uint32_t miplevel, uint32_t side);
	void init(Stage3D* s);
	public:
	Mutex rendermutex;
	Context3D(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	bool destruct() override;
	void finalize() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void prepareShutdown() override;

	void addAction(RENDER_ACTION type, ASObject* dataobject);
	void addAction(renderaction action);
	void addTextureToUpload(TextureBase* tex)
	{
		tex->incRef();
		texturestoupload.push_back(_MR(tex));
	}
	ASPROPERTY_GETTER(int,backBufferHeight);
	ASPROPERTY_GETTER(int,backBufferWidth);
	ASPROPERTY_GETTER(tiny_string,driverInfo);
	ASPROPERTY_GETTER_SETTER(bool,enableErrorChecking);
	ASPROPERTY_GETTER_SETTER(int,maxBackBufferHeight);
	ASPROPERTY_GETTER_SETTER(int,maxBackBufferWidth);
	ASFUNCTION_ATOM(getProfile);
	ASFUNCTION_ATOM(supportsVideoTexture);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(configureBackBuffer);
	ASFUNCTION_ATOM(createCubeTexture);
	ASFUNCTION_ATOM(createIndexBuffer);
	ASFUNCTION_ATOM(createProgram);
	ASFUNCTION_ATOM(createRectangleTexture);
	ASFUNCTION_ATOM(createTexture);
	ASFUNCTION_ATOM(createVertexBuffer);
	ASFUNCTION_ATOM(createVideoTexture);
	ASFUNCTION_ATOM(clear);
	ASFUNCTION_ATOM(drawToBitmapData);
	ASFUNCTION_ATOM(drawTriangles);
	ASFUNCTION_ATOM(setBlendFactors);
	ASFUNCTION_ATOM(setCulling);
	ASFUNCTION_ATOM(setColorMask);
	ASFUNCTION_ATOM(setDepthTest);
	ASFUNCTION_ATOM(setProgram);
	ASFUNCTION_ATOM(setProgramConstantsFromByteArray);
	ASFUNCTION_ATOM(setProgramConstantsFromMatrix);
	ASFUNCTION_ATOM(setProgramConstantsFromVector);
	ASFUNCTION_ATOM(setRenderToBackBuffer);
	ASFUNCTION_ATOM(setRenderToTexture);
	ASFUNCTION_ATOM(setSamplerStateAt);
	ASFUNCTION_ATOM(setScissorRectangle);
	ASFUNCTION_ATOM(present);
	ASFUNCTION_ATOM(setStencilActions);
	ASFUNCTION_ATOM(setStencilReferenceValue);
	ASFUNCTION_ATOM(setTextureAt);
	ASFUNCTION_ATOM(setVertexBufferAt);
};

class Context3DBlendFactor: public ASObject
{
public:
	Context3DBlendFactor(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DBufferUsage: public ASObject
{
public:
	Context3DBufferUsage(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DClearMask: public ASObject
{
public:
	Context3DClearMask(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DCompareMode: public ASObject
{
public:
	Context3DCompareMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DMipFilter: public ASObject
{
public:
	Context3DMipFilter(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DProfile: public ASObject
{
public:
	Context3DProfile(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DProgramType: public ASObject
{
public:
	Context3DProgramType(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DRenderMode: public ASObject
{
public:
	Context3DRenderMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DStencilAction: public ASObject
{
public:
	Context3DStencilAction(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DTextureFilter: public ASObject
{
public:
	Context3DTextureFilter(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DTextureFormat: public ASObject
{
public:
	Context3DTextureFormat(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DTriangleFace: public ASObject
{
public:
	Context3DTriangleFace(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DVertexBufferFormat: public ASObject
{
public:
	Context3DVertexBufferFormat(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class Context3DWrapMode: public ASObject
{
public:
	Context3DWrapMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class IndexBuffer3D: public ASObject
{
friend class Context3D;
protected:
	Context3D* context;
	uint32_t bufferID;
	vector<uint16_t> data;
	tiny_string bufferUsage;
	bool disposed;
public:
	IndexBuffer3D(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_INDEXBUFFER3D),bufferID(UINT32_MAX),disposed(false){}
	IndexBuffer3D(ASWorker* wrk,Class_base* c, Context3D* ctx,int _numVertices,tiny_string _bufferUsage);
	bool destruct() override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(uploadFromByteArray);
	ASFUNCTION_ATOM(uploadFromVector);
};

class Program3D: public ASObject
{
friend class Context3D;
private:
	Context3D* context;
	uint32_t gpu_program;
protected:
	uint32_t vcPositionScale;
	tiny_string vertexprogram;
	tiny_string fragmentprogram;
	std::vector<SamplerRegister> samplerState;
	std::vector<RegisterMapEntry> vertexregistermap;
	std::vector<RegisterMapEntry> vertexattributes;
	std::vector<RegisterMapEntry> fragmentregistermap;
	std::vector<RegisterMapEntry> fragmentattributes;
	bool disposed;
public:
	Program3D(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_PROGRAM3D),gpu_program(UINT32_MAX),vcPositionScale(UINT32_MAX),disposed(false){}
	Program3D(ASWorker* wrk,Class_base* c,Context3D* _ct):ASObject(wrk,c,T_OBJECT,SUBTYPE_PROGRAM3D),context(_ct),gpu_program(UINT32_MAX),vcPositionScale(UINT32_MAX),disposed(false){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(upload);
};
class VertexBuffer3D: public ASObject
{
friend class Context3D;
protected:
	Context3D* context;
	uint32_t numVertices;
	int32_t data32PerVertex;
	vector<float> data;
	tiny_string bufferUsage;
	bool disposed;
	uint32_t bufferID;
public:
	VertexBuffer3D(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_VERTEXBUFFER3D),context(nullptr),numVertices(0),data32PerVertex(0),disposed(false),bufferID(UINT32_MAX){}
	VertexBuffer3D(ASWorker* wrk,Class_base* c, Context3D* ctx,int _numVertices,int32_t _data32PerVertex,tiny_string _bufferUsage);
	bool destruct() override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(uploadFromByteArray);
	ASFUNCTION_ATOM(uploadFromVector);
};

}
#endif // FLASHDISPLAY3D_H
