/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#define SLIB_SUPPORT_STD_TYPES

#include "slib/core/string.h"

#include "slib/core/memory.h"
#include "slib/core/mio.h"
#include "slib/core/memory_traits.h"
#include "slib/core/string_traits.h"
#include "slib/core/locale.h"
#include "slib/core/time_zone.h"
#include "slib/core/variant.h"
#include "slib/core/string_buffer.h"
#include "slib/math/bigint.h"

#define EMPTY_SZ(CHAR_TYPE) ((CHAR_TYPE*)"\0\0\0\0")

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(StringStorage)

	StringStorage::StringStorage() noexcept: data(0), length(0), charSize(0) {}

	StringStorage::StringStorage(const String& str) noexcept: string8(str)
	{
		data8 = string8.getData(length);
		charSize = 1;
	}

	StringStorage::StringStorage(String&& str) noexcept: string8(Move(str))
	{
		data8 = string8.getData(length);
		charSize = 1;
	}

	StringStorage::StringStorage(const String16& str) noexcept: string16(str)
	{
		data16 = string16.getData(length);
		charSize = 2;
	}

	StringStorage::StringStorage(String16&& str) noexcept: string16(Move(str))
	{
		data16 = string16.getData(length);
		charSize = 2;
	}

	StringStorage::StringStorage(const String32& str) noexcept: string32(str)
	{
		data32 = string32.getData(length);
		charSize = 4;
	}

	StringStorage::StringStorage(String32&& str) noexcept: string32(Move(str))
	{
		data32 = string32.getData(length);
		charSize = 4;
	}

	namespace {

		enum STRING_CONTAINER_TYPES
		{
			STRING_CONTAINER_TYPE_NORMAL = 0,
			STRING_CONTAINER_TYPE_STD = 10,
			STRING_CONTAINER_TYPE_REF = 11,
			STRING_CONTAINER_TYPE_SUB = 12
		};

		static const sl_char8 g_empty_buf[] = {0, 0};
		static const StringContainer g_empty_container = {const_cast<sl_char8*>(g_empty_buf), 0, 0, STRING_CONTAINER_TYPE_NORMAL, -1};
		static const sl_char16 g_empty_buf16[] = {0, 0};
		static const StringContainer16 g_empty_container16 = {const_cast<sl_char16*>(g_empty_buf16), 0, 0, STRING_CONTAINER_TYPE_NORMAL, -1};
		static const sl_char32 g_empty_buf32[] = { 0, 0 };
		static const StringContainer32 g_empty_container32 = { const_cast<sl_char32*>(g_empty_buf32), 0, 0, STRING_CONTAINER_TYPE_NORMAL, -1 };

		static const char g_conv_radix_pattern_lower[65] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_";
		static const char g_conv_radix_pattern_upper[65] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@_";

		static const sl_uint8 g_conv_radix_inverse_pattern_big[128] = {
			/*00*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*08*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*10*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*18*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*20*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*28*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*30*/ 0,   1,   2,   3,   4,   5,   6,   7,
			/*38*/ 8,   9,   255, 255, 255, 255, 255, 255,
			/*40*/ 62,  36,  37,  38,  39,  40,  41,  42,
			/*48*/ 43,  44,  45,  46,  47,  48,  49,  50,
			/*50*/ 51,  52,  53,  54,  55,  56,  57,  58,
			/*58*/ 59,  60,  61,  255, 255, 255, 255, 63,
			/*60*/ 255, 10,  11,  12,  13,  14,  15,  16,
			/*68*/ 17,  18,  19,  20,  21,  22,  23,  24,
			/*70*/ 25,  26,  27,  28,  29,  30,  31,  32,
			/*78*/ 33,  34,  35,  255, 255, 255, 255, 255
		};

		static const sl_uint8 g_conv_radix_inverse_pattern_small[128] = {
			/*00*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*08*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*10*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*18*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*20*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*28*/ 255, 255, 255, 255, 255, 255, 255, 255,
			/*30*/ 0,   1,   2,   3,   4,   5,   6,   7,
			/*38*/ 8,   9,   255, 255, 255, 255, 255, 255,
			/*40*/ 255, 10,  11,  12,  13,  14,  15,  16,
			/*48*/ 17,  18,  19,  20,  21,  22,  23,  24,
			/*50*/ 25,  26,  27,  28,  29,  30,  31,  32,
			/*58*/ 33,  34,  35,  255, 255, 255, 255, 255,
			/*60*/ 255, 10,  11,  12,  13,  14,  15,  16,
			/*68*/ 17,  18,  19,  20,  21,  22,  23,  24,
			/*70*/ 25,  26,  27,  28,  29,  30,  31,  32,
			/*78*/ 33,  34,  35,  255, 255, 255, 255, 255
		};

	}

	namespace priv
	{
		namespace string
		{
			StringContainer* const g_null = sl_null;
			StringContainer* const g_empty = const_cast<StringContainer*>(&g_empty_container);
			StringContainer16* const g_null16 = sl_null;
			StringContainer16* const g_empty16 = const_cast<StringContainer16*>(&g_empty_container16);
			StringContainer32* const g_null32 = sl_null;
			StringContainer32* const g_empty32 = const_cast<StringContainer32*>(&g_empty_container32);

			const char* g_conv_radixPatternLower = g_conv_radix_pattern_lower;
			const char* g_conv_radixPatternUpper = g_conv_radix_pattern_upper;
			const sl_uint8* g_conv_radixInversePatternBig = g_conv_radix_inverse_pattern_big;
			const sl_uint8* g_conv_radixInversePatternSmall = g_conv_radix_inverse_pattern_small;
		}
	}

	namespace {

		struct StringViewContainer
		{
			const sl_char8* data;
			sl_reg length;
		};

		static StringViewContainer g_stringView_null = { sl_null, 0 };
		static StringViewContainer g_stringView_empty = { "\0\0\0\0", 0 };

		template <class CONTAINTER>
		class ConstContainers;

		template <>
		class ConstContainers<StringContainer>
		{
		public:
			constexpr static StringContainer* getEmpty()
			{
				return const_cast<StringContainer*>(&g_empty_container);
			}
		};

		template <>
		class ConstContainers<StringContainer16>
		{
		public:
			constexpr static StringContainer16* getEmpty()
			{
				return const_cast<StringContainer16*>(&g_empty_container16);
			}
		};

		template <>
		class ConstContainers<StringContainer32>
		{
		public:
			constexpr static StringContainer32* getEmpty()
			{
				return const_cast<StringContainer32*>(&g_empty_container32);
			}
		};

		template <class STRING_CONTAINER, class OBJECT>
		class ObjectContainer : public STRING_CONTAINER
		{
		public:
			OBJECT object;

		public:
			template <class TYPE>
			ObjectContainer(TYPE&& _object): object(Forward<TYPE>(_object)) {}

		};

		template <class STRING_CONTAINER>
		using StdContainer = ObjectContainer<STRING_CONTAINER, typename STRING_CONTAINER::StringType::StdString>;
		template <class STRING_CONTAINER>
		using RefContainer = ObjectContainer< STRING_CONTAINER, Ref<CRef> >;
		template <class STRING_CONTAINER>
		using SubContainer = ObjectContainer<STRING_CONTAINER, typename STRING_CONTAINER::StringType>;

		template <class CONTAINER>
		SLIB_INLINE static void Free(CONTAINER* _container) noexcept
		{
			sl_uint32 type = _container->type;
			if (type != STRING_CONTAINER_TYPE_NORMAL) {
				if (type == STRING_CONTAINER_TYPE_STD) {
					StdContainer<CONTAINER>* container = static_cast<StdContainer<CONTAINER>*>(_container);
					container->StdContainer<CONTAINER>::~ObjectContainer();
				} else if (type == STRING_CONTAINER_TYPE_REF) {
					RefContainer<CONTAINER>* container = static_cast<RefContainer<CONTAINER>*>(_container);
					container->RefContainer<CONTAINER>::~ObjectContainer();
				} else if (type == STRING_CONTAINER_TYPE_SUB) {
					SubContainer<CONTAINER>* container = static_cast<SubContainer<CONTAINER>*>(_container);
					container->SubContainer<CONTAINER>::~ObjectContainer();
				}
			}
			Base::freeMemory(_container);
		}

		template <class CONTAINER>
		static CONTAINER* Alloc(sl_size len) noexcept
		{
			if (!len) {
				return ConstContainers<CONTAINER>::getEmpty();
			}
			sl_uint8* buf = (sl_uint8*)(Base::createMemory(sizeof(CONTAINER) + (len + 1) * sizeof(typename CONTAINER::StringType::Char)));
			if (buf) {
				CONTAINER* container = reinterpret_cast<CONTAINER*>(buf);
				container->data = (typename CONTAINER::StringType::Char*)(buf + sizeof(CONTAINER));
				container->len = len;
				container->hash = 0;
				container->type = STRING_CONTAINER_TYPE_NORMAL;
				container->ref = 1;
				container->data[len] = 0;
				return container;
			}
			return sl_null;
		}

		template <class CONTAINER>
		static CONTAINER* Realloc(CONTAINER* old, sl_size len) noexcept
		{
			if (!len) {
				Free(old);
				return ConstContainers<CONTAINER>::getEmpty();
			}
			sl_uint8* buf = (sl_uint8*)(Base::reallocMemory(old, sizeof(CONTAINER) + (len + 1) * sizeof(typename CONTAINER::StringType::Char)));
			if (buf) {
				CONTAINER* container = reinterpret_cast<CONTAINER*>(buf);
				container->data = (typename CONTAINER::StringType::Char*)(buf + sizeof(CONTAINER));
				container->len = len;
				container->hash = 0;
				container->data[len] = 0;
				return container;
			} else {
				Free(old);
			}
			return sl_null;
		}

		template <class CONTAINER>
		static CONTAINER* AllocStatic(typename CONTAINER::StringType::Char const* sz, sl_size len) noexcept
		{
			if (!len) {
				return ConstContainers<CONTAINER>::getEmpty();
			}
			CONTAINER* container = (CONTAINER*)(Base::createMemory(sizeof(CONTAINER)));
			if (container) {
				container->data = (typename CONTAINER::StringType::Char*)sz;
				container->len = len;
				container->hash = 0;
				container->type = STRING_CONTAINER_TYPE_NORMAL;
				container->ref = 1;
				return container;
			}
			return sl_null;
		}

		template <class CONTAINER, class VALUE>
		static CONTAINER* AllocStd(VALUE&& str) noexcept
		{
			sl_size len = (sl_size)(str.length());
			if (!len) {
				return ConstContainers<CONTAINER>::getEmpty();
			}
			if (len < 40) {
				CONTAINER* container = Alloc<CONTAINER>(len);
				if (container) {
					MemoryTraits<typename CONTAINER::StringType::Char>::copy(container->data, str.c_str(), len);
					return container;
				}
			} else {
				StdContainer<CONTAINER>* container = (StdContainer<CONTAINER>*)(Base::createMemory(sizeof(StdContainer<CONTAINER>)));
				if (container) {
					new (container) StdContainer<CONTAINER>(Forward<VALUE>(str));
					container->data = (typename CONTAINER::StringType::Char*)(container->object.c_str());
					container->len = len;
					container->hash = 0;
					container->type = STRING_CONTAINER_TYPE_STD;
					container->ref = 1;
					return container;
				}
			}
			return sl_null;
		}

		template <class CONTAINER>
		static CONTAINER* AllocRef(CRef* obj, typename CONTAINER::StringType::Char const* sz, sl_size len) noexcept
		{
			if (!len) {
				return ConstContainers<CONTAINER>::getEmpty();
			}
			RefContainer<CONTAINER>* container = (RefContainer<CONTAINER>*)(Base::createMemory(sizeof(RefContainer<CONTAINER>)));
			if (container) {
				new (container) RefContainer<CONTAINER>(obj);
				container->data = (typename CONTAINER::StringType::Char*)(sz);
				container->len = len;
				container->hash = 0;
				container->type = STRING_CONTAINER_TYPE_REF;
				container->ref = 1;
				return container;
			}
			return sl_null;
		}

		template <class CONTAINER>
		static CONTAINER* AllocSub(typename CONTAINER::StringType const& str, typename CONTAINER::StringType::Char const* sz, sl_size len) noexcept
		{
			if (!len) {
				return ConstContainers<CONTAINER>::getEmpty();
			}
			SubContainer<CONTAINER>* container = (SubContainer<CONTAINER>*)(Base::createMemory(sizeof(SubContainer<CONTAINER>)));
			if (container) {
				new (container) SubContainer<CONTAINER>(str);
				container->data = (typename CONTAINER::StringType::Char*)(sz);
				container->len = len;
				container->hash = 0;
				container->type = STRING_CONTAINER_TYPE_SUB;
				container->ref = 1;
				return container;
			}
			return sl_null;
		}

		template <class CONTAINER>
		static CONTAINER* Create(typename CONTAINER::StringType::Char ch, sl_size nRepeatCount) noexcept
		{
			CONTAINER* container = Alloc<CONTAINER>(nRepeatCount);
			if (container && nRepeatCount) {
				MemoryTraits<typename CONTAINER::StringType::Char>::reset(container->data, nRepeatCount, ch);
			}
			return container;
		}

		template <class CHAR>
		static sl_size ConvertCharset(const CHAR* src, sl_reg lenSrc, CHAR* dst) noexcept
		{
			if (dst) {
				if (lenSrc >= 0) {
					MemoryTraits<CHAR>::copy(dst, src, lenSrc);
					return lenSrc;
				} else {
					return StringTraits<CHAR>::copy(dst, src);
				}
			} else {
				if (lenSrc >= 0) {
					return lenSrc;
				} else {
					return StringTraits<CHAR>::getLength(src);
				}
			}
		}

		SLIB_INLINE static sl_size ConvertCharset(const sl_char8* utf8, sl_reg lenUtf8, sl_char16* utf16) noexcept
		{
			return Charsets::utf8ToUtf16(utf8, lenUtf8, utf16, -1);
		}

		SLIB_INLINE static sl_size ConvertCharset(const sl_char8* utf8, sl_reg lenUtf8, sl_char32* utf32) noexcept
		{
			return Charsets::utf8ToUtf32(utf8, lenUtf8, utf32, -1);
		}

		SLIB_INLINE static sl_size ConvertCharset(const sl_char16* utf16, sl_reg lenUtf16, sl_char8* utf8) noexcept
		{
			return Charsets::utf16ToUtf8(utf16, lenUtf16, utf8, -1);
		}

		SLIB_INLINE static sl_size ConvertCharset(const sl_char16* utf16, sl_reg lenUtf16, sl_char32* utf32) noexcept
		{
			return Charsets::utf16ToUtf32(utf16, lenUtf16, utf32, -1);
		}

		SLIB_INLINE static sl_size ConvertCharset(const sl_char32* utf32, sl_reg lenUtf32, sl_char8* utf8) noexcept
		{
			return Charsets::utf32ToUtf8(utf32, lenUtf32, utf8, -1);
		}

		SLIB_INLINE static sl_size ConvertCharset(const sl_char32* utf32, sl_reg lenUtf32, sl_char16* utf16) noexcept
		{
			return Charsets::utf32ToUtf16(utf32, lenUtf32, utf16, -1);
		}

		template <class CONTAINER, class CHAR_TYPE>
		class CreatorFromString
		{
		public:
			static CONTAINER* create(const CHAR_TYPE* src, sl_reg lenSrc) noexcept
			{
				if (src) {
					sl_size lenDst = 0;
					if (lenSrc) {
						lenDst = ConvertCharset(src, lenSrc, (typename CONTAINER::StringType::Char*)sl_null);
					}
					CONTAINER* container = Alloc<CONTAINER>(lenDst);
					if (container && lenDst) {
						ConvertCharset(src, lenSrc, container->data);
						container->data[lenDst] = 0;
					}
					return container;
				}
				return sl_null;
			}
		};

		template <class CONTAINER>
		class CreatorFromString<CONTAINER, typename CONTAINER::StringType::Char>
		{
		public:
			static CONTAINER* create(typename CONTAINER::StringType::Char const* str, sl_reg len) noexcept
			{
				if (str) {
					if (len < 0) {
						len = StringTraits<typename CONTAINER::StringType::Char>::getLength(str);
					}
					CONTAINER* container = Alloc<CONTAINER>(len);
					if (container && len > 0) {
						MemoryTraits<typename CONTAINER::StringType::Char>::copy(container->data, str, len);
					}
					return container;
				}
				return sl_null;
			}
		};

		template <class CONTAINER, class CHAR_TYPE>
		SLIB_INLINE static CONTAINER* CreateFromSz(const CHAR_TYPE* src, sl_reg lenSrc) noexcept
		{
			return CreatorFromString<CONTAINER, CHAR_TYPE>::create(src, lenSrc);
		}

		template <class CONTAINER, class STRING>
		SLIB_INLINE static CONTAINER* CreateFromString(const STRING& str) noexcept
		{
			sl_size len;
			typename STRING::Char* sz = str.getData(len);
			return CreatorFromString<CONTAINER, typename STRING::Char>::create(sz, len);
		}

		SLIB_INLINE sl_size FromUtf16(EndianType endian, const void* utf16, sl_size sizeUtf16, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
		{
			return Charsets::utf16ToUtf8(endian, utf16, sizeUtf16, utf8, lenUtf8Buffer);
		}

		SLIB_INLINE sl_size FromUtf16(EndianType endian, const void* utf16, sl_size sizeUtf16, sl_char32* utf32, sl_reg lenUtf32Buffer) noexcept
		{
			return Charsets::utf16ToUtf32(endian, utf16, sizeUtf16, utf32, lenUtf32Buffer);
		}

		template <class CONTAINER>
		class CreatorFromUtf16
		{
		public:
			static CONTAINER* create(EndianType endian, const void* src, sl_size size) noexcept
			{
				if (src) {
					if (!size) {
						return ConstContainers<CONTAINER>::getEmpty();
					}
					sl_size lenDst = FromUtf16(endian, src, size, (typename CONTAINER::StringType::Char*)sl_null, -1);
					CONTAINER* container = Alloc<CONTAINER>(lenDst);
					if (container && lenDst) {
						FromUtf16(endian, src, size, container->data, lenDst);
						container->data[lenDst] = 0;
					}
					return container;
				}
				return sl_null;
			}
		};

		template <>
		class CreatorFromUtf16<StringContainer16>
		{
		public:
			static StringContainer16* create(EndianType endian, const void* src, sl_size size) noexcept
			{
				if (src) {
					sl_size len = size >> 1;
					StringContainer16* container = Alloc<StringContainer16>(len);
					if (container && len) {
						Charsets::utf16ToUtf16(endian, src, container->data, len);
						container->data[len] = 0;
					}
					return container;
				}
				return sl_null;
			}
		};

		template <class CONTAINER>
		SLIB_INLINE static CONTAINER* CreateFromUtf16(EndianType endian, const void* src, sl_size size) noexcept
		{
			return CreatorFromUtf16<CONTAINER>::create(endian, src, size);
		}

		template <class CONTAINER>
		static CONTAINER* CreateFromUtf(const void* _buf, sl_size size) noexcept
		{
			sl_char8* buf = (sl_char8*)_buf;
			if (!buf) {
				return sl_null;
			}
			if (!size) {
				return ConstContainers<CONTAINER>::getEmpty();
			}
			if (size >= 2) {
				if (buf[0] == (sl_char8)0xFF && buf[1] == (sl_char8)0xFE) {
					return CreateFromUtf16<CONTAINER>(Endian::Little, buf + 2, size - 2);
				}
				if (buf[0] == (sl_char8)0xFE && buf[1] == (sl_char8)0xFF) {
					return CreateFromUtf16<CONTAINER>(Endian::Big, buf + 2, size - 2);
				}
			}
			if (size >= 3) {
				if (buf[0] == (sl_char8)0xEF && buf[1] == (sl_char8)0xBB && buf[2] == (sl_char8)0xBF) {
					return CreateFromSz<CONTAINER>(buf + 3, size - 3);
				}
			}
			return CreateFromSz<CONTAINER>(buf, size);
		}

		template <class STRING>
		static typename STRING::Char* GetNullTerminatedData(typename STRING::Container* container, sl_size& outLength, STRING& outStringConverted) noexcept
		{
			if (container) {
				sl_uint32 type = container->type;
				sl_size len = container->len;
				if (type == STRING_CONTAINER_TYPE_NORMAL || type == STRING_CONTAINER_TYPE_STD) {
					outLength = len;
					return container->data;
				}
				typename STRING::Char* data = container->data;
				if (data[len]) {
					outStringConverted = STRING(data, len);
					outLength = len;
					return outStringConverted.getData();
				} else {
					outLength = len;
					return data;
				}
			}
			outLength = 0;
			return EMPTY_SZ(typename STRING::Char);
		}

		template <class STRING>
		static Memory ToMemory(STRING* thiz, typename STRING::Container* container) noexcept
		{
			if (container) {
				if (container->type == STRING_CONTAINER_TYPE_REF) {
					RefContainer<typename STRING::Container>* c = static_cast<RefContainer<typename STRING::Container>*>(container);
					if (IsInstanceOf<CMemory>(c->object)) {
						CMemory* mem = (CMemory*)(c->object.ptr);
						if (mem->data == container->data && mem->size == container->len) {
							return mem;
						}
					}
				}
				return Memory::createFromString(*thiz);
			}
			return sl_null;
		}

		template <class STRING>
		SLIB_INLINE static STRING MidPriv(const STRING* thiz, typename STRING::Char const* data, sl_size start, sl_size length)
		{
			if (length <= 32) {
				return STRING(data + start, length);
			} else {
				return AllocSub<typename STRING::Container>(*thiz, data + start, length);
			}
		}

		template <>
		SLIB_INLINE StringView MidPriv<StringView>(const StringView* thiz, const sl_char8* data, sl_size start, sl_size length)
		{
			return StringView(data + start, length);
		}

		template <>
		SLIB_INLINE StringView16 MidPriv<StringView16>(const StringView16* thiz, const sl_char16* data, sl_size start, sl_size length)
		{
			return StringView16(data + start, length);
		}

		template <>
		SLIB_INLINE StringView32 MidPriv<StringView32>(const StringView32* thiz, const sl_char32* data, sl_size start, sl_size length)
		{
			return StringView32(data + start, length);
		}

		template <class STRING>
		static STRING SubString(const STRING* thiz, sl_reg start, sl_reg end) noexcept
		{
			if (thiz->isNull()) {
				return sl_null;
			}
			sl_size length;
			typename STRING::Char* data = thiz->getData(length);
			if (start < 0) {
				start = 0;
			}
			if (end < 0 || (sl_size)end > length) {
				end = length;
			}
			if (start >= end) {
				return STRING::getEmpty();
			}
			if (!start && end == length) {
				return *thiz;
			}
			return MidPriv(thiz, data, start, end - start);
		}

		template <class STRING>
		static STRING Left(const STRING* thiz, sl_reg len) noexcept
		{
			if (thiz->isNull()) {
				return sl_null;
			}
			if (len <= 0) {
				return STRING::getEmpty();
			}
			sl_size total;
			typename STRING::Char* data = thiz->getData(total);
			if ((sl_size)len >= total) {
				return *thiz;
			}
			return MidPriv(thiz, data, 0, len);
		}

		template <class STRING>
		static STRING Right(const STRING* thiz, sl_reg len) noexcept
		{
			if (thiz->isNull()) {
				return sl_null;
			}
			if (len <= 0) {
				return STRING::getEmpty();
			}
			sl_size total;
			typename STRING::Char* data = thiz->getData(total);
			if ((sl_size)len >= total) {
				return *thiz;
			}
			return MidPriv(thiz, data, total - (sl_size)len, len);
		}

		template <class STRING>
		static STRING Mid(const STRING* thiz, sl_reg start, sl_reg len) noexcept
		{
			if (len < 0) {
				return SubString(thiz, start, -1);
			} else {
				return SubString(thiz, start, start + len);
			}
		}

		template <class CONTAINER, class CHAR1, class CHAR2>
		class Concatenator
		{
		public:
			static CONTAINER* concat(const CHAR1* src1, sl_reg lenSrc1, const CHAR2* src2, sl_reg lenSrc2) noexcept
			{
				sl_size len1 = 0;
				if (lenSrc1) {
					len1 = ConvertCharset(src1, lenSrc1, (typename CONTAINER::StringType::Char*)sl_null);
				}
				sl_size len2 = 0;
				if (lenSrc2) {
					len2 = ConvertCharset(src2, lenSrc2, (typename CONTAINER::StringType::Char*)sl_null);
				}
				sl_size len = len1 + len2;
				CONTAINER* container = Alloc<CONTAINER>(len);
				if (container && len) {
					if (len1) {
						ConvertCharset(src1, lenSrc1, container->data);
					}
					if (len2) {
						ConvertCharset(src2, lenSrc2, container->data + len1);
					}
				}
				return container;
			}
		};

		template <class CONTAINER>
		class Concatenator<CONTAINER, typename CONTAINER::StringType::Char, typename CONTAINER::StringType::Char>
		{
		public:
			static CONTAINER* concat(typename CONTAINER::StringType::Char const* s1, sl_reg len1, typename CONTAINER::StringType::Char const* s2, sl_reg len2) noexcept
			{
				if (len1 < 0) {
					len1 = StringTraits<typename CONTAINER::StringType::Char>::getLength(s1);
				}
				if (len2 < 0) {
					len2 = StringTraits<typename CONTAINER::StringType::Char>::getLength(s2);
				}
				sl_size len = len1 + len2;
				CONTAINER* container = Alloc<CONTAINER>(len);
				if (container && len) {
					MemoryTraits<typename CONTAINER::StringType::Char>::copy(container->data, s1, len1);
					MemoryTraits<typename CONTAINER::StringType::Char>::copy(container->data + len1, s2, len2);
				}
				return container;
			}
		};

		template <class CONTAINER, class CHAR_TYPE>
		class Concatenator<CONTAINER, typename CONTAINER::StringType::Char, CHAR_TYPE>
		{
		public:
			static CONTAINER* concat(typename CONTAINER::StringType::Char const* s1, sl_reg len1, const CHAR_TYPE* src2, sl_reg lenSrc2) noexcept
			{
				if (len1 < 0) {
					len1 = StringTraits<typename CONTAINER::StringType::Char>::getLength(s1);
				}
				if (lenSrc2 < 0) {
					lenSrc2 = StringTraits<CHAR_TYPE>::getLength(src2);
				}
				sl_size len2 = 0;
				if (lenSrc2) {
					len2 = ConvertCharset(src2, lenSrc2, (typename CONTAINER::StringType::Char*)sl_null);
				}
				sl_size len = len1 + len2;
				CONTAINER* container = Alloc<CONTAINER>(len);
				if (container && len) {
					MemoryTraits<typename CONTAINER::StringType::Char>::copy(container->data, s1, len1);
					if (len2) {
						ConvertCharset(src2, lenSrc2, container->data + len1);
					}
				}
				return container;
			}
		};

		template <class CONTAINER, class CHAR_TYPE>
		class Concatenator<CONTAINER, CHAR_TYPE, typename CONTAINER::StringType::Char>
		{
		public:
			static CONTAINER* concat(const CHAR_TYPE* src1, sl_reg lenSrc1, typename CONTAINER::StringType::Char const* s2, sl_reg len2) noexcept
			{
				if (lenSrc1 < 0) {
					lenSrc1 = StringTraits<CHAR_TYPE>::getLength(src1);
				}
				if (len2 < 0) {
					len2 = StringTraits<typename CONTAINER::StringType::Char>::getLength(s2);
				}
				sl_size len1 = 0;
				if (lenSrc1) {
					len1 = ConvertCharset(src1, lenSrc1, (typename CONTAINER::StringType::Char*)sl_null);
				}
				sl_size len = len1 + len2;
				CONTAINER* container = Alloc<CONTAINER>(len);
				if (container && len) {
					if (len1) {
						ConvertCharset(src1, lenSrc1, container->data);
					}
					MemoryTraits<typename CONTAINER::StringType::Char>::copy(container->data + len1, s2, len2);
				}
				return container;
			}
		};

		template <class CONTAINER, class CHAR1, class CHAR2>
		SLIB_INLINE static CONTAINER* Concat(const CHAR1* s1, sl_reg len1, const CHAR2* s2, sl_reg len2) noexcept
		{
			if (!s1) {
				s1 = EMPTY_SZ(CHAR1);
			}
			if (!s2) {
				s2 = EMPTY_SZ(CHAR2);
			}
			return Concatenator<CONTAINER, CHAR1, CHAR2>::concat(s1, len1, s2, len2);
		}

		template <class STRING>
		static STRING ConcatParams(const StringParam& _s1, const StringParam& _s2) noexcept
		{
			if (_s2.isNull()) {
				return STRING::from(_s1);
			}
			if (_s1.isEmpty()) {
				return STRING::from(_s2);
			}
			if (_s2.isEmpty()) {
				return STRING::from(_s1);
			}
			StringRawData s1, s2;
			_s1.getData(s1);
			_s2.getData(s2);
			if (s1.charSize == 1) {
				if (s2.charSize == 1) {
					return Concat<typename STRING::Container>(s1.data8, s1.length, s2.data8, s2.length);
				} else if (s2.charSize == 2) {
					return Concat<typename STRING::Container>(s1.data8, s1.length, s2.data16, s2.length);
				} else {
					return Concat<typename STRING::Container>(s1.data8, s1.length, s2.data32, s2.length);
				}
			} else if (s1.charSize == 2) {
				if (s2.charSize == 1) {
					return Concat<typename STRING::Container>(s1.data16, s1.length, s2.data8, s2.length);
				} else if (s2.charSize == 2) {
					return Concat<typename STRING::Container>(s1.data16, s1.length, s2.data16, s2.length);
				} else {
					return Concat<typename STRING::Container>(s1.data16, s1.length, s2.data32, s2.length);
				}
			} else {
				if (s2.charSize == 1) {
					return Concat<typename STRING::Container>(s1.data32, s1.length, s2.data8, s2.length);
				} else if (s2.charSize == 2) {
					return Concat<typename STRING::Container>(s1.data32, s1.length, s2.data16, s2.length);
				} else {
					return Concat<typename STRING::Container>(s1.data32, s1.length, s2.data32, s2.length);
				}
			}
		}

		template <class CONTAINER, class CHAR>
		class Appender
		{
		public:
			static CONTAINER* append(CONTAINER* container, const CHAR* src, sl_reg lenSrc)
			{
				if (lenSrc < 0) {
					lenSrc = StringTraits<CHAR>::getLength(src);
				}
				sl_size len2 = 0;
				if (lenSrc) {
					len2 = ConvertCharset(src, lenSrc, (typename CONTAINER::StringType::Char*)sl_null);
				}
				sl_size len1 = container->len;
				sl_size len = len1 + len2;
				container = Realloc<CONTAINER>(container, len);
				if (container && len) {
					if (len2) {
						ConvertCharset(src, lenSrc, container->data + len1);
					}
				}
				return container;
			}
		};

		template <class CONTAINER>
		class Appender<CONTAINER, typename CONTAINER::StringType::Char>
		{
		public:
			static CONTAINER* append(CONTAINER* container, typename CONTAINER::StringType::Char const* s2, sl_reg len2)
			{
				if (len2 < 0) {
					len2 = StringTraits<typename CONTAINER::StringType::Char>::getLength(s2);
				}
				sl_size len1 = container->len;
				sl_size len = len1 + len2;
				container = Realloc<CONTAINER>(container, len);
				if (container && len) {
					MemoryTraits<typename CONTAINER::StringType::Char>::copy(container->data + len1, s2, len2);
				}
				return container;
			}
		};

		template <class CONTAINER, class CHAR>
		static void Append(CONTAINER*& container, const CHAR* src, sl_reg lenSrc) noexcept
		{
			if (!src) {
				src = EMPTY_SZ(CHAR);
			}
			if (container->ref == 1) {
				container = Appender<CONTAINER, CHAR>::append(container, src, lenSrc);
			} else {
				CONTAINER* old = container;
				container = Concat<CONTAINER>(container->data, container->len, src, lenSrc);
				old->decreaseReference();
			}
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool EqualsString(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size len2) noexcept
		{
			if (len1 == len2) {
				if (!len1) {
					return sl_true;
				}
				if (str1 == str2) {
					return sl_true;
				}
				return MemoryTraits<CHAR>::equals(str1, str2, len1);
			} else {
				return sl_false;
			}
		}

		template <class STRING>
		static sl_bool EqualsString(const STRING& str1, const STRING& str2) noexcept
		{
			sl_size len1, len2;
			typename STRING::Char const* data1 = str1.getData(len1);
			typename STRING::Char const* data2 = str2.getData(len2);
			if (len1 == len2) {
				if (!len1) {
					return sl_true;
				}
				if (data1 == data2) {
					return sl_true;
				}
				typename STRING::Container* container1 = *((typename STRING::Container**)((void*)&str1));
				typename STRING::Container* container2 = *((typename STRING::Container**)((void*)&str2));
				sl_size h1 = container1->hash;
				if (h1) {
					sl_size h2 = container2->hash;
					if (h2) {
						if (h1 != h2) {
							return sl_false;
						}
					}
				}
				return MemoryTraits<typename STRING::Char>::equals(data1, data2, len1);
			} else {
				return sl_false;
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_bool EqualsString(const STRING& str1, typename STRING::Char const* str2, sl_size len2) noexcept
		{
			sl_size len1;
			typename STRING::Char const* data1 = str1.getData(len1);
			return EqualsString(data1, len1, str2, len2);
		}

		template <class CHAR>
		static sl_bool EqualsStringSzSub(const CHAR* str1, sl_size len1, const CHAR* str2) noexcept
		{
			for (sl_size i = 0; i < len1; i++) {
				CHAR c = str2[i];
				if (str1[i] != c) {
					return sl_false;
				}
				if (!c) {
					return sl_false;
				}
			}
			return !(str2[len1]);
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool EqualsStringSz(const CHAR* str1, sl_size len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len2 < 0) {
				if (!str2) {
					str2 = EMPTY_SZ(CHAR);
				}
				return EqualsStringSzSub(str1, len1, str2);
			} else {
				return EqualsString(str1, len1, str2, len2);
			}
		}

		template <class STRING>
		static sl_bool EqualsStringSz(const STRING& str1, typename STRING::Char const* str2, sl_reg len2) noexcept
		{
			sl_size len1;
			typename STRING::Char const* data1 = str1.getData(len1);
			return EqualsStringSz(data1, len1, str2, len2);
		}

		template <class CHAR>
		static sl_bool EqualsSz(const CHAR* str1, sl_reg len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len1 < 0) {
				if (!str1) {
					str1 = EMPTY_SZ(CHAR);
				}
				if (len2 < 0) {
					if (!str2) {
						str2 = EMPTY_SZ(CHAR);
					}
					if (str1 == str2) {
						return sl_true;
					}
					return StringTraits<CHAR>::equals(str1, str2);
				} else {
					return EqualsStringSzSub(str2, len2, str1);
				}
			} else {
				return EqualsStringSz(str1, len1, str2, len2);
			}
		}

		template <class CHAR>
		static sl_bool EqualsStringSub_IgnoreCase(const CHAR* str1, const CHAR* str2, sl_size len) noexcept
		{
			for (sl_size i = 0; i < len; i++) {
				CHAR c1 = str1[i];
				CHAR c2 = str2[i];
				if (SLIB_CHAR_LOWER_TO_UPPER(c1) != SLIB_CHAR_LOWER_TO_UPPER(c2)) {
					return sl_false;
				}
			}
			return sl_true;
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool EqualsString_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size len2) noexcept
		{
			if (len1 == len2) {
				if (!len1) {
					return sl_true;
				}
				if (str1 == str2) {
					return sl_true;
				}
				return EqualsStringSub_IgnoreCase(str1, str2, len1);
			} else {
				return sl_false;
			}
		}

		template <class CHAR>
		static sl_bool EqualsStringSzSub_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2) noexcept
		{
			for (sl_size i = 0; i < len1; i++) {
				CHAR c1 = str1[i];
				CHAR c2 = str2[i];
				if (SLIB_CHAR_LOWER_TO_UPPER(c1) != SLIB_CHAR_LOWER_TO_UPPER(c2)) {
					return sl_false;
				}
				if (!c2) {
					return sl_false;
				}
			}
			return !(str2[len1]);
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool EqualsStringSz_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len2 < 0) {
				if (!str2) {
					str2 = EMPTY_SZ(CHAR);
				}
				return EqualsStringSzSub_IgnoreCase(str1, len1, str2);
			} else {
				return EqualsString_IgnoreCase(str1, len1, str2, len2);
			}
		}

		template <class STRING>
		static sl_bool EqualsStringSz_IgnoreCase(const STRING& str1, typename STRING::Char const* str2, sl_reg len2) noexcept
		{
			sl_size len1;
			typename STRING::Char const* data1 = str1.getData(len1);
			return EqualsStringSz_IgnoreCase(data1, len1, str2, len2);
		}

		template <class CHAR>
		static sl_bool EqualsSz_IgnoreCase(const CHAR* str1, sl_reg len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len1 < 0) {
				if (!str1) {
					str1 = EMPTY_SZ(CHAR);
				}
				if (len2 < 0) {
					if (!str2) {
						str2 = EMPTY_SZ(CHAR);
					}
					if (str1 == str2) {
						return sl_true;
					}
					return StringTraits<CHAR>::equals_IgnoreCase(str1, str2);
				} else {
					return EqualsStringSzSub_IgnoreCase(str2, len2, str1);
				}
			} else {
				return EqualsStringSz_IgnoreCase(str1, len1, str2, len2);
			}
		}

		template <class CHAR>
		static sl_compare_result CompareString(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size len2) noexcept
		{
			if (len1 < len2) {
				if (!len1) {
					return -1;
				}
				if (str1 == str2) {
					return -1;
				}
				sl_compare_result r = MemoryTraits<CHAR>::compare(str1, str2, len1);
				if (r) {
					return r;
				} else {
					return -1;
				}
			} else if (len1 > len2) {
				if (!len2) {
					return 1;
				}
				if (str1 == str2) {
					return 1;
				}
				sl_compare_result r = MemoryTraits<CHAR>::compare(str1, str2, len2);
				if (r) {
					return r;
				} else {
					return 1;
				}
			} else {
				if (!len1) {
					return 0;
				}
				if (str1 == str2) {
					return 0;
				}
				return MemoryTraits<CHAR>::compare(str1, str2, len1);
			}
		}

		template <class CHAR>
		static sl_compare_result CompareStringSzSub(const CHAR* str1, sl_size len1, const CHAR* str2) noexcept
		{
			for (sl_size i = 0; i < len1; i++) {
				typename UnsignedType<CHAR>::Type c1 = str1[i];
				typename UnsignedType<CHAR>::Type c2 = str2[i];
				if (!c2) {
					return 1;
				}
				if (c1 < c2) {
					return -1;
				} else if (c1 > c2) {
					return 1;
				}
			}
			if (str2[len1]) {
				return -1;
			}
			return 0;
		}

		template <class CHAR>
		SLIB_INLINE static sl_compare_result CompareStringSz(const CHAR* str1, sl_size len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len2 < 0) {
				if (!str2) {
					str2 = EMPTY_SZ(CHAR);
				}
				return CompareStringSzSub(str1, len1, str2);
			} else {
				return CompareString(str1, len1, str2, len2);
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_compare_result CompareStringSz(const STRING& str1, typename STRING::Char const* str2, sl_reg len2) noexcept
		{
			sl_size len1;
			typename STRING::Char const* data1 = str1.getData(len1);
			return CompareStringSz(data1, len1, str2, len2);
		}

		template <class CHAR>
		static sl_compare_result CompareSz(const CHAR* str1, sl_reg len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len1 < 0) {
				if (!str1) {
					str1 = EMPTY_SZ(CHAR);
				}
				if (len2 < 0) {
					if (!str2) {
						str2 = EMPTY_SZ(CHAR);
					}
					if (str1 == str2) {
						return 0;
					}
					return StringTraits<CHAR>::compare(str1, str2);
				} else {
					return -(CompareStringSzSub(str2, len2, str1));
				}
			} else {
				return CompareStringSz(str1, len1, str2, len2);
			}
		}

		template <class CHAR>
		static sl_compare_result CompareString_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size len2) noexcept
		{
			sl_size len = SLIB_MIN(len1, len2);
			if (str1 != str2) {
				for (sl_size i = 0; i < len; i++) {
					typename UnsignedType<CHAR>::Type c1 = str1[i];
					typename UnsignedType<CHAR>::Type c2 = str2[i];
					c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
					c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
					if (c1 < c2) {
						return -1;
					} else if (c1 > c2) {
						return 1;
					}
				}
			}
			if (len1 < len2) {
				return -1;
			} else if (len1 > len2) {
				return 1;
			}
			return 0;
		}

		template <class CHAR>
		static sl_compare_result CompareStringSzSub_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2) noexcept
		{
			for (sl_size i = 0; i < len1; i++) {
				typename UnsignedType<CHAR>::Type c1 = str1[i];
				typename UnsignedType<CHAR>::Type c2 = str2[i];
				if (!c2) {
					return 1;
				}
				c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
				c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
				if (c1 < c2) {
					return -1;
				} else if (c1 > c2) {
					return 1;
				}
			}
			if (str2[len1]) {
				return -1;
			}
			return 0;
		}

		template <class CHAR>
		SLIB_INLINE static sl_compare_result CompareStringSz_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len2 < 0) {
				if (!str2) {
					str2 = EMPTY_SZ(CHAR);
				}
				return CompareStringSzSub_IgnoreCase(str1, len1, str2);
			} else {
				return CompareString_IgnoreCase(str1, len1, str2, len2);
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_compare_result CompareStringSz_IgnoreCase(const STRING& str1, typename STRING::Char const* str2, sl_reg len2) noexcept
		{
			sl_size len1;
			typename STRING::Char const* data1 = str1.getData(len1);
			return CompareStringSz_IgnoreCase(data1, len1, str2, len2);
		}

		template <class CHAR>
		static sl_compare_result CompareSz_IgnoreCase(const CHAR* str1, sl_reg len1, const CHAR* str2, sl_reg len2) noexcept
		{
			if (len1 < 0) {
				if (!str1) {
					str1 = EMPTY_SZ(CHAR);
				}
				if (len2 < 0) {
					if (!str2) {
						str2 = EMPTY_SZ(CHAR);
					}
					if (str1 == str2) {
						return 0;
					}
					return StringTraits<CHAR>::compare_IgnoreCase(str1, str2);
				} else {
					return -(CompareStringSzSub_IgnoreCase(str2, len2, str1));
				}
			} else {
				return CompareStringSz_IgnoreCase(str1, len1, str2, len2);
			}
		}

		template <class CHAR>
		SLIB_INLINE static sl_compare_result CompareStringLimited(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size len2, sl_size nLimit) noexcept
		{
			if (len1 > nLimit) {
				len1 = nLimit;
			}
			if (len2 > nLimit) {
				len2 = nLimit;
			}
			return CompareString(str1, len1, str2, len2);
		}

		template <class CHAR>
		static sl_compare_result CompareStringSzLimitedSub(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size nLimit) noexcept
		{
			if (len1 < nLimit) {
				return CompareStringSzSub(str1, len1, str2);
			}
			for (sl_size i = 0; i < nLimit; i++) {
				typename UnsignedType<CHAR>::Type c1 = str1[i];
				typename UnsignedType<CHAR>::Type c2 = str2[i];
				if (!c2) {
					return 1;
				}
				if (c1 < c2) {
					return -1;
				} else if (c1 > c2) {
					return 1;
				}
			}
			return 0;
		}

		template <class CHAR>
		SLIB_INLINE static sl_compare_result CompareStringSzLimited(const CHAR* str1, sl_size len1, const CHAR* str2, sl_reg len2, sl_size nLimit) noexcept
		{
			if (len2 < 0) {
				if (!str2) {
					str2 = EMPTY_SZ(CHAR);
				}
				return CompareStringSzLimitedSub(str1, len1, str2, nLimit);
			} else {
				return CompareStringLimited(str1, len1, str2, len2, nLimit);
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_compare_result CompareStringSzLimited(const STRING& str1, typename STRING::Char const* str2, sl_reg len2, sl_size nLimit) noexcept
		{
			sl_size len1;
			typename STRING::Char const* data1 = str1.getData(len1);
			return CompareStringSzLimited(data1, len1, str2, len2, nLimit);
		}

		template <class CHAR>
		static sl_compare_result CompareSzLimited(const CHAR* str1, sl_reg len1, const CHAR* str2, sl_reg len2, sl_size nLimit) noexcept
		{
			if (len1 < 0) {
				if (!str1) {
					str1 = EMPTY_SZ(CHAR);
				}
				if (len2 < 0) {
					if (!str2) {
						str2 = EMPTY_SZ(CHAR);
					}
					if (str1 == str2) {
						return 0;
					}
					return StringTraits<CHAR>::compare(str1, str2, nLimit);
				} else {
					return -(CompareStringSzLimitedSub(str2, len2, str1, nLimit));
				}
			} else {
				return CompareStringSzLimited(str1, len1, str2, len2, nLimit);
			}
		}

		template <class CHAR>
		SLIB_INLINE static sl_compare_result CompareStringLimited_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size len2, sl_size nLimit) noexcept
		{
			if (len1 > nLimit) {
				len1 = nLimit;
			}
			if (len2 > nLimit) {
				len2 = nLimit;
			}
			return CompareString_IgnoreCase(str1, len1, str2, len2);
		}

		template <class CHAR>
		static sl_compare_result CompareStringSzLimitedSub_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2, sl_size nLimit) noexcept
		{
			if (len1 < nLimit) {
				return CompareStringSzSub_IgnoreCase(str1, len1, str2);
			}
			for (sl_size i = 0; i < nLimit; i++) {
				typename UnsignedType<CHAR>::Type c1 = str1[i];
				typename UnsignedType<CHAR>::Type c2 = str2[i];
				if (!c2) {
					return 1;
				}
				c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
				c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
				if (c1 < c2) {
					return -1;
				} else if (c1 > c2) {
					return 1;
				}
			}
			return 0;
		}

		template <class CHAR>
		SLIB_INLINE static sl_compare_result CompareStringSzLimited_IgnoreCase(const CHAR* str1, sl_size len1, const CHAR* str2, sl_reg len2, sl_size nLimit) noexcept
		{
			if (len2 < 0) {
				if (!str2) {
					str2 = EMPTY_SZ(CHAR);
				}
				return CompareStringSzLimitedSub_IgnoreCase(str1, len1, str2, nLimit);
			} else {
				return CompareStringLimited_IgnoreCase(str1, len1, str2, len2, nLimit);
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_compare_result CompareStringSzLimited_IgnoreCase(const STRING& str1, typename STRING::Char const* str2, sl_reg len2, sl_size nLimit) noexcept
		{
			sl_size len1;
			typename STRING::Char const* data1 = str1.getData(len1);
			return CompareStringSzLimited_IgnoreCase(data1, len1, str2, len2, nLimit);
		}

		template <class CHAR>
		static sl_compare_result CompareSzLimited_IgnoreCase(const CHAR* str1, sl_reg len1, const CHAR* str2, sl_reg len2, sl_size nLimit) noexcept
		{
			if (len1 < 0) {
				if (!str1) {
					str1 = EMPTY_SZ(CHAR);
				}
				if (len2 < 0) {
					if (!str2) {
						str2 = EMPTY_SZ(CHAR);
					}
					if (str1 == str2) {
						return 0;
					}
					return StringTraits<CHAR>::compare_IgnoreCase(str1, str2, nLimit);
				} else {
					return -(CompareStringSzLimitedSub_IgnoreCase(str2, len2, str1, nLimit));
				}
			} else {
				return CompareStringSzLimited_IgnoreCase(str1, len1, str2, len2, nLimit);
			}
		}

		template <class CHAR>
		static sl_size GetHashCode(const CHAR* buf, sl_size len) noexcept
		{
			sl_size hash = 0;
			for (sl_size i = 0; i < len; i++) {
				sl_uint32 ch = buf[i];
				if (ch) {
					hash = hash * 31 + ch;
				} else {
					break;
				}
			}
			if (hash) {
				hash = Rehash(hash);
			}
			return hash;
		}

		template <class CHAR>
		static sl_size GetHashCode_IgnoreCase(const CHAR* buf, sl_size len) noexcept
		{
			sl_size hash = 0;
			for (sl_size i = 0; i < len; i++) {
				sl_uint32 ch = buf[i];
				if (ch) {
					ch = SLIB_CHAR_LOWER_TO_UPPER(ch);
					hash = hash * 31 + ch;
				} else {
					break;
				}
			}
			if (hash) {
				hash = Rehash(hash);
			}
			return hash;
		}

		template <class CHAR>
		static sl_reg IndexOfChar(const CHAR* str, sl_size len, CHAR ch, sl_reg _start) noexcept
		{
			if (!len) {
				return -1;
			}
			sl_size start;
			if (_start < 0) {
				start = 0;
			} else {
				start = _start;
				if (start >= len) {
					return -1;
				}
			}
			CHAR* pt = MemoryTraits<CHAR>::find(str + start, len - start, ch);
			if (pt) {
				return (sl_reg)(pt - str);
			} else {
				return -1;
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_reg IndexOfChar(const STRING& str, typename STRING::Char ch, sl_reg start) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return IndexOfChar(data, len, ch, start);
		}

		template <class CHAR>
		static sl_reg IndexOfCharSz(const CHAR* str, sl_reg len, CHAR chWhat, sl_reg _start) noexcept
		{
			if (!str) {
				return -1;
			}
			if (len >= 0) {
				return IndexOfChar(str, len, chWhat, _start);
			}
			sl_size start;
			if (_start < 0) {
				start = 0;
			} else {
				start = _start;
			}
			sl_size i = 0;
			for (; i < start; i++) {
				if (!(str[i])) {
					return -1;
				}
			}
			for (;;) {
				CHAR ch = str[i];
				if (ch == chWhat) {
					return i;
				}
				if (!ch) {
					break;
				}
				i++;
			}
			return -1;
		}

		template <class CHAR>
		static sl_reg IndexOf(const CHAR* str, sl_size count, const CHAR* pattern, sl_size nPattern, sl_reg _start) noexcept
		{
			if (count < nPattern) {
				return -1;
			}
			if (!nPattern) {
				return 0;
			}
			sl_size start;
			if (_start < 0) {
				start = 0;
			} else {
				start = _start;
				if (start > count - nPattern) {
					return -1;
				}
			}
			CHAR* pt = MemoryTraits<CHAR>::find(str + start, count - start, pattern, nPattern);
			if (pt) {
				return pt - str;
			}
			return -1;
		}

		template <class STRING>
		SLIB_INLINE static sl_reg IndexOf(const STRING& str, typename STRING::Char const* pattern, sl_size nPattern, sl_reg start) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return IndexOf(data, len, pattern, nPattern, start);
		}

		template <class CHAR>
		static sl_reg IndexOfChar_IgnoreCase(const CHAR* str, sl_size count, CHAR pattern)
		{
			pattern = SLIB_CHAR_LOWER_TO_UPPER(pattern);
			for (sl_size i = 0; i < count; i++) {
				CHAR ch = str[i];
				ch = SLIB_CHAR_LOWER_TO_UPPER(ch);
				if (ch == pattern) {
					return i;
				}
			}
			return -1;
		}

		template <class CHAR>
		static sl_reg IndexOf_IgnoreCase(const CHAR* str, sl_size count, const CHAR* pattern, sl_size nPattern, sl_reg _start) noexcept
		{
			if (count < nPattern) {
				return -1;
			}
			if (!nPattern) {
				return 0;
			}
			sl_size start;
			if (_start < 0) {
				start = 0;
			} else {
				start = _start;
				if (start > count - nPattern) {
					return -1;
				}
			}
			count -= start;
			str += start;
			CHAR pattern0 = *pattern;
			if (nPattern == 1) {
				sl_reg index = IndexOfChar_IgnoreCase(str, count, pattern0);
				if (index >= 0) {
					return index + start;
				} else {
					return -1;
				}
			}
			if (nPattern > count) {
				return -1;
			}
			nPattern--;
			const CHAR* pattern1 = pattern + 1;
			sl_size n = count - nPattern;
			sl_size pos = 0;
			do {
				sl_reg index = IndexOfChar_IgnoreCase(str + pos, n - pos, pattern0);
				if (index < 0) {
					return -1;
				}
				pos += index;
				if (EqualsStringSub_IgnoreCase(str + pos + 1, pattern1, nPattern)) {
					return start + pos;
				} else {
					pos++;
				}
			} while (pos < n);
			return -1;
		}

		template <class STRING>
		SLIB_INLINE static sl_reg IndexOf_IgnoreCase(const STRING& str, typename STRING::Char const* pattern, sl_size nPattern, sl_reg start) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return IndexOf_IgnoreCase(data, len, pattern, nPattern, start);
		}

		template <class CHAR>
		static sl_reg LastIndexOfChar(const CHAR* str, sl_size len, CHAR ch, sl_reg start) noexcept
		{
			if (!len) {
				return -1;
			}
			if (start >= 0 && (sl_size)start < len - 1) {
				len = start + 1;
			}
			CHAR* pt = MemoryTraits<CHAR>::findBackward(str, len, ch);
			if (pt == sl_null) {
				return -1;
			} else {
				return (sl_reg)(pt - str);
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_reg LastIndexOfChar(const STRING& str, typename STRING::Char ch, sl_reg start) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return LastIndexOfChar(data, len, ch, start);
		}

		template <class CHAR>
		static sl_reg LastIndexOf(const CHAR* str, sl_size count, const CHAR* pattern, sl_size nPattern, sl_reg start) noexcept
		{
			if (count < nPattern) {
				return -1;
			}
			if (!nPattern) {
				return count;
			}
			if (start >= 0 && (sl_size)start < count - nPattern) {
				count = start + nPattern;
			}
			CHAR* pt = MemoryTraits<CHAR>::findBackward(str, count, pattern, nPattern);
			if (pt) {
				return pt - str;
			}
			return -1;
		}

		template <class STRING>
		SLIB_INLINE static sl_reg LastIndexOf(const STRING& str, typename STRING::Char const* pattern, sl_size nPattern, sl_reg start) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return LastIndexOf(data, len, pattern, nPattern, start);
		}

		template <class CHAR>
		static sl_reg LastIndexOfChar_IgnoreCase(const CHAR* str, sl_size count, CHAR pattern) noexcept
		{
			pattern = SLIB_CHAR_LOWER_TO_UPPER(pattern);
			while (count) {
				count--;
				CHAR ch = str[count];
				ch = SLIB_CHAR_LOWER_TO_UPPER(ch);
				if (ch == pattern) {
					return count;
				}
			}
			return -1;
		}

		template <class CHAR>
		static sl_reg LastIndexOf_IgnoreCase(const CHAR* str, sl_size count, const CHAR* pattern, sl_size nPattern, sl_reg start) noexcept
		{
			if (count < nPattern) {
				return -1;
			}
			if (!nPattern) {
				return count;
			}
			if (start >= 0 && (sl_size)start < count - nPattern) {
				count = start + nPattern;
			}
			CHAR pattern0 = *pattern;
			if (nPattern == 1) {
				return LastIndexOfChar_IgnoreCase(str, count, pattern0);
			}
			if (nPattern > count) {
				return -1;
			}
			nPattern--;
			const CHAR* pattern1 = pattern + 1;
			sl_size n = count - nPattern;
			do {
				sl_reg index = LastIndexOfChar_IgnoreCase(str, n, pattern0);
				if (index < 0) {
					return -1;
				}
				if (EqualsStringSub_IgnoreCase(str + index + 1, pattern1, nPattern)) {
					return index;
				} else {
					n = index;
				}
			} while (n);
			return -1;
		}

		template <class STRING>
		SLIB_INLINE static sl_reg LastIndexOf_IgnoreCase(const STRING& str, typename STRING::Char const* pattern, sl_size nPattern, sl_reg start) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return LastIndexOf_IgnoreCase(data, len, pattern, nPattern, start);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool StartsWithChar(const STRING& str, typename STRING::Char ch) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			if (len) {
				return *data == ch;
			}
			return sl_false;
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool StartsWithCharSz(const CHAR* str, sl_reg len, CHAR ch) noexcept
		{
			if (str && len) {
				return *str == ch;
			}
			return sl_false;
		}

		template <class CHAR>
		static sl_bool StartsWithStringSub(const CHAR* str, sl_size count, const CHAR* pattern, sl_reg nPattern) noexcept
		{
			if (nPattern > 0) {
				if (count < (sl_size)nPattern) {
					return sl_false;
				} else {
					return MemoryTraits<CHAR>::equals(str, pattern, nPattern);
				}
			} else {
				for (sl_size i = 0; i < count; i++) {
					CHAR ch = pattern[i];
					if (!ch) {
						return sl_true;
					}
					if (str[i] != ch) {
						return sl_false;
					}
				}
				return !(pattern[count]);
			}
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool StartsWithString(const CHAR* str, sl_size count, const CHAR* pattern, sl_reg nPattern) noexcept
		{
			if (!(pattern && nPattern)) {
				return sl_true;
			}
			return StartsWithStringSub(str, count, pattern, nPattern);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool StartsWithString(const STRING& str, typename STRING::Char const* pattern, sl_reg nPattern) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return StartsWithString(data, len, pattern, nPattern);
		}

		template <class CHAR>
		static sl_bool StartsWithSz(const CHAR* str, sl_reg count, const CHAR* pattern, sl_reg nPattern) noexcept
		{
			if (!(pattern && nPattern)) {
				return sl_true;
			}
			if (count >= 0) {
				return StartsWithStringSub(str, count, pattern, nPattern);
			}
			if (nPattern > 0) {
				for (sl_reg i = 0; i < nPattern; i++) {
					CHAR ch = str[i];
					if (!ch) {
						return sl_false;
					}
					if (ch != pattern[i]) {
						return sl_false;
					}
				}
				return sl_true;
			} else {
				for (;;) {
					CHAR ch = *pattern;
					if (!ch) {
						return sl_true;
					}
					if (*str != ch) {
						return sl_false;
					}
					str++;
					pattern++;
				}
				return sl_true;
			}
		}
		
		template <class CHAR>
		static sl_bool StartsWithStringSub_IgnoreCase(const CHAR* str, sl_size count, const CHAR* pattern, sl_reg nPattern) noexcept
		{
			if (nPattern > 0) {
				if (count < (sl_size)nPattern) {
					return sl_false;
				} else {
					return EqualsStringSub_IgnoreCase(str, pattern, nPattern);
				}
			} else {
				for (sl_size i = 0; i < count; i++) {
					CHAR c1 = pattern[i];
					if (!c1) {
						return sl_true;
					}
					c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
					CHAR c2 = str[i];
					c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
					if (c1 != c2) {
						return sl_false;
					}
				}
				return !(pattern[count]);
			}
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool StartsWithString_IgnoreCase(const CHAR* str, sl_size count, const CHAR* pattern, sl_reg nPattern) noexcept
		{
			if (!(pattern && nPattern)) {
				return sl_true;
			}
			return StartsWithStringSub_IgnoreCase(str, count, pattern, nPattern);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool StartsWithString_IgnoreCase(const STRING& str, typename STRING::Char const* pattern, sl_reg nPattern) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return StartsWithString_IgnoreCase(data, len, pattern, nPattern);
		}

		template <class CHAR>
		static sl_bool StartsWithSz_IgnoreCase(const CHAR* str, sl_reg count, const CHAR* pattern, sl_reg nPattern) noexcept
		{
			if (!(pattern && nPattern)) {
				return sl_true;
			}
			if (count >= 0) {
				return StartsWithStringSub_IgnoreCase(str, count, pattern, nPattern);
			}
			if (nPattern > 0) {
				for (sl_reg i = 0; i < nPattern; i++) {
					CHAR c1 = str[i];
					if (!c1) {
						return sl_false;
					}
					c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
					CHAR c2 = pattern[i];
					c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
					if (c1 != c2) {
						return sl_false;
					}
				}
				return sl_true;
			} else {
				for (;;) {
					CHAR c1 = *pattern;
					if (!c1) {
						return sl_true;
					}
					c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
					CHAR c2 = *str;
					c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
					if (c1 != c2) {
						return sl_false;
					}
					str++;
					pattern++;
				}
				return sl_true;
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_bool EndsWithChar(const STRING& str, typename STRING::Char ch) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			if (len) {
				return data[len - 1] == ch;
			}
			return sl_false;
		}

		template <class CHAR>
		static sl_bool EndsWithCharSz(const CHAR* str, sl_reg len, CHAR chWhat) noexcept
		{
			if (str && len) {
				if (len > 0) {
					return str[len - 1] == chWhat;
				} else {
					CHAR old = *str;
					if (old) {
						for (;;) {
							CHAR ch = *(++str);
							if (!ch) {
								return old == chWhat;
							}
							old = ch;
						}
					}
				}
			}
			return sl_false;
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool EndsWith(const CHAR* str, sl_size count, const CHAR* pattern, sl_size nPattern) noexcept
		{
			if (!nPattern) {
				return sl_true;
			}
			if (count < nPattern) {
				return sl_false;
			} else {
				return MemoryTraits<CHAR>::equals(str + count - nPattern, pattern, nPattern);
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_bool EndsWith(const STRING& str, typename STRING::Char const* pattern, sl_size nPattern) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return EndsWith(data, len, pattern, nPattern);
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool EndsWith_IgnoreCase(const CHAR* str, sl_size count, const CHAR* pattern, sl_size nPattern) noexcept
		{
			if (!nPattern) {
				return sl_true;
			}
			if (count < nPattern) {
				return sl_false;
			} else {
				return EqualsStringSub_IgnoreCase(str + count - nPattern, pattern, nPattern);
			}
		}

		template <class STRING>
		SLIB_INLINE static sl_bool EndsWith_IgnoreCase(const STRING& str, typename STRING::Char const* pattern, sl_size nPattern) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return EndsWith_IgnoreCase(data, len, pattern, nPattern);
		}

		template <class CHAR>
		static sl_size CountOfChar(const CHAR* str, sl_size len, CHAR ch) noexcept
		{
			sl_size count = 0;
			for (sl_size i = 0; i < len; i++) {
				if (str[i] == ch) {
					count++;
				}
			}
			return count;
		}

		template <class STRING>
		SLIB_INLINE static sl_size CountOfChar(const STRING& str, typename STRING::Char ch) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return CountOfChar(data, len, ch);
		}

		template <class CHAR>
		static sl_size CountOfCharSz(const CHAR* str, sl_reg len, CHAR chWhat) noexcept
		{
			if (str && len) {
				if (len > 0) {
					return CountOfChar(str, len, chWhat);
				} else {
					sl_size count = 0;
					for (;;) {
						CHAR ch = *(str++);
						if (ch == chWhat) {
							count++;
						}
						if (!ch) {
							break;
						}
					}
					return count;
				}
			}
			return 0;
		}

		template <class CHAR>
		static sl_size CountOf(const CHAR* str, sl_size len, const CHAR* pattern, sl_size lenPattern) noexcept
		{
			if (!lenPattern) {
				return 0;
			}
			sl_size count = 0;
			sl_reg start = 0;
			for (;;) {
				start = IndexOf(str, len, pattern, lenPattern, start);
				if (start >= 0) {
					count++;
					start += lenPattern;
				} else {
					break;
				}
			}
			return count;
		}

		template <class STRING>
		SLIB_INLINE static sl_size CountOf(const STRING& str, typename STRING::Char const* pattern, sl_size lenPattern) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return CountOf(data, len, pattern, lenPattern);
		}

		template <class CHAR>
		static sl_size CountOf_IgnoreCase(const CHAR* str, sl_size len, const CHAR* pattern, sl_size lenPattern) noexcept
		{
			if (!lenPattern) {
				return 0;
			}
			sl_size count = 0;
			sl_reg start = 0;
			for (;;) {
				start = IndexOf_IgnoreCase(str, len, pattern, lenPattern, start);
				if (start >= 0) {
					count++;
					start += lenPattern;
				} else {
					break;
				}
			}
			return count;
		}

		template <class STRING>
		SLIB_INLINE static sl_size CountOf_IgnoreCase(const STRING& str, typename STRING::Char const* pattern, sl_size lenPattern) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return CountOf_IgnoreCase(data, len, pattern, lenPattern);
		}

		template <class CHAR>
		SLIB_INLINE static void ToUpper(CHAR* dst, const CHAR* src, sl_size len) noexcept
		{
			for (sl_size i = 0; i < len; i++) {
				CHAR ch = src[i];
				dst[i] = SLIB_CHAR_LOWER_TO_UPPER(ch);
			}
		}

		template <class CHAR>
		SLIB_INLINE static void ToLower(CHAR* dst, const CHAR* src, sl_size len) noexcept
		{
			for (sl_size i = 0; i < len; i++) {
				CHAR ch = src[i];
				dst[i] = SLIB_CHAR_UPPER_TO_LOWER(ch);
			}
		}

		template <class STRING>
		SLIB_INLINE static void MakeUpperString(STRING& str) noexcept
		{
			sl_size len;
			typename STRING::Char* data = str.getData(len);
			ToUpper(data, data, len);
		}

		template <class CHAR>
		static void MakeUpperSz(CHAR* str, sl_reg len) noexcept
		{
			if (!str) {
				return;
			}
			if (len >= 0) {
				ToUpper(str, str, len);
			} else {
				for (;;) {
					CHAR ch = *str;
					if (ch) {
						*str = SLIB_CHAR_LOWER_TO_UPPER(ch);
						str++;
					} else {
						break;
					}
				}
			}
		}

		template <class STRING>
		SLIB_INLINE static void MakeLowerString(STRING& str) noexcept
		{
			sl_size len;
			typename STRING::Char* data = str.getData(len);
			ToLower(data, data, len);
		}

		template <class CHAR>
		static void MakeLowerSz(CHAR* str, sl_reg len) noexcept
		{
			if (!str) {
				return;
			}
			if (len >= 0) {
				ToLower(str, str, len);
			} else {
				for (;;) {
					CHAR ch = *str;
					if (ch) {
						*str = SLIB_CHAR_UPPER_TO_LOWER(ch);
						str++;
					} else {
						break;
					}
				}
			}
		}

		template <class STRING>
		static STRING CreateUpperString(typename STRING::Char const* str, sl_reg _len) noexcept
		{
			if (!str) {
				return sl_null;
			}
			if (!_len) {
				return STRING::getEmpty();
			}
			sl_size len;
			if (_len < 0) {
				len = StringTraits<typename STRING::Char>::getLength(str);
			} else {
				len = _len;
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNull()) {
				return ret;
			}
			ToUpper(ret.getData(), str, len);
			return ret;
		}

		template <class STRING>
		static STRING CreateUpperString(const STRING& str) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			if (!len) {
				return STRING::getEmpty();
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNull()) {
				return ret;
			}
			ToUpper(ret.getData(), data, len);
			return ret;
		}

		template <class STRING>
		static STRING CreateLowerString(typename STRING::Char const* str, sl_reg _len) noexcept
		{
			if (!str) {
				return sl_null;
			}
			if (!_len) {
				return STRING::getEmpty();
			}
			sl_size len;
			if (_len < 0) {
				len = StringTraits<typename STRING::Char>::getLength(str);
			} else {
				len = _len;
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNull()) {
				return ret;
			}
			ToLower(ret.getData(), str, len);
			return ret;
		}

		template <class STRING>
		static STRING CreateLowerString(const STRING& str) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			if (!len) {
				return STRING::getEmpty();
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNull()) {
				return ret;
			}
			ToLower(ret.getData(), data, len);
			return ret;
		}

		template <class STRING>
		SLIB_INLINE static STRING ReplaceChar(const STRING& str, typename STRING::Char pattern, typename STRING::Char replace) noexcept
		{
			if (str.isNull()) {
				return str;
			}
			sl_size len;
			typename STRING::Char const* dataSrc = str.getData(len);
			sl_size posStart = 0;
			{
				sl_size i = 0;
				for (; i < len; i++) {
					typename STRING::Char ch = dataSrc[i];
					if (ch == pattern) {
						posStart = i;
						break;
					}
				}
				if (i == len) {
					return str;
				}
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNull()) {
				return sl_null;
			}
			typename STRING::Char* dataDst = ret.getData();
			{
				for (sl_size i = 0; i < posStart; i++) {
					dataDst[i] = dataSrc[i];
				}
			}
			if (replace) {
				dataDst[posStart] = replace;
				posStart++;
				for (sl_size i = posStart; i < len; i++) {
					typename STRING::Char ch = dataSrc[i];
					if (ch == pattern) {
						dataDst[i] = replace;
					} else {
						dataDst[i] = ch;
					}
				}
			} else {
				sl_size k = posStart;
				posStart++;
				for (sl_size i = posStart; i < len; i++) {
					typename STRING::Char ch = dataSrc[i];
					if (ch != pattern) {
						dataDst[k] = ch;
						k++;
					}
				}
				dataDst[k] = 0;
				ret.setLength(k);
			}
			return ret;
		}

		template <class STRING>
		static STRING ReplaceSzCharSub(typename STRING::Char const* str, sl_size count, typename STRING::Char pattern, typename STRING::Char replace) noexcept
		{
			if (!count) {
				return STRING::getEmpty();
			}
			STRING ret = STRING::allocate(count);
			if (ret.isNull()) {
				return sl_null;
			}
			typename STRING::Char* data = ret.getData();
			if (replace) {
				for (sl_size i = 0; i < count; i++) {
					typename STRING::Char ch = str[i];
					if (ch == pattern) {
						data[i] = replace;
					} else {
						data[i] = ch;
					}
				}
			} else {
				sl_reg k = 0;
				for (sl_size i = 0; i < count; i++) {
					typename STRING::Char ch = str[i];
					if (ch != pattern) {
						data[k] = ch;
						k++;
					}
				}
				if (k != count) {
					data[k] = 0;
				}
				ret.setLength(k);
			}
			return ret;
		}

		template <class STRING>
		SLIB_INLINE static STRING ReplaceSzChar(typename STRING::Char const* str, sl_reg len, typename STRING::Char pattern, typename STRING::Char replace) noexcept
		{
			if (!str) {
				return sl_null;
			}
			if (len < 0) {
				len = StringTraits<typename STRING::Char>::getLength(str);
			}
			return ReplaceSzCharSub<STRING>(str, len, pattern, replace);
		}

		struct STRING_REPLACE_SUBSET
		{
			sl_reg start;
			sl_reg len;
		};

		template <class STRING>
		static STRING ReplaceAllSub(typename STRING::Char const* src, sl_size countSrc, typename STRING::Char const* pattern, sl_size nPattern, typename STRING::Char const* strReplace, sl_size countReplace) noexcept
		{
			if (!nPattern) {
				return sl_null;
			}
			if (!countSrc) {
				return STRING::getEmpty();
			}
			LinkedQueue<STRING_REPLACE_SUBSET> queue;
			STRING_REPLACE_SUBSET subset;
			sl_size countNew = 0;
			sl_size start = 0;
			while (start <= countSrc + nPattern - 1) {
				sl_reg index = IndexOf(src, countSrc, pattern, nPattern, start);
				if (index < 0) {
					index = countSrc;
				} else {
					countNew += countReplace;
				}
				subset.start = start;
				subset.len = index - start;
				queue.push_NoLock(subset);
				countNew += subset.len;
				start = index + nPattern;
			}
			if (!countNew) {
				return STRING::getEmpty();
			}
			STRING ret = STRING::allocate(countNew);
			if (ret.isNotNull()) {
				typename STRING::Char* out = ret.getData();
				while (queue.pop_NoLock(&subset)) {
					MemoryTraits<typename STRING::Char>::copy(out, src + subset.start, subset.len);
					out += subset.len;
					if (queue.isNotEmpty()) {
						if (countReplace) {
							MemoryTraits<typename STRING::Char>::copy(out, strReplace, countReplace);
							out += countReplace;
						}
					}
				}
			}
			return ret;
		}

		template <class STRING>
		SLIB_INLINE static typename StringTypeFromCharType<typename STRING::Char>::Type ReplaceAll(const STRING& str, typename STRING::Char const* pattern, sl_size nPattern, typename STRING::Char const* strReplace, sl_size countReplace) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			return ReplaceAllSub<typename StringTypeFromCharType<typename STRING::Char>::Type>(data, len, pattern, nPattern, strReplace, countReplace);
		}

		template <class STRING>
		static STRING Trim(const STRING& str) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char* data = str.getData(len);
			sl_size i = 0;
			for (; i < len; i++) {
				typename STRING::Char c = data[i];
				if (!(SLIB_CHAR_IS_WHITE_SPACE(c))) {
					break;
				}
			}
			if (i >= len) {
				return STRING::getEmpty();
			}
			sl_size j = len - 1;
			for (; j > i; j--) {
				typename STRING::Char c = data[j];
				if (!(SLIB_CHAR_IS_WHITE_SPACE(c))) {
					break;
				}
			}
			return MidPriv(&str, data, i, j + 1 - i);
		}

		template <class STRING>
		static STRING TrimLeft(const STRING& str) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char* data = str.getData(len);
			sl_size i = 0;
			for (; i < len; i++) {
				typename STRING::Char c = data[i];
				if (!(SLIB_CHAR_IS_WHITE_SPACE(c))) {
					break;
				}
			}
			if (i >= len) {
				return STRING::getEmpty();
			}
			return MidPriv(&str, data, i, len - i);
		}

		template <class STRING>
		static STRING TrimRight(const STRING& str) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char* data = str.getData(len);
			sl_size j = len;
			for (; j > 0; j--) {
				typename STRING::Char c = data[j - 1];
				if (!(SLIB_CHAR_IS_WHITE_SPACE(c))) {
					break;
				}
			}
			if (!j) {
				return STRING::getEmpty();
			}
			return MidPriv(&str, data, 0, j);
		}

		template <class STRING>
		static STRING TrimLine(const STRING& str) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char* data = str.getData(len);
			sl_size i = 0;
			for (; i < len; i++) {
				typename STRING::Char c = data[i];
				if (c != '\r' && c != '\n') {
					break;
				}
			}
			if (i >= len) {
				return STRING::getEmpty();
			}
			sl_size j = len - 1;
			for (; j > i; j--) {
				typename STRING::Char c = data[j];
				if (c != '\r' && c != '\n') {
					break;
				}
			}
			return MidPriv(&str, data, i, j + 1 - i);
		}

		template <class STRING>
		SLIB_INLINE static void MakeReverse(STRING& str) noexcept
		{
			sl_size len;
			typename STRING::Char* data = str.getData(len);
			typename STRING::Char* end = data + (len - 1);
			while (data < end) {
				Swap(*data, *end);
				data++;
				end--;
			}
		}

		template <class CHAR>
		SLIB_INLINE static void DoReverse(CHAR* dst, const CHAR* src, sl_size len) noexcept
		{
			CHAR* end = dst + len;
			src = src + (len - 1);
			while (dst < end) {
				*dst = *src;
				dst++;
				src--;
			}
		}

		template <class STRING>
		static STRING CreateReverseString(typename STRING::Char const* str, sl_reg _len) noexcept
		{
			if (!str) {
				return sl_null;
			}
			if (!_len) {
				return STRING::getEmpty();
			}
			sl_size len;
			if (_len < 0) {
				len = StringTraits<typename STRING::Char>::getLength(str);
			} else {
				len = _len;
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNull()) {
				return ret;
			}
			DoReverse(ret.getData(), str, len);
			return ret;
		}

		template <class STRING>
		static STRING CreateReverseString(const STRING& str) noexcept
		{
			if (str.isNull()) {
				return sl_null;
			}
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			if (!len) {
				return STRING::getEmpty();
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNull()) {
				return ret;
			}
			DoReverse(ret.getData(), data, len);
			return ret;
		}

		template <class STRING>
		static List<STRING> Split(const STRING& str, typename STRING::Char const* pattern, sl_size countPattern, sl_size nMaxSplit) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			if (!(len && countPattern)) {
				return sl_null;
			}
			List<STRING> ret;
			sl_reg start = 0;
			for (sl_size nSplit = 0; nSplit < nMaxSplit; nSplit++) {
				sl_reg index = IndexOf(data, len, pattern, countPattern, start);
				if (index < 0) {
					break;
				}
				if (!(ret.add_NoLock(MidPriv(&str, data, start, index - start)))) {
					return sl_null;
				}
				start = index + countPattern;
			}
			if (ret.add_NoLock(MidPriv(&str, data, start, len - start))) {
				return ret;
			}
			return sl_null;
		}

		template <class STRING>
		static List<STRING> Split(const STRING& str, typename STRING::Char pattern, sl_size nMaxSplit) noexcept
		{
			sl_size len;
			typename STRING::Char const* data = str.getData(len);
			if (!len) {
				return sl_null;
			}
			List<STRING> ret;
			sl_reg start = 0;
			for (sl_size nSplit = 0; nSplit < nMaxSplit; nSplit++) {
				sl_reg index = IndexOfChar(data, len, pattern, start);
				if (index < 0) {
					break;
				}
				if (!(ret.add_NoLock(MidPriv(&str, data, start, index - start)))) {
					return sl_null;
				}
				start = index + 1;
			}
			if (ret.add_NoLock(MidPriv(&str, data, start, len - start))) {
				return ret;
			}
			return sl_null;
		}

		template <class STRING>
		static typename StringTypeFromCharType<typename STRING::Char>::Type Join(const STRING* strings, sl_size count) noexcept
		{
			if (!count) {
				return sl_null;
			}
			if (count == 1) {
				return *strings;
			}
			if (count == 2) {
				return *strings + strings[1];
			}
			typename StringBufferTypeFromCharType<typename STRING::Char>::Type buf;
			for (sl_size i = 0; i < count; i++) {
				sl_size len;
				typename STRING::Char* data = strings[i].getData(len);
				buf.addStatic(data, len);
			}
			return buf.merge();
		}

		template <class STRING>
		static typename StringTypeFromCharType<typename STRING::Char>::Type Join(const STRING* strings, sl_size count, typename StringViewTypeFromCharType<typename STRING::Char>::Type const& delimiter) noexcept
		{
			if (!count) {
				return sl_null;
			}
			if (count == 1) {
				return *strings;
			}
			sl_size lenDelimiter;
			typename STRING::Char* strDelimiter = delimiter.getData(lenDelimiter);
			typename StringBufferTypeFromCharType<typename STRING::Char>::Type buf;
			for (sl_size i = 0; i < count; i++) {
				if (i) {
					buf.addStatic(strDelimiter, lenDelimiter);
				}
				sl_size len;
				typename STRING::Char* data = strings[i].getData(len);
				buf.addStatic(data, len);
			}
			return buf.merge();
		}

		struct STRING_PARAM_ITEM
		{
			sl_uint32 type; // 8, 16, 32
			union {
				sl_char8* sz8;
				sl_char8* sz16;
				sl_char8* sz32;
			};
			sl_reg len;
		};

		template <class STRING>
		static STRING JoinParams(const StringParam* strings, sl_size count) noexcept
		{
			if (!count) {
				return sl_null;
			}
			if (count == 1) {
				return STRING::from(*strings);
			}
			if (count == 2) {
				return ConcatParams<STRING>(*strings, strings[1]);
			}
			sl_bool flagNotNull = sl_false;
			sl_size len = 0;
			sl_size i;
			for (i = 0; i < count; i++) {
				const StringParam& s = strings[i];
				if (s.isNotNull()) {
					flagNotNull = sl_true;
					StringRawData data;
					s.getData(data);
					if (data.length) {
						if (data.charSize == 1) {
							sl_size n = ConvertCharset(data.data8, data.length, (typename STRING::Char*)sl_null);
							if (n) {
								len += n;
							}
						} else if (data.charSize == 2) {
							sl_size n = ConvertCharset(data.data16, data.length, (typename STRING::Char*)sl_null);
							if (n) {
								len += n;
							}
						} else {
							sl_size n = ConvertCharset(data.data32, data.length, (typename STRING::Char*)sl_null);
							if (n) {
								len += n;
							}
						}
					}
				}
			}
			if (!flagNotNull) {
				return sl_null;
			}
			if (!len) {
				return STRING::getEmpty();
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNotNull()) {
				typename STRING::Char* dst = ret.getData();
				for (i = 0; i < count; i++) {
					const StringParam& s = strings[i];
					StringRawData data;
					s.getData(data);
					if (data.length) {
						if (data.charSize == 1) {
							sl_size n = ConvertCharset(data.data8, data.length, dst);
							dst += n;
						} else if (data.charSize == 2) {
							sl_size n = ConvertCharset(data.data16, data.length, dst);
							dst += n;
						} else {
							sl_size n = ConvertCharset(data.data32, data.length, dst);
							dst += n;
						}
					}
				}
				return ret;
			}
			return sl_null;
		}

		template <class STRING>
		static STRING JoinParams(const StringParam* strings, sl_size count, typename STRING::StringViewType const& delimiter) noexcept
		{
			if (!count) {
				return sl_null;
			}
			if (count == 1) {
				return STRING::from(*strings);
			}
			sl_size lenDelimiter;
			typename STRING::Char* strDelimiter = delimiter.getData(lenDelimiter);
			sl_size len = 0;
			sl_size i;
			for (i = 0; i < count; i++) {
				if (i) {
					len += lenDelimiter;
				}
				StringRawData data;
				const StringParam& s = strings[i];
				s.getData(data);
				if (data.length) {
					if (data.charSize == 1) {
						sl_size n = ConvertCharset(data.data8, data.length, (typename STRING::Char*)sl_null);
						if (n) {
							len += n;
						}
					} else if (data.charSize == 2) {
						sl_size n = ConvertCharset(data.data16, data.length, (typename STRING::Char*)sl_null);
						if (n) {
							len += n;
						}
					} else {
						sl_size n = ConvertCharset(data.data32, data.length, (typename STRING::Char*)sl_null);
						if (n) {
							len += n;
						}
					}
				}
			}
			if (!len) {
				return STRING::getEmpty();
			}
			STRING ret = STRING::allocate(len);
			if (ret.isNotNull()) {
				typename STRING::Char* dst = ret.getData();
				for (i = 0; i < count; i++) {
					if (i) {
						MemoryTraits<typename STRING::Char>::copy(dst, strDelimiter, lenDelimiter);
						dst += lenDelimiter;
					}
					StringRawData data;
					const StringParam& s = strings[i];
					s.getData(data);
					if (data.length) {
						if (data.charSize == 1) {
							sl_size n = ConvertCharset(data.data8, data.length, dst);
							dst += n;
						} else if (data.charSize == 2) {
							sl_size n = ConvertCharset(data.data16, data.length, dst);
							dst += n;
						} else {
							sl_size n = ConvertCharset(data.data32, data.length, dst);
							dst += n;
						}
					}
				}
				return ret;
			}
			return sl_null;
		}

#define MAX_NUMBER_STR_LEN 256
#define MAX_PRECISION 50

		template <class CHAR>
		static sl_uint32 DetermineRadix(const CHAR* str, sl_size& i, sl_size n)
		{
			if (str[i] == '0') {
				if (i + 1 < n) {
					CHAR ch = str[i + 1];
					if (ch == 'x' || ch == 'X') {
						i += 2;
						return 16;
					} else if (ch >= '0' && ch <= '7') {
						i += 1;
						return 8;
					} else if (ch == '8' || ch == '9') {
						return 0;
					}
				}
			}
			return 10;
		}

		template <class INTEGER, class CHAR>
		static sl_reg ParseInt(sl_uint32 radix, const CHAR* str, sl_size i, sl_size n, INTEGER* _out) noexcept
		{
			if (i >= n) {
				return SLIB_PARSE_ERROR;
			}

			sl_bool bMinus = sl_false;
			if (str[i] == '-') {
				i++;
				bMinus = sl_true;
			}
			for (; i < n; i++) {
				sl_uint32 c = (sl_uint32)(str[i]);
				if (c != '\t' && c != ' ') {
					break;
				}
			}
			if (!radix) {
				if (i >= n) {
					return SLIB_PARSE_ERROR;
				}
				radix = DetermineRadix(str, i, n);
				if (!radix) {
					return SLIB_PARSE_ERROR;
				}
			}
			const sl_uint8* pattern = radix <= 36 ? g_conv_radix_inverse_pattern_small : g_conv_radix_inverse_pattern_big;
			INTEGER v = 0;
			sl_bool bEmpty = sl_true;
			for (; i < n; i++) {
				sl_uint32 c = (sl_uint32)(str[i]);
				sl_uint32 m = c < 128 ? pattern[c] : 255;
				if (m < (sl_uint32)radix) {
					v = v * radix + m;
					bEmpty = sl_false;
				} else {
					break;
				}
			}
			if (bEmpty) {
				return SLIB_PARSE_ERROR;
			}
			if (bMinus) {
				v = -v;
			}
			if (_out) {
				*_out = v;
			}
			return i;
		}

		template <class VIEW, class INTEGER>
		static sl_bool ParseViewInt(const VIEW& view, sl_int32 radix, INTEGER* _out) noexcept
		{
			typename VIEW::Char* data = view.getData();
			sl_reg len = view.getUnsafeLength();
			if (data && len) {
				if (len > 0) {
					return ParseInt(radix, data, 0, len, _out) == (sl_reg)len;
				} else {
					sl_reg ret = ParseInt(radix, data, 0, len, _out);
					return ret != SLIB_PARSE_ERROR && !(data[ret]);
				}
			}
			return sl_false;
		}

		template <class INTEGER, class CHAR>
		static sl_reg ParseUint(sl_uint32 radix, const CHAR* str, sl_size i, sl_size n, INTEGER* _out) noexcept
		{
			if (i >= n) {
				return SLIB_PARSE_ERROR;
			}
			if (!radix) {
				radix = DetermineRadix(str, i, n);
				if (!radix) {
					return SLIB_PARSE_ERROR;
				}
			}
			sl_bool bEmpty = sl_true;
			const sl_uint8* pattern = radix <= 36 ? g_conv_radix_inverse_pattern_small : g_conv_radix_inverse_pattern_big;
			INTEGER v = 0;
			for (; i < n; i++) {
				sl_uint32 c = (sl_uint32)(str[i]);
				sl_uint32 m = c < 128 ? pattern[c] : 255;
				if (m < radix) {
					v = v * radix + m;
					bEmpty = sl_false;
				} else {
					break;
				}
			}
			if (bEmpty) {
				return SLIB_PARSE_ERROR;
			}
			if (_out) {
				*_out = v;
			}
			return i;
		}

		template <class VIEW, class INTEGER>
		static sl_bool ParseViewUint(const VIEW& view, sl_int32 radix, INTEGER* _out) noexcept
		{
			typename VIEW::Char* data = view.getData();
			sl_reg len = view.getUnsafeLength();
			if (data && len) {
				if (len > 0) {
					return ParseUint(radix, data, 0, len, _out) == (sl_reg)len;
				} else {
					sl_reg ret = ParseUint(radix, data, 0, len, _out);
					return ret != SLIB_PARSE_ERROR && !(data[ret]);
				}
			}
			return sl_false;
		}

		template <class FLOAT, class CHAR>
		static sl_reg ParseFloat(const CHAR* str, sl_size i, sl_size n, FLOAT* _out) noexcept
		{
			if (i >= n) {
				return SLIB_PARSE_ERROR; // input string is empty
			}

			sl_bool bMinus = sl_false;
			sl_bool bEmpty = sl_true;

			if (str[i] == '-') {
				i++;
				bMinus = sl_true;
			}
			for (; i < n; i++) {
				sl_uint32 c = (sl_uint32)(str[i]);
				if (!(SLIB_CHAR_IS_SPACE_TAB(c))) {
					break;
				}
			}

			FLOAT v = 0;
			sl_uint32 vi = 0;
			sl_bool flagVi = sl_true;

			for (; i < n; i++) {
				sl_uint32 c = (sl_uint32)(str[i]);
				if (SLIB_CHAR_IS_DIGIT(c)) {
					if (flagVi) {
						vi = vi * 10 + (c - '0');
						if (vi >= 214748364) {
							v = (FLOAT)vi;
							flagVi = sl_false;
						}
					} else {
						v = v * 10 + (c - '0');
					}
					bEmpty = sl_false;
				} else {
					if (c == '.') {
						bEmpty = sl_false;
					}
					break;
				}
			}
			if (bEmpty) {
				do {
					if (i + 3 <= n) {
						sl_uint32 c = (sl_uint32) (str[i]);
						if ((c == 'n' || c == 'N') && (str[i + 1] == 'a' || str[i + 1] == 'A') &&
						    (str[i + 2] == 'n' || str[i + 2] == 'N')) {
							i += 3;
							if (_out) {
								Math::getNaN(*_out);
							}
						} else if ((c == 'i' || c == 'I') &&
						           (str[i + 1] == 'n' || str[i + 1] == 'N') &&
						           (str[i + 2] == 'f' || str[i + 2] == 'F')) {
							i += 3;
							if (i + 5 <= n && (str[i] == 'i' || str[i] == 'I') &&
							    (str[i + 1] == 'n' || str[i + 1] == 'N') &&
							    (str[i + 2] == 'i' || str[i + 2] == 'I') &&
							    (str[i + 3] == 't' || str[i + 3] == 'T') &&
							    (str[i + 4] == 'y' || str[i + 4] == 'Y')) {
								i += 5;
							}
							if (_out) {
								if (bMinus) {
									Math::getNegativeInfinite(*_out);
								} else {
									Math::getPositiveInfinite(*_out);
								}
							}
						} else {
							break;
						}
						if (i < n) {
							c = (sl_uint32) (str[i]);
							if (c >= 128 || SLIB_CHAR_IS_ALNUM(c)) {
								break;
							}
						}
						return i;
					}
				} while (0);
				return SLIB_PARSE_ERROR; // integral number is required
			}
			if (flagVi) {
				v = (FLOAT)vi;
			}

			if (i < n) {
				if (str[i] == '.') {
					i++;
					bEmpty = sl_true;
					FLOAT weight = (FLOAT)(0.1);
					for (; i < n; i++) {
						sl_uint32 c = (sl_uint32)(str[i]);
						if (SLIB_CHAR_IS_DIGIT(c)) {
							v = v + (c - '0') * weight;
							weight /= 10;
							bEmpty = sl_false;
						} else {
							break;
						}
					}
					if (bEmpty) {
						return SLIB_PARSE_ERROR; // fraction number is required
					}
				}
				if (i < n) {
					if (str[i] == 'e' || str[i] == 'E') {
						i++;
						bEmpty = sl_true;
						sl_bool bMinuxExp = sl_false;
						FLOAT exp = 0;
						if (i < n && (str[i] == '+' || str[i] == '-')) {
							if (str[i] == '-') {
								bMinuxExp = sl_true;
							}
							i++;
						}
						for (; i < n; i++) {
							sl_uint32 c = (sl_uint32)(str[i]);
							if (SLIB_CHAR_IS_DIGIT(c)) {
								exp = exp * 10 + (c - '0');
								bEmpty = sl_false;
							} else {
								break; // invalid character
							}
						}
						if (bEmpty) {
							return SLIB_PARSE_ERROR; // exponent number is required
						}
						if (bMinuxExp) {
							exp = -exp;
						}
						v *= Math::pow((FLOAT)(10.0), exp);
					}
				}
			}
			if (bMinus) {
				v = -v;
			}
			if (_out) {
				*_out = v;
			}
			return i;
		}

		template <class VIEW, class FLOAT>
		static sl_bool ParseViewFloat(const VIEW& view, FLOAT* _out) noexcept
		{
			typename VIEW::Char* data = view.getData();
			sl_reg len = view.getUnsafeLength();
			if (data && len) {
				if (len > 0) {
					return ParseFloat(data, 0, len, _out) == (sl_reg)len;
				} else {
					sl_reg ret = ParseFloat(data, 0, len, _out);
					return ret != SLIB_PARSE_ERROR && !(data[ret]);
				}
			}
			return sl_false;
		}

		template <class CHAR>
		static sl_reg ParseBoolean(const CHAR* str, sl_size i, sl_size n, sl_bool* _out) noexcept
		{
			if (i >= n) {
				return SLIB_PARSE_ERROR;
			}
			sl_bool f = sl_false;
			switch (str[i]) {
				case '1':
					i++;
					f = sl_true;
					break;
				case '0':
					i++;
					f = sl_false;
					break;
				case 'T':
				case 't':
					if (i + 4 <= n) {
						i++;
						if (str[i] == 'R' || str[i] == 'r') {
							i++;
							if (str[i] == 'U' || str[i] == 'u') {
								i++;
								if (str[i] == 'E' || str[i] == 'e') {
									i++;
									f = sl_true;
									break;
								}
							}
						}
					}
					return SLIB_PARSE_ERROR;
				case 'F':
				case 'f':
					if (i + 5 <= n) {
						i++;
						if (str[i] == 'A' || str[i] == 'a') {
							i++;
							if (str[i] == 'L' || str[i] == 'l') {
								i++;
								if (str[i] == 'S' || str[i] == 's') {
									i++;
									if (str[i] == 'E' || str[i] == 'e') {
										i++;
										f = sl_false;
										break;
									}
								}
							}
						}
					}
					return SLIB_PARSE_ERROR;
				case 'Y':
				case 'y':
					i++;
					if (i + 2 <= n && (str[i] == 'E' || str[i] == 'e')) {
						i++;
						if (str[i] == 'S' || str[i] == 's') {
							i++;
							f = sl_true;
							break;
						}
					} else {
						f = sl_true;
						break;
					}
					return SLIB_PARSE_ERROR;
				case 'N':
				case 'n':
					i++;
					if (i + 1 <= n && (str[i] == 'O' || str[i] == 'o')) {
						i++;
					}
					f = sl_false;
					break;
				default:
					break;
			}
			if (i < n) {
				CHAR c = str[i];
				if (SLIB_CHAR_IS_C_NAME(c)) {
					return SLIB_PARSE_ERROR;
				}
			}
			if (_out) {
				*_out = f;
			}
			return i;
		}

		template <class VIEW>
		static sl_bool ParseViewBoolean(const VIEW& view, sl_bool* _out) noexcept
		{
			typename VIEW::Char* data = view.getData();
			sl_reg len = view.getUnsafeLength();
			if (data && len) {
				if (len > 0) {
					return ParseBoolean(data, 0, len, _out) == (sl_reg)len;
				} else {
					sl_reg ret = ParseBoolean(data, 0, len, _out);
					return ret != SLIB_PARSE_ERROR && !(data[ret]);
				}
			}
			return sl_false;
		}

		template <class CHAR>
		static sl_reg ParseHexString(const CHAR* str, sl_size i, sl_size n, void* _out) noexcept
		{
			if (i >= n) {
				return SLIB_PARSE_ERROR;
			}
			sl_uint8* buf = (sl_uint8*)(_out);
			sl_size k = 0;
			for (; i < n; i += 2) {
				sl_uint32 v1, v2;
				{
					sl_uint32 ch = (sl_uint32)(str[i]);
					if (ch >= '0' && ch <= '9') {
						v1 = ch - '0';
					} else if (ch >= 'A' && ch <= 'F') {
						v1 = ch - 'A' + 10;
					} else if (ch >= 'a' && ch <= 'f') {
						v1 = ch - 'a' + 10;
					} else {
						break;
					}
				}
				{
					sl_uint32 ch = (sl_uint32)(str[i + 1]);
					if (ch >= '0' && ch <= '9') {
						v2 = ch - '0';
					} else if (ch >= 'A' && ch <= 'F') {
						v2 = ch - 'A' + 10;
					} else if (ch >= 'a' && ch <= 'f') {
						v2 = ch - 'a' + 10;
					} else {
						return SLIB_PARSE_ERROR;
					}
				}
				buf[k++] = (sl_uint8)((v1 << 4) | v2);
			}
			return i;
		}

		template <class VIEW>
		static sl_bool ParseViewHexString(const VIEW& view, void* _out) noexcept
		{
			typename VIEW::Char* data = view.getData();
			sl_reg len = view.getUnsafeLength();
			if (data && len) {
				if (len > 0) {
					return ParseHexString(data, 0, len, _out) == (sl_reg)len;
				} else {
					sl_reg ret = ParseHexString(data, 0, len, _out);
					return ret != SLIB_PARSE_ERROR && !(data[ret]);
				}
			}
			return sl_false;
		}

		template <class STRING>
		static Memory ParseHexString(const STRING& str) noexcept
		{
			sl_size n;
			typename STRING::Char* data = str.getData(n);
			if (n > 0 && !(n & 1)) {
				Memory mem = Memory::create(n >> 1);
				if (mem.isNotNull()) {
					if (ParseHexString(data, 0, n, mem.getData()) == (sl_reg)n) {
						return mem;
					}
				}
			}
			return sl_null;
		}

		template <class INTEGER, class CHAR>
		static typename StringTypeFromCharType<CHAR>::Type FromInt(INTEGER _value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase, CHAR chGroup = sl_false, sl_bool flagSignPositive = sl_false, sl_bool flagLeadingSpacePositive = sl_false, sl_bool flagEncloseNagtive = sl_false) noexcept
		{
			if (radix < 2 || radix > 64) {
				return sl_null;
			}

			const char* pattern = flagUpperCase && radix <= 36 ? g_conv_radix_pattern_upper : g_conv_radix_pattern_lower;

			CHAR buf[MAX_NUMBER_STR_LEN];

			sl_uint32 pos = MAX_NUMBER_STR_LEN;

			if (minWidth < 1) {
				minWidth = 1;
			}

			sl_bool flagMinus = sl_false;
			typename UnsignedType<INTEGER>::Type value;
			if (_value < 0) {
				value = -_value;
				flagMinus = sl_true;
				if (flagEncloseNagtive) {
					pos--;
					buf[pos] = ')';
				}
			} else {
				value = _value;
			}

			sl_uint32 nDigits = 0;
			while (value || minWidth > 0) {
				if (chGroup) {
					if (nDigits > 0) {
						if (pos > 0) {
							if (!(nDigits % 3)) {
								pos--;
								buf[pos] = chGroup;
							}
						} else {
							break;
						}
					}
				}
				if (pos > 0) {
					pos--;
					buf[pos] = pattern[value % radix];
					value = value / radix;
					if (minWidth > 0) {
						minWidth--;
					}
					nDigits++;
				} else {
					break;
				}
			}

			if (flagMinus) {
				if (pos > 0) {
					pos--;
					buf[pos] = '-';
					if (flagEncloseNagtive) {
						if (pos > 0) {
							pos--;
							buf[pos] = '(';
						}
					}
				}
			} else {
				if (flagSignPositive) {
					if (pos > 0) {
						pos--;
						buf[pos] = '+';
					}
				}
				if (flagLeadingSpacePositive) {
					if (pos > 0) {
						pos--;
						buf[pos] = ' ';
					}
				}
			}
			return typename StringTypeFromCharType<CHAR>::Type(buf + pos, MAX_NUMBER_STR_LEN - pos);
		}

		template <class INTEGER, class CHAR>
		static typename StringTypeFromCharType<CHAR>::Type FromUint(INTEGER value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase, CHAR chGroup = 0, sl_bool flagSignPositive = sl_false, sl_bool flagLeadingSpacePositive = sl_false) noexcept
		{
			if (radix < 2 || radix > 64) {
				return sl_null;
			}

			const char* pattern = flagUpperCase && radix <= 36 ? g_conv_radix_pattern_upper : g_conv_radix_pattern_lower;

			CHAR buf[MAX_NUMBER_STR_LEN];

			sl_uint32 pos = MAX_NUMBER_STR_LEN;

			if (minWidth < 1) {
				minWidth = 1;
			}

			sl_uint32 nDigits = 0;
			while (value || minWidth > 0) {
				if (chGroup) {
					if (nDigits > 0) {
						if (pos > 0) {
							if (!(nDigits % 3)) {
								pos--;
								buf[pos] = chGroup;
							}
						} else {
							break;
						}
					}
				}
				if (pos > 0) {
					pos--;
					buf[pos] = pattern[value % radix];
					value = value / radix;
					if (minWidth > 0) {
						minWidth--;
					}
					nDigits++;
				} else {
					break;
				}
			}

			if (flagSignPositive) {
				if (pos > 0) {
					pos--;
					buf[pos] = '+';
				}
			}
			if (flagLeadingSpacePositive) {
				if (pos > 0) {
					pos--;
					buf[pos] = ' ';
				}
			}

			return typename StringTypeFromCharType<CHAR>::Type(buf + pos, MAX_NUMBER_STR_LEN - pos);
		}

		template <class FLOAT, class CHAR>
		static typename StringTypeFromCharType<CHAR>::Type FromFloat(FLOAT value, sl_int32 precision, sl_bool flagZeroPadding, sl_int32 minWidthIntegral, CHAR chConv = 'g', CHAR chGroup = 0, sl_bool flagSignPositive = sl_false, sl_bool flagLeadingSpacePositive = sl_false, sl_bool flagEncloseNagtive = sl_false) noexcept
		{

			CHAR buf[MAX_NUMBER_STR_LEN];

			if (Math::isNaN(value)) {
				static CHAR s[] = { 'N', 'a', 'N', 0 };
				return StringTypeFromCharType<CHAR>::Type::fromStatic(s);
			}
			if (Math::isPositiveInfinite(value)) {
				static CHAR s[] = { 'I', 'n', 'f', 'i', 'n', 'i', 't', 'y', 0 };
				return StringTypeFromCharType<CHAR>::Type::fromStatic(s);
			}
			if (Math::isNegativeInfinite(value)) {
				static CHAR s[] = { '-', 'I', 'n', 'f', 'i', 'n', 'i', 't', 'y', 0 };
				return StringTypeFromCharType<CHAR>::Type::fromStatic(s);
			}

			if (minWidthIntegral > MAX_PRECISION) {
				minWidthIntegral = MAX_PRECISION;
			}
			if (precision > MAX_PRECISION) {
				precision = MAX_PRECISION;
			}
			if (precision <= 0) {
				flagZeroPadding = sl_false;
			}

			if (!value) {
				sl_uint32 pos = 0;
				if (flagLeadingSpacePositive) {
					buf[pos++] = ' ';
				}
				if (flagSignPositive) {
					buf[pos++] = '+';
				}
				for (sl_int32 i = 0; i < minWidthIntegral; i++) {
					if (chGroup) {
						if (i > 0) {
							if (!((minWidthIntegral - i) % 3)) {
								buf[pos++] = chGroup;
							}
						}
					}
					buf[pos++] = '0';
				}
				if (precision != 0) {
					buf[pos++] = '.';
					if (flagZeroPadding) {
						for (sl_int32 k = 0; k < precision; k++) {
							buf[pos++] = '0';
						}
					} else {
						buf[pos++] = '0';
					}
				}
				return typename StringTypeFromCharType<CHAR>::Type(buf, pos);
			}

			CHAR* str = buf;
			CHAR* str_last = buf + MAX_NUMBER_STR_LEN - MAX_PRECISION;

			sl_int32 flagMinus;
			if (value < 0) {
				flagMinus = 1;
				value = -value;
				if (flagEncloseNagtive) {
					*(str++) = '(';
				}
				*(str++) = '-';
			} else {
				flagMinus = 0;
				if (flagLeadingSpacePositive) {
					*(str++) = ' ';
				}
				if (flagSignPositive) {
					*(str++) = '+';
				}
			}

			sl_int32 nExp;
			sl_int32 nInt;
			if (chConv == 'f') {
				nInt = (sl_int32)(Math::log10(value));
				nExp = 0;
			} else if (chConv == 'e' || chConv == 'E') {
				nInt = minWidthIntegral - 1;
				nExp = (sl_int32)(Math::log10(value));
			} else {
				nInt = (sl_int32)(Math::log10(value));
				nExp = 0;
				if (nInt >= 15) { // use exp
					nExp = nInt;
					nInt = 0;
				}
				if (nInt < -15) { // use exp
					nExp = nInt - 1;
					nInt = 0;
				}
			}

			if (nExp != 0) {
				value = value / Math::pow((FLOAT)10, (FLOAT)(nExp));
			}

			FLOAT min_value;
			if (precision < 0) {
				if (sizeof(FLOAT) == 4) {
					if (nInt >= 5) {
						precision = 10 - nInt;
					} else if (nInt >= 0) {
						precision = 5;
					} else {
						precision = 5 - nInt;
					}
				} else {
					if (nInt >= 5) {
						precision = 15 - nInt;
					} else if (nInt >= 0) {
						precision = 10;
					} else {
						precision = 10 - nInt;
					}
				}
				if (precision < 1) {
					precision = 1;
				} else if (precision > 30) {
					precision = 30;
				}
			}
			min_value = Math::pow((FLOAT)10, (FLOAT)(-precision));
			value += min_value / 3;
			if (flagZeroPadding) {
				min_value = 0;
			}

			if (nInt < minWidthIntegral - 1) {
				nInt = minWidthIntegral - 1;
			}
			FLOAT weight = 1;
			if (nInt != 0) {
				weight = Math::pow((FLOAT)10, (FLOAT)nInt);
			}
			while (str < str_last && nInt >= -precision && (nInt >= 0 || value >= min_value)) {
				if (nInt == -1) {
					if (value >= min_value) {
						*(str++) = '.';
					} else {
						break;
					}
				}
				if (weight > 0 && !(Math::isInfinite(weight))) {
					sl_int32 digit = (sl_int32)(value / weight);
					if (digit < 0) {
						digit = 0;
					}
					if (digit > 9) {
						digit = 9;
					}
					*(str++) = (CHAR)('0' + digit);
					value -= (digit * weight);
				}
				if (chGroup) {
					if (nInt > 0 && !(nInt % 3)) {
						*(str++) = chGroup;
					}
				}
				nInt--;
				weight /= 10;
			}
			if (precision > 0) {
				if (nInt >= -1) {
					*(str++) = '.';
					*(str++) = '0';
				}
			}

			if (nExp) {
				if (chConv == 'E' || chConv == 'G') {
					*(str++) = 'E';
				} else {
					*(str++) = 'e';
				}
				if (nExp > 0) {
					*(str++) = '+';
				} else {
					*(str++) = '-';
					nExp = -nExp;
				}
				CHAR* t1 = str;
				while (nExp > 0 && str < str_last) {
					*(str++) = (CHAR)('0' + (nExp % 10));
					nExp /= 10;
				}
				CHAR* t2 = str - 1;
				while (t1 < t2) {
					CHAR chTemp = *t1;
					*t1 = *t2;
					*t2 = chTemp;
					t1++;
					t2--;
				}
			}

			if (flagMinus) {
				if (flagEncloseNagtive) {
					*(str++) = ')';
				}
			}

			return typename StringTypeFromCharType<CHAR>::Type(buf, str - buf);
		}

		template <class CHAR>
		static typename StringTypeFromCharType<CHAR>::Type MakeHexString(const void* buf, sl_size size, sl_bool flagUseLowerChar) noexcept
		{
			if (!buf || size <= 0) {
				return sl_null;
			}
			typename StringTypeFromCharType<CHAR>::Type str = StringTypeFromCharType<CHAR>::Type::allocate(size * 2);
			if (str.isEmpty()) {
				return str;
			}
			CHAR* data = (CHAR*)(str.getData());
			if (flagUseLowerChar) {
				for (sl_size i = 0; i < size; i++) {
					sl_uint8 v = ((sl_uint8*)(buf))[i];
					data[i << 1] = g_conv_radix_pattern_lower[v >> 4];
					data[(i << 1) + 1] = g_conv_radix_pattern_lower[v & 15];
				}
			} else {
				for (sl_size i = 0; i < size; i++) {
					sl_uint8 v = ((sl_uint8*)(buf))[i];
					data[i << 1] = g_conv_radix_pattern_upper[v >> 4];
					data[(i << 1) + 1] = g_conv_radix_pattern_upper[v & 15];
				}
			}
			return str;
		}

		/*

			String Formatting is similar with Java Formatter

			https://docs.oracle.com/javase/7/docs/api/java/util/Formatter.html

			%[argument_index$][property_list][flags][width][.precision]conversion

			property:
				[name]: `name` is element index or item name

			precision:
				-precision: tailing zeros (except first zero) are ignored
				--precision: tailing zeros (including first zero) are ignored

			`%%` -> `%`
			`%...%` = `%...s`

		*/

		template <typename CHAR, typename STRING>
		class StringFormatter
		{
		public:
			CHAR* format;
			sl_size len;
			sl_bool flagSz;
			const Variant* params;
			sl_size nParams;
			Locale locale;

			typename StringBufferTypeFromCharType<CHAR>::Type sb;
			sl_size pos = 0;
			sl_size indexArgAuto = 0;
			sl_size indexArgLast = 0;

			sl_bool flagFatalError = sl_false;

		public:
			template <class ARG>
			sl_bool append(ARG&& text)
			{
				if (sb.add(Forward<ARG>(text))) {
					return sl_true;
				} else {
					flagFatalError = sl_true;
					return sl_false;
				}
			}
			
			sl_bool appendStatic(const CHAR* s, sl_size len)
			{
				if (!len) {
					return sl_true;
				}
				if (sb.addStatic(s, len)) {
					return sl_true;
				} else {
					flagFatalError = sl_true;
					return sl_false;
				}
			}

			SLIB_INLINE sl_bool getChar(CHAR& ch)
			{
				if (flagSz) {
					ch = format[pos];
					if (!ch) {
						return sl_false;
					}
				} else {
					if (pos >= len) {
						return sl_false;
					}
					ch = format[pos];
				}
				return sl_true;
			}

			sl_bool processPercentChar()
			{
				pos++;
				static const CHAR t = '%';
				return appendStatic(&t, 1);
			}

			sl_bool processLineBreak()
			{
				pos++;
				static const CHAR t[2] = { '\r', '\n' };
				return appendStatic(t, 2);
			}

			sl_bool parseArgumentIndex(CHAR ch, sl_size& indexArg)
			{
				if (ch == '<') {
					indexArg = indexArgLast;
					pos++;
					if (pos >= len) {
						return sl_false;
					}
				} else {
					sl_uint32 iv;
					sl_reg iRet = STRING::parseUint32(10, &iv, format, pos, len);
					if (iRet == SLIB_PARSE_ERROR) {
						indexArg = indexArgAuto;
						indexArgAuto++;
					} else {
						if ((sl_size)iRet >= len) {
							return sl_false;
						}
						if (format[iRet] == '$') {
							pos = (sl_size)iRet;
							if (iv > 0) {
								iv--;
							} else {
								return sl_false;
							}
							indexArg = iv;
							pos++;
							if (pos >= len) {
								return sl_false;
							}
						} else {
							indexArg = indexArgAuto;
							indexArgAuto++;
						}
					}
				}
				if (indexArg >= nParams) {
					indexArg = nParams - 1;
				}
				indexArgLast = indexArg;
				return sl_true;
			}

			const Variant& resolveProperties(const Variant& _arg, Variant& varg)
			{
				const Variant* arg = &_arg;
				while (pos < len) {
					CHAR ch = format[pos];
					if (ch != '[') {
						break;
					}
					pos++;
					sl_size posName = pos;
					for (;;) {
						if (!(getChar(ch))) {
							return *arg;
						}
						if (ch == ']') {
							varg = arg->getItem(String::from(format + posName, pos - posName));
							arg = &varg;
							pos++;
							break;
						}
						pos++;
					}
				}
				return *arg;
			}

			struct ConversionFlags
			{
				sl_bool flagAlignLeft = sl_false; // '-'
				sl_bool flagSignPositive = sl_false; // '+'
				sl_bool flagLeadingSpacePositive = sl_false; // ' '
				sl_bool flagZeroPadded = sl_false; // '0'
				sl_bool flagGroupingDigits = sl_false; // ','
				sl_bool flagEncloseNegative = sl_false; // '('

				sl_uint32 minWidth = 0;
				sl_uint32 precision = 0;
				sl_bool flagUsePrecision = sl_false;
				sl_bool flagZeroPaddedFraction = sl_true;
				sl_bool flagIgnoreZeroFraction = sl_false;
			};

			sl_bool parseFlags(ConversionFlags& flags)
			{
				do {
					CHAR ch = format[pos];
					if (ch == '-') {
						flags.flagAlignLeft = sl_true;
					} else if (ch == '+') {
						flags.flagSignPositive = sl_true;
					} else if (ch == ' ') {
						flags.flagLeadingSpacePositive = sl_true;
					} else if (ch == '0') {
						flags.flagZeroPadded = sl_true;
					} else if (ch == ',') {
						flags.flagGroupingDigits = sl_true;
					} else if (ch == '(') {
						flags.flagEncloseNegative = sl_true;
					} else {
						return sl_true;
					}
					pos++;
				} while (pos < len);
				return sl_false;
			}

			sl_bool parseMinWidth(ConversionFlags& flags)
			{
				sl_reg iRet = STRING::parseUint32(10, &(flags.minWidth), format, pos, len);
				if (iRet != SLIB_PARSE_ERROR) {
					pos = iRet;
					if (pos >= len) {
						return sl_false;
					}
				}
				return sl_true;
			}

			sl_bool parsePrecision(ConversionFlags& flags)
			{
				if (format[pos] == '.') {
					pos++;
					if (pos >= len) {
						return sl_false;
					}
					if (format[pos] == '-') {
						pos++;
						if (pos >= len) {
							return sl_false;
						}
						flags.flagZeroPaddedFraction = sl_false;
						if (format[pos] == '-') {
							pos++;
							if (pos >= len) {
								return sl_false;
							}
							flags.flagIgnoreZeroFraction = sl_true;
						}
					}
					sl_reg iRet = STRING::parseUint32(10, &(flags.precision), format, pos, len);
					if (iRet != SLIB_PARSE_ERROR) {
						pos = iRet;
						if (pos >= len) {
							return sl_false;
						}
						flags.flagUsePrecision = sl_true;
					}
				}
				return sl_true;
			}

			sl_bool append(const STRING& content, const ConversionFlags& flags)
			{
				sl_size lenContent = content.getLength();
				if (lenContent < flags.minWidth) {
					if (flags.flagAlignLeft) {
						if (!(append(content))) {
							return sl_false;
						}
						return append(STRING(' ', flags.minWidth - lenContent));
					} else {
						if (!(append(STRING(' ', flags.minWidth - lenContent)))) {
							return sl_false;
						}
						return append(content);
					}
				} else {
					return append(content);
				}
			}

			sl_bool processTime(const Time& time, const ConversionFlags& flags)
			{
				CHAR ch = format[pos];
				const TimeZone* zone;
				if (ch == 'u' || ch == 'U') {
					pos++;
					zone = &(TimeZone::UTC());
					if (!(getChar(ch))) {
						return sl_false;
					}
				} else {
					zone = &(TimeZone::Local);
				}
				sl_uint32 minWidth = 0;
				if (flags.flagZeroPadded) {
					minWidth = flags.minWidth;
				}
				STRING content;
				switch (ch) {
					case 'y':
						if (flags.flagZeroPadded) {
							if (minWidth < 4) {
								minWidth = 4;
							}
						}
						content = STRING::fromInt32(time.getYear(*zone), 10, minWidth);
						break;
					case 'Y':
						content = STRING::fromInt32(time.getYear(*zone) % 100, 10, 2);
						break;
					case 'm':
						if (flags.flagZeroPadded) {
							if (minWidth < 2) {
								minWidth = 2;
							}
						}
						content = STRING::fromInt32(time.getMonth(*zone), 10, minWidth);
						break;
					case 'd':
						if (flags.flagZeroPadded) {
							if (minWidth < 2) {
								minWidth = 2;
							}
						}
						content = STRING::fromInt32(time.getDay(*zone), 10, minWidth);
						break;
					case 'w':
						content = STRING::from(time.getWeekdayShort(*zone, locale));
						break;
					case 'W':
						content = STRING::from(time.getWeekdayLong(*zone, locale));
						break;
					case 'H':
						if (flags.flagZeroPadded) {
							if (minWidth < 2) {
								minWidth = 2;
							}
						}
						content = STRING::fromInt32(time.getHour(*zone), 10, minWidth);
						break;
					case 'h': // Hour-12
						if (flags.flagZeroPadded) {
							if (minWidth < 2) {
								minWidth = 2;
							}
						}
						content = STRING::fromInt32(time.getHour12(*zone), 10, minWidth);
						break;
					case 'a':
						content = STRING::from(time.getAM_PM(*zone, locale));
						break;
					case 'M':
						if (flags.flagZeroPadded) {
							if (minWidth < 2) {
								minWidth = 2;
							}
						}
						content = STRING::fromInt32(time.getMinute(*zone), 10, minWidth);
						break;
					case 'S':
						if (flags.flagZeroPadded) {
							if (minWidth < 2) {
								minWidth = 2;
							}
						}
						content = STRING::fromInt32(time.getSecond(*zone), 10, minWidth);
						break;
					case 'l':
						content = STRING::fromInt32(time.getMillisecond(), 10, minWidth);
						break;
					case 'D':
						content = STRING::from(time.getDateString(*zone));
						break;
					case 'T':
						content = STRING::from(time.getTimeString(*zone));
						break;
					case 'O':
						content = STRING::from(time.getMonthLong(*zone));
						break;
					case 'o':
						content = STRING::from(time.getMonthShort(*zone));
						break;
					case 's':
					case '%':
						content = STRING::from(time.toString(*zone));
						break;
					default:
						return sl_false;
				}
				pos++;
				return append(content, flags);
			}

			sl_bool processString(const Variant& arg, const ConversionFlags& flags)
			{
				CHAR* content = sl_null;
				sl_size lenContent = 0;
				STRING strTemp;
				StringRawData sd;
				if (arg.getStringData(sd)) {
					if (sd.charSize == sizeof(CHAR)) {
						content = (CHAR*)(sd.data);
						if (content) {
							if (sd.length < 0) {
								lenContent = StringTraits<CHAR>::getLength(content);
							} else {
								lenContent = sd.length;
							}
						}
					}
				}
				if (!content) {
					strTemp = STRING::from(arg);
					content = strTemp.getData(lenContent);
				}
				if (lenContent < flags.minWidth) {
					if (flags.flagAlignLeft) {
						if (strTemp.isNotNull()) {
							if (!(append(Move(strTemp)))) {
								return sl_false;
							}
						} else if (lenContent) {
							if (!(appendStatic(content, lenContent))) {
								return sl_false;
							}
						}
						return append(STRING(' ', flags.minWidth - lenContent));
					} else {
						if (!(append(STRING(' ', flags.minWidth - lenContent)))) {
							return sl_false;
						}
						if (strTemp.isNotNull()) {
							return append(Move(strTemp));
						} else if (lenContent) {
							return appendStatic(content, lenContent);
						} else {
							return sl_true;
						}
					}
				} else {
					if (strTemp.isNotNull()) {
						return append(Move(strTemp));
					} else if (lenContent) {
						return appendStatic(content, lenContent);
					} else {
						return sl_true;
					}
				}
			}

			sl_bool processInt(CHAR ch, const Variant& arg, const ConversionFlags& flags)
			{
				CHAR chGroup = 0;
				if (flags.flagGroupingDigits) {
					chGroup = ',';
				}
				sl_uint32 radix = 10;
				sl_bool flagUpperCase = sl_false;
				if (ch == 'x') {
					radix = 16;
				} else if (ch == 'X') {
					radix = 16;
					flagUpperCase = sl_true;
				} else if (ch == 'o') {
					radix = 8;
				}
				sl_uint32 minWidth = 0;
				if (flags.flagZeroPadded) {
					minWidth = flags.minWidth;
				}
				STRING content;
				if (arg.isUint32()) {
					content = FromUint<sl_uint32, CHAR>(arg.getUint32(), radix, minWidth, flagUpperCase, chGroup, flags.flagSignPositive, flags.flagLeadingSpacePositive);
				} else if (arg.isInt32()) {
					content = FromInt<sl_int32, CHAR>(arg.getInt32(), radix, minWidth, flagUpperCase, chGroup, flags.flagSignPositive, flags.flagLeadingSpacePositive, flags.flagEncloseNegative);
				} else if (arg.isUint64()) {
					content = FromUint<sl_uint64, CHAR>(arg.getUint64(), radix, minWidth, flagUpperCase, chGroup, flags.flagSignPositive, flags.flagLeadingSpacePositive);
				} else if (arg.isInt64()) {
					content = FromInt<sl_int64, CHAR>(arg.getInt64(), radix, minWidth, flagUpperCase, chGroup, flags.flagSignPositive, flags.flagLeadingSpacePositive, flags.flagEncloseNegative);
				} else if (arg.isBigInt()) {
					content = STRING::from(arg.getBigInt().toString(radix, flagUpperCase));
				} else if (arg.isMemory() && radix == 16) {
					content = STRING::makeHexString(arg.getMemory(), !flagUpperCase);
				} else {
					content = FromInt<sl_int64, CHAR>(arg.getInt64(), radix, minWidth, flagUpperCase, chGroup, flags.flagSignPositive, flags.flagLeadingSpacePositive, flags.flagEncloseNegative);
				}
				return append(content, flags);
			}

			sl_bool processFloat(CHAR ch, const Variant& arg, const ConversionFlags& flags)
			{
				CHAR chGroup = 0;
				if (flags.flagGroupingDigits) {
					chGroup = ',';
				}
				sl_int32 precision = -1;
				if (flags.flagUsePrecision) {
					precision = flags.precision;
				}
				sl_int32 minWidthIntegral = 1;
				sl_bool flagZeroPaddedFraction = flags.flagZeroPaddedFraction;
				if (flags.flagZeroPadded) {
					if (precision > 0) {
						minWidthIntegral = flags.minWidth - precision - 1;
					} else {
						minWidthIntegral = flags.minWidth;
					}
					if (minWidthIntegral < 1) {
						minWidthIntegral = 1;
					}
					flagZeroPaddedFraction = sl_true;
				}
				STRING content;
				if (arg.isFloat()) {
					content = FromFloat<float, CHAR>(arg.getFloat(), precision, flagZeroPaddedFraction, minWidthIntegral, ch, chGroup, flags.flagSignPositive, flags.flagLeadingSpacePositive, flags.flagEncloseNegative);
				} else {
					content = FromFloat<double, CHAR>(arg.getDouble(), precision, flagZeroPaddedFraction, minWidthIntegral, ch, chGroup, flags.flagSignPositive, flags.flagLeadingSpacePositive, flags.flagEncloseNegative);
				}
				if (flags.flagIgnoreZeroFraction) {
					sl_size len = content.getLength();
					if (len > 2) {
						CHAR* s = content.getData();
						if (s[len - 2] == '.' && s[len - 1] == '0') {
							content = content.substring(0, len - 2);
						}
					}
				}
				return append(content, flags);
			}

			sl_bool processChar(const Variant& arg, const ConversionFlags& flags)
			{
				sl_char32 unicode = (sl_char32)(arg.getUint32());
				return append(STRING::create(&unicode, 1), flags);
			}

			sl_bool processConversion(CHAR ch)
			{
				if (ch == '%') {
					return processPercentChar();
				}
				if (ch == 'n') {
					return processLineBreak();
				}
				sl_size indexArg;
				if (!(parseArgumentIndex(ch, indexArg))) {
					return sl_false;
				}
				Variant varg;
				const Variant& arg = resolveProperties(params[indexArg], varg);
				if (pos >= len) {
					return sl_false;
				}
				ConversionFlags flags;
				if (!(parseFlags(flags))) {
					return sl_false;
				}
				if (!(parseMinWidth(flags))) {
					return sl_false;
				}
				if (!(parsePrecision(flags))) {
					return sl_false;
				}
				if (arg.isTime()) {
					return processTime(arg.getTime(), flags);
				}
				ch = format[pos];
				switch (ch) {
					case 's':
					case '%':
						pos++;
						return processString(arg, flags);
					case 'd':
					case 'x':
					case 'X':
					case 'o':
						pos++;
						return processInt(ch, arg, flags);
					case 'f':
					case 'e':
					case 'E':
					case 'g':
					case 'G':
						pos++;
						return processFloat(ch, arg, flags);
					case 'c':
						pos++;
						return processChar(arg, flags);
					case 'D':
						return processTime(arg.getTime(), flags);
					default:
						return sl_false;
				}
			}

			STRING run() noexcept
			{
				sl_size posText = 0;
				for (;;) {
					CHAR ch;
					if (!(getChar(ch))) {
						break;
					}
					if (ch == '%') {
						if (!(appendStatic(format + posText, pos - posText))) {
							return sl_null;
						}
						posText = pos;
						pos++;
						if (!(getChar(ch))) {
							break;
						}
						if (processConversion(ch)) {
							posText = pos;
						} else {
							if (flagFatalError) {
								return sl_null;
							}
							pos = posText + 1;
						}
					} else {
						pos++;
					}
				}
				if (!(appendStatic(format + posText, pos - posText))) {
					return sl_null;
				}
				return sb.merge();
			}
		};

		template <class VIEW>
		static typename VIEW::StringType Format(const Locale& locale, const VIEW& view, const Variant* params, sl_size nParams) noexcept
		{
			if (view.isNull()) {
				return sl_null;
			}
			if (!nParams) {
				return view;
			}
			StringFormatter<typename VIEW::Char, typename VIEW::StringType> formator;
			formator.format = view.getData();
			sl_reg len = view.getUnsafeLength();
			formator.len = len;
			formator.flagSz = len < 0;
			formator.params = params;
			formator.nParams = nParams;
			formator.locale = locale;
			return formator.run();
		}

	}

#define DEFINE_STRING_CONTAINER_IMPL(CONTAINER) \
	SLIB_INLINE sl_reg CONTAINER::increaseReference() noexcept \
	{ \
		if (ref >= 0) { \
			return CRef::increaseReference(ref); \
		} \
		return -1; \
	} \
	\
	SLIB_INLINE sl_reg CONTAINER::decreaseReference() noexcept \
	{ \
		sl_reg nRef = CRef::decreaseReference(ref); \
		if (!nRef) { \
			Free(this); \
		} \
		return nRef; \
	}

	DEFINE_STRING_CONTAINER_IMPL(StringContainer)
	DEFINE_STRING_CONTAINER_IMPL(StringContainer16)
	DEFINE_STRING_CONTAINER_IMPL(StringContainer32)


#define DEFINE_STRING_INLINE_IMPL(STRING) \
	SLIB_INLINE void STRING::_replaceContainer(typename STRING::Container* container) noexcept \
	{ \
		if (m_container) { \
			m_container->decreaseReference(); \
		} \
		m_container = container; \
	} \
	\
	SLIB_INLINE void Atomic<STRING>::_replaceContainer(typename STRING::Container* container) noexcept \
	{ \
		m_lock.lock(); \
		Container* before = m_container; \
		m_container = container; \
		m_lock.unlock(); \
		if (before) { \
			before->decreaseReference(); \
		} \
	} \
	\
	SLIB_INLINE typename STRING::Container* Atomic<STRING>::_retainContainer() const noexcept \
	{ \
		if (!m_container) { \
			return sl_null; \
		} \
		m_lock.lock(); \
		Container* container = m_container; \
		if (container) { \
			container->increaseReference(); \
		} \
		m_lock.unlock(); \
		return container; \
	} \
	\
	SLIB_INLINE typename STRING::Container* Atomic<STRING>::_releaseContainer() noexcept \
	{ \
		if (!m_container) { \
			return sl_null; \
		} \
		m_lock.lock(); \
		Container* container = m_container; \
		m_container = sl_null; \
		m_lock.unlock(); \
		return container; \
	}

	DEFINE_STRING_INLINE_IMPL(String)
	DEFINE_STRING_INLINE_IMPL(String16)
	DEFINE_STRING_INLINE_IMPL(String32)


#define DEFINE_STRING_CONSTRUCTOR_IMPL(STRING, CONSTRUCTOR) \
	STRING::CONSTRUCTOR(typename RemoveAtomic<STRING>::Type const& src) noexcept \
	{ \
		Container* container = src.m_container; \
		if (container) { \
			container->increaseReference(); \
		} \
		m_container= container; \
	} \
	\
	STRING::CONSTRUCTOR(typename AddAtomic<STRING>::Type&& src) noexcept \
	{ \
		m_container = src._releaseContainer(); \
	} \
	\
	STRING::CONSTRUCTOR(typename AddAtomic<STRING>::Type const& src) noexcept \
	{ \
		m_container = src._retainContainer(); \
	} \
	\
	STRING::CONSTRUCTOR(typename STRING::StringViewType const& src) noexcept \
	{ \
		m_container = CreateFromSz<Container>(src.getData(), src.getUnsafeLength()); \
	} \
	\
	STRING::~CONSTRUCTOR() \
	{ \
		Container* container = m_container; \
		if (container) { \
			container->decreaseReference(); \
		} \
	} \
	\
	STRING::CONSTRUCTOR(typename STRING::Char ch, sl_size nRepeatCount) noexcept \
	{ \
		m_container = Create<Container>(ch, nRepeatCount); \
	} \
	\
	STRING::CONSTRUCTOR(typename STRING::Char const* str) noexcept \
	{ \
		m_container = CreateFromSz<Container>(str, -1); \
	} \
	\
	STRING::CONSTRUCTOR(typename STRING::Char const* str, sl_reg length) noexcept \
	{ \
		m_container = CreateFromSz<Container>(str, length); \
	} \
	\
	STRING::CONSTRUCTOR(typename STRING::StdString const& str) noexcept \
	{ \
		m_container = AllocStd<Container>(str); \
	} \
	\
	STRING::CONSTRUCTOR(typename STRING::StdString&& str) noexcept \
	{ \
		m_container = AllocStd<Container>(Move(str)); \
	}

	DEFINE_STRING_CONSTRUCTOR_IMPL(String, String)
	DEFINE_STRING_CONSTRUCTOR_IMPL(Atomic<String>, Atomic)
	DEFINE_STRING_CONSTRUCTOR_IMPL(String16, String16)
	DEFINE_STRING_CONSTRUCTOR_IMPL(Atomic<String16>, Atomic)
	DEFINE_STRING_CONSTRUCTOR_IMPL(String32, String32)
	DEFINE_STRING_CONSTRUCTOR_IMPL(Atomic<String32>, Atomic)

#define DEFINE_STRING_FUNC_IMPL(STRING) \
	STRING STRING::allocate(sl_size len) noexcept \
	{ \
		return Alloc<Container>(len); \
	} \
	\
	STRING STRING::create(const String& str) noexcept \
	{ \
		if (str.isNotNull()) { \
			return CreateFromString<Container>(str); \
		} \
		return sl_null; \
	} \
	\
	STRING STRING::create(const String16& str) noexcept \
	{ \
		if (str.isNotNull()) { \
			return CreateFromString<Container>(str); \
		} \
		return sl_null; \
	} \
	\
	STRING STRING::create(const String32& str) noexcept \
	{ \
		if (str.isNotNull()) { \
			return CreateFromString<Container>(str); \
		} \
		return sl_null; \
	} \
	\
	STRING STRING::create(const StringView& str) noexcept \
	{ \
		return CreateFromSz<Container>(str.getData(), str.getUnsafeLength()); \
	} \
	\
	STRING STRING::create(const StringView16& str) noexcept \
	{ \
		return CreateFromSz<Container>(str.getData(), str.getUnsafeLength()); \
	} \
	\
	STRING STRING::create(const StringView32& str) noexcept \
	{ \
		return CreateFromSz<Container>(str.getData(), str.getUnsafeLength()); \
	} \
	\
	STRING STRING::create(const char* str, sl_reg length) noexcept \
	{ \
		return CreateFromSz<Container>(str, length); \
	} \
	\
	STRING STRING::create(const wchar_t* str, sl_reg length) noexcept \
	{ \
		if (sizeof(wchar_t) == 2) { \
			return CreateFromSz<Container>((sl_char16*)str, length); \
		} else { \
			return CreateFromSz<Container>((sl_char32*)str, length); \
		} \
	} \
	\
	STRING STRING::create(const char16_t* str, sl_reg length) noexcept \
	{ \
		return CreateFromSz<Container>(str, length); \
	} \
	\
	STRING STRING::create(const char32_t* str, sl_reg length) noexcept \
	{ \
		return CreateFromSz<Container>(str, length); \
	} \
	\
	STRING STRING::create(const std::string& str) noexcept \
	{ \
		return CreateFromSz<Container>(str.c_str(), str.length()); \
	} \
	\
	STRING STRING::create(const std::wstring& str) noexcept \
	{ \
		return create(str.c_str(), str.length()); \
	} \
	\
	STRING STRING::create(const std::u16string& str) noexcept \
	{ \
		return CreateFromSz<Container>(str.c_str(), str.length()); \
	} \
	\
	STRING STRING::create(const std::u32string& str) noexcept \
	{ \
		return CreateFromSz<Container>(str.c_str(), str.length()); \
	} \
	\
	STRING STRING::fromStatic(typename STRING::Char const* sz, sl_reg len) noexcept \
	{ \
		if (sz) { \
			if (len < 0) { \
				len = StringTraits<Char>::getLength(sz); \
			} \
			return AllocStatic<Container>(sz, len); \
		} \
		return sl_null; \
	} \
	\
	STRING STRING::fromRef(CRef* ref, typename STRING::Char const* str, sl_size len) noexcept \
	{ \
		if (str) { \
			return AllocRef<Container>(ref, str, len); \
		} \
		return sl_null; \
	} \
	\
	STRING STRING::fromUtf8(const void* utf8, sl_reg len) noexcept \
	{ \
		return create((const sl_char8*)utf8, len); \
	} \
	\
	STRING STRING::fromUtf8(const MemoryView& mem) noexcept \
	{ \
		return fromUtf8(mem.data, mem.size); \
	} \
	\
	STRING STRING::fromUtf16(const sl_char16* utf16, sl_reg len) noexcept \
	{ \
		return create(utf16, len); \
	} \
	\
	STRING STRING::fromUtf16BE(const void* utf16, sl_size size) noexcept \
	{ \
		return CreateFromUtf16<Container>(Endian::Big, utf16, size); \
	} \
	\
	STRING STRING::fromUtf16BE(const MemoryView& mem) noexcept \
	{ \
		return fromUtf16BE(mem.data, mem.size); \
	} \
	\
	STRING STRING::fromUtf16LE(const void* utf16, sl_size size) noexcept \
	{ \
		return CreateFromUtf16<Container>(Endian::Little, utf16, size); \
	} \
	\
	STRING STRING::fromUtf16LE(const MemoryView& mem) noexcept \
	{ \
		return fromUtf16LE(mem.data, mem.size); \
	} \
	\
	STRING STRING::fromUtf32(const sl_char32* utf32, sl_reg len) noexcept \
	{ \
		return create(utf32, len); \
	} \
	\
	STRING STRING::fromUtf(const void* buf, sl_size size) noexcept \
	{ \
		return CreateFromUtf<Container>(buf, size); \
	} \
	\
	STRING STRING::fromUtf(const MemoryView& mem) noexcept \
	{ \
		return fromUtf(mem.data, mem.size); \
	} \
	\
	STRING STRING::from(const StringView& str) noexcept \
	{ \
		return create(str); \
	} \
	\
	STRING STRING::from(const StringView16& str) noexcept \
	{ \
		return create(str); \
	} \
	\
	STRING STRING::from(const StringView32& str) noexcept \
	{ \
		return create(str); \
	} \
	\
	STRING STRING::from(const char* str, sl_reg length) noexcept \
	{ \
		return create(str, length); \
	} \
	\
	STRING STRING::from(const wchar_t* str, sl_reg length) noexcept \
	{ \
		return create(str, length); \
	} \
	\
	STRING STRING::from(const char16_t* str, sl_reg length) noexcept \
	{ \
		return create(str, length); \
	} \
	\
	STRING STRING::from(const char32_t* str, sl_reg length) noexcept \
	{ \
		return create(str, length); \
	} \
	\
	STRING STRING::from(const std::string& str) noexcept \
	{ \
		return create(str.c_str(), str.length()); \
	} \
	\
	STRING STRING::from(const std::wstring& str) noexcept \
	{ \
		return create(str.c_str(), str.length()); \
	} \
	\
	STRING STRING::from(const std::u16string& str) noexcept \
	{ \
		return create(str.c_str(), str.length()); \
	} \
	\
	STRING STRING::from(const std::u32string& str) noexcept \
	{ \
		return create(str.c_str(), str.length()); \
	} \
	\
	void STRING::setNull() noexcept \
	{ \
		_replaceContainer(sl_null); \
	} \
	\
	void STRING::setLength(sl_size len) noexcept \
	{ \
		if (m_container && m_container != ConstContainers<Container>::getEmpty()) { \
			m_container->len = len; \
		} \
	} \
	\
	typename STRING::Char* STRING::getNullTerminatedData(sl_size& outLength, STRING& outStringConverted) const noexcept \
	{ \
		return GetNullTerminatedData(m_container, outLength, outStringConverted); \
	} \
	\
	void STRING::setHashCode(sl_size hash) noexcept \
	{ \
		if (m_container && m_container != ConstContainers<Container>::getEmpty()) { \
			m_container->hash = hash; \
		} \
	} \
	\
	typename STRING::Char STRING::getFirst() const noexcept \
	{ \
		if (m_container) { \
			if (m_container->len) { \
				return *(m_container->data); \
			} \
		} \
		return 0; \
	} \
	\
	typename STRING::Char STRING::getLast() const noexcept \
	{ \
		if (m_container) { \
			sl_size n = m_container->len; \
			if (n) { \
				return m_container->data[n - 1]; \
			} \
		} \
		return 0; \
	} \
	\
	typename STRING::Char STRING::getAt(sl_reg index) const noexcept \
	{ \
		if (m_container) { \
			if (index >= 0 && index < (sl_reg)(m_container->len)) { \
				return m_container->data[index]; \
			} \
		} \
		return 0; \
	} \
	\
	sl_bool STRING::setAt(sl_reg index, typename STRING::Char ch) noexcept \
	{ \
		if (m_container) { \
			if (index >= 0 && index < (sl_reg)(m_container->len)) { \
				m_container->data[index] = ch; \
				return sl_true; \
			} \
		} \
		return sl_false; \
	} \
	\
	typename STRING::Char STRING::operator[](sl_size index) const noexcept \
	{ \
		return m_container->data[index]; \
	} \
	\
	typename STRING::Char& STRING::operator[](sl_size index) noexcept \
	{ \
		return m_container->data[index]; \
	} \
	typename STRING::StdString STRING::toStd() const noexcept \
	{ \
		if (m_container) { \
			if (m_container->type == STRING_CONTAINER_TYPE_STD) { \
				return (static_cast<StdContainer<Container>*>(m_container))->object; \
			} else { \
				return StdString(m_container->data, m_container->len); \
			} \
		} \
		return StdString(); \
	} \
	\
	STRING& STRING::operator=(sl_null_t) noexcept \
	{ \
		_replaceContainer(sl_null); \
		return *this; \
	} \
	\
	STRING STRING::duplicate() const noexcept \
	{ \
		Container* container = m_container; \
		if (container) { \
			return STRING(container->data, container->len); \
		} \
		return sl_null; \
	} \
	\
	STRING STRING::toNullTerminated() const noexcept \
	{ \
		Container* container = m_container; \
		if (container) { \
			if (container->type == STRING_CONTAINER_TYPE_NORMAL || container->type == STRING_CONTAINER_TYPE_STD) { \
				return *this; \
			} \
			if (container->data[container->len]) { \
				return STRING(container->data, container->len); \
			} else { \
				return *this; \
			} \
		} \
		return sl_null; \
	} \
	\
	Memory STRING::toMemory() const noexcept \
	{ \
		return ToMemory(this, m_container); \
	} \
	\
	STRING STRING::substring(sl_reg start, sl_reg end) const noexcept \
	{ \
		return SubString(this, start, end); \
	} \
	\
	STRING STRING::from(const Atomic<STRING>& str) noexcept \
	{ \
		return str; \
	} \
	\
	STRING STRING::from(typename StringTypeFromCharType<typename OtherCharType<STRING::Char>::Type1>::Type const& str) noexcept \
	{ \
		return create(str); \
	} \
	\
	STRING STRING::from(Atomic<typename StringTypeFromCharType<typename OtherCharType<STRING::Char>::Type1>::Type> const& str) noexcept \
	{ \
		return create(str); \
	} \
	\
	STRING STRING::from(typename StringTypeFromCharType<typename OtherCharType<STRING::Char>::Type2>::Type const& str) noexcept \
	{ \
		return create(str); \
	} \
	\
	STRING STRING::from(Atomic<typename StringTypeFromCharType<typename OtherCharType<STRING::Char>::Type2>::Type> const& str) noexcept \
	{ \
		return create(str); \
	} \
	\
	STRING& STRING::operator+=(STRING&& _other) noexcept \
	{ \
		Container* other = _other.m_container; \
		if (other) { \
			if (isEmpty()) { \
				return *this = Move(_other); \
			} \
			if (other->len) { \
				Append<Container>(m_container, other->data, other->len); \
			} \
		} \
		return *this; \
	} \
	\
	STRING& STRING::operator+=(const STRING& _other) noexcept \
	{ \
		Container* other = _other.m_container; \
		if (other) { \
			if (isEmpty()) { \
				return *this = _other; \
			} \
			if (other->len) { \
				Append<Container>(m_container, other->data, other->len); \
			} \
		} \
		return *this; \
	} \
	\
	STRING& STRING::operator+=(typename STRING::StringViewType const& other) noexcept \
	{ \
		if (other.isNotNull()) { \
			if (isEmpty()) { \
				return *this = other; \
			} \
			if (other.isNotEmpty()) { \
				Append<Container>(m_container, other.getData(), other.getUnsafeLength()); \
			} \
		} \
		return *this; \
	} \
	\
	STRING& STRING::operator+=(typename STRING::Char const* sz) noexcept \
	{ \
		if (sz) { \
			if (isEmpty()) { \
				return *this = sz; \
			} \
			if (*sz) { \
				Append<Container>(m_container, sz, -1); \
			} \
		} \
		return *this; \
	} \
	\
	STRING STRING::operator+(STRING&& _other) const& noexcept \
	{ \
		STRING ret; \
		Container* other = _other.m_container; \
		if (other) { \
			if (isEmpty()) { \
				ret = Move(_other); \
			} else if (other->len) { \
				Container* thiz = m_container; \
				ret = Concat<Container>(thiz->data, thiz->len, other->data, other->len); \
			} else { \
				ret = *this; \
			} \
		} else { \
			ret = *this; \
		} \
		return ret; \
	} \
	\
	STRING STRING::operator+(const STRING& _other) const& noexcept \
	{ \
		Container* other = _other.m_container; \
		if (other) { \
			if (isEmpty()) { \
				return _other; \
			} \
			if (other->len) { \
				Container* thiz = m_container; \
				return Concat<Container>(thiz->data, thiz->len, other->data, other->len); \
			} \
		} \
		return *this; \
	} \
	\
	STRING STRING::operator+(typename STRING::StringViewType const& other) const& noexcept \
	{ \
		if (other.isNotNull()) { \
			if (isEmpty()) { \
				return other; \
			} \
			if (other.isNotEmpty()) { \
				Container* thiz = m_container; \
				return Concat<Container>(thiz->data, thiz->len, other.getData(), other.getUnsafeLength()); \
			} \
		} \
		return *this; \
	} \
	\
	STRING STRING::operator+(typename STRING::Char const* sz) const& noexcept \
	{ \
		if (sz) { \
			if (isEmpty()) { \
				return sz; \
			} \
			if (*sz) { \
				Container* thiz = m_container; \
				return Concat<Container>(thiz->data, thiz->len, sz, -1); \
			} \
		} \
		return *this; \
	} \
	\
	STRING STRING::operator+(STRING&& _other)&& noexcept \
	{ \
		STRING ret; \
		Container* other = _other.m_container; \
		if (other) { \
			if (isEmpty()) { \
				ret = Move(_other); \
			} else { \
				if (other->len) { \
					Append<Container>(m_container, other->data, other->len); \
				} \
				ret = Move(*this); \
			} \
		} else { \
			ret = Move(*this); \
		} \
		return ret; \
	} \
	\
	STRING STRING::operator+(const STRING& _other)&& noexcept \
	{ \
		STRING ret; \
		Container* other = _other.m_container; \
		if (other) { \
			if (isEmpty()) { \
				ret = _other; \
			} else { \
				if (other->len) { \
					Append<Container>(m_container, other->data, other->len); \
				} \
				ret = Move(*this); \
			} \
		} else { \
			ret = Move(*this); \
		} \
		return ret; \
	} \
	\
	STRING STRING::operator+(typename STRING::StringViewType const& other)&& noexcept \
	{ \
		STRING ret; \
		if (other.isNotNull()) { \
			if (isEmpty()) { \
				ret = other; \
			} else { \
				if (other.isNotEmpty()) { \
					Append<Container>(m_container, other.getData(), other.getUnsafeLength()); \
				} \
				ret = Move(*this); \
			} \
		} else { \
			ret = Move(*this); \
		} \
		return ret; \
	} \
	\
	STRING STRING::operator+(typename STRING::Char const* sz)&& noexcept \
	{ \
		STRING ret; \
		if (sz) { \
			if (isEmpty()) { \
				ret = sz; \
			} else { \
				if (*sz) { \
					Append<Container>(m_container, sz, -1); \
				} \
				ret = Move(*this); \
			} \
		} else { \
			ret = Move(*this); \
		} \
		return ret; \
	} \
	\
	STRING operator+(typename STRING::Char const* sz, const STRING& str) noexcept \
	{ \
		if (!sz) { \
			return str; \
		} \
		if (str.isEmpty()) { \
			return sz; \
		} \
		if (!(*sz)) { \
			return str; \
		} \
		typename STRING::Container* container = str.m_container; \
		return Concat<typename STRING::Container>(sz, -1, container->data, container->len); \
	} \
	\
	STRING STRING::operator+(typename STRING::StdString const& other) const noexcept \
	{ \
		if (isEmpty()) { \
			return other; \
		} \
		if (!(other.empty())) { \
			return *this; \
		} \
		Container* thiz = m_container; \
		return Concat<Container>(thiz->data, thiz->len, other.data(), (sl_reg)(other.length())); \
	} \
	\
	STRING operator+(typename STRING::StdString const& other, const STRING& str) noexcept \
	{ \
		if (str.isEmpty()) { \
			return other; \
		} \
		if (!(other.empty())) { \
			return str; \
		} \
		typename STRING::Container* container = str.m_container; \
		return Concat<typename STRING::Container>(other.data(), (sl_reg)(other.length()), container->data, container->len); \
	} \
	\
	STRING STRING::concat(const StringParam& s1, const StringParam& s2) noexcept \
	{ \
		return ConcatParams<STRING>(s1, s2); \
	} \
	sl_bool operator==(typename STRING::Char const* sz, const STRING& str) noexcept { return str.equals(sz); } \
	sl_bool operator!=(typename STRING::Char const* sz, const STRING& str) noexcept { return !(str.equals(sz)); } \
	sl_compare_result operator>=(typename STRING::Char const* sz, const STRING& str) { return str.compare(sz) <= 0; } \
	sl_compare_result operator<=(typename STRING::Char const* sz, const STRING& str) { return str.compare(sz) >= 0; } \
	sl_compare_result operator>(typename STRING::Char const* sz, const STRING& str) { return str.compare(sz) < 0; } \
	sl_compare_result operator<(typename STRING::Char const* sz, const STRING& str) { return str.compare(sz) > 0; } \
	sl_bool operator==(typename STRING::StdString const& s1, const STRING& s2) noexcept { return s2.equals(s1); } \
	sl_bool operator!=(typename STRING::StdString const& s1, const STRING& s2) noexcept { return !(s2.equals(s1)); } \
	sl_compare_result operator>=(typename STRING::StdString const& s1, const STRING& s2) { return s2.compare(s1) <= 0; } \
	sl_compare_result operator<=(typename STRING::StdString const& s1, const STRING& s2) { return s2.compare(s1) >= 0; } \
	sl_compare_result operator>(typename STRING::StdString const& s1, const STRING& s2) { return s2.compare(s1) < 0; } \
	sl_compare_result operator<(typename STRING::StdString const& s1, const STRING& s2) { return s2.compare(s1) > 0; } \
	\
	sl_bool STRING::equals(const STRING& other) const noexcept \
	{ \
		return EqualsString(*this, other); \
	} \
	\
	sl_bool STRING::equals(typename STRING::StringViewType const& other) const noexcept \
	{ \
		return EqualsStringSz(*this, other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_bool STRING::equals(typename STRING::Char const* sz) const noexcept \
	{ \
		return EqualsStringSz(*this, sz, -1); \
	} \
	\
	sl_bool STRING::equals(typename STRING::StdString const& other) const noexcept \
	{ \
		return EqualsString(*this, other.data(), other.length()); \
	} \
	\
	sl_bool STRING::equals_IgnoreCase(typename STRING::StringViewType const& other) const noexcept \
	{ \
		return EqualsStringSz_IgnoreCase(*this, other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_compare_result STRING::compare(typename STRING::StringViewType const& other) const noexcept \
	{ \
		return CompareStringSz(*this, other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_compare_result STRING::compare_IgnoreCase(typename STRING::StringViewType const& other) const noexcept \
	{ \
		return CompareStringSz_IgnoreCase(*this, other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_compare_result STRING::compare(typename STRING::StringViewType const& other, sl_size len) const noexcept \
	{ \
		return CompareStringSzLimited(*this, other.getData(), other.getUnsafeLength(), len); \
	} \
	\
	sl_compare_result STRING::compare_IgnoreCase(typename STRING::StringViewType const& other, sl_size len) const noexcept \
	{ \
		return CompareStringSzLimited_IgnoreCase(*this, other.getData(), other.getUnsafeLength(), len); \
	} \
	\
	sl_size STRING::getHashCode() const noexcept \
	{ \
		if (m_container) { \
			sl_size n = m_container->len; \
			if (n > 0) { \
				sl_size hash = m_container->hash; \
				if (!hash) { \
					hash = GetHashCode(m_container->data, n); \
					m_container->hash = hash; \
				} \
				return hash; \
			} \
		} \
		return 0; \
	} \
	\
	sl_size STRING::getHashCode(typename STRING::Char const* str, sl_reg len) noexcept \
	{ \
		return GetHashCode(str, len); \
	} \
	\
	sl_size STRING::getHashCode_IgnoreCase() const noexcept \
	{ \
		if (m_container) { \
			return GetHashCode_IgnoreCase(m_container->data, m_container->len); \
		} \
		return 0; \
	} \
	\
	sl_size STRING::getHashCode_IgnoreCase(typename STRING::Char const* str, sl_reg len) noexcept \
	{ \
		return GetHashCode_IgnoreCase(str, len); \
	} \
	\
	STRING STRING::left(sl_reg len) const noexcept \
	{ \
		return Left(this, len); \
	} \
	\
	STRING STRING::right(sl_reg len) const noexcept \
	{ \
		return Right(this, len); \
	} \
	\
	STRING STRING::mid(sl_reg start, sl_reg len) const noexcept \
	{ \
		return Mid(this, start, len); \
	} \
	\
	sl_reg STRING::indexOf(typename STRING::Char ch, sl_reg start) const noexcept \
	{ \
		return IndexOfChar(*this, ch, start); \
	} \
	\
	sl_reg STRING::indexOf(typename STRING::StringViewType const& pattern, sl_reg start) const noexcept \
	{ \
		return IndexOf(*this, pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_reg STRING::indexOf_IgnoreCase(typename STRING::StringViewType const& pattern, sl_reg start) const noexcept \
	{ \
		return IndexOf_IgnoreCase(*this, pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_reg STRING::lastIndexOf(typename STRING::Char ch, sl_reg start) const noexcept \
	{ \
		return LastIndexOfChar(*this, ch, start); \
	} \
	\
	sl_reg STRING::lastIndexOf(typename STRING::StringViewType const& pattern, sl_reg start) const noexcept \
	{ \
		return LastIndexOf(*this, pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_reg STRING::lastIndexOf_IgnoreCase(typename STRING::StringViewType const& pattern, sl_reg start) const noexcept \
	{ \
		return LastIndexOf_IgnoreCase(*this, pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_bool STRING::startsWith(typename STRING::Char ch) const noexcept \
	{ \
		return StartsWithChar(*this, ch); \
	} \
	\
	sl_bool STRING::startsWith(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return StartsWithString(*this, pattern.getData(), pattern.getUnsafeLength()); \
	} \
	\
	sl_bool STRING::startsWith_IgnoreCase(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return StartsWithString_IgnoreCase(*this, pattern.getData(), pattern.getUnsafeLength()); \
	} \
	\
	sl_bool STRING::endsWith(typename STRING::Char ch) const noexcept \
	{ \
		return EndsWithChar(*this, ch); \
	} \
	\
	sl_bool STRING::endsWith(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return EndsWith(*this, pattern.getData(), pattern.getLength()); \
	} \
	\
	sl_bool STRING::endsWith_IgnoreCase(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return EndsWith_IgnoreCase(*this, pattern.getData(), pattern.getLength()); \
	} \
	\
	sl_bool STRING::contains(typename STRING::Char ch) const noexcept \
	{ \
		return indexOf(ch) >= 0; \
	} \
	\
	sl_bool STRING::contains(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return indexOf(pattern) >= 0; \
	} \
	\
	sl_bool STRING::contains_IgnoreCase(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return indexOf_IgnoreCase(pattern) >= 0; \
	} \
	\
	sl_size STRING::countOf(typename STRING::Char ch) const noexcept \
	{ \
		return CountOfChar(*this, ch); \
	} \
	\
	sl_size STRING::countOf(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return CountOf(*this, pattern.getData(), pattern.getLength()); \
	} \
	\
	sl_size STRING::countOf_IgnoreCase(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return CountOf_IgnoreCase(*this, pattern.getData(), pattern.getLength()); \
	} \
	\
	void STRING::makeUpper() noexcept \
	{ \
		MakeUpperString(*this); \
	} \
	\
	void STRING::makeLower() noexcept \
	{ \
		MakeLowerString(*this); \
	} \
	\
	STRING STRING::toUpper() const noexcept \
	{ \
		return CreateUpperString(*this); \
	} \
	\
	STRING STRING::toLower() const noexcept \
	{ \
		return CreateLowerString(*this); \
	} \
	\
	STRING STRING::replaceAll(typename STRING::Char pattern, typename STRING::Char replacement) const noexcept \
	{ \
		return ReplaceChar(*this, pattern, replacement); \
	} \
	\
	STRING STRING::replaceAll(typename STRING::StringViewType const& pattern, typename STRING::StringViewType const& replacement) const noexcept \
	{ \
		return ReplaceAll(*this, pattern.getData(), pattern.getLength(), replacement.getData(), replacement.getLength()); \
	} \
	\
	STRING STRING::removeAll(typename STRING::Char pattern) const noexcept \
	{ \
		return ReplaceChar(*this, pattern, 0); \
	} \
	\
	STRING STRING::removeAll(typename STRING::StringViewType const& pattern) const noexcept \
	{ \
		return ReplaceAll(*this, pattern.getData(), pattern.getLength(), (typename STRING::Char*)sl_null, 0); \
	} \
	\
	STRING STRING::trim() const noexcept \
	{ \
		return Trim(*this); \
	} \
	\
	STRING STRING::trimLeft() const noexcept \
	{ \
		return TrimLeft(*this); \
	} \
	\
	STRING STRING::trimRight() const noexcept \
	{ \
		return TrimRight(*this); \
	} \
	\
	STRING STRING::trimLine() const noexcept \
	{ \
		return TrimLine(*this); \
	} \
	\
	void STRING::makeReverse() noexcept \
	{ \
		MakeReverse(*this); \
	} \
	\
	STRING STRING::reverse() const noexcept \
	{ \
		return CreateReverseString(*this); \
	} \
	\
	List<STRING> STRING::split(typename STRING::StringViewType const& pattern, sl_reg nMaxSplit) const noexcept \
	{ \
		return Split(*this, pattern.getData(), pattern.getLength(), nMaxSplit); \
	} \
	\
	List<STRING> STRING::split(typename STRING::Char pattern, sl_reg nMaxSplit) const noexcept \
	{ \
		return Split(*this, pattern, nMaxSplit); \
	} \
	\
	STRING STRING::join(const STRING* strings, sl_size count, typename STRING::StringViewType const& delimiter) noexcept \
	{ \
		return Join(strings, count, delimiter); \
	} \
	\
	STRING STRING::join(const STRING* strings, sl_size count) noexcept \
	{ \
		return Join(strings, count); \
	} \
	\
	STRING STRING::join(typename STRING::StringViewType const* strings, sl_size count, typename STRING::StringViewType const& delimiter) noexcept \
	{ \
		return Join(strings, count, delimiter); \
	} \
	\
	STRING STRING::join(typename STRING::StringViewType const* strings, sl_size count) noexcept \
	{ \
		return Join(strings, count); \
	} \
	\
	STRING STRING::join(const StringParam* strings, sl_size count, typename STRING::StringViewType const& delimiter) noexcept \
	{ \
		return JoinParams<STRING>(strings, count, delimiter); \
	} \
	\
	STRING STRING::join(const StringParam* strings, sl_size count) noexcept \
	{ \
		return JoinParams<STRING>(strings, count); \
	} \
	\
	STRING STRING::join(const ListParam<STRING>& list, typename STRING::StringViewType const& delimiter) noexcept \
	{ \
		ListLocker<STRING> items(list); \
		return Join(items.data, items.count, delimiter); \
	} \
	\
	STRING STRING::join(const ListParam<STRING>& list) noexcept \
	{ \
		ListLocker<STRING> items(list); \
		return Join(items.data, items.count); \
	} \
	\
	STRING STRING::join(const ListParam<typename STRING::StringViewType>& list, typename STRING::StringViewType const& delimiter) noexcept \
	{ \
		ListLocker<typename STRING::StringViewType> items(list); \
		return Join(items.data, items.count, delimiter); \
	} \
	\
	STRING STRING::join(const ListParam<typename STRING::StringViewType>& list) noexcept \
	{ \
		ListLocker<typename STRING::StringViewType> items(list); \
		return Join(items.data, items.count); \
	} \
	\
	STRING STRING::join(const ListParam<StringParam>& list, typename STRING::StringViewType const& delimiter) noexcept \
	{ \
		ListLocker<StringParam> items(list); \
		return JoinParams<STRING>(items.data, items.count, delimiter); \
	} \
	\
	STRING STRING::join(const ListParam<StringParam>& list) noexcept \
	{ \
		ListLocker<StringParam> items(list); \
		return JoinParams<STRING>(items.data, items.count); \
	} \
	\
	sl_reg STRING::parseInt32(sl_int32 radix, sl_int32* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseInt(radix, str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseInt32(sl_int32 radix, sl_int32* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseInt(radix, getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	sl_bool STRING::parseInt32(sl_int32* _out) const noexcept \
	{ \
		return parseInt32(10, _out); \
	} \
	\
	sl_int32 STRING::parseInt32(sl_int32 radix, sl_int32 def) const noexcept \
	{ \
		sl_int32 _out = 0; \
		if (parseInt32(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_reg STRING::parseUint32(sl_int32 radix, sl_uint32* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseUint(radix, str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseUint32(sl_int32 radix, sl_uint32* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseUint(radix, getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	sl_bool STRING::parseUint32(sl_uint32* _out) const noexcept \
	{ \
		return parseUint32(10, _out); \
	} \
	\
	sl_uint32 STRING::parseUint32(sl_int32 radix, sl_uint32 def) const noexcept \
	{ \
		sl_uint32 _out = 0; \
		if (parseUint32(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_reg STRING::parseInt64(sl_int32 radix, sl_int64* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseInt(radix, str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseInt64(sl_int32 radix, sl_int64* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseInt(radix, getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	sl_bool STRING::parseInt64(sl_int64* _out) const noexcept \
	{ \
		return parseInt64(10, _out); \
	} \
	\
	sl_int64 STRING::parseInt64(sl_int32 radix, sl_int64 def) const noexcept \
	{ \
		sl_int64 _out = 0; \
		if (parseInt64(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_reg STRING::parseUint64(sl_int32 radix, sl_uint64* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseUint(radix, str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseUint64(sl_int32 radix, sl_uint64* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseUint(radix, getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	sl_bool STRING::parseUint64(sl_uint64* _out) const noexcept \
	{ \
		return parseUint64(10, _out); \
	} \
	\
	sl_uint64 STRING::parseUint64(sl_int32 radix, sl_uint64 def) const noexcept \
	{ \
		sl_uint64 _out = 0; \
		if (parseUint64(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool STRING::parseInt(sl_int32 radix, sl_reg* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseInt(radix, getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	sl_bool STRING::parseInt(sl_reg* _out) const noexcept \
	{ \
		return parseInt(10, _out); \
	} \
	\
	sl_reg STRING::parseInt(sl_int32 radix, sl_reg def) const noexcept \
	{ \
		sl_reg _out = 0; \
		if (parseInt(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool STRING::parseSize(sl_int32 radix, sl_size* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseUint(radix, getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	sl_bool STRING::parseSize(sl_size* _out) const noexcept \
	{ \
		return parseSize(10, _out); \
	} \
	\
	sl_size STRING::parseSize(sl_int32 radix, sl_size def) const noexcept \
	{ \
		sl_size _out = 0; \
		if (parseSize(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_reg STRING::parseFloat(float* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseFloat(str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseFloat(float* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseFloat(getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	float STRING::parseFloat(float def) const noexcept \
	{ \
		float _out = 0; \
		if (parseFloat(&_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_reg STRING::parseDouble(double* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseFloat(str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseDouble(double* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseFloat(getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	double STRING::parseDouble(double def) const noexcept \
	{ \
		double _out = 0; \
		if (parseDouble(&_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_reg STRING::parseBoolean(sl_bool* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseBoolean(str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseBoolean(sl_bool* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseBoolean(getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	sl_bool STRING::parseBoolean(sl_bool def) const noexcept \
	{ \
		sl_bool _out = 0; \
		if (parseBoolean(&_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_reg STRING::parseHexString(void* _out, typename STRING::Char const* str, sl_size posBegin, sl_size posEnd) noexcept \
	{ \
		return ParseHexString(str, posBegin, posEnd, _out); \
	} \
	\
	sl_bool STRING::parseHexString(void* _out) const noexcept \
	{ \
		sl_size n = getLength(); \
		if (!n) { \
			return sl_false; \
		} \
		return ParseHexString(getData(), 0, n, _out) == (sl_reg)n; \
	} \
	\
	Memory STRING::parseHexString() const noexcept \
	{ \
		return ParseHexString(*this); \
	} \
	\
	STRING STRING::fromInt32(sl_int32 value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase) noexcept \
	{ \
		return FromInt<sl_int32, typename STRING::Char>(value, radix, minWidth, flagUpperCase); \
	} \
	\
	STRING STRING::fromUint32(sl_uint32 value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase) noexcept \
	{ \
		return FromUint<sl_uint32, typename STRING::Char>(value, radix, minWidth, flagUpperCase); \
	} \
	\
	STRING STRING::fromInt64(sl_int64 value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase) noexcept \
	{ \
		return FromInt<sl_int64, typename STRING::Char>(value, radix, minWidth, flagUpperCase); \
	} \
	\
	STRING STRING::fromUint64(sl_uint64 value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase) noexcept \
	{ \
		return FromUint<sl_uint64, typename STRING::Char>(value, radix, minWidth, flagUpperCase); \
	} \
	\
	STRING STRING::fromInt(sl_reg value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase) noexcept \
	{ \
		return FromInt<sl_reg, typename STRING::Char>(value, radix, minWidth, flagUpperCase); \
	} \
	\
	STRING STRING::fromSize(sl_size value, sl_uint32 radix, sl_uint32 minWidth, sl_bool flagUpperCase) noexcept \
	{ \
		return FromUint<sl_reg, typename STRING::Char>(value, radix, minWidth, flagUpperCase); \
	} \
	\
	STRING STRING::fromFloat(float value, sl_int32 precision, sl_bool flagZeroPadding, sl_uint32 minWidthIntegral) noexcept \
	{ \
		return FromFloat<float, typename STRING::Char>(value, precision, flagZeroPadding, minWidthIntegral); \
	} \
	\
	STRING STRING::fromDouble(double value, sl_int32 precision, sl_bool flagZeroPadding, sl_uint32 minWidthIntegral) noexcept \
	{ \
		return FromFloat<double, typename STRING::Char>(value, precision, flagZeroPadding, minWidthIntegral); \
	} \
	\
	STRING STRING::fromPointerValue(const void* pointer) noexcept \
	{ \
		return fromSize((sl_size)(pointer), 16, sizeof(sl_size) << 1, sl_true); \
	} \
	\
	STRING STRING::makeHexString(const void* buf, sl_size size, sl_bool flagUseLowerChar) noexcept \
	{ \
		return MakeHexString<typename STRING::Char>(buf, size, flagUseLowerChar); \
	} \
	\
	STRING STRING::makeHexString(const MemoryView& mem, sl_bool flagUseLowerChar) noexcept \
	{ \
		return makeHexString(mem.data, mem.size, flagUseLowerChar); \
	} \
	\
	STRING STRING::from(signed char value) noexcept \
	{ \
		return fromInt32(value); \
	} \
	\
	STRING STRING::from(unsigned char value) noexcept \
	{ \
		return fromUint32(value); \
	} \
	\
	STRING STRING::from(short value) noexcept \
	{ \
		return fromInt32(value); \
	} \
	\
	STRING STRING::from(unsigned short value) noexcept \
	{ \
		return fromUint32(value); \
	} \
	\
	STRING STRING::from(int value) noexcept \
	{ \
		return fromInt32((sl_int32)value); \
	} \
	\
	STRING STRING::from(unsigned int value) noexcept \
	{ \
		return fromUint32((sl_uint32)value); \
	} \
	\
	STRING STRING::from(long value) noexcept \
	{ \
		return fromInt32((sl_int32)value); \
	} \
	\
	STRING STRING::from(unsigned long value) noexcept \
	{ \
		return fromUint32((sl_uint32)value); \
	} \
	\
	STRING STRING::from(sl_int64 value) noexcept \
	{ \
		return fromInt64(value); \
	} \
	\
	STRING STRING::from(sl_uint64 value) noexcept \
	{ \
		return fromUint64(value); \
	} \
	\
	STRING STRING::from(float value) noexcept \
	{ \
		return fromFloat(value); \
	} \
	\
	STRING STRING::from(double value) noexcept \
	{ \
		return fromDouble(value); \
	} \
	\
	STRING STRING::from(sl_bool value) noexcept \
	{ \
		return fromBoolean(value); \
	} \
	\
	STRING STRING::formatBy(typename STRING::StringViewType const& format, const Variant* params, sl_size nParams) noexcept \
	{\
		return Format(Locale::Unknown, format, params, nParams); \
	} \
	\
	STRING STRING::formatBy(typename STRING::StringViewType const& format, const ListParam<Variant>& _params) noexcept \
	{ \
		ListLocker<Variant> params(_params); \
		return Format(Locale::Unknown, format, params.data, params.count); \
	} \
		\
	STRING STRING::formatBy(const Locale& locale, typename STRING::StringViewType const& format, const Variant* params, sl_size nParams) noexcept \
	{\
		return Format(locale, format, params, nParams); \
	} \
	\
	STRING STRING::formatBy(const Locale& locale, typename STRING::StringViewType const& format, const ListParam<Variant>& _params) noexcept \
	{ \
		ListLocker<Variant> params(_params); \
		return Format(locale, format, params.data, params.count); \
	} \
	\
	STRING STRING::format(typename STRING::StringViewType const& strFormat) noexcept \
	{ \
		return strFormat; \
	} \
	\
	signed char Cast<STRING, signed char>::operator()(const STRING& v) const noexcept \
	{ \
		return (signed char)(v.parseInt32()); \
	} \
	\
	unsigned char Cast<STRING, unsigned char>::operator()(const STRING& v) const noexcept \
	{ \
		return (unsigned char)(v.parseUint32()); \
	} \
	\
	short Cast<STRING, short>::operator()(const STRING& v) const noexcept \
	{ \
		return (short)(v.parseInt32()); \
	} \
	\
	unsigned short Cast<STRING, unsigned short>::operator()(const STRING& v) const noexcept \
	{ \
		return (unsigned short)(v.parseUint32()); \
	} \
	\
	int Cast<STRING, int>::operator()(const STRING& v) const noexcept \
	{ \
		return (int)(v.parseInt32()); \
	} \
	\
	unsigned int Cast<STRING, unsigned int>::operator()(const STRING& v) const noexcept \
	{ \
		return (unsigned int)(v.parseUint32()); \
	} \
	\
	long Cast<STRING, long>::operator()(const STRING& v) const noexcept \
	{ \
		return (long)(v.parseInt32()); \
	} \
	\
	unsigned long Cast<STRING, unsigned long>::operator()(const STRING& v) const noexcept \
	{ \
		return (unsigned long)(v.parseUint32()); \
	} \
	\
	sl_int64 Cast<STRING, sl_int64>::operator()(const STRING& v) const noexcept \
	{ \
		return v.parseInt64(); \
	} \
	\
	sl_uint64 Cast<STRING, sl_uint64>::operator()(const STRING& v) const noexcept \
	{ \
		return v.parseUint64(); \
	} \
	\
	float Cast<STRING, float>::operator()(const STRING& v) const noexcept \
	{ \
		return v.parseFloat(); \
	} \
	\
	double Cast<STRING, double>::operator()(const STRING& v) const noexcept \
	{ \
		return v.parseDouble(); \
	} \
	\
	typename STRING::StdString Cast<STRING, typename STRING::StdString>::operator()(const STRING& v) const noexcept \
	{ \
		return v.toStd(); \
	} \
	\
	STRING Cast<StringParam, STRING>::operator()(const StringParam& v) const noexcept \
	{ \
		return STRING::from(v); \
	} \
	\
	StringParam Cast<STRING, StringParam>::operator()(const STRING& v) const noexcept \
	{ \
		return v; \
	} \

	DEFINE_STRING_FUNC_IMPL(String)
	DEFINE_STRING_FUNC_IMPL(String16)
	DEFINE_STRING_FUNC_IMPL(String32)

#define DEFINE_ATOMIC_STRING_FUNC_IMPL(STRING) \
	void Atomic<STRING>::setNull() noexcept \
	{ \
		if (m_container) { \
			_replaceContainer(sl_null); \
		} \
	} \
	STRING Atomic<STRING>::release() noexcept \
	{ \
		return _releaseContainer(); \
	} \
	\
	Atomic<STRING>& Atomic<STRING>::operator=(sl_null_t) noexcept \
	{ \
		if (m_container) { \
			_replaceContainer(sl_null); \
		} \
		return *this; \
	}

DEFINE_ATOMIC_STRING_FUNC_IMPL(String)
DEFINE_ATOMIC_STRING_FUNC_IMPL(String16)
DEFINE_ATOMIC_STRING_FUNC_IMPL(String32)

#define DEFINE_COMMON_STRING_FUNC_IMPL(STRING) \
	void STRING::setEmpty() noexcept \
	{ \
		if (m_container != ConstContainers<Container>::getEmpty()) { \
			_replaceContainer(ConstContainers<Container>::getEmpty()); \
		} \
	} \
	\
	STRING& STRING::operator=(typename RemoveAtomic<STRING>::Type&& other) noexcept \
	{ \
		if ((void*)this != (void*)&other) { \
			_replaceContainer(other.m_container); \
			other.m_container = sl_null; \
		} \
		return *this; \
	} \
	\
	STRING& STRING::operator=(typename RemoveAtomic<STRING>::Type const& other) noexcept \
	{ \
		Container* container = other.m_container; \
		if (m_container != container) { \
			if (container) { \
				container->increaseReference(); \
			} \
			_replaceContainer(container); \
		} \
		return *this; \
	} \
	\
	STRING& STRING::operator=(typename AddAtomic<STRING>::Type const& other) noexcept \
	{ \
		if (m_container != other.m_container) { \
			_replaceContainer(other._retainContainer()); \
		} \
		return *this; \
	} \
	\
	STRING& STRING::operator=(typename AddAtomic<STRING>::Type&& other) noexcept \
	{ \
		if (m_container != other.m_container) { \
			_replaceContainer(other._releaseContainer()); \
		} \
		return *this; \
	} \
	\
	STRING& STRING::operator=(typename STRING::StringViewType const& other) noexcept \
	{ \
		_replaceContainer(CreateFromSz<Container>(other.getData(), other.getUnsafeLength())); \
		return *this; \
	} \
	\
	STRING& STRING::operator=(typename STRING::Char const* str) noexcept \
	{ \
		_replaceContainer(CreateFromSz<Container>(str, -1)); \
		return *this; \
	} \
	\
	STRING& STRING::operator=(typename STRING::StdString const& other) noexcept \
	{ \
		_replaceContainer(AllocStd<Container>(other)); \
		return *this; \
	} \
	\
	STRING& STRING::operator=(typename STRING::StdString&& other) noexcept \
	{ \
		_replaceContainer(AllocStd<Container>(Move(other))); \
		return *this; \
	}

DEFINE_COMMON_STRING_FUNC_IMPL(String)
DEFINE_COMMON_STRING_FUNC_IMPL(Atomic<String>)
DEFINE_COMMON_STRING_FUNC_IMPL(String16)
DEFINE_COMMON_STRING_FUNC_IMPL(Atomic<String16>)
DEFINE_COMMON_STRING_FUNC_IMPL(String32)
DEFINE_COMMON_STRING_FUNC_IMPL(Atomic<String32>)

	String ToString(const Atomic<String>& str) noexcept
	{
		return str;
	}

	String ToString(const StringView& str) noexcept
	{
		return str;
	}

#define DEFINE_TO_STRING(TYPE) \
	String ToString(TYPE str) noexcept \
	{ \
		return String::from(str); \
	}

	DEFINE_TO_STRING(const String16&)
	DEFINE_TO_STRING(const Atomic<String16>&)
	DEFINE_TO_STRING(const String32&)
	DEFINE_TO_STRING(const Atomic<String32>&)
	DEFINE_TO_STRING(const StringView16&)
	DEFINE_TO_STRING(const StringView32&)
	DEFINE_TO_STRING(const StringParam&)
	DEFINE_TO_STRING(const char*)
	DEFINE_TO_STRING(const wchar_t*)
	DEFINE_TO_STRING(const char16_t*)
	DEFINE_TO_STRING(const char32_t*)
	DEFINE_TO_STRING(const std::string&)
	DEFINE_TO_STRING(const std::wstring&)
	DEFINE_TO_STRING(const std::u16string&)
	DEFINE_TO_STRING(const std::u32string&)
	DEFINE_TO_STRING(signed char)
	DEFINE_TO_STRING(unsigned char)
	DEFINE_TO_STRING(short)
	DEFINE_TO_STRING(unsigned short)
	DEFINE_TO_STRING(int)
	DEFINE_TO_STRING(unsigned int)
	DEFINE_TO_STRING(long)
	DEFINE_TO_STRING(unsigned long)
	DEFINE_TO_STRING(sl_int64)
	DEFINE_TO_STRING(sl_uint64)
	DEFINE_TO_STRING(float)
	DEFINE_TO_STRING(double)
	DEFINE_TO_STRING(sl_bool)
	DEFINE_TO_STRING(const Time&)
	DEFINE_TO_STRING(const Variant&)


#define DEFINE_STRING_VIEW_FUNC_IMPL(VIEW) \
	VIEW::VIEW(typename VIEW::StringType const& value) noexcept \
	{ \
		data = value.getData(*((sl_size*)&length)); \
	} \
	\
	VIEW::VIEW(typename VIEW::Char const* str) noexcept \
	{ \
		data = (Char*)str; \
		if (str) { \
			length = -1; \
		} else { \
			length = 0; \
		} \
	} \
	\
	VIEW::VIEW(typename VIEW::Char const* str, sl_reg _length) noexcept \
	{ \
		data = (Char*)str; \
		if (str) { \
			length = _length; \
		} else { \
			length = 0; \
		} \
	} \
	\
	VIEW::VIEW(typename VIEW::StdString const& str) noexcept: data((Char*)(str.c_str())), length((sl_reg)(str.length())) {} \
	\
	const VIEW& VIEW::null() noexcept \
	{ \
		return *((VIEW*)((void*)&g_stringView_null)); \
	} \
	\
	const VIEW& VIEW::getEmpty() noexcept \
	{ \
		return *((VIEW*)((void*)&g_stringView_empty)); \
	} \
	\
	void VIEW::setNull() noexcept \
	{ \
		data = sl_null; \
		length = 0; \
	} \
	\
	VIEW& VIEW::operator=(sl_null_t) noexcept \
	{ \
		data = sl_null; \
		length = 0; \
		return *this; \
	} \
	\
	VIEW& VIEW::operator=(const VIEW& other) noexcept \
	{ \
		data = other.data; \
		length = other.length; \
		return *this; \
	} \
	\
	VIEW& VIEW::operator=(typename VIEW::StringType const& value) noexcept \
	{ \
		data = value.getData(*((sl_size*)&length)); \
		return *this; \
	} \
	\
	VIEW& VIEW::operator=(typename VIEW::Char const* str) noexcept \
	{ \
		data = (Char*)str; \
		if (str) { \
			length = -1; \
		} else { \
			length = 0; \
		} \
		return *this; \
	} \
	\
	VIEW& VIEW::operator=(typename VIEW::StdString const& str) noexcept \
	{ \
		data = (Char*)(str.c_str()); \
		length = (sl_reg)(str.length()); \
		return *this; \
	} \
	\
	typename VIEW::Char* VIEW::getData(sl_size& outLength) const noexcept \
	{ \
		if (data) { \
			if (length < 0) { \
				length = StringTraits<Char>::getLength(data); \
			} \
			outLength = length; \
			return data; \
		} else { \
			outLength = 0; \
			return sl_null; \
		} \
	} \
	\
	sl_size VIEW::getLength() const noexcept \
	{ \
		if (length < 0) { \
			length = StringTraits<Char>::getLength(data); \
		} \
		return length; \
	} \
	\
	typename VIEW::StringType VIEW::operator+(const VIEW& other) const noexcept \
	{ \
		return Concat<typename StringType::Container>(getData(), getUnsafeLength(), other.getData(), other.getUnsafeLength()); \
	} \
	\
	typename VIEW::StringType VIEW::operator+(typename VIEW::StringType const& other) const noexcept \
	{ \
		if (isEmpty()) { \
			return other.getNotNull(); \
		} \
		return Concat<typename StringType::Container>(getData(), getUnsafeLength(), other.getData(), other.getLength()); \
	} \
	\
	typename VIEW::StringType VIEW::operator+(typename VIEW::Char const* sz) const noexcept \
	{ \
		return Concat<typename StringType::Container>(getData(), getUnsafeLength(), sz, -1); \
	} \
	\
	typename VIEW::StringType operator+(typename VIEW::Char const* sz, const VIEW& str) noexcept \
	{ \
		return Concat<typename VIEW::StringType::Container>(sz, -1, str.getData(), str.getUnsafeLength()); \
	} \
	\
	typename VIEW::StringType VIEW::operator+(typename VIEW::StdString const& other) const noexcept \
	{ \
		return Concat<typename StringType::Container>(getData(), getUnsafeLength(), other.data(), (sl_reg)(other.length())); \
	} \
	\
	typename VIEW::StringType operator+(typename VIEW::StdString const& other, const VIEW& str) noexcept \
	{ \
		return Concat<typename VIEW::StringType::Container>(other.data(), (sl_reg)(other.length()), str.getData(), str.getUnsafeLength()); \
	} \
	\
	sl_bool operator==(typename VIEW::Char const* sz, const VIEW& str) noexcept { return str.equals(sz); } \
	sl_bool operator!=(typename VIEW::Char const* sz, const VIEW& str) noexcept { return !(str.equals(sz)); } \
	sl_compare_result operator>=(typename VIEW::Char const* sz, const VIEW& str) { return str.compare(sz) <= 0; } \
	sl_compare_result operator<=(typename VIEW::Char const* sz, const VIEW& str) { return str.compare(sz) >= 0; } \
	sl_compare_result operator>(typename VIEW::Char const* sz, const VIEW& str) { return str.compare(sz) < 0; } \
	sl_compare_result operator<(typename VIEW::Char const* sz, const VIEW& str) { return str.compare(sz) > 0; } \
	sl_bool operator==(typename VIEW::StdString const& s1, const VIEW& s2) noexcept { return s2.equals(s1); } \
	sl_bool operator!=(typename VIEW::StdString const& s1, const VIEW& s2) noexcept { return !(s2.equals(s1)); } \
	sl_compare_result operator>=(typename VIEW::StdString const& s1, const VIEW& s2) { return s2.compare(s1) <= 0; } \
	sl_compare_result operator<=(typename VIEW::StdString const& s1, const VIEW& s2) { return s2.compare(s1) >= 0; } \
	sl_compare_result operator>(typename VIEW::StdString const& s1, const VIEW& s2) { return s2.compare(s1) < 0; } \
	sl_compare_result operator<(typename VIEW::StdString const& s1, const VIEW& s2) { return s2.compare(s1) > 0; } \
	\
	sl_bool VIEW::equals(const VIEW& other) const noexcept \
	{ \
		return EqualsSz(getData(), getUnsafeLength(), other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_bool VIEW::equals_IgnoreCase(const VIEW& other) const noexcept \
	{ \
		return EqualsSz_IgnoreCase(getData(), getUnsafeLength(), other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_compare_result VIEW::compare(const VIEW& other) const noexcept \
	{ \
		return CompareSz(getData(), getUnsafeLength(), other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_compare_result VIEW::compare_IgnoreCase(const VIEW& other) const noexcept \
	{ \
		return CompareSz_IgnoreCase(getData(), getUnsafeLength(), other.getData(), other.getUnsafeLength()); \
	} \
	\
	sl_compare_result VIEW::compare(const VIEW& other, sl_size len) const noexcept \
	{ \
		return CompareSzLimited(getData(), getUnsafeLength(), other.getData(), other.getUnsafeLength(), len); \
	} \
	\
	sl_compare_result VIEW::compare_IgnoreCase(const VIEW& other, sl_size len) const noexcept \
	{ \
		return CompareSzLimited_IgnoreCase(getData(), getUnsafeLength(), other.getData(), other.getUnsafeLength(), len); \
	} \
	\
	sl_size VIEW::getHashCode() const noexcept \
	{ \
		return GetHashCode(data, length); \
	} \
	\
	sl_size VIEW::getHashCode_IgnoreCase() const noexcept \
	{ \
		return GetHashCode_IgnoreCase(data, length); \
	} \
	\
	VIEW VIEW::substring(sl_reg start, sl_reg end) const noexcept\
	{ \
		return SubString(this, start, end); \
	} \
	\
	VIEW VIEW::left(sl_reg len) const noexcept \
	{ \
		return Left(this, len); \
	} \
	\
	VIEW VIEW::right(sl_reg len) const noexcept \
	{ \
		return Right(this, len); \
	} \
	\
	VIEW VIEW::mid(sl_reg start, sl_reg len) const noexcept \
	{ \
		return Mid(this, start, len); \
	} \
	\
	sl_reg VIEW::indexOf(typename VIEW::Char ch, sl_reg start) const noexcept \
	{ \
		return IndexOfCharSz(getData(), getUnsafeLength(), ch, start); \
	} \
	\
	sl_reg VIEW::indexOf(const VIEW& pattern, sl_reg start) const noexcept \
	{ \
		return IndexOf(getData(), getLength(), pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_reg VIEW::indexOf_IgnoreCase(const VIEW& pattern, sl_reg start) const noexcept \
	{ \
		return IndexOf_IgnoreCase(getData(), getLength(), pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_reg VIEW::lastIndexOf(typename VIEW::Char ch, sl_reg start) const noexcept \
	{ \
		return LastIndexOfChar(getData(), getLength(), ch, start); \
	} \
	\
	sl_reg VIEW::lastIndexOf(const VIEW& pattern, sl_reg start) const noexcept \
	{ \
		return LastIndexOf(getData(), getLength(), pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_reg VIEW::lastIndexOf_IgnoreCase(const VIEW& pattern, sl_reg start) const noexcept \
	{ \
		return LastIndexOf_IgnoreCase(getData(), getLength(), pattern.getData(), pattern.getLength(), start); \
	} \
	\
	sl_bool VIEW::startsWith(typename VIEW::Char ch) const noexcept \
	{ \
		return StartsWithCharSz(getData(), getUnsafeLength(), ch); \
	} \
	\
	sl_bool VIEW::startsWith(const VIEW& pattern) const noexcept \
	{ \
		return StartsWithSz(getData(), getUnsafeLength(), pattern.getData(), pattern.getUnsafeLength()); \
	} \
	\
	sl_bool VIEW::startsWith_IgnoreCase(const VIEW& pattern) const noexcept \
	{ \
		return StartsWithSz_IgnoreCase(getData(), getUnsafeLength(), pattern.getData(), pattern.getUnsafeLength()); \
	} \
	\
	sl_bool VIEW::endsWith(typename VIEW::Char ch) const noexcept \
	{ \
		return EndsWithCharSz(getData(), getUnsafeLength(), ch); \
	} \
	\
	sl_bool VIEW::endsWith(const VIEW& pattern) const noexcept \
	{ \
		return EndsWith(getData(), getLength(), pattern.getData(), pattern.getLength()); \
	} \
	\
	sl_bool VIEW::endsWith_IgnoreCase(const VIEW& pattern) const noexcept \
	{ \
		return EndsWith_IgnoreCase(getData(), getLength(), pattern.getData(), pattern.getLength()); \
	} \
	\
	sl_bool VIEW::contains(typename VIEW::Char ch) const noexcept \
	{ \
		return indexOf(ch) >= 0; \
	} \
	\
	sl_bool VIEW::contains(const VIEW& pattern) const noexcept \
	{ \
		return indexOf(pattern) >= 0; \
	} \
	\
	sl_bool VIEW::contains_IgnoreCase(const VIEW& pattern) const noexcept \
	{ \
		return indexOf_IgnoreCase(pattern) >= 0; \
	} \
	\
	sl_size VIEW::countOf(typename VIEW::Char ch) const noexcept \
	{ \
		return CountOfCharSz(getData(), getUnsafeLength(), ch); \
	} \
	\
	sl_size VIEW::countOf(const VIEW& pattern) const noexcept \
	{ \
		return CountOf(getData(), getLength(), pattern.getData(), pattern.getLength()); \
	} \
	\
	sl_size VIEW::countOf_IgnoreCase(const VIEW& pattern) const noexcept \
	{ \
		return CountOf_IgnoreCase(getData(), getLength(), pattern.getData(), pattern.getLength()); \
	} \
	\
	void VIEW::makeUpper() noexcept \
	{ \
		MakeUpperSz(getData(), getUnsafeLength()); \
	} \
	\
	void VIEW::makeLower() noexcept \
	{ \
		MakeLowerSz(getData(), getUnsafeLength()); \
	} \
	\
	typename VIEW::StringType VIEW::toUpper() const noexcept \
	{ \
		return CreateUpperString<StringType>(getData(), getUnsafeLength()); \
	} \
	\
	typename VIEW::StringType VIEW::toLower() const noexcept \
	{ \
		return CreateLowerString<StringType>(getData(), getUnsafeLength()); \
	} \
	\
	typename VIEW::StringType VIEW::replaceAll(typename VIEW::Char pattern, typename VIEW::Char replacement) const noexcept \
	{ \
		return ReplaceSzChar<StringType>(getData(), getUnsafeLength(), pattern, replacement); \
	} \
	\
	typename VIEW::StringType VIEW::replaceAll(const VIEW& pattern, const VIEW& replacement) const noexcept \
	{ \
		return ReplaceAll(*this, pattern.getData(), pattern.getLength(), replacement.getData(), replacement.getLength()); \
	} \
	\
	typename VIEW::StringType VIEW::removeAll(typename VIEW::Char pattern) const noexcept \
	{ \
		return ReplaceSzChar<StringType>(getData(), getUnsafeLength(), pattern, 0); \
	} \
	\
	typename VIEW::StringType VIEW::removeAll(const VIEW& pattern) const noexcept \
	{ \
		return ReplaceAll(*this, pattern.getData(), pattern.getLength(), (typename VIEW::Char*)sl_null, 0); \
	} \
	\
	VIEW VIEW::trim() const noexcept \
	{ \
		return Trim(*this); \
	} \
	\
	VIEW VIEW::trimLeft() const noexcept \
	{ \
		return TrimLeft(*this); \
	} \
	\
	VIEW VIEW::trimRight() const noexcept \
	{ \
		return TrimRight(*this); \
	} \
	\
	VIEW VIEW::trimLine() const noexcept \
	{ \
		return TrimLine(*this); \
	} \
	\
	void VIEW::makeReverse() noexcept \
	{ \
		MakeReverse(*this); \
	} \
	\
	typename VIEW::StringType VIEW::reverse() const noexcept \
	{ \
		return CreateReverseString<StringType>(getData(), getUnsafeLength()); \
	} \
	\
	List<VIEW> VIEW::split(const VIEW& pattern, sl_reg nMaxSplit) const noexcept \
	{ \
		return Split(*this, pattern.getData(), pattern.getLength(), nMaxSplit); \
	} \
	\
	List<VIEW> VIEW::split(typename VIEW::Char pattern, sl_reg nMaxSplit) const noexcept \
	{ \
		return Split(*this, pattern, nMaxSplit); \
	} \
	\
	sl_bool VIEW::parseInt32(sl_int32 radix, sl_int32* _out) const noexcept \
	{ \
		return ParseViewInt(*this, radix, _out); \
	} \
	\
	sl_bool VIEW::parseInt32(sl_int32* _out) const noexcept \
	{ \
		return parseInt32(10, _out); \
	} \
	\
	sl_int32 VIEW::parseInt32(sl_int32 radix, sl_int32 def) const noexcept \
	{ \
		sl_int32 _out = 0; \
		if (parseInt32(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseUint32(sl_int32 radix, sl_uint32* _out) const noexcept \
	{ \
		return ParseViewUint(*this, radix, _out); \
	} \
	\
	sl_bool VIEW::parseUint32(sl_uint32* _out) const noexcept \
	{ \
		return parseUint32(10, _out); \
	} \
	\
	sl_uint32 VIEW::parseUint32(sl_int32 radix, sl_uint32 def) const noexcept \
	{ \
		sl_uint32 _out = 0; \
		if (parseUint32(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseInt64(sl_int32 radix, sl_int64* _out) const noexcept \
	{ \
		return ParseViewInt(*this, radix, _out); \
	} \
	\
	sl_bool VIEW::parseInt64(sl_int64* _out) const noexcept \
	{ \
		return parseInt64(10, _out); \
	} \
	\
	sl_int64 VIEW::parseInt64(sl_int32 radix, sl_int64 def) const noexcept \
	{ \
		sl_int64 _out = 0; \
		if (parseInt64(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseUint64(sl_int32 radix, sl_uint64* _out) const noexcept \
	{ \
		return ParseViewUint(*this, radix, _out); \
	} \
	\
	sl_bool VIEW::parseUint64(sl_uint64* _out) const noexcept \
	{ \
		return parseUint64(10, _out); \
	} \
	\
	sl_uint64 VIEW::parseUint64(sl_int32 radix, sl_uint64 def) const noexcept \
	{ \
		sl_uint64 _out = 0; \
		if (parseUint64(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseInt(sl_int32 radix, sl_reg* _out) const noexcept \
	{ \
		return ParseViewInt(*this, radix, _out); \
	} \
	\
	sl_bool VIEW::parseInt(sl_reg* _out) const noexcept \
	{ \
		return parseInt(10, _out); \
	} \
	\
	sl_reg VIEW::parseInt(sl_int32 radix, sl_reg def) const noexcept \
	{ \
		sl_reg _out = 0; \
		if (parseInt(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseSize(sl_int32 radix, sl_size* _out) const noexcept \
	{ \
		return ParseViewUint(*this, radix, _out); \
	} \
	\
	sl_bool VIEW::parseSize(sl_size* _out) const noexcept \
	{ \
		return parseSize(10, _out); \
	} \
	\
	sl_size VIEW::parseSize(sl_int32 radix, sl_size def) const noexcept \
	{ \
		sl_size _out = 0; \
		if (parseSize(radix, &_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseFloat(float* _out) const noexcept \
	{ \
		return ParseViewFloat(*this, _out); \
	} \
	\
	float VIEW::parseFloat(float def) const noexcept \
	{ \
		float _out = 0; \
		if (parseFloat(&_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseDouble(double* _out) const noexcept \
	{ \
		return ParseViewFloat(*this, _out); \
	} \
	\
	double VIEW::parseDouble(double def) const noexcept \
	{ \
		double _out = 0; \
		if (parseDouble(&_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseBoolean(sl_bool* _out) const noexcept \
	{ \
		return ParseViewBoolean(*this, _out); \
	} \
	\
	sl_bool VIEW::parseBoolean(sl_bool def) const noexcept \
	{ \
		sl_bool _out = sl_false; \
		if (parseBoolean(&_out)) { \
			return _out; \
		} else { \
			return def; \
		} \
	} \
	\
	sl_bool VIEW::parseHexString(void* _out) const noexcept \
	{ \
		return ParseViewHexString(*this, _out); \
	} \
	\
	Memory VIEW::parseHexString() const noexcept \
	{ \
		return ParseHexString(*this); \
	} \

	DEFINE_STRING_VIEW_FUNC_IMPL(StringView)
	DEFINE_STRING_VIEW_FUNC_IMPL(StringView16)
	DEFINE_STRING_VIEW_FUNC_IMPL(StringView32)

#define DEFINE_STRING_COMPARE_OP_IMPL_SUB(CLASS, VIEW, RET, OP)


	String String::fromMemory(const Memory& _mem) noexcept
	{
		CMemory* mem = _mem.ref.get();
		if (mem) {
			return mem->getString();
		}
		return sl_null;
	}

	String16 String16::fromMemory(const Memory& _mem) noexcept
	{
		CMemory* mem = _mem.ref.get();
		if (mem) {
			return mem->getString16();
		}
		return sl_null;
	}

	String32 String32::fromMemory(const Memory& _mem) noexcept
	{
		CMemory* mem = _mem.ref.get();
		if (mem) {
			return mem->getString32();
		}
		return sl_null;
	}

	String String::from(const StringParam& str) noexcept
	{
		return str.toString();
	}

	String16 String16::from(const StringParam& str) noexcept
	{
		return str.toString16();
	}

	String32 String32::from(const StringParam& str) noexcept
	{
		return str.toString32();
	}

	String String::fromBoolean(sl_bool value) noexcept
	{
		if (value) {
			SLIB_RETURN_STRING("true")
		} else {
			SLIB_RETURN_STRING("false")
		}
	}

	String16 String16::fromBoolean(sl_bool value) noexcept
	{
		if (value) {
			SLIB_RETURN_STRING16("true")
		} else {
			SLIB_RETURN_STRING16("false")
		}
	}

	String32 String32::fromBoolean(sl_bool value) noexcept
	{
		if (value) {
			SLIB_RETURN_STRING32("true")
		} else {
			SLIB_RETURN_STRING32("false")
		}
	}

	String String::from(const Time& value) noexcept
	{
		return value.toString();
	}

	String16 String16::from(const Time& value) noexcept
	{
		return String16::create(value.toString());
	}

	String32 String32::from(const Time& value) noexcept
	{
		return String32::create(value.toString());
	}

	String String::from(const Variant& var) noexcept
	{
		return var.toString();
	}

	String16 String16::from(const Variant& var) noexcept
	{
		String16 s = var.getString16();
		if (s.isNotNull()) {
			return s;
		}
		return create(var.toString());
	}

	String32 String32::from(const Variant& var) noexcept
	{
		String32 s = var.getString32();
		if (s.isNotNull()) {
			return s;
		}
		return create(var.toString());
	}

	sl_size String::getUtf16(sl_char16* utf16, sl_size len) const noexcept
	{
		return Charsets::utf8ToUtf16(getData(), getLength(), utf16, len);
	}

	sl_bool String::getUtf16(StringStorage& output) const noexcept
	{
		output.charSize = 2;
		if (isEmpty()) {
			output.data16 = EMPTY_SZ(sl_char16);
			output.length = 0;
			return sl_true;
		}
		Memory mem = toUtf16();
		output.ref = Move(mem.ref);
		if (mem.isNotNull()) {
			output.data16 = (sl_char16*)(mem.getData());
			output.length = mem.getSize() / 2 - 1;
			if (output.data16) {
				return sl_true;
			}
		}
		output.data16 = EMPTY_SZ(sl_char16);
		output.length = 0;
		return sl_false;
	}

	Memory String::toUtf16() const noexcept
	{
		sl_char8* sz8 = getData();
		sl_size len8 = getLength();
		sl_size len16 = Charsets::utf8ToUtf16(sz8, len8, sl_null, -1);
		Memory memory = Memory::create((len16 + 1) << 1);
		if (memory.isNotNull()) {
			sl_char16* buf = (sl_char16*)(memory.getData());
			Charsets::utf8ToUtf16(sz8, len8, buf, len16);
			buf[len16] = 0;
			return memory;
		}
		return sl_null;
	}

	sl_size String::getUtf32(sl_char32* utf32, sl_size len) const noexcept
	{
		return Charsets::utf8ToUtf32(getData(), getLength(), utf32, len);
	}

	sl_bool String::getUtf32(StringStorage& output) const noexcept
	{
		output.charSize = 4;
		if (isEmpty()) {
			output.data32 = EMPTY_SZ(sl_char32);
			output.length = 0;
			return sl_true;
		}
		Memory mem = toUtf32();
		if (mem.isNotNull()) {
			output.data32 = (sl_char32*)(mem.getData());
			output.length = mem.getSize() / 4 - 1;
			if (output.data32) {
				return sl_true;
			}
		}
		output.data32 = EMPTY_SZ(sl_char32);
		output.length = 0;
		return sl_false;
	}

	Memory String::toUtf32() const noexcept
	{
		sl_char8* sz8 = getData();
		sl_size len8 = getLength();
		sl_size len32 = Charsets::utf8ToUtf32(sz8, len8, sl_null, -1);
		Memory memory = Memory::create((len32 + 1) << 2);
		if (memory.isNotNull()) {
			sl_char32* buf = (sl_char32*)(memory.getData());
			Charsets::utf8ToUtf32(sz8, len8, buf, len32);
			buf[len32] = 0;
			return memory;
		}
		return sl_null;
	}

	sl_size String16::getUtf8(sl_char8* utf8, sl_size len) const noexcept
	{
		return Charsets::utf16ToUtf8(getData(), getLength(), utf8, len);
	}

	sl_bool String16::getUtf8(StringStorage& output) const noexcept
	{
		output.charSize = 1;
		if (isEmpty()) {
			output.data8 = EMPTY_SZ(sl_char8);
			output.length = 0;
			return sl_true;
		}
		Memory mem = toUtf8();
		if (mem.isNotNull()) {
			output.data8 = (sl_char8*)(mem.getData());
			output.length = mem.getSize() - 1;
			if (output.data8) {
				return sl_true;
			}
		}
		output.data8 = EMPTY_SZ(sl_char8);
		output.length = 0;
		return sl_false;
	}

	Memory String16::toUtf8() const noexcept
	{
		sl_char16* sz16 = getData();
		sl_size len16 = getLength();
		sl_size len8 = Charsets::utf16ToUtf8(sz16, len16, sl_null, -1);
		Memory memory = Memory::create(len8 + 1);
		if (memory.isNotNull()) {
			sl_char8* buf = (sl_char8*)(memory.getData());
			Charsets::utf16ToUtf8(sz16, len16, buf, len8);
			buf[len8] = 0;
		}
		return memory;
	}

	sl_size String32::getUtf8(sl_char8* utf8, sl_size len) const noexcept
	{
		return Charsets::utf32ToUtf8(getData(), getLength(), utf8, len);
	}

	sl_bool String32::getUtf8(StringStorage& output) const noexcept
	{
		output.charSize = 1;
		if (isEmpty()) {
			output.data8 = EMPTY_SZ(sl_char8);
			output.length = 0;
			return sl_true;
		}
		Memory mem = toUtf8();
		if (mem.isNotNull()) {
			output.data8 = (sl_char8*)(mem.getData());
			output.length = mem.getSize() - 1;
			if (output.data8) {
				return sl_true;
			}
		}
		output.data8 = EMPTY_SZ(sl_char8);
		output.length = 0;
		return sl_false;
	}

	Memory String32::toUtf8() const noexcept
	{
		sl_char32* sz32 = getData();
		sl_size len32 = getLength();
		sl_size len8 = Charsets::utf32ToUtf8(sz32, len32, sl_null, -1);
		Memory memory = Memory::create(len8 + 1);
		if (memory.isNotNull()) {
			sl_char8* buf = (sl_char8*)(memory.getData());
			Charsets::utf32ToUtf8(sz32, len32, buf, len8);
			buf[len8] = 0;
		}
		return memory;
	}

}
