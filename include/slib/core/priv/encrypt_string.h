/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_ENCRYPT_STRING
#define CHECKHEADER_SLIB_CORE_ENCRYPT_STRING

#ifdef __cpp_constexpr
#	if __cpp_constexpr	>= 201304
#		define SLIB_SUPPORT_ENCRYPT_STRING
#	endif
#endif

#if defined(SLIB_SUPPORT_ENCRYPT_STRING)

#	ifdef _MSC_VER
#		define PRIV_SLIB_ENCRYPT_STRING_CAT(X,Y) PRIV_SLIB_ENCRYPT_STRING_CAT2(X,Y)
#		define PRIV_SLIB_ENCRYPT_STRING_CAT2(X,Y) X##Y
#		define PRIV_SLIB_ENCRYPT_STRING_LINE (sl_size)(PRIV_SLIB_ENCRYPT_STRING_CAT(__LINE__,U))
#	else
#		define PRIV_SLIB_ENCRYPT_STRING_LINE __LINE__
#	endif

namespace slib
{

	namespace priv
	{
		namespace string
		{

			constexpr sl_uint64 GenEncryptKey()
			{
				sl_uint64 k = PRIV_SLIB_ENCRYPT_STRING_LINE;
				{
					const char* time = __TIME__;
					sl_uint32 sum = 0;
					sl_size i = 0;
					do {
						sum += time[i];
						i++;
					} while (time[i]);
					k *= sum;
				}
				k ^= k >> 33;
				k = (((sl_uint64)((sl_uint32)k) * 0xff51afd7ul + (k >> 32) * 0xed558ccdul) << 32) + ((sl_uint64)((sl_uint32)k) * 0xed558ccdul);
				k ^= k >> 33;
				k = (((sl_uint64)((sl_uint32)k) * 0xc4ceb9feul + (k >> 32) * 0x1a85ec53ul) << 32) + ((sl_uint64)((sl_uint32)k) * 0x1a85ec53ul);
				k ^= k >> 33;
				return k;
			}

			template <typename CHAR>
			constexpr void RunEncryptor(CHAR* dst, const CHAR* src, sl_size n, sl_uint64 key)
			{
				for (sl_size i = 0; i < n; i++) {
					dst[i] = (CHAR)(src[i] ^ (CHAR)key);
					key = (key >> 8) | (key << 56);
				}
			}

			template <typename CHAR, sl_size LEN, sl_uint64 KEY>
			struct EncString
			{
				typename StringContainerTypeFromCharType<CHAR>::Type* container;
				CHAR const* volatile encData;
			};

			template <typename CHAR, sl_size LEN, sl_uint64 KEY>
			struct EncData
			{
				typedef CHAR CharType;
				typedef EncString<CHAR, LEN, KEY> EncStringType;
				CHAR encData[LEN];
			};

			template <typename CHAR, sl_size N, sl_uint64 KEY = GenEncryptKey()>
			constexpr EncData<CHAR, N - 1, KEY> EncryptData(const CHAR(&data)[N])
			{
				EncData<CHAR, N - 1, KEY> ret = { 0 };
				RunEncryptor(ret.encData, data, N - 1, KEY);
				return ret;
			}

			template <typename CHAR, sl_size LEN, sl_uint64 KEY>
			static typename StringTypeFromCharType<CHAR>::Type const& Decrypt(EncString<CHAR, LEN, KEY>& enc) noexcept
			{
				if (enc.encData) {
					RunEncryptor(enc.container->data, enc.encData, LEN, KEY);
					enc.container->data[LEN] = 0;
					enc.encData = sl_null;
				}
				return *(reinterpret_cast<typename StringTypeFromCharType<CHAR>::Type*>(&(enc.container)));
			}

		}
	}

}

#	define SLIB_ENCRYPT_STRING(str) ([]() { \
		constexpr static auto encData = slib::priv::string::EncryptData(str); \
		typedef typename decltype(encData)::CharType CharType; \
		static CharType data[sizeof(str) / sizeof(CharType)]; \
		static typename slib::StringContainerTypeFromCharType<CharType>::Type container = {data, sizeof(str) / sizeof(CharType) - 1, 0, 0, -1}; \
		static typename decltype(encData)::EncStringType encStr = {&container, encData.encData}; \
		return slib::priv::string::Decrypt(encStr); \
	})()
#	define SLIB_ENCRYPT_STRING16(str) SLIB_ENCRYPT_STRING(SLIB_UNICODE(str))
#	define SLIB_ENCRYPT_STRING32(str) SLIB_ENCRYPT_STRING(SLIB_UNICODE32(str))
#else
#	define SLIB_ENCRYPT_STRING(str) str
#	define SLIB_ENCRYPT_STRING16(str) SLIB_UNICODE(str)
#	define SLIB_ENCRYPT_STRING32(str) SLIB_UNICODE32(str)
#endif

#endif
