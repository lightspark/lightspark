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
#include "platforms/engineutils.h"

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
	int32_t w; // wrap (0=clamp 1=repeat)
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
	uint32_t arraycount;
	RegisterMapEntry():program_register_id(UINT32_MAX) {}
};

namespace lightspark
{
class RenderContext;
class VertexBuffer3D;
class Program3D;

enum RENDER_ACTION { RENDER_CLEAR,RENDER_CONFIGUREBACKBUFFER,RENDER_SETPROGRAM,RENDER_RENDERTOBACKBUFFER,RENDER_TOTEXTURE,RENDER_DELETEPROGRAM,
					 RENDER_SETVERTEXBUFFER,RENDER_DRAWTRIANGLES,RENDER_DELETEBUFFER,
					 RENDER_SETPROGRAMCONSTANTS_FROM_MATRIX,RENDER_SETPROGRAMCONSTANTS_FROM_VECTOR,RENDER_SETTEXTUREAT,
					 RENDER_SETBLENDFACTORS,RENDER_SETDEPTHTEST,RENDER_SETCULLING,RENDER_LOADTEXTURE,RENDER_LOADCUBETEXTURE,
					 RENDER_SETSCISSORRECTANGLE, RENDER_SETCOLORMASK };
struct renderaction
{
	RENDER_ACTION action;
	uint32_t udata1;
	uint32_t udata2;
	uint32_t udata3;
	float *fdata;
	_NR<ASObject> dataobject;
	renderaction():udata1(0),udata2(0),udata3(0),fdata(NULL) {}
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
#define CONTEXT3D_SAMPLER_COUNT 8
#define CONTEXT3D_ATTRIBUTE_COUNT 8
#define CONTEXT3D_PROGRAM_REGISTERS 128

class Context3D: public EventDispatcher
{
friend class Stage3D;
private:
	std::vector<renderaction> actions[2];
	constantregister vertexConstants[CONTEXT3D_PROGRAM_REGISTERS];
	constantregister fragmentConstants[CONTEXT3D_PROGRAM_REGISTERS];
	attribregister attribs[CONTEXT3D_ATTRIBUTE_COUNT];
	uint32_t samplers[CONTEXT3D_SAMPLER_COUNT];
	float vcposdata[4] = { 1.0,1.0,1.0,1.0 };
	int currentactionvector;
	uint32_t textureframebuffer;
	uint32_t textureframebufferID;
	uint32_t depthRenderBuffer;
	uint32_t stencilRenderBuffer;
	Program3D* currentprogram;
	bool renderingToTexture;
	bool enableDepthAndStencilBackbuffer;
	bool enableDepthAndStencilTextureBuffer;
	bool swapbuffers;
	void handleRenderAction(EngineData *engineData, renderaction &action);
	void setRegisters(EngineData *engineData, std::vector<RegisterMapEntry> &registermap, constantregister *constants, bool isVertex);
	void setAttribs(EngineData* engineData, std::vector<RegisterMapEntry> &attributes);
	void setSamplers(EngineData* engineData);
	void setPositionScale(EngineData *engineData);
protected:
	bool renderImpl(RenderContext &ctxt);
	void loadTexture(TextureBase* tex);
	void loadCubeTexture(CubeTexture* tex);
public:
	Mutex rendermutex;
	Context3D(Class_base* c);
	static void sinit(Class_base* c);

	void addAction(RENDER_ACTION type, ASObject* dataobject);
	void addAction(renderaction action);
	ASPROPERTY_GETTER(int,backBufferHeight);
	ASPROPERTY_GETTER(int,backBufferWidth);
	ASPROPERTY_GETTER(tiny_string,driverInfo);
	ASPROPERTY_GETTER_SETTER(bool,enableErrorChecking);
	ASPROPERTY_GETTER_SETTER(int,maxBackBufferHeight);
	ASPROPERTY_GETTER_SETTER(int,maxBackBufferWidth);
	ASPROPERTY_GETTER(tiny_string,profile);
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
	Context3DBlendFactor(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DBufferUsage: public ASObject
{
public:
	Context3DBufferUsage(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DClearMask: public ASObject
{
public:
	Context3DClearMask(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DCompareMode: public ASObject
{
public:
	Context3DCompareMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DMipFilter: public ASObject
{
public:
	Context3DMipFilter(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DProfile: public ASObject
{
public:
	Context3DProfile(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DProgramType: public ASObject
{
public:
	Context3DProgramType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DRenderMode: public ASObject
{
public:
	Context3DRenderMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DStencilAction: public ASObject
{
public:
	Context3DStencilAction(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DTextureFilter: public ASObject
{
public:
	Context3DTextureFilter(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DTextureFormat: public ASObject
{
public:
	Context3DTextureFormat(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DTriangleFace: public ASObject
{
public:
	Context3DTriangleFace(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DVertexBufferFormat: public ASObject
{
public:
	Context3DVertexBufferFormat(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Context3DWrapMode: public ASObject
{
public:
	Context3DWrapMode(Class_base* c):ASObject(c){}
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
public:
	IndexBuffer3D(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_INDEXBUFFER3D),bufferID(UINT32_MAX){}
	IndexBuffer3D(Class_base* c, Context3D* ctx,int _numVertices,tiny_string _bufferUsage);
	~IndexBuffer3D();
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(uploadFromByteArray);
	ASFUNCTION_ATOM(uploadFromVector);
};

class Program3D: public ASObject
{
friend class Context3D;
private:
	_NR<Context3D> context3D;
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
public:
	Program3D(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_PROGRAM3D),gpu_program(UINT32_MAX),vcPositionScale(UINT32_MAX){}
	Program3D(Class_base* c,_NR<Context3D> _ct):ASObject(c,T_OBJECT,SUBTYPE_PROGRAM3D),context3D(_ct),gpu_program(UINT32_MAX),vcPositionScale(UINT32_MAX){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(upload);
};
class VertexBuffer3D: public ASObject
{
friend class Context3D;
protected:
	Context3D* context;
	uint32_t bufferID;
	uint32_t numVertices;
	int32_t data32PerVertex;
	vector<float> data;
	tiny_string bufferUsage;
	bool upload_needed;
public:
	VertexBuffer3D(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_VERTEXBUFFER3D),context(NULL),bufferID(UINT32_MAX),numVertices(0),data32PerVertex(0),upload_needed(false){}
	VertexBuffer3D(Class_base* c, Context3D* ctx,int _numVertices,int32_t _data32PerVertex,tiny_string _bufferUsage);
	~VertexBuffer3D();
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(uploadFromByteArray);
	ASFUNCTION_ATOM(uploadFromVector);
};

}
#endif // FLASHDISPLAY3D_H
