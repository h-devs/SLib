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

	sl_bool Asn1Element::getObjectIdentifier(Asn1ObjectIdentifier& _id) const
	{
		return getBody(SLIB_ASN1_TAG_OID, _id);
	}

	sl_bool Asn1Element::getOctetString(Asn1String& _out) const
	{
		return getBody(SLIB_ASN1_TAG_OCTET_STRING, _out);
	}

}
