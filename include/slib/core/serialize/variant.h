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

	template <class OUTPUT>
	sl_bool Serialize(OUTPUT* output, const Variant& _in)
	{
		switch (_in._type) {
			case VariantType::Int32:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((sl_int32*)(void*)&(_in._value)));
			case VariantType::Uint32:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((sl_uint32*)(void*)&(_in._value)));
			case VariantType::Int64:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((sl_int64*)(void*)&(_in._value)));
			case VariantType::Uint64:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((sl_uint64*)(void*)&(_in._value)));
			case VariantType::Float:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((float*)(void*)&(_in._value)));
			case VariantType::Double:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((double*)(void*)&(_in._value)));
			case VariantType::Boolean:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((bool*)(void*)&(_in._value)));
			case VariantType::String8:
			case VariantType::String16:
			case VariantType::Sz8:
			case VariantType::Sz16:
				if (!(SerializeByte(output, (sl_uint8)(VariantType::String8)))) {
					return sl_false;
				}
				return Serialize(output, _in.getString());
			case VariantType::Time:
				if (!(SerializeByte(output, (sl_uint8)(_in._type)))) {
					return sl_false;
				}
				return Serialize(output, *((Time*)(void*)&(_in._value)));
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
						MemoryBuffer buf;
						if (ref->toJsonBinary(buf)) {
							MemoryData data;
							while (buf.pop(data)) {
								if (!(SerializeRaw(output, data))) {
									return sl_false;
								}
							}
							return sl_true;
						}
					}
				}
				break;
		}
		return SerializeStatic(output, "", 1);
	}

	template <class INPUT>
	sl_size Deserialize(INPUT* input, Variant& _out)
	{
		sl_uint8 v;
		if (!(DeserializeByte(input, v))) {
			return sl_false;
		}
		VariantType type = (VariantType)v;
		switch (type) {
			case VariantType::Int32:
				{
					sl_int32 value;
					if (Deserialize(input, value)) {
						_out.setInt32(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Uint32:
				{
					sl_uint32 value;
					if (Deserialize(input, value)) {
						_out.setUint32(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Int64:
				{
					sl_int64 value;
					if (Deserialize(input, value)) {
						_out.setInt64(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Uint64:
				{
					sl_uint64 value;
					if (Deserialize(input, value)) {
						_out.setUint64(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Float:
				{
					float value;
					if (Deserialize(input, value)) {
						_out.setFloat(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Double:
				{
					double value;
					if (Deserialize(input, value)) {
						_out.setDouble(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Boolean:
				{
					bool value;
					if (Deserialize(input, value)) {
						_out.setBoolean(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::String8:
				{
					String value;
					if (Deserialize(input, value)) {
						_out.setString(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Time:
				{
					Time value;
					if (Deserialize(input, value)) {
						_out.setTime(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Memory:
				{
					Memory value;
					if (Deserialize(input, value)) {
						_out.setMemory(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Collection:
				{
					VariantList value;
					if (Deserialize(input, value)) {
						_out.setVariantList(value);
						return sl_true;
					}
					return sl_false;
				}
			case VariantType::Object:
				{
					VariantMap value;
					if (Deserialize(input, value)) {
						_out.setVariantMap(value);
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
	static sl_size Deserialize(INPUT* input, Json& _out)
	{
		return Deserialize(input, *((Variant*)(void*)&_out));
	}

}

#endif
