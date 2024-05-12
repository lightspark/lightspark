#ifndef AGALCONVERTER_H
#define AGALCONVERTER_H

#include "tiny_string.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include <map>
#include <vector>
#include <algorithm>

using namespace lightspark;

// routines to convert AGAL-Bytecode to GLSL shader language
// AGAL bytecode format is documented in http://help.adobe.com/en_US/as3/dev/WSd6a006f2eb1dc31e-310b95831324724ec56-8000.html
// conversion algorithm is taken from https://github.com/openfl/openfl/blob/develop/src/openfl/display3D/_internal/AGALConverter.hx (converted to c++)

tiny_string prefixFromType (RegisterType regType, bool isVertexProgram)
{
	switch (regType)
	{
		case RegisterType::ATTRIBUTE: return "va";
		case RegisterType::CONSTANT: return isVertexProgram ? "vc" : "fc";
		case RegisterType::TEMPORARY: return isVertexProgram ? "vt" : "ft";
		case RegisterType::OUTPUT: return "output_";
		case RegisterType::VARYING: return "v";
		case RegisterType::SAMPLER: return "sampler";
		default:
			LOG(LOG_ERROR,"AGAL:prefixFromType has Invalid data!");
			return "";
	}
}

struct DestRegister
{
	int32_t mask;
	int32_t n;
	bool isVertexProgram;
	RegisterType type;

	tiny_string getWriteMask ()
	{
		tiny_string str = ".";
		if ((mask & 1) != 0) str += "x";
		if ((mask & 2) != 0) str += "y";
		if ((mask & 4) != 0) str += "z";
		if ((mask & 8) != 0) str += "w";
		return str;
	}
	static DestRegister parse (uint32_t v, bool isVertexProgram)
	{
		DestRegister dr;
		dr.isVertexProgram= isVertexProgram;
		dr.type = (RegisterType)((v >> 24) & 0xF);
		dr.mask = (v >> 16) & 0xF;
		dr.n = (v & 0xFFFF);
		return dr;
	}
	tiny_string toGLSL (bool useMask = true)
	{
		tiny_string str;
		if (type == RegisterType::OUTPUT)
			str = isVertexProgram ? "gl_Position" : "gl_FragColor";
		else
		{
			char buf[100];
			sprintf(buf,"%d",n);
			str = prefixFromType (type, isVertexProgram) +  buf;
		}
		if (useMask && mask != 0xF)
			str += getWriteMask ();
		return str;
	}
};

struct SourceRegister
{
	int32_t d;
	RegisterType itype;
	int32_t n;
	int32_t o;
	bool isVertexProgram;
	int32_t q;
	int32_t s;
	int32_t sourceMask;
	RegisterType type;
	static SourceRegister parse (uint64_t v, bool isVertexProgram, int32_t sourceMask)
	{
		SourceRegister sr;
		sr.isVertexProgram= isVertexProgram;
		sr.d = ((v >> 63) & 1); // Direct=0/Indirect=1 for direct Q and I are ignored, 1bit
		sr.q = ((v >> 48) & 0x3); // index register component select
		sr.itype = (RegisterType) ((v >> 40) & 0xF); // index register type
		sr.type = (RegisterType) ((v >> 32) & 0xF); // type
		sr.s = ((v >> 24) & 0xFF); // swizzle
		sr.o = ((v >> 16) & 0xFF); // indirect offset
		sr.n = (v & 0xFFFF); // number
		sr.sourceMask = sourceMask;
		return sr;
	}
	tiny_string toGLSL (bool emitSwizzle = true, int32_t offset = 0)
	{
		if (type == RegisterType::OUTPUT) {
			return isVertexProgram ? "gl_Position" : "gl_FragColor";
		}
		bool fullxyzw = (s == 228) && (sourceMask == 0xF);
		tiny_string swizzle;

		if (type != RegisterType::SAMPLER && !fullxyzw)
		{
			for (int i =0; i < 4; i++)
			{
				// only output swizzles for each source mask
				if ((sourceMask & (1 << i)) != 0)
				{
					switch ((s >> (i * 2)) & 3)
					{
						case 0: swizzle += "x"; break;
						case 1: swizzle += "y"; break;
						case 2: swizzle += "z"; break;
						case 3: swizzle += "w"; break;
					}
				}
			}
		}
		tiny_string str = prefixFromType (type, isVertexProgram);
		if (d == 0) {
			// direct register
			char buf[100];
			sprintf(buf,"%d",n + offset);
			str += buf;
		} else {
			// indirect register
			char buf[100];
			char indexComponent = ' ';
			switch (q)
			{
				case 0:
					indexComponent = 'x';
					break;
				case 1:
					indexComponent = 'y';
					break;
				case 2:
					indexComponent = 'z';
					break;
				case 3:
					indexComponent = 'w';
					break;
			}
			sprintf(buf,"%d[ int(%s%d.%c) +%d]",o,prefixFromType (itype, isVertexProgram).raw_buf(),n,indexComponent,offset);
			str += buf;
		}
		if (emitSwizzle && swizzle != "") {
			str += ".";
			str += swizzle;
		}
		return str;
	}
};

class RegisterMap
{
private:
	std::vector<RegisterMapEntry> mEntries;
public:
	void add (RegisterType type, const tiny_string& name, uint32_t number, RegisterUsage usage)
	{
		for (auto it = mEntries.begin(); it != mEntries.end(); it++) {
			if (it->type == type && it->name == name && it->number == number) {
				if (it->usage != usage)
					LOG(LOG_ERROR,"AGAL:Cannot use register in multiple ways yet (mat4/vec4)");
				return;
			}
		}
		RegisterMapEntry entry;
		entry.type = type;
		entry.name = name;
		entry.number = number;
		entry.usage = usage;
		mEntries.push_back(entry);
	}
	void addDR (DestRegister dr, RegisterUsage usage)
	{
		add (dr.type, dr.toGLSL(false), dr.n, usage);
	}
	void addSaR (SamplerRegister sr, RegisterUsage usage)
	{
		add (sr.type, sr.toGLSL (), sr.n, usage);
	}
	void addSR (SourceRegister sr, RegisterUsage usage, int offset = 0)
	{
		if (sr.d != 0) {
			char buf[100];
			sprintf(buf,"%s%d",prefixFromType (sr.itype, sr.isVertexProgram).raw_buf(),sr.n);
			add (sr.itype, buf, sr.n, RegisterUsage::VECTOR_4);
			sprintf(buf,"%s%d",prefixFromType (sr.type, sr.isVertexProgram).raw_buf(),sr.o);
			add (sr.type, buf, sr.o, RegisterUsage::VECTOR_4_ARRAY);
			return;
		}
		add (sr.type, sr.toGLSL (false, offset), sr.n + offset, usage);
	}
	RegisterUsage getRegisterUsage (SourceRegister sr)
	{
		if (sr.d != 0) {
			return RegisterUsage::VECTOR_4_ARRAY;
		}
		return getUsage (sr.type, sr.toGLSL (false), sr.n);
	}
	RegisterUsage getUsage (RegisterType type, tiny_string name, uint32_t number)
	{
		for (auto it = mEntries.begin(); it != mEntries.end(); it++) {
			if (it->type == type && it->name == name && it->number == number) {
				return it->usage;
			}
		}
		return RegisterUsage::UNUSED;
	}
	tiny_string toGLSL (bool tempRegistersOnly)
	{
		RegisterMapEntry entry;

		std::sort(mEntries.begin(),mEntries.end(), [](const RegisterMapEntry& a, const RegisterMapEntry& b) {
			return a.type < b.type || (a.type==b.type && a.number<b.number);
		});
		tiny_string sb;
		for (uint32_t i = 0; i < mEntries.size(); i++)
		{
			entry = mEntries [i];

			// only emit temporary registers based on Boolean passed in
			// this is so temp registers can be grouped in the main() block
			if ((tempRegistersOnly && entry.type != RegisterType::TEMPORARY) || (!tempRegistersOnly && entry.type == RegisterType::TEMPORARY)) {
				continue;
			}

			// dont emit output registers
			if (entry.type == RegisterType::OUTPUT) {
				continue;
			}

			switch (entry.type)
			{
				case RegisterType::ATTRIBUTE:
					// sb.AppendFormat("layout(location = {0}) ", entry.number);
					sb += "attribute ";
					break;
				case RegisterType::CONSTANT:
					//sb.AppendFormat("layout(location = {0}) ", entry.number);
					sb += ("uniform ");
					break;
				case RegisterType::TEMPORARY:
					sb += "\t";
					break;
				case RegisterType::OUTPUT:
					break;
				case RegisterType::VARYING:
					sb += "varying ";
					break;
				case RegisterType::SAMPLER:
					sb += "uniform ";
					break;
				default:
					LOG(LOG_ERROR,"AGAL:illegal RegisterType");
					break;
			}

			switch (entry.usage)
			{
				case RegisterUsage::VECTOR_4:
					sb += ("vec4 ");
					break;
				case RegisterUsage::VECTOR_4_ARRAY:
					sb += ("vec4 ");
					break;
				case RegisterUsage::MATRIX_4_4:
					sb += ("mat4 ");
					break;
				case RegisterUsage::SAMPLER_2D:
					sb += ("sampler2D ");
					break;
				case RegisterUsage::SAMPLER_CUBE:
					sb += ("samplerCube ");
					break;
				case RegisterUsage::UNUSED:
					LOG(LOG_ERROR, "AGAL:Missing switch patten: RegisterUsage::UNUSED");
					break;
				case RegisterUsage::SAMPLER_2D_ALPHA:
				case RegisterUsage::SAMPLER_CUBE_ALPHA:
					break;
			}
			if (entry.usage == RegisterUsage::SAMPLER_2D_ALPHA)
			{
				sb += "sampler2D ";
				sb += entry.name;
				sb += ";\n";

				sb += "uniform ";
				sb += "sampler2D ";
				sb += entry.name + "_alpha";
				sb += ";\n";
				
				sb += "uniform ";
				sb += "bool ";
				sb += entry.name + "_alphaEnabled";
				sb += ";\n";
			}
			else if (entry.usage == RegisterUsage::SAMPLER_CUBE_ALPHA)
			{
				sb += "samplerCube ";
				sb += entry.name;
				sb += ";\n";
				
				sb += "uniform ";
				sb += "samplerCube ";
				sb += entry.name + "_alpha";
				sb += ";\n";
				
				sb += "uniform ";
				sb += "bool ";
				sb += entry.name + "_alphaEnabled";
				sb += ";\n";
			}
			else if (entry.usage == RegisterUsage::VECTOR_4_ARRAY)
			{
				sb += entry.name + "[128]"; // this is an array of "count" elements.
				sb += ";\n";
			}
			else
			{
				sb += entry.name;
				sb += ";\n";
			}
		}
		return sb;
	}
	void getConstants( std::vector<RegisterMapEntry>& constantmap)
	{
		constantmap.clear();
		for (auto it = mEntries.begin(); it != mEntries.end(); it++) {
			switch (it->type)
			{
				case CONSTANT:
					constantmap.push_back(*it);
					break;
				default:
					break;
			}
		}
	}
	void getAttributes( std::vector<RegisterMapEntry>& attributes)
	{
		attributes.clear();
		for (auto it = mEntries.begin(); it != mEntries.end(); it++) {
			switch (it->type)
			{
				case ATTRIBUTE:
					attributes.push_back(*it);
					break;
				default:
					break;
			}
		}
	}
};
uint64_t readUInt64 (ByteArray* byteArray)
{
	uint32_t low;
	byteArray->readUnsignedInt(low);
	uint32_t high;
	byteArray->readUnsignedInt(high);
	return ((uint64_t)high)<<32 | low;
}

tiny_string AGALtoGLSL(ByteArray* agal,bool isVertexProgram,std::vector<SamplerRegister>& samplerState,std::vector<RegisterMapEntry>& constants,std::vector<RegisterMapEntry>& attributes,RegisterMap& vertexregistermap)
{
	agal->setPosition(0);
	uint8_t by;
	agal->readByte(by);
	if (by == 0xB0) {
		// use embedded GLSL shader instead
		tiny_string res;
		agal->readUTF(res);
		return res;
	}
	if (by != 0xA0) {
		LOG(LOG_ERROR,"invalid magic byte for AGAL");
		return "";
	}
	uint32_t version;
	agal->readUnsignedInt(version);
	agal->readByte(by);
	if (by != 0xA1) {
		LOG(LOG_ERROR,"invalid shaderTypeID for AGAL:"<<hex<<(uint32_t)by);
		return "";
	}
	agal->readByte(by);
	if (isVertexProgram && by != 0)
		LOG(LOG_ERROR,"AGAL:expected vertex shader, got fragment shader");
	if (!isVertexProgram && by == 0)
		LOG(LOG_ERROR,"AGAL:expected fragment shader, got vertex shader");

	tiny_string sb;
	RegisterMap map;

	while (agal->getPosition() < agal->getLength()) {
		// fetch instruction info
		uint32_t opcode;
		agal->readUnsignedInt(opcode);
		uint32_t dest;
		agal->readUnsignedInt(dest);
		uint64_t source1 = readUInt64 (agal);
		uint64_t source2 = readUInt64 (agal);
		// parse registers
		DestRegister dr = DestRegister::parse (dest, isVertexProgram);
		SourceRegister sr1 = SourceRegister::parse (source1, isVertexProgram, dr.mask);
		SourceRegister sr2 = SourceRegister::parse (source2, isVertexProgram, dr.mask);
		// switch on opcode and emit GLSL
		sb += "\t";
		switch (opcode)
		{
			case 0x00: // mov
				sb += dr.toGLSL () + " = " + sr1.toGLSL () + "; // mov";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x01: // add
				sb += dr.toGLSL () + " = " + sr1.toGLSL () + " + " + sr2.toGLSL () + "; // add";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x02: // sub
				sb += dr.toGLSL () + " = " + sr1.toGLSL () + " - " + sr2.toGLSL () + "; // sub";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x03: // mul
				sb += dr.toGLSL () + " = " + sr1.toGLSL () + " * " + sr2.toGLSL () + "; // mul";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x04: // div
				sb += dr.toGLSL () + " = " + sr1.toGLSL () + " / " + sr2.toGLSL () + "; // div";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x05: // rcp
			{
				tiny_string sr = sr1.toGLSL ();
				if (sr.find(".") !=  tiny_string::npos) { // swizzle
					sb += dr.toGLSL () + " = 1.0 / " + sr1.toGLSL () + "; // rcp";
				} else {
					sb += dr.toGLSL () + " = vec4(1) / " + sr1.toGLSL () + "; // rcp";
				}
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			}
			case 0x06: // min
				sb += dr.toGLSL () + " = min(" + sr1.toGLSL () + ", " + sr2.toGLSL () + "); // min";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x07: // max
				sb += dr.toGLSL () + " = max(" + sr1.toGLSL () + ", " + sr2.toGLSL () + "); // max";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x08: // frc
				sb += dr.toGLSL () + " = fract(" + sr1.toGLSL () + "); // frc";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x09: // sqrt
				sb += dr.toGLSL () + " = sqrt(" + sr1.toGLSL () + "); // sqrt";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x0A: // rsq
				sb += dr.toGLSL () + " = inversesqrt(" + sr1.toGLSL () + "); // rsq";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x0B: // pow
				sb += dr.toGLSL () + " = pow(" + sr1.toGLSL () + ", " + sr2.toGLSL () + "); // pow";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x0C: // log
				sb += dr.toGLSL () + " = log2(" + sr1.toGLSL () + "); // log";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x0D: // exp
				sb += dr.toGLSL () + " = exp2(" + sr1.toGLSL () + "); // exp";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x0E: // normalize
				sb += dr.toGLSL () + " = normalize(" + sr1.toGLSL () + "); // normalize";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x0F: // sin
				sb += dr.toGLSL () + " = sin(" + sr1.toGLSL () + "); // sin";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x10: // cos
				sb += dr.toGLSL () + " = cos(" + sr1.toGLSL () + "); // cos";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x11: // crs
				sr1.sourceMask = sr2.sourceMask = 7; // adjust source mask for xyz input to dot product
				sb += dr.toGLSL () + " = cross(vec3(" + sr1.toGLSL () + "), vec3(" + sr2.toGLSL () + ")); // crs";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x12: // dp3
				sr1.sourceMask = sr2.sourceMask = 7; // adjust source mask for xyz input to dot product
				sb += dr.toGLSL () + " = vec4(dot(vec3(" + sr1.toGLSL () + "), vec3(" + sr2.toGLSL () + ")))" + dr.getWriteMask () + "; // dp3";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x13: // dp4
				sr1.sourceMask = sr2.sourceMask = 0xF; // adjust source mask for xyzw input to dot product
				sb += dr.toGLSL () + " = vec4(dot(vec4(" + sr1.toGLSL () + "), vec4(" + sr2.toGLSL () + ")))" + dr.getWriteMask () + "; // dp4";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x14: // abs
				sb += dr.toGLSL () + " = abs(" + sr1.toGLSL () + "); // abs";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x15: // neg
				sb += dr.toGLSL () + " = -" + sr1.toGLSL () + "; // neg";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x16: // saturate
				sb += dr.toGLSL () + " = clamp(" + sr1.toGLSL () + ", 0.0, 1.0); // saturate";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				break;
			case 0x17: // m33
			{
				//destination.x = (source1.x * source2[0].x) + (source1.y * source2[0].y) + (source1.z * source2[0].z)
				//destination.y = (source1.x * source2[1].x) + (source1.y * source2[1].y) + (source1.z * source2[1].z)
				//destination.z = (source1.x * source2[2].x) + (source1.y * source2[2].y) + (source1.z * source2[2].z)
				RegisterUsage existingUsage = map.getRegisterUsage (sr2);
				RegisterUsage existingUsageVertex = vertexregistermap.getRegisterUsage (sr2);
				if (existingUsage != RegisterUsage::VECTOR_4 && existingUsage != RegisterUsage::VECTOR_4_ARRAY
					&& existingUsageVertex != RegisterUsage::VECTOR_4 && existingUsageVertex != RegisterUsage::VECTOR_4_ARRAY)
				{
					sb += dr.toGLSL () + " = " + sr1.toGLSL () + " * mat3(" + sr2.toGLSL (false) + "); // m33";
					map.addDR (dr, RegisterUsage::VECTOR_4);
					map.addSR (sr1, RegisterUsage::VECTOR_4);
					map.addSR (sr2, RegisterUsage::MATRIX_4_4); // 33?
				} else {
					// compose the matrix multiply from dot products
					sr1.sourceMask = sr2.sourceMask = 7;
					sb += dr.toGLSL () + " = vec3(" +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 0) + "), " +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 1) + ")," +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 2) + ")); // m33";
					map.addDR (dr, RegisterUsage::VECTOR_4);
					map.addSR (sr1, RegisterUsage::VECTOR_4);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 0);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 1);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 2);
				}
				break;
			}
			case 0x18: // m44
			{
				//multiply matrix 4x4
				//destination.x = (source1.x * source2[0].x) + (source1.y * source2[0].y) + (source1.z * source2[0].z)+ (source1.w * source2[0].w)
				//destination.y = (source1.x * source2[1].x) + (source1.y * source2[1].y) + (source1.z * source2[1].z)+ (source1.w * source2[1].w)
				//destination.z = (source1.x * source2[2].x) + (source1.y * source2[2].y) + (source1.z * source2[2].z)+ (source1.w * source2[2].w)
				//destination.w = (source1.x * source2[3].x) + (source1.y * source2[3].y) + (source1.z * source2[3].z)+ (source1.w * source2[3].w)
				RegisterUsage existingUsage = map.getRegisterUsage (sr2);
				RegisterUsage existingUsageVertex = vertexregistermap.getRegisterUsage (sr2);
				if (existingUsage != RegisterUsage::VECTOR_4 && existingUsage != RegisterUsage::VECTOR_4_ARRAY
					&& existingUsageVertex != RegisterUsage::VECTOR_4 && existingUsageVertex != RegisterUsage::VECTOR_4_ARRAY)
				{
					sb += dr.toGLSL () + " = " + sr1.toGLSL () + " * " + sr2.toGLSL (false) + "; // m44";
					map.addDR (dr, RegisterUsage::VECTOR_4);
					map.addSR (sr1, RegisterUsage::VECTOR_4);
					map.addSR (sr2, RegisterUsage::MATRIX_4_4);
				} else {
					// compose the matrix multiply from dot products
					sr1.sourceMask = sr2.sourceMask = 0xF;
					sb += dr.toGLSL () + " = vec4(" +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 0) + "), " +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 1) + "), " +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 2) + "), " +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 3) + ")); // m44";
					map.addDR (dr, RegisterUsage::VECTOR_4);
					map.addSR (sr1, RegisterUsage::VECTOR_4);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 0);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 1);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 2);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 3);
				}
				break;
			}
			case 0x19: // m34
			{
				//m34 0x19 multiply matrix 3x4
				//destination.x = (source1.x * source2[0].x) + (source1.y * source2[0].y) + (source1.z * source2[0].z)+ (source1.w * source2[0].w)
				//destination.y = (source1.x * source2[1].x) + (source1.y * source2[1].y) + (source1.z * source2[1].z)+ (source1.w * source2[1].w)
				//destination.z = (source1.x * source2[2].x) + (source1.y * source2[2].y) + (source1.z * source2[2].z)+ (source1.w * source2[2].w)
				// prevent w from being written for a m34
				dr.mask &= 7;
				RegisterUsage existingUsage = map.getRegisterUsage (sr2);
				RegisterUsage existingUsageVertex = vertexregistermap.getRegisterUsage (sr2);
				if (existingUsage != RegisterUsage::VECTOR_4 && existingUsage != RegisterUsage::VECTOR_4_ARRAY
					&& existingUsageVertex != RegisterUsage::VECTOR_4 && existingUsageVertex != RegisterUsage::VECTOR_4_ARRAY)
				{
					sb += dr.toGLSL () + " = vec3(" + sr1.toGLSL (false) + " * " + sr2.toGLSL (false) + "); // m34";
					map.addDR (dr, RegisterUsage::VECTOR_4);
					map.addSR (sr1, RegisterUsage::VECTOR_4);
					map.addSR (sr2, RegisterUsage::MATRIX_4_4);
				}
				else
				{
					// compose the matrix multiply from dot products
					sr1.sourceMask = sr2.sourceMask = 0xF;
					sb += dr.toGLSL () + " = vec3(" +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 0) + "), " +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 1) + ")," +
							"dot(" + sr1.toGLSL (true) + "," + sr2.toGLSL (true, 2) + ")); // m34";
					map.addDR (dr, RegisterUsage::VECTOR_4);
					map.addSR (sr1, RegisterUsage::VECTOR_4);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 0);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 1);
					map.addSR (sr2, RegisterUsage::VECTOR_4, 2);
				}
				break;
			}
			case 0x1c: //AGAL2 ife
				sb += tiny_string("if (") + sr1.toGLSL () + " == " + sr2.toGLSL () + "){ // ife";
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x1d: //AGAL2 ine
				sb += tiny_string("if (") + sr1.toGLSL () + " != " + sr2.toGLSL () + "){ // ine";
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x1e: //AGAL2 ifg
				sb += tiny_string("if (") + sr1.toGLSL () + " > " + sr2.toGLSL () + "){ // ifg";
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x1f: //AGAL2 ifl
				sb += tiny_string("if (") + sr1.toGLSL () + " < " + sr2.toGLSL () + "){ // ifl";
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x20: //AGAL2 els
				sb += "} else { // els";
				break;
			case 0x21: //AGAL2 eif
				sb += "}; // eif";
				break;
			case 0x27: // kill /  discard
				if (true) { //(openfl.display.Stage3D.allowDiscard) {
					// ensure we have a full source mask since there is no destination register
					sr1.sourceMask = 0xF;
					sb += "if (any(lessThan(";
					sb += sr1.toGLSL () + ", vec4(0)))) discard;";
					map.addSR (sr1, RegisterUsage::VECTOR_4);
				}
				break;
			case 0x28: // tex
			{
				SamplerRegister sampler = SamplerRegister::parse (source2, isVertexProgram);
				switch (sampler.d)
				{
					case 0: // 2d texture
						// we don't need extra handling for dxt5 textures, as they are uploaded as normal textures
						// if (sampler.t == 2)
						// { // dxt5, sampler alpha
						// 	sr1.sourceMask = 0x3;
						// 	map.addSaR(sampler, RegisterUsage::SAMPLER_2D_ALPHA);
						// 	sb += "if (";
						// 	sb += sampler.toGLSL() + "_alphaEnabled) {\n";
						// 	sb += "\t\t";
						// 	sb += dr.toGLSL() + " = vec4(texture2D(" + sampler.toGLSL() + ", " + sr1.toGLSL() + ").xyz, texture2D("
						// 		+ sampler.toGLSL() + "_alpha, " + sr1.toGLSL() + ").x); // tex + alpha\n";
						// 	sb += "\t} else {\n";
						// 	sb += "\t\t";
						// 	sb += dr.toGLSL() + " = texture2D(" + sampler.toGLSL() + ", " + sr1.toGLSL() + "); // tex\n";
						// 	sb += "\t}";
						// }
						// else
						{
							sr1.sourceMask = 0x3;
							map.addSaR (sampler, RegisterUsage::SAMPLER_2D);
							sb += dr.toGLSL () + " = texture2D(" + sampler.toGLSL () + ", " + sr1.toGLSL () + "); // tex";
						}
						break;
					case 1: // cube texture
						// we don't need extra handling for dxt5 textures, as they are uploaded as normal textures
						// if (sampler.t == 2)
						// { // dxt5, sampler alpha

						// 	sr1.sourceMask = 0x7;
						// 	map.addSaR(sampler, RegisterUsage::SAMPLER_CUBE_ALPHA);
						// 	sb += "if (";
						// 	sb += sampler.toGLSL() + "_alphaEnabled) {\n";
						// 	sb += "\t\t";
						// 	sb += dr.toGLSL() + " = vec4(textureCube(" + sampler.toGLSL() + ", " + sr1.toGLSL() + ").xyz, textureCube("
						// 		+ sampler.toGLSL() + "_alpha, " + sr1.toGLSL() + ").x); // tex + alpha\n";
						// 	sb += "\t} else {\n";
						// 	sb += "\t\t";
						// 	sb += dr.toGLSL() + " = textureCube(" + sampler.toGLSL() + ", " + sr1.toGLSL() + "); // tex";
						// 	sb += "\t}";
						// }
						// else
						{
							sr1.sourceMask = 0x7;
							sb += dr.toGLSL () + " = textureCube(" + sampler.toGLSL () + ", " + sr1.toGLSL () + "); // tex";
							map.addSaR (sampler, RegisterUsage::SAMPLER_CUBE);
						}
						break;
				}
				//sb.AppendFormat("{0} = vec4(0,1,0,1);", dr.toGLSL () );
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);

				// add sampler state to output list for caller
				samplerState.push_back(sampler);
				break;
			}
			case 0x29: // sge
				sr1.sourceMask = sr2.sourceMask = 0xF; // sge only supports vec4
				sb += dr.toGLSL () + " = vec4(greaterThanEqual(" + sr1.toGLSL () + ", " + sr2.toGLSL () + "))" + dr.getWriteMask () + "; // ste";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x2A: // slt
				sr1.sourceMask = sr2.sourceMask = 0xF; // slt only supports vec4
				sb += dr.toGLSL () + " = vec4(lessThan(" + sr1.toGLSL () + ", " + sr2.toGLSL () + "))" + dr.getWriteMask () + "; // slt";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x2C: // seq
				sr1.sourceMask = sr2.sourceMask = 0xF; // seq only supports vec4
				sb += dr.toGLSL () + " = vec4(equal(" + sr1.toGLSL () + ", " + sr2.toGLSL () + "))" + dr.getWriteMask () + "; // seq";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			case 0x2D: // sne
				sr1.sourceMask = sr2.sourceMask = 0xF; // sne only supports vec4
				sb += dr.toGLSL () + " = vec4(notEqual(" + sr1.toGLSL () + ", " + sr2.toGLSL () + "))" + dr.getWriteMask () + "; // sne";
				map.addDR (dr, RegisterUsage::VECTOR_4);
				map.addSR (sr1, RegisterUsage::VECTOR_4);
				map.addSR (sr2, RegisterUsage::VECTOR_4);
				break;
			default:
				//sb.AppendFormat ("unsupported opcode" + opcode);
				LOG(LOG_ERROR,"AGAL:illegal Opcode " <<hex<<(int)opcode);
				break;
		}
		sb += "\n";
	}


	// combine parts into final progam
	tiny_string glsl;
	glsl += "// AGAL ";
	glsl += (isVertexProgram ? "vertex" : "fragment");
	glsl += " shader\n";

#ifdef ENABLE_GLES2
	glsl += "#version 100\n";
	// Required to set the default precision of vectors
	glsl += "precision highp float;\n";
#else
	glsl += "#version 120\n";
#endif
	glsl += map.toGLSL (false);
	if (isVertexProgram) {
		// this is needed for flipping render textures upside down
		glsl += "uniform vec4 vcPositionScale;\n";
		vertexregistermap=map;
	}
	glsl += "void main() {\n";
	glsl += map.toGLSL (true);
	glsl += sb;
	if (isVertexProgram) {
		// this is needed for flipping render textures upside down
		glsl += "\tgl_Position *= vcPositionScale;\n";
	}
	glsl += "}\n";
	
	map.getConstants(constants);
	map.getAttributes(attributes);
	return glsl;
}

#endif // AGALCONVERTER_H
