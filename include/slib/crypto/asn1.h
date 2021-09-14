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

#ifndef CHECKHEADER_SLIB_CRYPTO_ASN1
#define CHECKHEADER_SLIB_CRYPTO_ASN1

#include "definition.h"

#include "../core/memory.h"
#include "../core/serialize/io.h"

/*

X.690 is an ITU-T standard specifying several ASN.1 encoding formats:
	Basic Encoding Rules (BER)
	Canonical Encoding Rules (CER)
	Distinguished Encoding Rules (DER)

*/

#define SLIB_ASN1_TAG_CLASS_UNIVERSAL 0
#define SLIB_ASN1_TAG_CLASS_APP 1
#define SLIB_ASN1_TAG_CLASS_CONTEXT 2
#define SLIB_ASN1_TAG_CLASS_PRIVATE 3

#define SLIB_ASN1_TAG_PC_PRIVATE 0
#define SLIB_ASN1_TAG_PC_CONSTRUCTED 1

#define SLIB_ASN1_TAG_TYPE_EOC 0 // End Of Content, Primitive
#define SLIB_ASN1_TAG_TYPE_BOOL 1 // Primitive
#define SLIB_ASN1_TAG_TYPE_INT 2 // Primitive
#define SLIB_ASN1_TAG_TYPE_BIT_STRING 3 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_OCTET_STRING 4 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_NULL 5 // Primitive
#define SLIB_ASN1_TAG_TYPE_OID 6 // Object Identifier, Primitive
#define SLIB_ASN1_TAG_TYPE_OBJECT_DESCRIPTOR 7 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_EXTERNAL 8 // Constructed
#define SLIB_ASN1_TAG_TYPE_REAL 9 // Float, Primitive
#define SLIB_ASN1_TAG_TYPE_ENUMERATED 0x0A // Primitive
#define SLIB_ASN1_TAG_TYPE_EMBEDDED_PDV 0x0B // Constructed
#define SLIB_ASN1_TAG_TYPE_UTF8_STRING 0x0C // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_RELATIVE_OID 0x0D // Primitive
// 0x0E, 0x0F: Reserved
#define SLIB_ASN1_TAG_TYPE_SEQUENCE 0x10 // Constructed
#define SLIB_ASN1_TAG_TYPE_SET 0x11 // Constructed
#define SLIB_ASN1_TAG_TYPE_NUMERIC_STRING 0x12 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_PRINTABLE_STRING 0x13 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_T61_STRING 0x14 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_VIDEOTEX_STRING 0x15 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_IA5_STRING 0x16 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_UTC_TIME 0x17 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_GENERALIZED_TIME 0x18 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_GRAPHIC_STRING 0x19 // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_VISIBLE_STRING 0x1A // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_GENERAL_STRING 0x1B // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_UNIVERSAL_STRING 0x1C // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_CHARACTER_STRING 0x1D // Primitive or Constructed
#define SLIB_ASN1_TAG_TYPE_BMP_STRING 0x1E // Primitive or Constructed
// 0x1F: More tag number octets

#define SLIB_ASN1_TAG_TYPE_0 0
#define SLIB_ASN1_TAG_TYPE_1 1
#define SLIB_ASN1_TAG_TYPE_2 2
#define SLIB_ASN1_TAG_TYPE_3 3
#define SLIB_ASN1_TAG_TYPE_4 4
#define SLIB_ASN1_TAG_TYPE_5 5
#define SLIB_ASN1_TAG_TYPE_6 6
#define SLIB_ASN1_TAG_TYPE_7 7
#define SLIB_ASN1_TAG_TYPE_8 8
#define SLIB_ASN1_TAG_TYPE_9 9
#define SLIB_ASN1_TAG_TYPE_10 10
#define SLIB_ASN1_TAG_TYPE_11 11
#define SLIB_ASN1_TAG_TYPE_12 12
#define SLIB_ASN1_TAG_TYPE_13 13
#define SLIB_ASN1_TAG_TYPE_14 14
#define SLIB_ASN1_TAG_TYPE_15 15

#define SLIB_ASN1_TAG_RAW(CLASS, CONSTRUCTED, TYPE) (((CLASS) << 6) | ((CONSTRUCTED) << 5) | (TYPE))
#define SLIB_ASN1_TAG(CLASS, PRIVATE_OR_CONSTRUCTED, TYPE) SLIB_ASN1_TAG_RAW(SLIB_ASN1_TAG_CLASS_##CLASS, SLIB_ASN1_TAG_PC_##PRIVATE_OR_CONSTRUCTED, SLIB_ASN1_TAG_TYPE_##TYPE)

#define SLIB_ASN1_TAG_EOC SLIB_ASN1_TAG_TYPE_EOC // 0x00
#define SLIB_ASN1_TAG_BOOL SLIB_ASN1_TAG_TYPE_BOOL // 0x01
#define SLIB_ASN1_TAG_INT 2 SLIB_ASN1_TAG_TYPE_INT // 0x02
#define SLIB_ASN1_TAG_BIT_STRING SLIB_ASN1_TAG_TYPE_BIT_STRING // 0x03
#define SLIB_ASN1_TAG_OCTET_STRING SLIB_ASN1_TAG_TYPE_OCTET_STRING // 0x04
#define SLIB_ASN1_TAG_NULL SLIB_ASN1_TAG_TYPE_NULL // 0x05
#define SLIB_ASN1_TAG_OID SLIB_ASN1_TAG_TYPE_OID // 0x06
#define SLIB_ASN1_TAG_OBJECT_DESCRIPTOR SLIB_ASN1_TAG_TYPE_OBJECT_DESCRIPTOR // 0x07
#define SLIB_ASN1_TAG_EXTERNAL SLIB_ASN1_TAG(UNIVERSAL, CONSTRUCTED, EXTERNAL) // 0x28
#define SLIB_ASN1_TAG_REAL SLIB_ASN1_TAG_TYPE_REAL // 0x09
#define SLIB_ASN1_TAG_ENUMERATED SLIB_ASN1_TAG_TYPE_ENUMERATED // 0x0A
#define SLIB_ASN1_TAG_EMBEDDED_PDV SLIB_ASN1_TAG(UNIVERSAL, CONSTRUCTED, EMBEDDED_PDV) // 0x2B
#define SLIB_ASN1_TAG_UTF8_STRING SLIB_ASN1_TAG_TYPE_UTF8_STRING // 0x0C
#define SLIB_ASN1_TAG_RELATIVE_OID SLIB_ASN1_TAG_TYPE_RELATIVE_OID // 0x0D
#define SLIB_ASN1_TAG_SEQUENCE SLIB_ASN1_TAG(UNIVERSAL, CONSTRUCTED, SEQUENCE) // 0x30
#define SLIB_ASN1_TAG_SET SLIB_ASN1_TAG(UNIVERSAL, CONSTRUCTED, SET) // 0x31
#define SLIB_ASN1_TAG_NUMERIC_STRING SLIB_ASN1_TAG_TYPE_NUMERIC_STRING // 0x12
#define SLIB_ASN1_TAG_PRINTABLE_STRING SLIB_ASN1_TAG_TYPE_PRINTABLE_STRING // 0x13
#define SLIB_ASN1_TAG_T61_STRING SLIB_ASN1_TAG_TYPE_T61_STRING // 0x14
#define SLIB_ASN1_TAG_VIDEOTEX_STRING SLIB_ASN1_TAG_TYPE_VIDEOTEX_STRING // 0x15
#define SLIB_ASN1_TAG_IA5_STRING SLIB_ASN1_TAG_TYPE_IA5_STRING // 0x16
#define SLIB_ASN1_TAG_UTC_TIME SLIB_ASN1_TAG_TYPE_UTC_TIME // 0x17
#define SLIB_ASN1_TAG_GENERALIZED_TIME SLIB_ASN1_TAG_TYPE_GENERALIZED_TIME // 0x18
#define SLIB_ASN1_TAG_GRAPHIC_STRING SLIB_ASN1_TAG_TYPE_GRAPHIC_STRING // 0x19
#define SLIB_ASN1_TAG_VISIBLE_STRING SLIB_ASN1_TAG_TYPE_VISIBLE_STRING // 0x1A
#define SLIB_ASN1_TAG_GENERAL_STRING SLIB_ASN1_TAG_TYPE_GENERAL_STRING // 0x1B
#define SLIB_ASN1_TAG_UNIVERSAL_STRING SLIB_ASN1_TAG_TYPE_UNIVERSAL_STRING // 0x1C
#define SLIB_ASN1_TAG_CHARACTER_STRING SLIB_ASN1_TAG_TYPE_CHARACTER_STRING // 0x1D
#define SLIB_ASN1_TAG_BMP_STRING SLIB_ASN1_TAG_TYPE_BMP_STRING // 0x1E

#define SLIB_ASN1_TAG_APP(TYPE) SLIB_ASN1_TAG(APP, CONSTRUCTED, TYPE) // 0x60
#define SLIB_ASN1_TAG_CONTEXT(TYPE) SLIB_ASN1_TAG(CONTEXT, CONSTRUCTED, TYPE) // 0xA0

#define SLIB_ASN1_ENCODED_OID_SPNEGO "\x06\x06\x2b\x06\x01\x05\x05\x02" // 1.3.6.1.5.5.2 (Simple Protected Negotiation)
#define SLIB_ASN1_ENCODED_OID_NTLMSSP "\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x02\x02\x0a" // 1.3.6.1.4.1.311.2.2.10 (Microsoft NTLM Security Support Provider)

namespace slib
{

	class SLIB_EXPORT Asn1
	{
	public:
		template <class OUTPUT>
		static sl_bool serializeLength(OUTPUT* output, sl_size value)
		{
			if (value < 128) {
				return SerializeByte(output, (sl_uint8)value);
			} else {
				sl_uint8 octets[8];
				sl_uint32 pos = 7;
				octets[7] = (sl_uint8)value;
				value >>= 8;
				while (value) {
					pos--;
					octets[pos] = (sl_uint8)value;
					value >>= 8;
				}
				sl_uint32 n = 8 - pos;
				if (SerializeByte(output, (sl_uint8)(0x80 | n))) {
					return SerializeRaw(output, octets + pos, n);
				}
			}
			return sl_false;
		}

		static sl_size getSerializedLengthSize(sl_size value) noexcept;

		template <class OUTPUT>
		static sl_bool serializeElement(OUTPUT* output, sl_uint8 tag, const void* data, sl_size size)
		{
			if (SerializeByte(output, tag)) {
				if (serializeLength(output, size)) {
					return SerializeRaw(output, data, size);
				}
			}
			return sl_false;
		}

	};

	template <sl_uint8 TAG, class BODY>
	class SLIB_EXPORT Asn1Tag
	{
	public:
		template <class INPUT>
		static sl_size getSize(INPUT& input)
		{
			sl_size n = BODY::getSize(input);
			sl_size l = Asn1::getSerializedLengthSize(n);
			return 1 + l + n;
		}

		template <class OUTPUT, class INPUT>
		static sl_bool serialize(OUTPUT* output, INPUT& input)
		{
			if (SerializeByte(output, TAG)) {
				sl_size n = BODY::getSize(input);
				if (Asn1::serializeLength(output, n)) {
					return BODY::serialize(output, input);
				}
			}
			return sl_false;
		}

	};

	class SLIB_EXPORT Asn1Body
	{
	public:
		static sl_size getSize(const Memory& input) noexcept;

		template <class OUTPUT>
		static sl_bool serialize(OUTPUT* output, const Memory& input)
		{
			return SerializeRaw(output, input.getData(), input.getSize());
		}

		static sl_size getSize(const MemoryData& input) noexcept;

		template <class OUTPUT>
		static sl_bool serialize(OUTPUT* output, const MemoryData& input)
		{
			return SerializeRaw(output, input.data, input.size);
		}

		static sl_size getSize(MemoryBuffer& input) noexcept;

		static sl_bool serialize(MemoryBuffer* output, MemoryBuffer& input) noexcept;

		template <sl_size N>
		static sl_size getSize(const char(&s)[N]) noexcept
		{
			return N - 1;
		}

		template <class OUTPUT, sl_size N>
		static sl_bool serialize(OUTPUT* output, const char(&s)[N]) noexcept
		{
			return SerializeStatic(output, s, N - 1);
		}

	};

}

#endif
