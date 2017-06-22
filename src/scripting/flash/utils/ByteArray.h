/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_UTILS_BYTEARRAY_H
#define SCRIPTING_FLASH_UTILS_BYTEARRAY_H 1

#include "compat.h"
#include "swftypes.h"
#include "scripting/flash/utils/flashutils.h"

namespace lightspark
{

class ByteArray: public ASObject, public IDataInput, public IDataOutput
{
friend class LoaderThread;
friend class URLLoader;
friend class ApplicationDomain;
friend class LoaderInfo;
protected:
	bool littleEndian;
	uint8_t objectEncoding;
	uint8_t currentObjectEncoding;
	uint32_t position;
	uint8_t* bytes;
	uint32_t real_len;
	uint32_t len;
	void compress_zlib();
	void uncompress_zlib();
	Mutex mutex;
	void lock();
	void unlock();
public:
	ByteArray(Class_base* c, uint8_t* b = NULL, uint32_t l = 0);
	~ByteArray();
	//Helper interface for serialization
	bool peekByte(uint8_t& b);
	bool readByte(uint8_t& b);
	bool readShort(uint16_t& ret);
	bool readUnsignedInt(uint32_t& ret);
	bool readU29(uint32_t& ret);
	bool readUTF(tiny_string& ret);
	void writeByte(uint8_t b);
	void writeBytes(uint8_t* data, int length);
	void writeShort(uint16_t val);
	void writeUnsignedInt(uint32_t val);
	void writeUTF(const tiny_string& str);
	uint32_t writeObject(ASObject* obj);
	void writeStringVR(std::map<tiny_string, uint32_t>& stringMap, const tiny_string& s);
	void writeStringAMF0(const tiny_string& s);
	void writeXMLString(std::map<const ASObject*, uint32_t>& objMap, ASObject *xml, const tiny_string& s);
	void writeU29(uint32_t val);

	void serializeDouble(number_t val);

	void setLength(uint32_t newLen);
	uint32_t getPosition() const;
	void setPosition(uint32_t p);
	
	void append(std::streambuf* data, int length);
	/**
	 * @brief remove bytes from front of buffer
	 * @param count number of bytes to remove
	 */
	void removeFrontBytes(int count);

	uint8_t getObjectEncoding() const { return objectEncoding; }
	void setObjectEncoding(uint8_t encoding) { objectEncoding = encoding; }
	uint8_t getCurrentObjectEncoding() const { return currentObjectEncoding; }
	void setCurrentObjectEncoding(uint8_t encoding) { currentObjectEncoding = encoding; }
	
	ASFUNCTION(_constructor);
	ASFUNCTION(_getBytesAvailable);
	ASFUNCTION(_getLength);
	ASFUNCTION(_setLength);
	ASFUNCTION(_getPosition);
	ASFUNCTION(_setPosition);
	ASFUNCTION(_getEndian);
	ASFUNCTION(_setEndian);
	ASFUNCTION(_getObjectEncoding);
	ASFUNCTION(_setObjectEncoding);
	ASFUNCTION(_getDefaultObjectEncoding);
	ASFUNCTION(_setDefaultObjectEncoding);
	ASFUNCTION(_compress);
	ASFUNCTION(_uncompress);
	ASFUNCTION(_deflate);
	ASFUNCTION(_inflate);
	ASFUNCTION(clear);
	ASFUNCTION(readBoolean);
	ASFUNCTION(readByte);
	ASFUNCTION(readBytes);
	ASFUNCTION(readDouble);
	ASFUNCTION(readFloat);
	ASFUNCTION(readInt);
	ASFUNCTION(readMultiByte);
	ASFUNCTION_ATOM(readObject);
	ASFUNCTION(readShort);
	ASFUNCTION(readUnsignedByte);
	ASFUNCTION(readUnsignedInt);
	ASFUNCTION(readUnsignedShort);
	ASFUNCTION(readUTF);
	ASFUNCTION(readUTFBytes);
	ASFUNCTION(writeBoolean);
	ASFUNCTION(writeByte);
	ASFUNCTION(writeBytes);
	ASFUNCTION(writeDouble);
	ASFUNCTION(writeFloat);
	ASFUNCTION(writeInt);
	ASFUNCTION(writeUnsignedInt);
	ASFUNCTION(writeMultiByte);
	ASFUNCTION(writeObject);
	ASFUNCTION(writeShort);
	ASFUNCTION(writeUTF);
	ASFUNCTION(writeUTFBytes);
	ASFUNCTION(_toString);
	ASPROPERTY_GETTER_SETTER(bool,shareable);
	ASFUNCTION(atomicCompareAndSwapIntAt);
	ASFUNCTION(atomicCompareAndSwapLength);
	ASFUNCTION(_toJSON);

	// these are internal methods used if the generic Array-Methods are called on a ByteArray
	ASFUNCTION_ATOM(pop);
	ASFUNCTION_ATOM(push);
	ASFUNCTION_ATOM(shift);
	ASFUNCTION_ATOM(unshift);
	/**
		Get ownership over the passed buffer
		@param buf Pointer to the buffer to acquire, ownership and delete authority is acquired
		@param bufLen Lenght of the buffer
		@pre buf must be allocated using new[]
	*/
	void acquireBuffer(uint8_t* buf, int bufLen);
	uint8_t* getBuffer(unsigned int size, bool enableResize);
	uint32_t getLength() const { return len; }

	uint16_t endianIn(uint16_t value);
	uint32_t endianIn(uint32_t value);
	uint64_t endianIn(uint64_t value);

	uint16_t endianOut(uint16_t value);
	uint32_t endianOut(uint32_t value);
	uint64_t endianOut(uint64_t value);

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	asAtom getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	int32_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultiname_i(const multiname& name, int32_t value);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);

	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

}

#endif /* SCRIPTING_FLASH_UTILS_BYTEARRAY_H */
