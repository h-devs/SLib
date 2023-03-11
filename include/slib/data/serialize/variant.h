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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_VARIANT
#define CHECKHEADER_SLIB_DATA_SERIALIZE_VARIANT

#include "primitive.h"
#include "string.h"
#include "time.h"
#include "memory.h"
#include "list.h"
#include "map.h"
#include "bytes.h"
#include "generic.h"

#include "../../core/variant.h"
#include "../../core/memory_buffer.h"

namespace slib
{

	sl_bool SerializeJsonBinary(MemoryBuffer* output, CRef* ref);

	template <class OUTPUT>
	static sl_bool SerializeJsonBinary(OUTPUT* output, CRef* ref)
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
	sl_bool Variant::serialize(OUTPUT* output) const
	{
		sl_uint8 buf[32];
		sl_size nWritten = SerializeVariantPrimitive(*this, buf, sizeof(buf));
		if (nWritten) {
			return SerializeRaw(output, buf, nWritten);
		}
		switch (_type) {
			case VariantType::String8:
			case VariantType::String16:
			case VariantType::Sz8:
			case VariantType::Sz16:
				if (_tag) {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::String8) | 0x80))) {
						return sl_false;
					}
					if (!(SerializeByte(output, _tag))) {
						return sl_false;
					}
				} else {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::String8)))) {
						return sl_false;
					}
				}
				return Serialize(output, getString());
			case VariantType::Memory:
				if (_tag) {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::Memory) | 0x80))) {
						return sl_false;
					}
					if (!(SerializeByte(output, _tag))) {
						return sl_false;
					}
				} else {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::Memory)))) {
						return sl_false;
					}
				}
				return Serialize(output, *((Memory*)((void*)&_value)));
			case VariantType::List:
				if (_tag) {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::Collection) | 0x80))) {
						return sl_false;
					}
					if (!(SerializeByte(output, _tag))) {
						return sl_false;
					}
				} else {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::Collection)))) {
						return sl_false;
					}
				}
				return Serialize(output, *((VariantList*)((void*)&_value)));
			case VariantType::Map:
				if (_tag) {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::Object) | 0x80))) {
						return sl_false;
					}
					if (!(SerializeByte(output, _tag))) {
						return sl_false;
					}
				} else {
					if (!(SerializeByte(output, (sl_uint8)(VariantType::Object)))) {
						return sl_false;
					}
				}
				return Serialize(output, *((VariantMap*)((void*)&_value)));
			default:
				if (isRef()) {
					Ref<CRef> ref = getRef();
					if (ref.isNotNull()) {
						return SerializeJsonBinary(output, ref.get());
					}
				}
				break;
		}
		return SerializeStatic(output, "", 1);
	}

	template <class INPUT>
	sl_bool Variant::deserialize(INPUT* input)
	{
		sl_uint8 type;
		if (!(DeserializeByte(input, type))) {
			return sl_false;
		}
		sl_uint8 tag;
		if (type & 0x80) {
			type &= 0x7f;
			if (!(DeserializeByte(input, tag))) {
				return sl_false;
			}
		} else {
			tag = 0;
		}
		switch (type) {
			case VariantType::Null:
				setNull();
				break;
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Float:
				{
					sl_uint8 buf[4];
					if (DeserializeRaw(input, buf, 4)) {
						_free(_type, _value);
						_type = type;
						*((sl_uint32*)((void*)&_value)) = MIO::readUint32LE(buf);
					} else {
						return sl_false;
					}
					break;
				}
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Double:
			case VariantType::Time:
				{
					sl_uint8 buf[8];
					if (DeserializeRaw(input, buf, 8)) {
						_free(_type, _value);
						_type = type;
						_value = MIO::readUint64LE(buf);
					} else {
						return sl_false;
					}
					break;
				}
			case VariantType::Boolean:
				{
					sl_uint8 v;
					if (DeserializeByte(input, v)) {
						setBoolean(v ? true : false);
					} else {
						return sl_false;
					}
					break;
				}
			case VariantType::ObjectId:
				{
					ObjectId objectId;
					if (Deserialize(input, objectId)) {
						setObjectId(objectId);
					} else {
						return sl_false;
					}
					break;
				}
			case VariantType::String8:
				{
					String value;
					if (Deserialize(input, value)) {
						setString(Move(value));
					} else {
						return sl_false;
					}
					break;
				}
			case VariantType::Memory:
				{
					Memory value;
					if (Deserialize(input, value)) {
						setMemory(Move(value));
					} else {
						return sl_false;
					}
					break;
				}
			case VariantType::Collection:
				{
					VariantList value;
					if (Deserialize(input, value)) {
						setVariantList(Move(value));
					} else {
						return sl_false;
					}
					break;
				}
			case VariantType::Object:
				{
					VariantMap value;
					if (Deserialize(input, value)) {
						setVariantMap(Move(value));
					} else {
						return sl_false;
					}
					break;
				}
			default:
				return sl_false;
		}
		_tag = tag;
		return sl_true;
	}

}

#endif
