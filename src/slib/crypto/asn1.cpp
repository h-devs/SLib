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

#include "slib/crypto/asn1.h"

#include "slib/core/memory_buffer.h"
#include "slib/core/string_buffer.h"
#include "slib/core/time_zone.h"

namespace slib
{

	sl_size Asn1::getSerializedLengthSize(sl_size value) noexcept
	{
		if (value < 128) {
			return 1;
		} else {
			value >>= 8;
			sl_size count = 1;
			while (value) {
				count++;
				value >>= 8;
			}
			return 1 + count;
		}
		return sl_false;
	}

	String Asn1::getObjectIdentifierString(const void* _data, sl_size length)
	{
		sl_uint8* data = (sl_uint8*)_data;
		if (!data || !length) {
			return sl_null;
		}
		StringBuffer buf;
		sl_bool flagFirst = sl_true;
		sl_uint32 current = 0;
		for (sl_size i = 0; i < length; i++) {
			sl_uint8 n = data[i];
			current = (current << 7) | (n & 0x7f);
			if (!(n & 0x80)) {
				if (flagFirst) {
					flagFirst = sl_false;
					sl_uint32 n1, n2;
					if (current >= 80) {
						n1 = 2;
						n2 = current - 80;
					} else if (current >= 40) {
						n1 = 1;
						n2 = current - 40;
					} else {
						n1 = 0;
						n2 = current;
					}
					buf.add(String::fromUint32(n1));
					buf.addStatic(".");
					buf.add(String::fromUint32(n2));
				} else {
					buf.addStatic(".");
					buf.add(String::fromUint32(current));
				}
				current = 0;
			}
		}
		return buf.merge();
	}


	sl_size Asn1Body::getSize(const Memory& input) noexcept
	{
		return input.getSize();
	}

	sl_size Asn1Body::getSize(const MemoryData& input) noexcept
	{
		return input.size;
	}

	sl_size Asn1Body::getSize(MemoryBuffer& input) noexcept
	{
		return input.getSize();
	}

	sl_bool Asn1Body::serialize(MemoryBuffer* output, MemoryBuffer& input) noexcept
	{
		output->link(input);
		return sl_true;
	}


	Asn1MemoryReader::Asn1MemoryReader(const Memory& mem): current((sl_uint8*)(mem.getData())), end((sl_uint8*)(mem.getData()) + mem.getSize())
	{
	}

	sl_bool Asn1MemoryReader::readByte(sl_uint8& _out)
	{
		if (current < end) {
			_out = *current;
			current++;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readAndCheckTag(sl_uint8 tag)
	{
		if (current < end) {
			if (*current == tag) {
				current++;
				return sl_true;
			}
			current++;
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readElementBody(Asn1String& body)
	{
		sl_size len;
		if (readLength(len)) {
			const sl_uint8* _end = current + len;
			if (_end <= end) {
				body.data = current;
				body.length = (sl_uint32)len;
				current = _end;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readElement(Asn1Element& _out)
	{
		if (readByte(_out.tag)) {
			return readElementBody(_out);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readElement(sl_uint8 tag, Asn1String& body, sl_bool flagInNotUniversal)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getBody(tag, body, flagInNotUniversal);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readSequence(Asn1MemoryReader& elements)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getSequence(elements);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readSet(Asn1MemoryReader& elements)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getSet(elements);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readBigInt(BigInt& n)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getBigInt(n);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readObjectIdentifier(Asn1ObjectIdentifier& _id)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getObjectIdentifier(_id);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readOctetString(Asn1String& _out)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getOctetString(_out);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readUtf8String(Asn1String& _out)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getUtf8String(_out);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readBitString(Asn1String& _out, sl_uint8& outBitsRemain)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getBitString(_out, outBitsRemain);
		}
		return sl_false;
	}

	sl_bool Asn1MemoryReader::readTime(Time& _out)
	{
		Asn1Element element;
		if (readElement(element)) {
			return element.getTime(_out);
		}
		return sl_false;
	}


	sl_bool Asn1Element::getBody(sl_uint8 reqTag, Asn1String& body, sl_bool flagInNotUniversal) const
	{
		if (flagInNotUniversal) {
			if (tag & 0xC0) {
				// Not Universal
				Asn1MemoryReader reader(*this);
				return reader.readElement(reqTag, body, sl_false);
			}
		}
		switch (reqTag) {
			case SLIB_ASN1_TAG_BIT_STRING:
			case SLIB_ASN1_TAG_OCTET_STRING:
			case SLIB_ASN1_TAG_OBJECT_DESCRIPTOR:
			case SLIB_ASN1_TAG_UTF8_STRING:
				// both encoding
				break;
			default:
				if (reqTag >= SLIB_ASN1_TAG_NUMERIC_STRING && reqTag <= SLIB_ASN1_TAG_BMP_STRING) {
					// both encoding
					break;
				}
				if (tag == reqTag) {
					body = *this;
					return sl_true;
				}
				return sl_false;
		}
		// Primitive or Constructed
		if (tag == reqTag || tag == (0x20 | reqTag)) {
			body = *this;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Asn1Element::getSequence(Asn1MemoryReader& elements) const
	{
		Asn1String body;
		if (getBody(SLIB_ASN1_TAG_SEQUENCE, body)) {
			elements.current = body.data;
			elements.end = body.data + body.length;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Asn1Element::getSet(Asn1MemoryReader& elements) const
	{
		Asn1String body;
		if (getBody(SLIB_ASN1_TAG_SET, body)) {
			elements.current = body.data;
			elements.end = body.data + body.length;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Asn1Element::getBigInt(BigInt& n) const
	{
		Asn1String body;
		if (getBody(SLIB_ASN1_TAG_INT, body)) {
			if (!(body.length)) {
				n.setNull();
				return sl_true;
			}
			n = BigInt::fromBytesBE(body.data, body.length, sl_true);
			return n.isNotNull();
		}
		return sl_false;
	}

	sl_bool Asn1Element::getObjectIdentifier(Asn1ObjectIdentifier& _id) const
	{
		return getBody(SLIB_ASN1_TAG_OID, _id);
	}

	sl_bool Asn1Element::getOctetString(Asn1String& _out) const
	{
		return getBody(SLIB_ASN1_TAG_OCTET_STRING, _out);
	}

	sl_bool Asn1Element::getUtf8String(Asn1String& _out) const
	{
		return getBody(SLIB_ASN1_TAG_UTF8_STRING, _out);
	}

	sl_bool Asn1Element::getBitString(Asn1String& _out, sl_uint8& outBitsRemain) const
	{
		Asn1String body;
		if (getBody(SLIB_ASN1_TAG_BIT_STRING, body)) {
			if (body.length) {
				outBitsRemain = *(body.data);
				if (outBitsRemain > 7) {
					return sl_false;
				}
				_out.data = body.data + 1;
				_out.length = body.length - 1;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Asn1Element::getTime(Time& _out) const
	{
		Asn1String body;
		if (getBody(SLIB_ASN1_TAG_UTC_TIME, body)) {
			if (body.length == 13) {
				const sl_uint8* c = body.data;
				if (c[12] != 'Z') {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < 12; i++) {
					if (!(SLIB_CHAR_IS_DIGIT(c[i]))) {
						return sl_false;
					}
				}
				TimeComponents tc;
				tc.year = 2000 + (c[0] - '0') * 10 + (c[1] - '0');
				tc.month = (c[2] - '0') * 10 + (c[3] - '0');
				tc.day = (c[4] - '0') * 10 + (c[5] - '0');
				tc.hour = (c[6] - '0') * 10 + (c[7] - '0');
				tc.minute = (c[8] - '0') * 10 + (c[9] - '0');
				tc.second = (c[10] - '0') * 10 + (c[11] - '0');
				_out = Time(tc, TimeZone::UTC());
				return sl_true;
			}
		}
		return sl_false;
	}

}
