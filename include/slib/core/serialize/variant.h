/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_VARIANT
#define CHECKHEADER_SLIB_CORE_SERIALIZE_VARIANT

#include "primitive.h"
#include "string.h"
#include "time.h"
#include "memory.h"
#include "list.h"
#include "map.h"

#include "../variant.h"
#include "../memory_buffer.h"

namespace slib
{

	sl_bool SerializeJsonBinary(MemoryBuffer* output, Referable* ref);

	template <class OUTPUT>
	static sl_bool SerializeJsonBinary(OUTPUT* output, Referable* ref)
	{
		MemoryBuffer buf;
		if (ref->toJsonBinary(buf)) {
			return SerializeRaw(output, buf);
		}
		return sl_false;
	}

	sl_size SerializeVariantPrimitive(const Variant& var, void* buf, sl_size size);

	sl_size SerializeVariant(const Variant& var, void* buf, sl_size size, Memory* pOutMemoryIfInsufficient, const void* prefix = sl_null, sl_size sizePrefix = 0);

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const Variant& _in)
	{
		sl_uint8 buf[16];
		sl_size nWritten = SerializeVariantPrimitive(_in, buf, sizeof(buf));
		if (nWritten) {
			return SerializeRaw(output, buf, nWritten);
		}
		switch (_in._type) {
			case VariantType::String8:
			case VariantType::String16:
			case VariantType::Sz8:
			case VariantType::Sz16:
				if (!(SerializeByte(output, (sl_uint8)(VariantType::String8)))) {
					return sl_false;
				}
				return Serialize(output, _in.getString());
			case VariantType::Memory:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((Memory*)(void*)&(_in._value)));
			case VariantType::List:
				if (!(SerializeByte(output, (sl_uint8)(VariantType::Collection)))) {
					return sl_false;
				}
				return Serialize(output, *((VariantList*)(void*)&(_in._value)));
			case VariantType::Map:
				if (!(SerializeByte(output, (sl_uint8)(VariantType::Object)))) {
					return sl_false;
				}
				return Serialize(output, *((VariantMap*)(void*)&(_in._value)));
			default:
				if (_in.isRef()) {
					Ref<Referable> ref = _in.getRef();
					if (ref.isNotNull()) {
						return SerializeJsonBinary(output, ref.get());
					}
				}
				break;
		}
		return SerializeStatic(output, "", 1);
	}

	template <class INPUT>
	static sl_bool DeserializeVariantPrimitive(Variant& _out, VariantType type, INPUT* input)
	{
		switch (type) {
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Float:
				{
					sl_uint8 buf[4];
					if (DeserializeRaw(input, buf, 4)) {
						*((sl_uint32*)(void*)&(_out._value)) = MIO::readUint32LE(buf);
						break;
					}
					return sl_false;
				}
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Double:
			case VariantType::Time:
				{
					sl_uint8 buf[8];
					if (DeserializeRaw(input, buf, 4)) {
						*((sl_uint64*)(void*)&(_out._value)) = MIO::readUint64LE(buf);
						break;
					}
					return sl_false;
				}
			case VariantType::Boolean:
				{
					sl_uint8 v;
					if (DeserializeByte(input, v)) {
						*((bool*)(void*)&(_out._value)) = v ? true : false;
						break;
					}
					return sl_false;
				}
				break;
			case VariantType::Null:
				_out.setNull();
				return sl_true;
			default:
				return sl_false;
		}
		_out._type = type;
		return sl_true;
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, Variant& _out)
	{
		sl_uint8 v;
		if (!(DeserializeByte(input, v))) {
			return sl_false;
		}
		VariantType type = (VariantType)v;
		if (DeserializeVariantPrimitive(_out, type, input)) {
			return sl_true;
		}
		switch (type) {
			case VariantType::String8:
				{
					String value;
					if (Deserialize(input, value)) {
						_out.setString(Move(value));
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Memory:
				{
					Memory value;
					if (Deserialize(input, value)) {
						_out.setMemory(Move(value));
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Collection:
				{
					VariantList value;
					if (Deserialize(input, value)) {
						_out.setVariantList(Move(value));
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Object:
				{
					VariantMap value;
					if (Deserialize(input, value)) {
						_out.setVariantMap(Move(value));
						return sl_true;
					}
					return sl_false;
				}
			default:
				break;
		}
		return sl_false;
	}

	class Json;

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const Json& _in)
	{
		return Serialize(output, *((Variant*)(void*)&_in));
	}

	template <class INPUT, class T>
	static sl_bool Deserialize(INPUT* input, Json& _out)
	{
		return Deserialize(input, *((Variant*)(void*)&_out));
	}

}

#endif
