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
#include "threading.h"
#include "scripting/flash/utils/flashutils.h"

namespace lightspark
{

class DLL_PUBLIC ByteArray: public ASObject, public IDataInput, public IDataOutput
{
friend class LoaderThread;
friend class URLLoader;
friend class ApplicationDomain;
friend class LoaderInfo;
friend class ASWorker;
protected:
	bool littleEndian;
	uint8_t objectEncoding;
	uint8_t currentObjectEncoding;
	uint32_t position;
	uint8_t* bytes;
	uint32_t real_len;
	uint32_t len;
	void compress_zlib(bool raw);
	void uncompress_zlib(bool raw);
	Mutex mutex;
	uint8_t* getBufferIntern(unsigned int size, bool enableResize);
public:
	FORCE_INLINE void lock()
	{
		if (shareable) mutex.lock();
	}
	FORCE_INLINE void unlock()
	{
		if (shareable) mutex.unlock();
	}
	ByteArray(ASWorker* wrk,Class_base* c, uint8_t* b = nullptr, uint32_t l = 0);
	~ByteArray();
	bool destruct() override;
	void finalize() override;
	//Helper interface for serialization
	bool peekByte(uint8_t& b);
	FORCE_INLINE bool readByte(uint8_t& b)
	{
		if (len <= position)
		{
			b=0;
			return false;
		}
		b=bytes[position++];
		return true;
	}
	bool readShort(uint16_t& ret);
	bool readUnsignedInt(uint32_t& ret);
	bool readU29(uint32_t& ret);
	bool readUTF(tiny_string& ret);
	bool readUTFBytes(uint32_t length,tiny_string& ret);
	bool readBytes(uint32_t offset, uint32_t length, uint8_t* ret);
	inline bool readFloat(float& ret, uint32_t pos)
	{
		if(len < pos+4)
			return false;
		union
		{
			uint32_t u;
			float f;
		} res;
		memcpy(&res,bytes+pos,4);
		res.u = endianOut(res.u);
		ret=res.f;
		return true;
	}
	inline bool readDouble(number_t& ret, uint32_t pos)
	{
		if(len < pos+8)
			return false;
	
		union
		{
			uint64_t u;
			double d;
		} res;
		memcpy(&res,bytes+pos,8);
		pos+=8;
		res.u = endianOut(res.u);
		ret=res.d;
		return true;
	}
	
	asAtom readObject();
	ASObject* readSharedObject();
	FORCE_INLINE void writeByte(uint8_t b)
	{
		getBuffer(position+1,true);
		bytes[position++] = b;
	}
	FORCE_INLINE void writeBytes(uint8_t* data, int length)
	{
		getBuffer(position+length,true);
		memcpy(bytes+position,data,length);
		position+=length;
	}
	void writeShort(uint16_t val);
	void writeUnsignedInt(uint32_t val);
	void writeUTF(const tiny_string& str);
	uint32_t writeObject(ASObject* obj,ASWorker* wrk);
	uint32_t writeAtomObject(asAtom obj,ASWorker* wrk);
	void writeSharedObject(ASObject* obj, const tiny_string& name, ASWorker* wrk);
	void writeStringVR(std::map<tiny_string, uint32_t>& stringMap, const tiny_string& s);
	void writeStringAMF0(const tiny_string& s);
	void writeXMLString(std::map<const ASObject*, uint32_t>& objMap, ASObject *xml, const tiny_string& s);
	void writeU29(uint32_t val);
	void serializeDouble(number_t val);

	void setLength(uint32_t newLen);
	FORCE_INLINE uint32_t getPosition() const
	{
		return position;
	}
	FORCE_INLINE void setPosition(uint32_t p)
	{
		lock();
		position=p;
		unlock();
	}
	
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
	bool getLittleEndian() const { return littleEndian; }
	void setLittleEndian(bool l) { littleEndian = l; }
	
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getBytesAvailable);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(_setLength);
	ASFUNCTION_ATOM(_getPosition);
	ASFUNCTION_ATOM(_setPosition);
	ASFUNCTION_ATOM(_getEndian);
	ASFUNCTION_ATOM(_setEndian);
	ASFUNCTION_ATOM(_getObjectEncoding);
	ASFUNCTION_ATOM(_setObjectEncoding);
	ASFUNCTION_ATOM(_getDefaultObjectEncoding);
	ASFUNCTION_ATOM(_setDefaultObjectEncoding);
	ASFUNCTION_ATOM(_compress);
	ASFUNCTION_ATOM(_uncompress);
	ASFUNCTION_ATOM(_deflate);
	ASFUNCTION_ATOM(_inflate);
	ASFUNCTION_ATOM(clear);
	ASFUNCTION_ATOM(readBoolean);
	ASFUNCTION_ATOM(readByte);
	ASFUNCTION_ATOM(readBytes);
	ASFUNCTION_ATOM(readDouble);
	ASFUNCTION_ATOM(readFloat);
	ASFUNCTION_ATOM(readInt);
	ASFUNCTION_ATOM(readMultiByte);
	ASFUNCTION_ATOM(readObject);
	ASFUNCTION_ATOM(readShort);
	ASFUNCTION_ATOM(readUnsignedByte);
	ASFUNCTION_ATOM(readUnsignedInt);
	ASFUNCTION_ATOM(readUnsignedShort);
	ASFUNCTION_ATOM(readUTF);
	ASFUNCTION_ATOM(readUTFBytes);
	ASFUNCTION_ATOM(writeBoolean);
	ASFUNCTION_ATOM(writeByte);
	ASFUNCTION_ATOM(writeBytes);
	ASFUNCTION_ATOM(writeDouble);
	ASFUNCTION_ATOM(writeFloat);
	ASFUNCTION_ATOM(writeInt);
	ASFUNCTION_ATOM(writeUnsignedInt);
	ASFUNCTION_ATOM(writeMultiByte);
	ASFUNCTION_ATOM(writeObject);
	ASFUNCTION_ATOM(writeShort);
	ASFUNCTION_ATOM(writeUTF);
	ASFUNCTION_ATOM(writeUTFBytes);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER_SETTER(bool,shareable);
	ASFUNCTION_ATOM(atomicCompareAndSwapIntAt);
	ASFUNCTION_ATOM(atomicCompareAndSwapLength);
	ASFUNCTION_ATOM(_toJSON);

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
	inline uint8_t* getBufferNoCheck() const { return bytes; }
	inline uint8_t* getBuffer(unsigned int size, bool enableResize)
	{
		if (size <= real_len && size > 0)
		{
			if(len<size)
			{
				len=size;
			}
			return bytes;
		}
		return getBufferIntern(size,enableResize);
	}
	uint32_t getLength() const { return len; }

	FORCE_INLINE uint16_t endianIn(uint16_t value)
	{
		if(littleEndian)
			return GUINT16_TO_LE(value);
		else
			return GUINT16_TO_BE(value);
	}
	FORCE_INLINE uint32_t endianIn(uint32_t value)
	{
		if(littleEndian)
			return GUINT32_TO_LE(value);
		else
			return GUINT32_TO_BE(value);
	}
	FORCE_INLINE uint64_t endianIn(uint64_t value)
	{
		if(littleEndian)
			return GUINT64_TO_LE(value);
		else
			return GUINT64_TO_BE(value);
	}

	FORCE_INLINE uint16_t endianOut(uint16_t value)
	{
		if(littleEndian)
			return GUINT16_FROM_LE(value);
		else
			return GUINT16_FROM_BE(value);
	}
	FORCE_INLINE uint32_t endianOut(uint32_t value)
	{
		if(littleEndian)
			return GUINT32_FROM_LE(value);
		else
			return GUINT32_FROM_BE(value);
	}
	FORCE_INLINE uint64_t endianOut(uint64_t value)
	{
		if(littleEndian)
			return GUINT64_FROM_LE(value);
		else
			return GUINT64_FROM_BE(value);
	}

	static void sinit(Class_base* c);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom &ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	GET_VARIABLE_RESULT getVariableByInteger(asAtom& ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	int32_t getVariableByMultiname_i(const multiname& name, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	void setVariableByInteger(int index, asAtom& o, CONST_ALLOWED_FLAG allowConst,bool *alreadyset,ASWorker* wrk) override;
	void setVariableByMultiname_i(multiname& name, int32_t value,ASWorker* wrk) override;
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk) override;

	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
};

}

#endif /* SCRIPTING_FLASH_UTILS_BYTEARRAY_H */
