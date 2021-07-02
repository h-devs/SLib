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

#define SLIB_SUPPORT_STD_TYPES

#include "slib/core/json.h"

#include "slib/core/string_buffer.h"
#include "slib/core/parse_util.h"
#include "slib/core/math.h"
#include "slib/core/list_collection.h"
#include "slib/core/map_object.h"
#include "slib/core/serialize.h"

#define PTR_VAR(TYPE, x) ((TYPE*)((void*)(&(x))))
#define REF_VAR(TYPE, x) (*PTR_VAR(TYPE, x))

namespace slib
{

	namespace priv
	{
		namespace variant
		{

			const ConstContainer g_undefined = {0, {0}, VariantType::Null, 0};
			const ConstContainer g_null = {1, {0}, VariantType::Null, 0};

			SLIB_INLINE static void CopyBytes16(void* _dst, const void* _src)
			{
				sl_uint64* dst = (sl_uint64*)_dst;
				const sl_uint64* src = (const sl_uint64*)_src;
				*(dst++) = *(src++);
				*dst = *src;
			}

			SLIB_INLINE static void CopyBytes12(void* dst, const void* src)
			{
				*((sl_uint64*)dst) = *((const sl_uint64*)src);
				*((sl_uint32*)dst + 2) = *((const sl_uint32*)src + 2);
			}

			SLIB_INLINE static void ZeroBytes12(void* dst)
			{
				*((sl_uint64*)dst) = 0;
				*((sl_uint32*)dst + 2) = 0;
			}

			SLIB_INLINE static sl_bool IsReferable(sl_uint8 type)
			{
				return type >= VariantType::Referable;
			}

			SLIB_INLINE static void Copy(sl_uint8 src_type, const sl_uint64* _src_value, sl_uint64* _dst_value) noexcept
			{
				sl_uint64& dst_value = *_dst_value;
				const sl_uint64& src_value = *_src_value;
				switch (src_type) {
					case VariantType::String8:
						new PTR_VAR(String, dst_value) String(REF_VAR(String, src_value));
						break;
					case VariantType::String16:
						new PTR_VAR(String16, dst_value) String16(REF_VAR(String16, src_value));
						break;
					case VariantType::ObjectId:
						CopyBytes12(_dst_value, _src_value);
						break;
					default:
						if (src_type >= VariantType::Referable) {
							new PTR_VAR(Ref<Referable>, dst_value) Ref<Referable>(REF_VAR(Ref<Referable>, src_value));
						} else {
							dst_value = src_value;
						}
						break;
				}
			}

			SLIB_INLINE static void Free(sl_uint8 type, sl_uint64 value) noexcept
			{
				switch (type)
				{
					case VariantType::String8:
						REF_VAR(String, value).String::~String();
						break;
					case VariantType::String16:
						REF_VAR(String16, value).String16::~String16();
						break;
					default:
						if (type >= VariantType::Referable) {
							REF_VAR(Ref<Referable>, value).Ref<Referable>::~Ref();
						}
						break;
				}
			}

			template <class T, sl_uint8 type>
			SLIB_INLINE static sl_bool IsObject(const Variant& v)
			{
				if (v._type == type) {
					return sl_true;
				}
				if (v._type == VariantType::Weak) {
					Ref<Referable> ref(REF_VAR(WeakRef<Referable> const, v._value));
					if (ref.isNotNull()) {
						return IsInstanceOf<T>(ref);
					}
				} else if (IsReferable(v._type)) {
					return IsInstanceOf<T>(REF_VAR(Ref<Referable> const, v._value));
				}
				return sl_false;
			}

			template <class T, class OT, sl_uint8 type>
			SLIB_INLINE static OT GetObjectT(const Variant& v)
			{
				if (v._type == type) {
					return REF_VAR(OT, v._value);
				}
				if (v._type == VariantType::Weak) {
					Ref<Referable> ref(REF_VAR(WeakRef<Referable> const, v._value));
					if (IsInstanceOf<T>(ref)) {
						return REF_VAR(OT, ref);
					}
				} else if (IsReferable(v._type)) {
					if (IsInstanceOf<T>(REF_VAR(Referable*, v._value))) {
						return REF_VAR(OT, v._value);
					}
				}
				return OT();
			}

#define GET_COLLECTION(v) GetObjectT<Collection, Ref<Collection>, VariantType::Collection>(v)
#define GET_OBJECT(v) GetObjectT<Object, Ref<Object>, VariantType::Object>(v)

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const sl_int32* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const sl_uint32* v2) noexcept
			{
				sl_int32 n = *v1;
				if (n >= 0) {
					return (sl_uint32)(n) == *v2;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const sl_int64* v2) noexcept
			{
				return (sl_int64)(*v1) == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const sl_uint64* v2) noexcept
			{
				sl_int32 n = *v1;
				if (n >= 0) {
					return (sl_uint64)((sl_uint32)n) == *v2;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const float* v2) noexcept
			{
				return Math::isAlmostZero((float)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const double* v2) noexcept
			{
				return Math::isAlmostZero((double)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const String* v2) noexcept
			{
				sl_int32 n;
				if (v2->parseInt32(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, const String16* v2) noexcept
			{
				sl_int32 n;
				if (v2->parseInt32(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, sl_char8 const* const* v2) noexcept
			{
				sl_int32 n;
				if (String::parseInt32(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int32* v1, sl_char16 const* const* v2) noexcept
			{
				sl_int32 n;
				if (String16::parseInt32(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const sl_int32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, const sl_uint32* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, const sl_int64* v2) noexcept
			{
				return (sl_int64)(*v1) == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, const sl_uint64* v2) noexcept
			{
				return (sl_uint64)(*v1) == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, const float* v2) noexcept
			{
				return Math::isAlmostZero((float)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, const double* v2) noexcept
			{
				return Math::isAlmostZero((double)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, const String* v2) noexcept
			{
				sl_uint32 n;
				if (v2->parseUint32(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, const String16* v2) noexcept
			{
				sl_uint32 n;
				if (v2->parseUint32(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, sl_char8 const* const* v2) noexcept
			{
				sl_uint32 n;
				if (String::parseUint32(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint32* v1, sl_char16 const* const* v2) noexcept
			{
				sl_uint32 n;
				if (String16::parseUint32(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const sl_uint32* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, const sl_int64* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, const sl_uint64* v2) noexcept
			{
				sl_int64 n = *v1;
				if (n >= 0) {
					return (sl_uint64)(n) == *v2;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v2, const sl_int64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, const float* v2) noexcept
			{
				return Math::isAlmostZero((float)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v2, const sl_int64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, const double* v2) noexcept
			{
				return Math::isAlmostZero((double)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v2, const sl_int64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, const String* v2) noexcept
			{
				sl_int64 n;
				if (v2->parseInt64(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v2, const sl_int64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, const String16* v2) noexcept
			{
				sl_int64 n;
				if (v2->parseInt64(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v2, const sl_int64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, sl_char8 const* const* v2) noexcept
			{
				sl_int64 n;
				if (String::parseInt64(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const sl_int64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_int64* v1, sl_char16 const* const* v2) noexcept
			{
				sl_int64 n;
				if (String16::parseInt64(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const sl_int64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v1, const sl_uint64* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v1, const float* v2) noexcept
			{
				return Math::isAlmostZero((float)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v2, const sl_uint64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v1, const double* v2) noexcept
			{
				return Math::isAlmostZero((double)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v2, const sl_uint64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v1, const String* v2) noexcept
			{
				sl_uint64 n;
				if (v2->parseUint64(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v2, const sl_uint64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v1, const String16* v2) noexcept
			{
				sl_uint64 n;
				if (v2->parseUint64(10, &n)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v2, const sl_uint64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v1, sl_char8 const* const* v2) noexcept
			{
				sl_uint64 n;
				if (String::parseUint64(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const sl_uint64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const sl_uint64* v1, sl_char16 const* const* v2) noexcept
			{
				sl_uint64 n;
				if (String16::parseUint64(10, &n, *v2)) {
					return *v1 == n;
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const sl_uint64* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(const float* v1, const float* v2) noexcept
			{
				return Math::isAlmostZero(*v1 - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v1, const double* v2) noexcept
			{
				return Math::isAlmostZero((double)(*v1) - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v2, const float* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v1, const String* v2) noexcept
			{
				float n;
				if (v2->parseFloat(&n)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v2, const float* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v1, const String16* v2) noexcept
			{
				float n;
				if (v2->parseFloat(&n)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v2, const float* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v1, sl_char8 const* const* v2) noexcept
			{
				float n;
				if (String::parseFloat(&n, *v2)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const float* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const float* v1, sl_char16 const* const* v2) noexcept
			{
				float n;
				if (String16::parseFloat(&n, *v2)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const float* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(const double* v1, const double* v2) noexcept
			{
				return Math::isAlmostZero(*v1 - *v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v1, const String* v2) noexcept
			{
				double n;
				if (v2->parseDouble(&n)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v2, const double* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v1, const String16* v2) noexcept
			{
				double n;
				if (v2->parseDouble(&n)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v2, const double* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v1, sl_char8 const* const* v2) noexcept
			{
				double n;
				if (String::parseDouble(&n, *v2)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const double* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const double* v1, sl_char16 const* const* v2) noexcept
			{
				double n;
				if (String16::parseDouble(&n, *v2)) {
					return Math::isAlmostZero(*v1 - n);
				}
				return sl_false;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const double* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(const String* v1, const String* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v1, const String16* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v2, const String* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v1, sl_char8 const* const* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const String* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const String* v1, sl_char16 const* const* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const String* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(const String16* v1, const String16* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v1, sl_char8 const* const* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v2, const String16* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}

			SLIB_INLINE static sl_bool EqualsElement(const String16* v1, sl_char16 const* const* v2) noexcept
			{
				return *v1 == *v2;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, const String16* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v1, sl_char8 const* const* v2) noexcept
			{
				return Base::compareString(*v1, *v2) == 0;
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char8 const* const* v1, sl_char16 const* const* v2) noexcept
			{
				return String::equals(*v1, -1, *v2, -1);
			}

			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v2, sl_char8 const* const* v1) noexcept
			{
				return EqualsElement(v1, v2);
			}


			SLIB_INLINE static sl_bool EqualsElement(sl_char16 const* const* v1, sl_char16 const* const* v2) noexcept
			{
				return Base::compareString2(*v1, *v2) == 0;
			}


			template <class T>
			SLIB_INLINE static sl_bool EqualsVariant(const T* v1, const Variant& v2) noexcept
			{
				sl_uint8 type = v2._type;
				switch (type) {
				case VariantType::Int32:
					return EqualsElement(v1, PTR_VAR(sl_int32 const, v2._value));
				case VariantType::Uint32:
					return EqualsElement(v1, PTR_VAR(sl_uint32 const, v2._value));
				case VariantType::Int64:
					return EqualsElement(v1, PTR_VAR(sl_int64 const, v2._value));
				case VariantType::Uint64:
					return EqualsElement(v1, PTR_VAR(sl_uint64 const, v2._value));
				case VariantType::Float:
					return EqualsElement(v1, PTR_VAR(float const, v2._value));
				case VariantType::Double:
					return EqualsElement(v1, PTR_VAR(double const, v2._value));
				case VariantType::String8:
					return EqualsElement(v1, PTR_VAR(String const, v2._value));
				case VariantType::String16:
					return EqualsElement(v1, PTR_VAR(String16 const, v2._value));
				case VariantType::Sz8:
					return EqualsElement(v1, PTR_VAR(sl_char8 const* const, v2._value));
				case VariantType::Sz16:
					return EqualsElement(v1, PTR_VAR(sl_char16 const* const, v2._value));
				default:
					break;
				}
				return sl_false;
			}


			SLIB_INLINE static sl_compare_result CompareElement(const String* v1, const String* v2) noexcept
			{
				return v1->compare(*v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(const String* v1, const String16* v2) noexcept
			{
				return v1->compare(*v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(const String16* v2, const String* v1) noexcept
			{
				return -CompareElement(v1, v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(const String* v1, sl_char8 const* const* v2) noexcept
			{
				return v1->compare(*v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(sl_char8 const* const* v2, const String* v1) noexcept
			{
				return -CompareElement(v1, v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(const String* v1, sl_char16 const* const* v2) noexcept
			{
				return v1->compare(*v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(sl_char16 const* const* v2, const String* v1) noexcept
			{
				return -CompareElement(v1, v2);
			}


			SLIB_INLINE static sl_compare_result CompareElement(const String16* v1, const String16* v2) noexcept
			{
				return v1->compare(*v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(const String16* v1, sl_char8 const* const* v2) noexcept
			{
				return v1->compare(*v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(sl_char8 const* const* v2, const String16* v1) noexcept
			{
				return -CompareElement(v1, v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(const String16* v1, sl_char16 const* const* v2) noexcept
			{
				return v1->compare(*v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(sl_char16 const* const* v2, const String16* v1) noexcept
			{
				return -CompareElement(v1, v2);
			}


			SLIB_INLINE static sl_compare_result CompareElement(sl_char8 const* const* v1, sl_char8 const* const* v2) noexcept
			{
				return Base::compareString(*v1, *v2);
			}

			SLIB_INLINE static sl_compare_result CompareElement(sl_char8 const* const* v1, sl_char16 const* const* v2) noexcept
			{
				return String::compare(*v1, -1, *v2, -1);
			}

			SLIB_INLINE static sl_compare_result CompareElement(sl_char16 const* const* v2, sl_char8 const* const* v1) noexcept
			{
				return -CompareElement(v1, v2);
			}


			SLIB_INLINE static sl_compare_result CompareElement(sl_char16 const* const* v1, sl_char16 const* const* v2) noexcept
			{
				return Base::compareString2(*v1, *v2) == 0;
			}


			template <class T>
			SLIB_INLINE static sl_compare_result CompareString(const T* v1, const Variant& v2) noexcept
			{
				sl_uint8 type = v2._type;
				switch (type) {
				case VariantType::String8:
					return CompareElement(v1, PTR_VAR(String const, v2._value));
				case VariantType::String16:
					return CompareElement(v1, PTR_VAR(String16 const, v2._value));
				case VariantType::Sz8:
					return CompareElement(v1, PTR_VAR(sl_char8 const* const, v2._value));
				case VariantType::Sz16:
					return CompareElement(v1, PTR_VAR(sl_char16 const* const, v2._value));
				default:
					break;
				}
				return -1;
			}

		}
	}

	using namespace priv::variant;

	void Variant::_assign(const Variant& other) noexcept
	{
		if (this != &other) {
			Free(_type, _value);
			_type = other._type;
			Copy(_type, &(other._value), &_value);
		}
	}

	void Variant::_assignMove(Variant& other) noexcept
	{
		if (this != &other) {
			Free(_type, _value);
			CopyBytes16(this, &other);
			other._type = VariantType::Null;
		}
	}

	void Variant::_constructorRef(const void* ptr, sl_uint8 type) noexcept
	{
		const Ref<Referable>& ref = *reinterpret_cast<Ref<Referable> const*>(ptr);
		if (ref.isNotNull()) {
			_type = type;
			new (reinterpret_cast<Ref<Referable>*>(&_value)) Ref<Referable>(ref);
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	void Variant::_constructorMoveRef(void* ptr, sl_uint8 type) noexcept
	{
		Ref<Referable>& ref = *reinterpret_cast<Ref<Referable>*>(ptr);
		if (ref.isNotNull()) {
			_type = type;
			new (reinterpret_cast<Ref<Referable>*>(&_value)) Ref<Referable>(Move(ref));
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	void Variant::_assignRef(const void* ptr, sl_uint8 type) noexcept
	{
		Free(_type, _value);
		_constructorRef(ptr, type);
	}

	void Variant::_assignMoveRef(void* ptr, sl_uint8 type) noexcept
	{
		Free(_type, _value);
		_constructorMoveRef(ptr, type);
	}

	void Variant::_free(sl_uint8 type, sl_uint64 value) noexcept
	{
		Free(type, value);
	}

	Variant::Variant(const Variant& other) noexcept
	{
		_type = other._type;
		Copy(_type, &(other._value), &_value);
	}

	Variant::Variant(Variant&& other) noexcept
	{
		CopyBytes16(this, &other);
		other._type = VariantType::Null;
	}

	Variant::Variant(const Atomic<Variant>& other) noexcept
	{
		other._retain_construct(this);
	}

	Variant::Variant(const Json& other) noexcept
	{
		_type = other._type;
		Copy(_type, &(other._value), &_value);
	}

	Variant::Variant(Json&& other) noexcept
	{
		CopyBytes16(this, &other);
		other._type = VariantType::Null;
	}

	Variant::~Variant() noexcept
	{
		Free(_type, _value);
	}
	
	Variant::Variant(signed char value) noexcept
	{
		_type = VariantType::Int32;
		REF_VAR(sl_int32, _value) = value;
	}
	
	Variant::Variant(unsigned char value) noexcept
	{
		_type = VariantType::Uint32;
		REF_VAR(sl_uint32, _value) = value;
	}

	Variant::Variant(short value) noexcept
	{
		_type = VariantType::Int32;
		REF_VAR(sl_int32, _value) = value;
	}

	Variant::Variant(unsigned short value) noexcept
	{
		_type = VariantType::Uint32;
		REF_VAR(sl_uint32, _value) = value;
	}
	
	Variant::Variant(int value) noexcept
	{
		_type = VariantType::Int32;
		REF_VAR(sl_int32, _value) = (sl_int32)value;
	}
	
	Variant::Variant(unsigned int value) noexcept
	{
		_type = VariantType::Uint32;
		REF_VAR(sl_uint32, _value) = (sl_uint32)value;
	}
	
	Variant::Variant(long value) noexcept
	{
#if SLIB_LONG_SIZE == 8
		_type = VariantType::Int64;
		REF_VAR(sl_int64, _value) = (sl_int64)value;
#else
		_type = VariantType::Int32;
		REF_VAR(sl_int32, _value) = (sl_int32)value;
#endif
	}
	
	Variant::Variant(unsigned long value) noexcept
	{
#if SLIB_LONG_SIZE == 8
		_type = VariantType::Uint64;
		REF_VAR(sl_uint64, _value) = (sl_uint64)value;
#else
		_type = VariantType::Uint32;
		REF_VAR(sl_uint32, _value) = (sl_uint32)value;
#endif
	}

	Variant::Variant(sl_int64 value) noexcept
	{
		_type = VariantType::Int64;
		REF_VAR(sl_int64, _value) = value;
	}

	Variant::Variant(sl_uint64 value) noexcept
	{
		_type = VariantType::Uint64;
		REF_VAR(sl_uint64, _value) = value;
	}

	Variant::Variant(float value) noexcept
	{
		_type = VariantType::Float;
		REF_VAR(float, _value) = value;
	}

	Variant::Variant(double value) noexcept
	{
		_type = VariantType::Double;
		REF_VAR(double, _value) = value;
	}

	Variant::Variant(sl_bool value) noexcept
	{
		_type = VariantType::Boolean;
		REF_VAR(sl_bool, _value) = value;
	}

	Variant::Variant(const String& value) noexcept
	{
		if (value.isNotNull()) {
			_type = VariantType::String8;
			new PTR_VAR(String, _value) String(value);
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(String&& value) noexcept
	{
		if (value.isNotNull()) {
			_type = VariantType::String8;
			new PTR_VAR(String, _value) String(Move(value));
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(const String16& value) noexcept
	{
		if (value.isNotNull()) {
			_type = VariantType::String16;
			new PTR_VAR(String16, _value) String16(value);
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(String16&& value) noexcept
	{
		if (value.isNotNull()) {
			_type = VariantType::String16;
			new PTR_VAR(String16, _value) String16(Move(value));
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(const StringView& value) noexcept: Variant(String(value))
	{
	}

	Variant::Variant(const StringView16& value) noexcept: Variant(String16(value))
	{
	}

	Variant::Variant(const sl_char8* sz8) noexcept
	{
		if (sz8) {
			_type = VariantType::Sz8;
			REF_VAR(const sl_char8*, _value) = sz8;
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(sl_char8* sz8) noexcept
	{
		if (sz8) {
			_type = VariantType::Sz8;
			REF_VAR(sl_char8*, _value) = sz8;
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(const sl_char16* sz16) noexcept
	{
		if (sz16) {
			_type = VariantType::Sz16;
			REF_VAR(const sl_char16*, _value) = sz16;
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(sl_char16* sz16) noexcept
	{
		if (sz16) {
			_type = VariantType::Sz16;
			REF_VAR(sl_char16*, _value) = sz16;
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	Variant::Variant(const StringParam& str) noexcept: Variant(str.toVariant())
	{
	}
	
	Variant::Variant(const std::string& value) noexcept: Variant(String::create(value))
	{
	}
	
	Variant::Variant(const std::u16string& value) noexcept: Variant(String16::create(value))
	{
	}

	Variant::Variant(const Time& value) noexcept
	{
		_type = VariantType::Time;
		REF_VAR(Time, _value) = value;
	}

	Variant::Variant(const ObjectId& _id) noexcept
	{
		_type = VariantType::ObjectId;
		CopyBytes12(&_value, _id.data);
	}

	Variant::Variant(const VariantList& list) noexcept
	{
		_constructorRef(&list, VariantType::List);
	}

	Variant::Variant(VariantList&& list) noexcept
	{
		_constructorMoveRef(&list, VariantType::List);
	}

	Variant::Variant(const VariantMap& map) noexcept
	{
		_constructorRef(&map, VariantType::Map);
	}

	Variant::Variant(VariantMap&& map) noexcept
	{
		_constructorMoveRef(&map, VariantType::Map);
	}

	Variant::Variant(const JsonList& list) noexcept
	{
		_constructorRef(&list, VariantType::List);
	}

	Variant::Variant(JsonList&& list) noexcept
	{
		_constructorMoveRef(&list, VariantType::List);
	}

	Variant::Variant(const JsonMap& map) noexcept
	{
		_constructorRef(&map, VariantType::Map);
	}

	Variant::Variant(JsonMap&& map) noexcept
	{
		_constructorMoveRef(&map, VariantType::Map);
	}

	Variant::Variant(const Memory& mem) noexcept
	{
		_constructorRef(&mem, VariantType::Memory);
	}

	Variant::Variant(Memory&& mem) noexcept
	{
		_constructorMoveRef(&mem, VariantType::Memory);
	}

	Variant::Variant(const Promise<Variant>& promise) noexcept
	{
		_constructorRef(&promise, VariantType::Promise);
	}

	Variant::Variant(Promise<Variant>&& promise) noexcept
	{
		_constructorMoveRef(&promise, VariantType::Promise);
	}

	Variant& Variant::operator=(const Variant& other) noexcept
	{
		_assign(other);
		return *this;
	}

	Variant& Variant::operator=(Variant&& other) noexcept
	{
		_assignMove(other);
		return *this;
	}

	Variant& Variant::operator=(const Atomic<Variant>& other) noexcept
	{
		return *this = Variant(other);
	}

	Variant& Variant::operator=(const Json& other) noexcept
	{
		_assign(other);
		return *this;
	}

	Variant& Variant::operator=(Json&& other) noexcept
	{
		_assignMove(other);
		return *this;
	}

	Variant& Variant::operator=(sl_null_t) noexcept
	{
		setNull();
		return *this;
	}

	Variant Variant::operator[](sl_uint64 index) const noexcept
	{
		return getElement(index);
	}

	Variant Variant::operator[](const StringParam& key) const noexcept
	{
		return getItem(key);
	}

	void Variant::setUndefined() noexcept
	{
		if (_type != VariantType::Null) {
			Free(_type, _value);
			_type = VariantType::Null;
		}
		_value = 0;
	}

	void Variant::setNull() noexcept
	{
		if (_type != VariantType::Null) {
			Free(_type, _value);
			_type = VariantType::Null;
		}
		_value = 1;
	}

	sl_bool Variant::isInt32() const noexcept
	{
		return _type == VariantType::Int32;
	}

	sl_bool Variant::getInt32(sl_int32* _out) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(sl_int32 const, _value));
				}
				return sl_true;
			case VariantType::Uint32:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(sl_uint32 const, _value));
				}
				return sl_true;
			case VariantType::Int64:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(sl_int64 const, _value));
				}
				return sl_true;
			case VariantType::Uint64:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(sl_uint64 const, _value));
				}
				return sl_true;
			case VariantType::Float:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(float const, _value));
				}
				return sl_true;
			case VariantType::Double:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(double const, _value));
				}
				return sl_true;
			case VariantType::Boolean:
				if (_out) {
					*_out = (REF_VAR(sl_bool const, _value)) ? 1 : 0;
				}
				return sl_true;
			case VariantType::String8:
				return REF_VAR(String const, _value).parseInt32(_out);
			case VariantType::String16:
				return REF_VAR(String16 const, _value).parseInt32(_out);
			case VariantType::Pointer:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(const sl_size, _value));
				}
				return sl_true;
			case VariantType::Sz8:
				{
					const sl_char8* str = REF_VAR(sl_char8 const* const , _value);
					sl_reg pos = String::parseInt32(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Sz16:
				{
					const sl_char16* str = REF_VAR(sl_char16 const* const , _value);
					sl_reg pos = String16::parseInt32(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Time:
				if (_out) {
					*_out = (sl_int32)(REF_VAR(Time const, _value).toUnixTime());
				}
				return sl_true;
			default:
				break;
		}
		return sl_false;
	}

	sl_int32 Variant::getInt32(sl_int32 def) const noexcept
	{
		sl_int32 ret;
		if (getInt32(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setInt32(sl_int32 value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Int32;
		REF_VAR(sl_int32, _value) = value;
	}

	sl_bool Variant::isUint32() const noexcept
	{
		return _type == VariantType::Uint32;
	}

	sl_bool Variant::getUint32(sl_uint32* _out) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(sl_int32 const, _value));
				}
				return sl_true;
			case VariantType::Uint32:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(sl_uint32 const, _value));
				}
				return sl_true;
			case VariantType::Int64:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(sl_int64 const, _value));
				}
				return sl_true;
			case VariantType::Uint64:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(sl_uint64 const, _value));
				}
				return sl_true;
			case VariantType::Float:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(float const, _value));
				}
				return sl_true;
			case VariantType::Double:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(double const, _value));
				}
				return sl_true;
			case VariantType::Boolean:
				if (_out) {
					*_out = (REF_VAR(sl_bool const, _value)) ? 1 : 0;
				}
				return sl_true;
			case VariantType::String8:
				return REF_VAR(String const, _value).parseUint32(_out);
			case VariantType::String16:
				return REF_VAR(String16 const, _value).parseUint32(_out);
			case VariantType::Pointer:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(sl_size const, _value));
				}
				return sl_true;
			case VariantType::Sz8:
				{
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseUint32(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Sz16:
				{
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseUint32(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Time:
				if (_out) {
					*_out = (sl_uint32)(REF_VAR(Time const, _value).toUnixTime());
				}
				return sl_true;
			default:
				break;
		}
		return sl_false;
	}

	sl_uint32 Variant::getUint32(sl_uint32 def) const noexcept
	{
		sl_uint32 ret;
		if (getUint32(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setUint32(sl_uint32 value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Uint32;
		REF_VAR(sl_uint32, _value) = value;
	}

	sl_bool Variant::isInt64() const noexcept
	{
		return _type == VariantType::Int64;
	}

	sl_bool Variant::getInt64(sl_int64* _out) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				if (_out) {
					*_out = (sl_int64)(REF_VAR(sl_int32 const, _value));
				}
				return sl_true;
			case VariantType::Uint32:
				if (_out) {
					*_out = (sl_int64)(REF_VAR(sl_uint32 const, _value));
				}
				return sl_true;
			case VariantType::Int64:
				if (_out) {
					*_out = (sl_int64)(REF_VAR(sl_int64 const, _value));
				}
				return sl_true;
			case VariantType::Uint64:
				if (_out) {
					*_out = (sl_int64)(REF_VAR(sl_uint64 const, _value));
				}
				return sl_true;
			case VariantType::Float:
				if (_out) {
					*_out = (sl_int64)(REF_VAR(float const, _value));
				}
				return sl_true;
			case VariantType::Double:
				if (_out) {
					*_out = (sl_int64)(REF_VAR(double const, _value));
				}
				return sl_true;
			case VariantType::Boolean:
				if (_out) {
					*_out = (REF_VAR(sl_bool const, _value)) ? 1 : 0;
				}
				return sl_true;
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseInt64(_out);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseInt64(_out);
			case VariantType::Pointer:
				if (_out) {
					*_out = (sl_int64)(REF_VAR(sl_size const, _value));
				}
				return sl_true;
			case VariantType::Sz8:
				{
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseInt64(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Sz16:
				{
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseInt64(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Time:
				if (_out) {
					*_out = REF_VAR(Time const, _value).toUnixTime();
				}
				return sl_true;
			default:
				break;
		}
		return sl_false;
	}

	sl_int64 Variant::getInt64(sl_int64 def) const noexcept
	{
		sl_int64 ret;
		if (getInt64(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setInt64(sl_int64 value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Int64;
		REF_VAR(sl_int64, _value) = value;
	}

	sl_bool Variant::isUint64() const noexcept
	{
		return _type == VariantType::Uint64;
	}

	sl_bool Variant::getUint64(sl_uint64* _out) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				if (_out) {
					*_out = (sl_uint64)(REF_VAR(sl_int32 const, _value));
				}
				return sl_true;
			case VariantType::Uint32:
				if (_out) {
					*_out = (sl_uint64)(REF_VAR(sl_uint32 const, _value));
				}
				return sl_true;
			case VariantType::Int64:
				if (_out) {
					*_out = (sl_uint64)(REF_VAR(sl_int64 const, _value));
				}
				return sl_true;
			case VariantType::Uint64:
				if (_out) {
					*_out = (sl_uint64)(REF_VAR(sl_uint64 const, _value));
				}
				return sl_true;
			case VariantType::Float:
				if (_out) {
					*_out = (sl_uint64)(REF_VAR(float const, _value));
				}
				return sl_true;
			case VariantType::Double:
				if (_out) {
					*_out = (sl_uint64)(REF_VAR(double const, _value));
				}
				return sl_true;
			case VariantType::Boolean:
				if (_out) {
					*_out = (REF_VAR(sl_bool const, _value)) ? 1 : 0;
				}
				return sl_true;
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseUint64(_out);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseUint64(_out);
			case VariantType::Pointer:
				if (_out) {
					*_out = (sl_uint64)(REF_VAR(sl_size const, _value));
				}
				return sl_true;
			case VariantType::Sz8:
				{
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseUint64(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Sz16:
				{
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseUint64(10, _out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Time:
				if (_out) {
					*_out = REF_VAR(Time const, _value).toUnixTime();
				}
				return sl_true;
			default:
				break;
		}
		return sl_false;
	}

	sl_uint64 Variant::getUint64(sl_uint64 def) const noexcept
	{
		sl_uint64 ret;
		if (getUint64(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setUint64(sl_uint64 value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Uint64;
		REF_VAR(sl_uint64, _value) = value;
	}

	sl_bool Variant::isInteger() const noexcept
	{
		return _type == VariantType::Int32 || _type == VariantType::Uint32 || _type == VariantType::Int64 || _type == VariantType::Uint64;
	}

	sl_bool Variant::isSignedInteger() const noexcept
	{
		return _type == VariantType::Int32 || _type == VariantType::Int64;
	}

	sl_bool Variant::isUnsignedInteger() const noexcept
	{
		return _type == VariantType::Uint32 || _type == VariantType::Uint64;
	}

	sl_bool Variant::isFloat() const noexcept
	{
		return _type == VariantType::Float;
	}

	sl_bool Variant::getFloat(float* _out) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				if (_out) {
					*_out = (float)(REF_VAR(sl_int32 const, _value));
				}
				return sl_true;
			case VariantType::Uint32:
				if (_out) {
					*_out = (float)(REF_VAR(sl_uint32 const, _value));
				}
				return sl_true;
			case VariantType::Int64:
				if (_out) {
					*_out = (float)(REF_VAR(sl_int64 const, _value));
				}
				return sl_true;
			case VariantType::Uint64:
				if (_out) {
					*_out = (float)(REF_VAR(sl_uint64 const, _value));
				}
				return sl_true;
			case VariantType::Float:
				if (_out) {
					*_out = (float)(REF_VAR(float const, _value));
				}
				return sl_true;
			case VariantType::Double:
				if (_out) {
					*_out = (float)(REF_VAR(double const, _value));
				}
				return sl_true;
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseFloat(_out);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseFloat(_out);
			case VariantType::Sz8:
				{
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseFloat(_out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Sz16:
				{
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseFloat(_out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Time:
				if (_out) {
					*_out = (float)(REF_VAR(Time const, _value).toUnixTimef());
				}
				return sl_true;
			default:
				break;
		}
		return sl_false;
	}

	float Variant::getFloat(float def) const noexcept
	{
		float ret;
		if (getFloat(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setFloat(float value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Float;
		REF_VAR(float, _value) = value;
	}

	sl_bool Variant::isDouble() const noexcept
	{
		return _type == VariantType::Double;
	}

	sl_bool Variant::getDouble(double* _out) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				if (_out) {
					*_out = (double)(REF_VAR(sl_int32 const, _value));
				}
				return sl_true;
			case VariantType::Uint32:
				if (_out) {
					*_out = (double)(REF_VAR(sl_uint32 const, _value));
				}
				return sl_true;
			case VariantType::Int64:
				if (_out) {
					*_out = (double)(REF_VAR(sl_int64 const, _value));
				}
				return sl_true;
			case VariantType::Uint64:
				if (_out) {
					*_out = (double)(REF_VAR(sl_uint64 const, _value));
				}
				return sl_true;
			case VariantType::Float:
				if (_out) {
					*_out = (double)(REF_VAR(float const, _value));
				}
				return sl_true;
			case VariantType::Double:
				if (_out) {
					*_out = (double)(REF_VAR(double const, _value));
				}
				return sl_true;
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseDouble(_out);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseDouble(_out);
			case VariantType::Sz8:
				{
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseDouble(_out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Sz16:
				{
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseDouble(_out, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return sl_true;
					}
				}
				break;
			case VariantType::Time:
				if (_out) {
					*_out = REF_VAR(Time const, _value).toUnixTimef();
				}
				return sl_true;
			default:
				break;
		}
		return sl_false;
	}

	double Variant::getDouble(double def) const noexcept
	{
		double ret;
		if (getDouble(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setDouble(double value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Double;
		REF_VAR(double, _value) = value;
	}

	sl_bool Variant::isNaN() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isNaN(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isNaN(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isInfinite() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isInfinite(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isInfinite(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isPositiveInfinite() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isPositiveInfinite(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isPositiveInfinite(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isNegativeInfinite() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isNegativeInfinite(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isNegativeInfinite(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isNumber() const noexcept
	{
		return isInteger() || _type == VariantType::Float || _type == VariantType::Double;
	}

	sl_bool Variant::isBoolean() const noexcept
	{
		return _type == VariantType::Boolean;
	}

	sl_bool Variant::isTrue() const noexcept
	{
		return _type == VariantType::Boolean && REF_VAR(sl_bool const, _value) != sl_false;
	}

	sl_bool Variant::isFalse() const noexcept
	{
		return _type == VariantType::Boolean && REF_VAR(sl_bool const, _value) == sl_false;
	}

	sl_bool Variant::getBoolean(sl_bool def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				{
					sl_int32 n = REF_VAR(sl_int32 const, _value);
					if (n != 0) {
						return sl_true;
					} else {
						return sl_false;
					}
				}
			case VariantType::Uint32:
				{
					sl_uint32 n = REF_VAR(sl_uint32 const, _value);
					if (n != 0) {
						return sl_true;
					} else {
						return sl_false;
					}
				}
			case VariantType::Int64:
				{
					sl_int64 n = REF_VAR(sl_int64 const, _value);
					if (n != 0) {
						return sl_true;
					} else {
						return sl_false;
					}
				}
			case VariantType::Uint64:
				{
					sl_uint64 n = REF_VAR(sl_uint64 const, _value);
					if (n != 0) {
						return sl_true;
					} else {
						return sl_false;
					}
				}
			case VariantType::Boolean:
				return REF_VAR(sl_bool const, _value);
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseBoolean(def);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseBoolean(def);
			case VariantType::Sz8:
				{
					sl_bool ret;
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseBoolean(&ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Sz16:
				{
					sl_bool ret;
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseBoolean(&ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			default:
				break;
		}
		return def;
	}

	void Variant::setBoolean(sl_bool value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Boolean;
		REF_VAR(sl_bool, _value) = value;
	}

	sl_bool Variant::isString() const noexcept
	{
		return _type == VariantType::String8 || _type == VariantType::String16 || _type == VariantType::Sz8 || _type == VariantType::Sz16;
	}

	sl_bool Variant::isString8() const noexcept
	{
		return _type == VariantType::String8;
	}

	sl_bool Variant::isString16() const noexcept
	{
		return _type == VariantType::String16;
	}

	sl_bool Variant::isSz8() const noexcept
	{
		return _type == VariantType::Sz8;
	}

	sl_bool Variant::isSz16() const noexcept
	{
		return _type == VariantType::Sz16;
	}

	String Variant::getString(const String& def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return String::fromInt32(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return String::fromUint32(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return String::fromInt64(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return String::fromUint64(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return String::fromFloat(REF_VAR(float const, _value));
			case VariantType::Double:
				return String::fromDouble(REF_VAR(double const, _value));
			case VariantType::Boolean:
				if (REF_VAR(sl_bool const, _value)) {
					SLIB_RETURN_STRING("true")
				} else {
					SLIB_RETURN_STRING("false")
				}
			case VariantType::Time:
				return REF_VAR(Time const, _value).toString();
			case VariantType::String8:
				return REF_VAR(String const, _value);
			case VariantType::String16:
				return String::create(REF_VAR(String16 const, _value));
			case VariantType::Sz8:
				return String::create(REF_VAR(sl_char8 const* const, _value));
			case VariantType::Sz16:
				return String::create(REF_VAR(sl_char16 const* const, _value));
			case VariantType::Pointer:
				return "#" + String::fromPointerValue(REF_VAR(void const* const, _value));
			case VariantType::ObjectId:
				return REF_VAR(ObjectId, _value).toString();
			case VariantType::Null:
				if (_value) {
					return sl_null;
				}
				break;
			default:
				if (isMemory()) {
					CMemory* mem = REF_VAR(CMemory*, _value);
					return mem->getString();
				}
				break;
		}
		return def;
	}

	String Variant::getString() const noexcept
	{
		return getString(String::null());
	}

	String16 Variant::getString16(const String16& def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return String16::fromInt32(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return String16::fromUint32(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return String16::fromInt64(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return String16::fromUint64(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return String16::fromFloat(REF_VAR(float const, _value));
			case VariantType::Double:
				return String16::fromDouble(REF_VAR(double const, _value));
			case VariantType::Boolean:
				if (REF_VAR(sl_bool const, _value)) {
					SLIB_RETURN_STRING16("true")
				} else {
					SLIB_RETURN_STRING16("false")
				}
			case VariantType::Time:
				return String16::create(REF_VAR(Time const, _value).toString());
			case VariantType::String8:
				return String16::create(REF_VAR(String const, _value));
			case VariantType::String16:
				return REF_VAR(String16 const, _value);
			case VariantType::Sz8:
				return String16::create(REF_VAR(sl_char8 const* const, _value));
			case VariantType::Sz16:
				return String16::create(REF_VAR(sl_char16 const* const, _value));
			case VariantType::Pointer:
				return "#" + String16::fromPointerValue(REF_VAR(void const* const, _value));
			case VariantType::ObjectId:
				return String16::create(REF_VAR(ObjectId, _value).toString());
			default:
				if (isMemory()) {
					CMemory* mem = REF_VAR(CMemory*, _value);
					return String16::fromUtf8(mem->data, mem->size);
				}
				break;
		}
		return def;
	}

	String16 Variant::getString16() const noexcept
	{
		return getString16(String16::null());
	}

	const sl_char8* Variant::getSz8(const sl_char8* def) const noexcept
	{
		switch (_type) {
			case VariantType::Boolean:
				if (REF_VAR(sl_bool const, _value)) {
					return "true";
				} else {
					return "false";
				}
			case VariantType::Sz8:
				return REF_VAR(sl_char8 const* const, _value);
			default:
				break;
		}
		return def;
	}

	const sl_char16* Variant::getSz16(const sl_char16* def) const noexcept
	{
		switch (_type) {
			case VariantType::Boolean:
				if (REF_VAR(sl_bool const, _value)) {
					static const sl_char16 _s[] = {'t', 'r', 'u', 'e', 0};
					return _s;
				} else {
					static const sl_char16 _s[] = {'f', 'a', 'l', 's', 'e', 0};
					return _s;
				}
			case VariantType::Sz16:
				return REF_VAR(sl_char16 const* const, _value);
			default:
				break;
		}
		return def;
	}

	StringParam Variant::getStringParam(const StringParam& def) const noexcept
	{
		switch (_type) {
			case VariantType::String8:
				return REF_VAR(String const, _value);
			case VariantType::String16:
				return REF_VAR(String16 const, _value);
			case VariantType::Sz8:
				return StringParam(REF_VAR(sl_char8 const* const, _value), -1);
			case VariantType::Sz16:
				return StringParam(REF_VAR(sl_char16 const* const, _value), -1);
			case VariantType::Null:
				break;
			default:
				{
					String str = getString(String::null());
					if (str.isNotNull()) {
						return str;
					}
				}
				break;
		}
		return def;
	}
	
	StringParam Variant::getStringParam() const noexcept
	{
		return getStringParam(StringParam::null());
	}

	void Variant::setString(const String& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			_type = VariantType::String8;
			new PTR_VAR(String, _value) String(value);
		} else {
			setNull();
		}
	}

	void Variant::setString(String&& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			_type = VariantType::String8;
			new PTR_VAR(String, _value) String(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const String16& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			_type = VariantType::String16;
			new PTR_VAR(String16, _value) String16(value);
		} else {
			setNull();
		}
	}

	void Variant::setString(String16&& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			_type = VariantType::String16;
			new PTR_VAR(String16, _value) String16(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const AtomicString& s) noexcept
	{
		String value(s);
		if (value.isNotNull()) {
			Free(_type, _value);
			_type = VariantType::String8;
			new PTR_VAR(String, _value) String(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const AtomicString16& s) noexcept
	{
		String16 value(s);
		if (value.isNotNull()) {
			Free(_type, _value);
			_type = VariantType::String16;
			new PTR_VAR(String16, _value) String16(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const StringView& value) noexcept
	{
		setString(String(value));
	}

	void Variant::setString(const StringView16& value) noexcept
	{
		setString(String16(value));
	}

	void Variant::setString(const sl_char8* value) noexcept
	{
		if (value) {
			Free(_type, _value);
			_type = VariantType::Sz8;
			REF_VAR(const sl_char8*, _value) = value;
		} else {
			setNull();
		}
	}

	void Variant::setString(const sl_char16* value) noexcept
	{
		if (value) {
			Free(_type, _value);
			_type = VariantType::Sz16;
			REF_VAR(const sl_char16*, _value) = value;
		} else {
			setNull();
		}
	}
	
	std::string Variant::getStdString() const noexcept
	{
		return getString().toStd();
	}
	
	std::u16string Variant::getStdString16() const noexcept
	{
		return getString16().toStd();
	}
	
	void Variant::setString(const std::string& value) noexcept
	{
		setString(String::create(value));
	}
	
	void Variant::setString(const std::u16string& value) noexcept
	{
		setString(String16::create(value));
	}
	
	void Variant::setString(const StringParam& value) noexcept
	{
		set(value.toVariant());
	}

	sl_bool Variant::isTime() const noexcept
	{
		return _type == VariantType::Time;
	}

	sl_bool Variant::getTime(Time* _out) const noexcept
	{
		switch (_type) {
		case VariantType::Int32:
			if (_out) {
				*_out = Time::fromUnixTime(REF_VAR(sl_int32 const, _value));
			}
			return sl_true;
		case VariantType::Uint32:
			if (_out) {
				*_out = Time::fromUnixTime(REF_VAR(sl_uint32 const, _value));
			}
			return sl_true;
		case VariantType::Int64:
			if (_out) {
				*_out = Time::fromUnixTime(REF_VAR(sl_int64 const, _value));
			}
			return sl_true;
		case VariantType::Uint64:
			if (_out) {
				*_out = Time::fromUnixTime(REF_VAR(sl_uint64 const, _value));
			}
			return sl_true;
		case VariantType::Float:
			if (_out) {
				*_out = Time::fromUnixTimef(REF_VAR(float const, _value));
			}
			return sl_true;
		case VariantType::Double:
			if (_out) {
				*_out = Time::fromUnixTimef(REF_VAR(double const, _value));
			}
			return sl_true;
		case VariantType::Time:
			if (_out) {
				*_out = REF_VAR(Time const, _value);
			}
			return sl_true;
		case VariantType::String8:
			if (_out) {
				return _out->parse(REF_VAR(String const, _value));
			} else {
				Time t;
				return t.parse(REF_VAR(String const, _value));
			}
		case VariantType::String16:
			if (_out) {
				return _out->parse(REF_VAR(String16 const, _value));
			} else {
				Time t;
				return t.parse(REF_VAR(String16 const, _value));
			}
		case VariantType::Sz8:
			if (_out) {
				return _out->parse(StringParam(REF_VAR(sl_char8 const* const, _value), -1));
			} else {
				Time t;
				return t.parse(StringParam(REF_VAR(sl_char8 const* const, _value), -1));
			}
		case VariantType::Sz16:
			if (_out) {
				return _out->parse(StringParam(REF_VAR(sl_char16 const* const, _value), -1));
			} else {
				Time t;
				return t.parse(StringParam(REF_VAR(sl_char16 const* const, _value), -1));
			}
		default:
			break;
		}
		return sl_false;
	}

	Time Variant::getTime(const Time& def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return Time::fromUnixTime(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return Time::fromUnixTime(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return Time::fromUnixTime(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return Time::fromUnixTime(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return Time::fromUnixTimef(REF_VAR(float const, _value));
			case VariantType::Double:
				return Time::fromUnixTimef(REF_VAR(double const, _value));
			case VariantType::Time:
				return REF_VAR(Time const, _value);
			case VariantType::String8:
				return Time::fromString(REF_VAR(String const, _value));
			case VariantType::String16:
				return Time::fromString(REF_VAR(String16 const, _value));
			case VariantType::Sz8:
				return Time::fromString(StringParam(REF_VAR(sl_char8 const* const, _value), -1));
			case VariantType::Sz16:
				return Time::fromString(StringParam(REF_VAR(sl_char16 const* const, _value), -1));
			default:
				break;
		}
		return def;
	}

	Time Variant::getTime() const noexcept
	{
		return getTime(Time::zero());
	}

	void Variant::setTime(const Time& value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Time;
		REF_VAR(Time, _value) = value;
	}

	sl_bool Variant::isPointer() const noexcept
	{
		return _type == VariantType::Pointer || _type == VariantType::Sz8 || _type == VariantType::Sz16 || _type >= VariantType::Referable;
	}

	void* Variant::getPointer(const void* def) const noexcept
	{
		if (_type == VariantType::Pointer || _type == VariantType::Sz8 || _type == VariantType::Sz16 || _type >= VariantType::Referable) {
			return REF_VAR(void* const, _value);
		}
		return (void*)def;
	}

	void Variant::setPointer(const void* ptr) noexcept
	{
		if (ptr) {
			Free(_type, _value);
			_type = VariantType::Pointer;
			REF_VAR(const void*, _value) = ptr;
		} else {
			setNull();
		}
	}

	sl_bool Variant::isObjectId() const noexcept
	{
		return _type == VariantType::ObjectId;
	}

	ObjectId Variant::getObjectId() const noexcept
	{
		if (_type == VariantType::ObjectId) {
			return REF_VAR(ObjectId, _value);
		} else if (isString()) {
			return ObjectId(getString());
		}
		return sl_null;
	}

	sl_bool Variant::getObjectId(ObjectId* _out) const noexcept
	{
		if (_type == VariantType::ObjectId) {
			if (_out) {
				*_out = REF_VAR(ObjectId, _value);
			}
			return sl_true;
		} else if (isString()) {
			if (_out) {
				if (_out->parse(getString())) {
					return sl_true;
				}
				return sl_false;
			} else {
				ObjectId ret;
				return ret.parse(getString());
			}
		}
		return sl_false;
	}

	void Variant::setObjectId(const ObjectId& _id) noexcept
	{
		Free(_type, _value);
		_type = VariantType::ObjectId;
		CopyBytes12(&_value, _id.data);
	}

	sl_bool Variant::isRef() const noexcept
	{
		return IsReferable(_type);
	}

	Ref<Referable> Variant::getRef() const noexcept
	{
		if (_type == VariantType::Weak) {
			return REF_VAR(WeakRef<Referable> const, _value);
		} else if (IsReferable(_type)) {
			return REF_VAR(Ref<Referable> const, _value);
		}
		return sl_null;
	}

	sl_object_type Variant::getObjectType() const noexcept
	{
		if (_type == VariantType::Weak) {
			Ref<Referable> ref(REF_VAR(WeakRef<Referable> const, _value));
			if (ref.isNotNull()) {
				return ref->getObjectType();
			}
		} else if (IsReferable(_type)) {
			return REF_VAR(Ref<Referable> const, _value)->getObjectType();
		}
		return 0;
	}

	sl_bool Variant::isWeak() const noexcept
	{
		return _type == VariantType::Weak;
	}

	sl_bool Variant::isCollection() const noexcept
	{
		if (_type == VariantType::List) {
			return sl_true;
		}
		return IsObject<Collection, VariantType::Collection>(*this);
	}

	Ref<Collection> Variant::getCollection() const noexcept
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value).toCollection();
		}
		return GET_COLLECTION(*this);
	}

	sl_bool Variant::isVariantList() const noexcept
	{
		return _type == VariantType::List;
	}

	VariantList Variant::getVariantList() const noexcept
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return VariantList::create(collection.get());
			}
		}
		return sl_null;
	}

	void Variant::setVariantList(const VariantList& list) noexcept
	{
		_assignRef(&list, VariantType::List);
	}

	void Variant::setVariantList(VariantList&& list) noexcept
	{
		_assignMoveRef(&list, VariantType::List);
	}

	sl_bool Variant::isJsonList() const noexcept
	{
		return _type == VariantType::List;
	}

	JsonList Variant::getJsonList() const noexcept
	{
		if (_type == VariantType::List) {
			return REF_VAR(JsonList, _value);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return JsonList::create(collection.get());
			}
		}
		return sl_null;
	}

	void Variant::setJsonList(const JsonList& list) noexcept
	{
		_assignRef(&list, VariantType::List);
	}

	void Variant::setJsonList(JsonList&& list) noexcept
	{
		_assignMoveRef(&list, VariantType::List);
	}

	sl_uint64 Variant::getElementsCount() const
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value).getCount();
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->getElementsCount();
			}
		}
		return 0;
	}

	Variant Variant::getElement_NoLock(sl_uint64 index) const
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value).getValueAt_NoLock((sl_size)index);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->getElement(index);
			}
		}
		return Variant();
	}

	Variant Variant::getElement(sl_uint64 index) const
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value).getValueAt((sl_size)index);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->getElement(index);
			}
		}
		return Variant();
	}

	sl_bool Variant::setElement_NoLock(sl_uint64 index, const Variant& value) const
	{
		if (_type == VariantType::List) {
			if (value.isNotUndefined()) {
				return REF_VAR(VariantList, _value).setAt_NoLock((sl_size)index, value);
			} else {
				return REF_VAR(VariantList, _value).removeAt_NoLock((sl_size)index);
			}
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->setElement(index, value);
			}
		}
		return sl_false;
	}

	sl_bool Variant::setElement(sl_uint64 index, const Variant& value) const
	{
		if (_type == VariantType::List) {
			if (value.isNotUndefined()) {
				return REF_VAR(VariantList, _value).setAt((sl_size)index, value);
			} else {
				return REF_VAR(VariantList, _value).removeAt((sl_size)index);
			}
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->setElement(index, value);
			}
		}
		return sl_false;
	}

	sl_bool Variant::addElement_NoLock(const Variant& value) const
	{
		if (isNotNull()) {
			if (_type == VariantType::List) {
				return REF_VAR(VariantList, _value).add_NoLock(value);
			} else {
				Ref<Collection> collection(GET_COLLECTION(*this));
				if (collection.isNotNull()) {
					return collection->addElement(value);
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::addElement_NoLock(const Variant& value)
	{
		if (isNotNull()) {
			if (_type == VariantType::List) {
				return REF_VAR(VariantList, _value).add_NoLock(value);
			} else {
				Ref<Collection> collection(GET_COLLECTION(*this));
				if (collection.isNotNull()) {
					return collection->addElement(value);
				}
			}
		} else {
			auto list = VariantList::createFromElement(value);
			if (list.isNotNull()) {
				setVariantList(list);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Variant::addElement(const Variant& value) const
	{
		if (isNotNull()) {
			if (_type == VariantType::List) {
				return REF_VAR(VariantList, _value).add(value);
			} else {
				Ref<Collection> collection(GET_COLLECTION(*this));
				if (collection.isNotNull()) {
					return collection->addElement(value);
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::addElement(const Variant& value)
	{
		if (isNotNull()) {
			if (_type == VariantType::List) {
				return REF_VAR(VariantList, _value).add(value);
			} else {
				Ref<Collection> collection(GET_COLLECTION(*this));
				if (collection.isNotNull()) {
					return collection->addElement(value);
				}
			}
		} else {
			auto list = VariantList::createFromElement(value);
			if (list.isNotNull()) {
				setVariantList(list);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Variant::isObject() const noexcept
	{
		if (_type == VariantType::Map) {
			return sl_true;
		}
		return IsObject<Object, VariantType::Object>(*this);
	}

	Ref<Object> Variant::getObject() const noexcept
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).toObject();
		} else {
			return GET_OBJECT(*this);
		}
	}

	sl_bool Variant::isVariantMap() const noexcept
	{
		return _type == VariantType::Map;
	}

	VariantMap Variant::getVariantMap() const noexcept
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return VariantMap::create(object.get());
			}
		}
		return sl_null;
	}

	void Variant::setVariantMap(const VariantMap& map) noexcept
	{
		_assignRef(&map, VariantType::Map);
	}

	void Variant::setVariantMap(VariantMap&& map) noexcept
	{
		_assignMoveRef(&map, VariantType::Map);
	}

	sl_bool Variant::isJsonMap() const noexcept
	{
		return _type == VariantType::Map;
	}

	JsonMap Variant::getJsonMap() const noexcept
	{
		if (_type == VariantType::Map) {
			return REF_VAR(JsonMap, _value);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return JsonMap::create(object.get());
			}
		}
		return sl_null;
	}

	void Variant::setJsonMap(const JsonMap& map) noexcept
	{
		_assignRef(&map, VariantType::Map);
	}

	void Variant::setJsonMap(JsonMap&& map) noexcept
	{
		_assignMoveRef(&map, VariantType::Map);
	}

	Variant Variant::getItem_NoLock(const StringParam& key) const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).getValue_NoLock(key.toString());
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->getProperty(key);
			}
		}
		return Variant();
	}

	Variant Variant::getItem(const StringParam& key) const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).getValue(key.toString());
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->getProperty(key);
			}
		}
		return Variant();
	}

	sl_bool Variant::putItem_NoLock(const StringParam& key, const Variant& value) const
	{
		if (value.isUndefined()) {
			return removeItem_NoLock(key);
		}
		if (isNotNull()) {
			if (_type == VariantType::Map) {
				return REF_VAR(VariantMap, _value).put_NoLock(key.toString(), value) != sl_null;
			} else {
				Ref<Object> object(GET_OBJECT(*this));
				if (object.isNotNull()) {
					return object->setProperty(key, value);
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::putItem_NoLock(const StringParam& key, const Variant& value)
	{
		if (value.isUndefined()) {
			return removeItem_NoLock(key);
		}
		if (isNotNull()) {
			if (_type == VariantType::Map) {
				return REF_VAR(VariantMap, _value).put_NoLock(key.toString(), value) != sl_null;
			} else {
				Ref<Object> object(GET_OBJECT(*this));
				if (object.isNotNull()) {
					return object->setProperty(key, value);
				}
			}
		} else {
			VariantMap map = VariantMap::create();
			if (map.isNotNull()) {
				if (map.put_NoLock(key.toString(), value)) {
					setVariantMap(map);
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::putItem(const StringParam& key, const Variant& value) const
	{
		if (value.isUndefined()) {
			return removeItem(key);
		}
		if (isNotNull()) {
			if (_type == VariantType::Map) {
				return REF_VAR(VariantMap, _value).put(key.toString(), value);
			} else {
				Ref<Object> object(GET_OBJECT(*this));
				if (object.isNotNull()) {
					return object->setProperty(key, value);
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::putItem(const StringParam& key, const Variant& value)
	{
		if (value.isUndefined()) {
			return removeItem(key);
		}
		if (isNotNull()) {
			if (_type == VariantType::Map) {
				return REF_VAR(VariantMap, _value).put(key.toString(), value);
			} else {
				Ref<Object> object(GET_OBJECT(*this));
				if (object.isNotNull()) {
					return object->setProperty(key, value);
				}
			}
		} else {
			VariantMap map = VariantMap::create();
			if (map.isNotNull()) {
				if (map.put_NoLock(key.toString(), value)) {
					setVariantMap(map);
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::removeItem_NoLock(const StringParam& key) const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).remove_NoLock(key.toString());
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->clearProperty(key);
			}
		}
		return sl_false;
	}

	sl_bool Variant::removeItem(const StringParam& key) const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).remove(key.toString());
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->clearProperty(key);
			}
		}
		return sl_false;
	}

	PropertyIterator Variant::getItemIterator() const
	{
		if (_type == VariantType::Map) {
			VariantMap& map = REF_VAR(VariantMap, _value);
			return new MapIterator< CHashMap<String, Variant> >(map.ref);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->getPropertyIterator();
			}
		}
		return sl_null;
	}

	sl_bool Variant::isMemory() const noexcept
	{
		return IsObject<CMemory, VariantType::Memory>(*this);
	}

	Memory Variant::getMemory() const noexcept
	{
		if (_type == VariantType::Memory) {
			return REF_VAR(Memory, _value);
		} else if (isString()) {
			return getString().toMemory();
		} else if (isRef()) {
			return GetObjectT<CMemory, Memory, VariantType::Memory>(*this);
		}
		return sl_null;
	}

	void Variant::setMemory(const Memory& mem) noexcept
	{
		_assignRef(&mem, VariantType::Memory);
	}

	void Variant::setMemory(Memory&& mem) noexcept
	{
		_assignMoveRef(&mem, VariantType::Memory);
	}

	sl_bool Variant::isVariantPromise() const noexcept
	{
		return _type == VariantType::Promise;
	}

	Promise<Variant> Variant::getVariantPromise() const noexcept
	{
		if (_type == VariantType::Promise) {
			return REF_VAR(Promise<Variant> const, _value);
		}
		return sl_null;
	}

	void Variant::setVariantPromise(const Promise<Variant>& promise) noexcept
	{
		_assignRef(&promise, VariantType::Promise);
	}

	void Variant::setVariantPromise(Promise<Variant>&& promise) noexcept
	{
		_assignMoveRef(&promise, VariantType::Promise);
	}

	void Variant::merge(const Variant& other)
	{
		if (other.isNull()) {
			return;
		}
		if (isNull()) {
			*this = other;
			return;
		}
		if (_type == VariantType::Map) {
			VariantMap& dst = REF_VAR(VariantMap, _value);
			if (other._type == VariantType::Map) {
				VariantMap& src = REF_VAR(VariantMap, other._value);
				dst.putAll(src);
			} else {
				Ref<Object> src(getObject());
				if (src.isNotNull()) {
					MutexLocker lock(dst.getLocker());
					PropertyIterator iterator = src->getPropertyIterator();
					while (iterator.moveNext()) {
						dst.put_NoLock(iterator.getKey(), iterator.getValue());
					}
				}
			}
		} else if (_type == VariantType::List) {
			VariantList& dst = REF_VAR(VariantList, _value);
			if (other._type == VariantType::List) {
				VariantList& src = REF_VAR(VariantList, other._value);
				dst.addAll(src);
			} else {
				Ref<Collection> src(getCollection());
				if (src.isNotNull()) {
					MutexLocker lock(dst.getLocker());
					sl_size n = (sl_size)(src->getElementsCount());
					for (sl_size i = 0; i < n; i++) {
						dst.add_NoLock(src->getElement(i));
					}
				}
			}
		} else if (IsReferable(_type)) {
			Ref<Referable> ref = getRef();
			if (IsInstanceOf<Object>(ref)) {
				Ref<Object>& dst = Ref<Object>::from(ref);
				if (other._type == VariantType::Map) {
					VariantMap& src = REF_VAR(VariantMap, other._value);
					MutexLocker lock(src.getLocker());
					auto node = src.getFirstNode();
					while (node) {
						dst->setProperty(node->key, node->value);
						node = node->getNext();
					}
				} else {
					Ref<Object> src(getObject());
					if (src.isNotNull()) {
						PropertyIterator iterator = src->getPropertyIterator();
						while (iterator.moveNext()) {
							dst->setProperty(iterator.getKey(), iterator.getValue());
						}
					}
				}
			} else if (IsInstanceOf<Collection>(ref)) {
				Ref<Collection>& dst = Ref<Collection>::from(ref);
				if (other._type == VariantType::List) {
					ListLocker<Variant> src(REF_VAR(VariantList, other._value));
					for (sl_size i = 0; i < src.count; i++) {
						dst->addElement(src[i]);
					}
				} else {
					Ref<Collection> src(getCollection());
					if (src.isNotNull()) {
						sl_size n = (sl_size)(src->getElementsCount());
						for (sl_size i = 0; i < n; i++) {
							dst->addElement(src->getElement(i));
						}
					}
				}
			}
		}
	}

	String Variant::toString() const
	{
		switch (_type) {
			case VariantType::Null:
				return String::null();
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Float:
			case VariantType::Double:
			case VariantType::Boolean:
			case VariantType::String8:
			case VariantType::String16:
			case VariantType::Sz8:
			case VariantType::Sz16:
			case VariantType::Time:
			case VariantType::Pointer:
			case VariantType::ObjectId:
			case VariantType::Memory:
				return getString();
			case VariantType::Weak:
				{
					Ref<Referable> ref(getRef());
					if (ref.isNotNull()) {
						return ref->toString();
					} else {
						return "<null>";
					}
				}
			case VariantType::List:
			case VariantType::Map:
				return toJsonString();
			default:
				if (IsReferable(_type)) {
					return REF_VAR(Referable*, _value)->toString();
				}
				return "<error-type>";
		}
	}

	sl_bool Variant::toJsonString(StringBuffer& buf) const
	{
		switch (_type) {
			case VariantType::List:
				{
					ListLocker<Variant> list(REF_VAR(VariantList, _value));
					if (!(buf.addStatic("["))) {
						return sl_false;
					}
					for (sl_size i = 0; i < list.count; i++) {
						if (i) {
							if (!(buf.addStatic(", "))) {
								return sl_false;
							}
						}
						if (!(list[i].toJsonString(buf))) {
							return sl_false;
						}
					}
					if (!(buf.addStatic("]"))) {
						return sl_false;
					}
					return sl_true;
				}
			case VariantType::Map:
				{
					VariantMap& map = REF_VAR(VariantMap, _value);
					MutexLocker locker(map.getLocker());
					if (!(buf.addStatic("{"))) {
						return sl_false;
					}
					sl_bool flagFirst = sl_true;
					auto node = map.getFirstNode();
					while (node) {
						Variant& v = node->value;
						if (v.isNotUndefined()) {
							if (!flagFirst) {
								if (!(buf.addStatic(", "))) {
									return sl_false;
								}
							}
							if (!(buf.add(ParseUtil::applyBackslashEscapes(node->key)))) {
								return sl_false;
							}
							if (!(buf.addStatic(": "))) {
								return sl_false;
							}
							if (!(v.toJsonString(buf))) {
								return sl_false;
							}
							flagFirst = sl_false;
						}
						node = node->getNext();
					}
					if (!(buf.addStatic("}"))) {
						return sl_false;
					}
					return sl_true;
				}
			case VariantType::Weak:
				{
					Ref<Referable> ref(getRef());
					if (ref.isNotNull()) {
						return ref->toJsonString(buf);
					} else {
						return buf.addStatic("null");
					}
				}
			default:
				if (IsReferable(_type)) {
					return REF_VAR(Referable*, _value)->toJsonString(buf);
				} else {
					return buf.add(toJsonString());
				}
		}
	}

	String Variant::toJsonString() const
	{
		switch (_type) {
			case VariantType::Null:
				break;
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Float:
			case VariantType::Double:
			case VariantType::Boolean:
				return getString();
			case VariantType::Time:
			case VariantType::String8:
			case VariantType::Sz8:
				return ParseUtil::applyBackslashEscapes(getString());
			case VariantType::String16:
			case VariantType::Sz16:
				return String::create(ParseUtil::applyBackslashEscapes16(getString16()));
			case VariantType::ObjectId:
				return REF_VAR(ObjectId, _value).toJson().toJsonString();
			default:
				if (IsReferable(_type)) {
					StringBuffer buf;
					if (toJsonString(buf)) {
						return buf.merge();
					}
				}
				break;
		}
		SLIB_RETURN_STRING("null")
	}

	sl_bool SerializeJsonBinary(MemoryBuffer* output, Referable* ref)
	{
		return ref->toJsonBinary(*output);
	}

	sl_size SerializeVariantPrimitive(const Variant& var, void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		switch (var._type) {
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Float:
				if (size < 5) {
					return 0;
				}
				MIO::writeUint32LE(buf + 1, *((sl_uint32*)(void*)&(var._value)));
				size = 5;
				break;
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Double:
			case VariantType::Time:
				if (size < 9) {
					return 0;
				}
				MIO::writeUint64LE(buf + 1, *((sl_uint64*)(void*)&(var._value)));
				size = 9;
				break;
			case VariantType::ObjectId:
				if (size < 13) {
					return 0;
				}
				Base::copyMemory(buf + 1, &(var._value), 12);
				size = 13;
				break;
			case VariantType::Boolean:
				if (size < 2) {
					return 0;
				}
				buf[1] = *((bool*)(void*)&(var._value)) ? 1 : 0;
				size = 2;
				break;
			case VariantType::Null:
				if (size < 1) {
					return 0;
				}
				size = 1;
				break;
			default:
				return 0;
		}
		buf[0] = var._type;
		return size;
	}

#define SERIALIZE_PREPARE_MEMORY(BUF, SIZE, REQ_SIZE, PMEM) \
	if (SIZE < REQ_SIZE) { \
		if (PMEM) { \
			Memory mem = Memory::create(REQ_SIZE); \
			if (mem.isNotNull()) { \
				BUF = (sl_uint8*)(mem.getData()); \
				*PMEM = Move(mem); \
			} else { \
				return 0; \
			} \
		} else { \
			return 0; \
		} \
	}

	sl_size SerializeVariant(const Variant& var, void* _buf, sl_size size, Memory* pOutMemoryIfInsufficient, const void* prefix, sl_size sizePrefix)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size > sizePrefix) {
			sl_size nWritten = SerializeVariantPrimitive(var, buf + sizePrefix, size - sizePrefix);
			if (nWritten) {
				if (sizePrefix) {
					Base::copyMemory(buf, prefix, sizePrefix);
				}
				return sizePrefix + nWritten;
			}
		}
		switch (var._type) {
			case VariantType::String8:
			case VariantType::String16:
			case VariantType::Sz8:
			case VariantType::Sz16:
				{
					String str = var.getString();
					sl_size n = str.getLength();
					sl_size nReq = sizePrefix + 11 + n;
					SERIALIZE_PREPARE_MEMORY(buf, size, nReq, pOutMemoryIfInsufficient)
					if (sizePrefix) {
						Base::copyMemory(buf, prefix, sizePrefix);
					}
					buf[sizePrefix] = VariantType::String8;
					sl_size l = sizePrefix + 1;
					l += CVLI::serialize(buf + l, n);
					Base::copyMemory(buf + l, str.getData(), n);
					return l + n;
				}
			case VariantType::Memory:
				{
					Memory& m = *((Memory*)(void*)&(var._value));
					sl_size n = m.getSize();
					sl_size nReq = sizePrefix + 11 + n;
					SERIALIZE_PREPARE_MEMORY(buf, size, nReq, pOutMemoryIfInsufficient)
					if (sizePrefix) {
						Base::copyMemory(buf, prefix, sizePrefix);
					}
					buf[sizePrefix] = VariantType::Memory;
					sl_size l = sizePrefix + 1;
					l += CVLI::serialize(buf + l, n);
					Base::copyMemory(buf + l, m.getData(), n);
					return l + n;
				}
			default:
				if (IsReferable(var._type)) {
					MemoryBuffer mb;
					if (sizePrefix) {
						if (!(mb.addStatic(prefix, sizePrefix))) {
							return 0;
						}
					}
					if (var.serialize(&mb)) {
						Memory mem = mb.merge();
						sl_size n = mem.getSize();
						if (n) {
							if (size >= n) {
								Base::copyMemory(buf, mem.getData(), n);
								return n;
							} else {
								if (pOutMemoryIfInsufficient) {
									*pOutMemoryIfInsufficient = Move(mem);
									return n;
								}
							}
						}
					}
				}
				break;
		}
		return 0;
	}

	Memory Variant::serialize() const
	{
		Memory mem;
		SerializeVariant(*this, sl_null, 0, &mem);
		return mem;
	}

	sl_bool Variant::serialize(MemoryBuffer* buf) const
	{
		return serialize<MemoryBuffer>(buf);
	}

	sl_size Variant::deserialize(const void* data, sl_size size)
	{
		SerializeBuffer buf(data, size);
		if (Deserialize(&buf, *this)) {
			return buf.current - buf.begin;
		} else {
			return 0;
		}
	}

	sl_size Variant::deserialize(const MemoryData& mem)
	{
		DeserializeBuffer buf(mem);
		if (Deserialize(&buf, *this)) {
			return buf.current - buf.begin;
		} else {
			return 0;
		}
	}

	sl_size Variant::deserialize(MemoryData&& mem)
	{
		DeserializeBuffer buf(Move(mem));
		if (Deserialize(&buf, *this)) {
			return buf.current - buf.begin;
		} else {
			return 0;
		}
	}

	sl_size Variant::deserialize(const Memory& mem)
	{
		DeserializeBuffer buf(mem);
		if (Deserialize(&buf, *this)) {
			return buf.current - buf.begin;
		} else {
			return 0;
		}
	}

	sl_size Variant::deserialize(Memory&& mem)
	{
		DeserializeBuffer buf(Move(mem));
		if (Deserialize(&buf, *this)) {
			return buf.current - buf.begin;
		} else {
			return 0;
		}
	}

	sl_compare_result Variant::compare(const Variant& v2) const noexcept
	{
		const Variant& v1 = *this;
		sl_uint8 type = v1._type;
		if (type == v2._type) {
			switch (type) {
				case VariantType::Null:
					return 0;
				case VariantType::Int32:
					return ComparePrimitiveValues(REF_VAR(sl_int32 const, v1._value), REF_VAR(sl_int32 const, v2._value));
				case VariantType::Uint32:
				case VariantType::Boolean:
					return ComparePrimitiveValues(REF_VAR(sl_uint32 const, v1._value), REF_VAR(sl_uint32 const, v2._value));
				case VariantType::Int64:
					return ComparePrimitiveValues(REF_VAR(sl_int64 const, v1._value), REF_VAR(sl_int64 const, v2._value));
				case VariantType::Float:
					return ComparePrimitiveValues(REF_VAR(float const, v1._value), REF_VAR(float const, v2._value));
				case VariantType::Double:
					return ComparePrimitiveValues(REF_VAR(double const, v1._value), REF_VAR(double const, v2._value));
				case VariantType::Sz8:
					return Base::compareString(REF_VAR(sl_char8 const* const, v1._value), REF_VAR(sl_char8 const* const, v2._value));
				case VariantType::Sz16:
					return Base::compareString2(REF_VAR(sl_char16 const* const, v1._value), REF_VAR(sl_char16 const* const, v2._value));
				case VariantType::String8:
					return REF_VAR(String const, v1._value).compare(REF_VAR(String const, v2._value));
				case VariantType::String16:
					return REF_VAR(String16 const, v1._value).compare(REF_VAR(String16 const, v2._value));
				case VariantType::Pointer:
					return ComparePrimitiveValues(REF_VAR(sl_size const, v1._value), REF_VAR(sl_size const, v2._value));
				case VariantType::ObjectId:
					return REF_VAR(ObjectId const, v1._value).compare(REF_VAR(ObjectId const, v2._value));
				default:
					if (type >= VariantType::Referable) {
						return ComparePrimitiveValues(REF_VAR(sl_size const, v1._value), REF_VAR(sl_size const, v2._value));
					} else {
						return ComparePrimitiveValues(v1._value, v2._value);
					}
			}
		} else {
			switch (type) {
				case VariantType::String8:
					return CompareString(PTR_VAR(String const, v1._value), v2);
				case VariantType::String16:
					return CompareString(PTR_VAR(String16 const, v1._value), v2);
				case VariantType::Sz8:
					return CompareString(PTR_VAR(sl_char8 const* const, v1._value), v2);
				case VariantType::Sz16:
					return CompareString(PTR_VAR(sl_char16 const* const, v1._value), v2);
				default:
					ComparePrimitiveValues((int)type, (int)(v2._type));
			}
		}
		return 0;
	}

	sl_bool Variant::equals(const Variant& v2) const noexcept
	{
		const Variant& v1 = *this;
		sl_uint8 type = v1._type;
		if (type == v2._type) {
			switch (type) {
				case VariantType::Null:
					return sl_true;
				case VariantType::Int32:
				case VariantType::Uint32:
					return REF_VAR(sl_int32 const, v1._value) == REF_VAR(sl_int32 const, v2._value);
				case VariantType::Float:
					return REF_VAR(float const, v1._value) == REF_VAR(float const, v2._value);
				case VariantType::Double:
					return REF_VAR(double const, v1._value) == REF_VAR(double const, v2._value);
				case VariantType::Boolean:
					return REF_VAR(sl_bool const, v1._value) == REF_VAR(sl_bool const, v2._value);
				case VariantType::Sz8:
					return Base::compareString(REF_VAR(sl_char8 const* const, v1._value), REF_VAR(sl_char8 const* const, v2._value)) == 0;
				case VariantType::Sz16:
					return Base::compareString2(REF_VAR(sl_char16 const* const, v1._value), REF_VAR(sl_char16 const* const, v2._value)) == 0;
				case VariantType::String8:
					return REF_VAR(String const, v1._value) == REF_VAR(String const, v2._value);
				case VariantType::String16:
					return REF_VAR(String16 const, v1._value) == REF_VAR(String16 const, v2._value);
				case VariantType::Pointer:
				case VariantType::ObjectId:
					return REF_VAR(ObjectId const, v1._value).equals(REF_VAR(ObjectId const, v2._value));
				default:
					if (type >= VariantType::Referable) {
						return REF_VAR(void const* const, v1._value) == REF_VAR(void const* const, v2._value);
					} else {
						return v1._value == v2._value;
					}
			}
		} else {
			switch (type) {
				case VariantType::Int32:
					return EqualsVariant(PTR_VAR(sl_int32 const, v1._value), v2);
				case VariantType::Uint32:
					return EqualsVariant(PTR_VAR(sl_uint32 const, v1._value), v2);
				case VariantType::Int64:
					return EqualsVariant(PTR_VAR(sl_int64 const, v1._value), v2);
				case VariantType::Uint64:
					return EqualsVariant(PTR_VAR(sl_uint64 const, v1._value), v2);
				case VariantType::Float:
					return EqualsVariant(PTR_VAR(float const, v1._value), v2);
				case VariantType::Double:
					return EqualsVariant(PTR_VAR(double const, v1._value), v2);
				case VariantType::String8:
					return EqualsVariant(PTR_VAR(String const, v1._value), v2);
				case VariantType::String16:
					return EqualsVariant(PTR_VAR(String16 const, v1._value), v2);
				case VariantType::Sz8:
					return EqualsVariant(PTR_VAR(sl_char8 const* const, v1._value), v2);
				case VariantType::Sz16:
					return EqualsVariant(PTR_VAR(sl_char16 const* const, v1._value), v2);
				default:
					break;
			}
		}
		return sl_false;
	}

	sl_size Variant::getHashCode() const noexcept
	{
		sl_uint8 type = _type;
		switch (type) {
			case VariantType::Null:
				return 0;
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Boolean:
			case VariantType::Float:
				return Rehash32(REF_VAR(sl_int32 const, _value));
			case VariantType::Sz8:
				return getString().getHashCode();
			case VariantType::String8:
				return REF_VAR(String const, _value).getHashCode();
			case VariantType::Sz16:
				return getString16().getHashCode();
			case VariantType::String16:
				return REF_VAR(String16 const, _value).getHashCode();
			case VariantType::Pointer:
			case VariantType::Object:
				return Rehash(REF_VAR(sl_size const, _value));
			default:
				return Rehash64ToSize(_value);
		}
		return 0;
	}

	void Variant::get(Variant& _out) const noexcept
	{
		_out._assign(*this);
	}
	
	void Variant::get(Atomic<Variant>& _out) const noexcept
	{
		_out._assign_copy(this);
	}

	void Variant::get(Json& _out) const noexcept
	{
		_out._assign(*this);
	}

	void Variant::get(signed char& _out) const noexcept
	{
		_out = (signed char)(getInt32());
	}
	
	void Variant::get(signed char& _out, signed char def) const noexcept
	{
		_out = (signed char)(getInt32((sl_int32)def));
	}
	
	void Variant::get(unsigned char& _out) const noexcept
	{
		_out = (unsigned char)(getUint32());
	}
	
	void Variant::get(unsigned char& _out, unsigned char def) const noexcept
	{
		_out = (unsigned char)(getUint32((sl_uint32)def));
	}
	
	void Variant::get(short& _out) const noexcept
	{
		_out = (short)(getInt32());
	}
	
	void Variant::get(short& _out, short def) const noexcept
	{
		_out = (short)(getInt32((sl_int32)def));
	}
	
	void Variant::get(unsigned short& _out) const noexcept
	{
		_out = (unsigned short)(getUint32());
	}
	
	void Variant::get(unsigned short& _out, unsigned short def) const noexcept
	{
		_out = (unsigned short)(getUint32((sl_uint32)def));
	}
	
	void Variant::get(int& _out) const noexcept
	{
		_out = (int)(getInt32());
	}
	
	void Variant::get(int& _out, int def) const noexcept
	{
		_out = (int)(getInt32((sl_int32)def));
	}
	
	void Variant::get(unsigned int& _out) const noexcept
	{
		_out = (unsigned int)(getUint32());
	}
	
	void Variant::get(unsigned int& _out, unsigned int def) const noexcept
	{
		_out = (unsigned int)(getUint32((sl_uint32)def));
	}
	
	void Variant::get(long& _out) const noexcept
	{
		_out = (long)(getInt32());
	}
	
	void Variant::get(long& _out, long def) const noexcept
	{
		_out = (long)(getInt32((sl_int32)def));
	}
	
	void Variant::get(unsigned long& _out) const noexcept
	{
		_out = (unsigned long)(getUint32());
	}
	
	void Variant::get(unsigned long& _out, unsigned long def) const noexcept
	{
		_out = (unsigned long)(getUint32((sl_uint32)def));
	}
	
	void Variant::get(sl_int64& _out) const noexcept
	{
		_out = getInt64();
	}
	
	void Variant::get(sl_int64& _out, sl_int64 def) const noexcept
	{
		_out = getInt64(def);
	}
	
	void Variant::get(sl_uint64& _out) const noexcept
	{
		_out = getUint64();
	}
	
	void Variant::get(sl_uint64& _out, sl_uint64 def) const noexcept
	{
		_out = getUint64(def);
	}
	
	void Variant::get(float& _out) const noexcept
	{
		_out = getFloat();
	}
	
	void Variant::get(float& _out, float def) const noexcept
	{
		_out = getFloat(def);
	}
	
	void Variant::get(double& _out) const noexcept
	{
		_out = getDouble();
	}
	
	void Variant::get(double& _out, double def) const noexcept
	{
		_out = getDouble(def);
	}
	
	void Variant::get(bool& _out) const noexcept
	{
		_out = getBoolean();
	}
	
	void Variant::get(bool& _out, bool def) const noexcept
	{
		_out = getBoolean(def);
	}
	
	void Variant::get(String& _out) const noexcept
	{
		_out = getString();
	}
	
	void Variant::get(String& _out, const String& def) const noexcept
	{
		_out = getString(def);
	}
	
	void Variant::get(AtomicString& _out) const noexcept
	{
		_out = getString();
	}
	
	void Variant::get(AtomicString& _out, const String& def) const noexcept
	{
		_out = getString(def);
	}
	
	void Variant::get(String16& _out) const noexcept
	{
		_out = getString16();
	}
	
	void Variant::get(String16& _out, const String16& def) const noexcept
	{
		_out = getString16(def);
	}
	
	void Variant::get(AtomicString16& _out) const noexcept
	{
		_out = getString16();
	}
	
	void Variant::get(AtomicString16& _out, const String16& def) const noexcept
	{
		_out = getString16(def);
	}
	
	void Variant::get(std::string& _out) const noexcept
	{
		_out = getString().toStd();
	}
	
	void Variant::get(std::u16string& _out) const noexcept
	{
		_out = getString16().toStd();
	}
	
	void Variant::get(Time& _out) const noexcept
	{
		_out = getTime();
	}
	
	void Variant::get(Time& _out, const Time& def) const noexcept
	{
		_out = getTime(def);
	}

	void Variant::get(VariantList& _out) const noexcept
	{
		_out = getVariantList();
	}

	void Variant::get(VariantMap& _out) const noexcept
	{
		_out = getVariantMap();
	}

	void Variant::get(JsonList& _out) const noexcept
	{
		_out = getJsonList();
	}

	void Variant::get(JsonMap& _out) const noexcept
	{
		_out = getJsonMap();
	}

	void Variant::get(Memory& _out) const noexcept
	{
		_out = getMemory();
	}

	void Variant::get(Promise<Variant>& _out) const noexcept
	{
		_out = getVariantPromise();
	}
	

	SLIB_DEFINE_DEFAULT_COMPARE_OPERATORS(Variant)

	
	const Variant& Cast<Variant, Variant>::operator()(const Variant& var) const noexcept
	{
		return var;
	}

	String Cast<Variant, String>::operator()(const Variant& var) const noexcept
	{
		return String::from(var);
	}

	String16 Cast<Variant, String16>::operator()(const Variant& var) const noexcept
	{
		return String16::from(var);
	}


	ObjectId::ObjectId() noexcept
	{
	}

	ObjectId::ObjectId(sl_null_t) noexcept
	{
		ZeroBytes12(data);
	}

	ObjectId::ObjectId(const StringParam& _id) noexcept
	{
		if (!(parse(_id))) {
			ZeroBytes12(data);
		}
	}

	ObjectId::ObjectId(const sl_uint8* _id) noexcept: Bytes(_id)
	{
	}

	ObjectId ObjectId::generate() noexcept
	{
		static sl_uint64 random = 0;
		if (!random) {
			Math::randomMemory(&random, sizeof(random));
		}
		static sl_reg counter = 0;
		sl_reg n = Base::interlockedIncrement((sl_reg*)&counter);
		ObjectId ret;
		MIO::writeUint32BE(ret.data, (sl_uint32)(Time::now().toUnixTime()));
		MIO::writeUint64BE(ret.data + 4, random + n);
		return ret;
	}

	sl_size ObjectId::getHashCode() const noexcept
	{
		return Rehash64ToSize(*(sl_uint64*)data ^ *(sl_uint32*)(data + 8));
	}

	String ObjectId::toString() const noexcept
	{
		return Bytes::toString();
	}

	sl_bool ObjectId::parse(const StringParam& str) noexcept
	{
		return Bytes::parse(str);
	}

}
