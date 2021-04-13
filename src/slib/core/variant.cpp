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
			
			char g_variantMap_ClassID[1];
			char g_variantHashMap_ClassID[1];
			char g_variantList_ClassID[1];
			char g_variantMapList_ClassID[1];
			char g_variantHashMapList_ClassID[1];
		
			char g_variantPromise_ClassID[1];

			const ConstContainer g_undefined = {0, VariantType::Null, 0};
			const ConstContainer g_null = {1, VariantType::Null, 0};

			SLIB_INLINE static sl_bool IsSharedPtr(VariantType type)
			{
				return type >= VariantType::SharedPtr && type < VariantType::Referable;
			}

			SLIB_INLINE static sl_bool IsReferable(VariantType type)
			{
				return type >= VariantType::Referable;
			}

			SLIB_INLINE static void Copy(VariantType src_type, sl_uint64 src_value, sl_uint64& dst_value) noexcept
			{
				switch (src_type) {
					case VariantType::String8:
						new PTR_VAR(String, dst_value) String(REF_VAR(String, src_value));
						break;
					case VariantType::String16:
						new PTR_VAR(String16, dst_value) String16(REF_VAR(String16, src_value));
						break;
					default:
						if (IsReferable(src_type)) {
							new PTR_VAR(Ref<Referable>, dst_value) Ref<Referable>(REF_VAR(Ref<Referable>, src_value));
						} else {
							dst_value = src_value;
						}
						break;
				}
			}

			SLIB_INLINE static void Free(VariantType type, sl_uint64 value) noexcept
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
						} else if (type >= VariantType::SharedPtr) {
							REF_VAR(SharedPtr<void>, value).SharedPtr<void>::~SharedPtr();
						}
						break;
				}
			}

			template <class T, VariantType type>
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

			template <class T, class OT, VariantType type>
			SLIB_INLINE static OT GetObjectT(const Variant& v)
			{
				if (v._type == type) {
					return REF_VAR(OT, v._type);
				}
				if (v._type == VariantType::Weak) {
					Ref<Referable> ref(REF_VAR(WeakRef<Referable> const, v._value));
					if (IsInstanceOf<T>(ref)) {
						return REF_VAR(OT, ref);
					}
				} else if (IsReferable(v._type)) {
					if (IsInstanceOf<T>(REF_VAR(Referable*, v._type))) {
						return REF_VAR(OT, v._type);
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
				VariantType type = v2._type;
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
				VariantType type = v2._type;
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
			Copy(_type, other._value, _value);
		}
	}

	void Variant::_assignMove(Variant& other) noexcept
	{
		if (this != &other) {
			Free(_type, _value);
			_type = other._type;
			_value = other._value;
			other._type = VariantType::Null;
		}
	}

	void Variant::_constructorSharedPtr(const void* ptr, VariantType type) noexcept
	{
		const SharedPtr<void>& ref = *reinterpret_cast<SharedPtr<void> const*>(ptr);
		if (ref.isNotNull()) {
			_type = type;
			new (reinterpret_cast<SharedPtr<void>*>(&_value)) SharedPtr<void>(ref);
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	void Variant::_constructorMoveSharedPtr(void* ptr, VariantType type) noexcept
	{
		SharedPtr<void>& ref = *reinterpret_cast<SharedPtr<void>*>(ptr);
		if (ref.isNotNull()) {
			_type = type;
			new (reinterpret_cast<SharedPtr<void>*>(&_value)) SharedPtr<void>(Move(ref));
		} else {
			_type = VariantType::Null;
			_value = 1;
		}
	}

	void Variant::_assignSharedPtr(const void* ptr, VariantType type) noexcept
	{
		Free(_type, _value);
		_constructorSharedPtr(ptr, type);
	}

	void Variant::_assignMoveSharedPtr(void* ptr, VariantType type) noexcept
	{
		Free(_type, _value);
		_constructorMoveSharedPtr(ptr, type);
	}

	void Variant::_constructorRef(const void* ptr, VariantType type) noexcept
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

	void Variant::_constructorMoveRef(void* ptr, VariantType type) noexcept
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

	void Variant::_assignRef(const void* ptr, VariantType type) noexcept
	{
		Free(_type, _value);
		_constructorRef(ptr, type);
	}

	void Variant::_assignMoveRef(void* ptr, VariantType type) noexcept
	{
		Free(_type, _value);
		_constructorMoveRef(ptr, type);
	}

	void Variant::_free(VariantType type, sl_uint64 value) noexcept
	{
		Free(type, value);
	}

	Variant::Variant(const Variant& other) noexcept
	{
		_type = other._type;
		Copy(_type, other._value, _value);
	}

	Variant::Variant(Variant&& other) noexcept
	{
		_type = other._type;
		_value = other._value;
		other._type = VariantType::Null;
	}

	Variant::Variant(const Atomic<Variant>& other) noexcept
	{
		other._retain_construct(this);
	}

	Variant::Variant(const Json& other) noexcept
	{
		_type = other._type;
		Copy(_type, other._value, _value);
	}

	Variant::Variant(Json&& other) noexcept
	{
		_type = other._type;
		_value = other._value;
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
		_type = VariantType::Int32;
		REF_VAR(sl_int32, _value) = (sl_int32)value;
	}
	
	Variant::Variant(unsigned long value) noexcept
	{
		_type = VariantType::Uint32;
		REF_VAR(sl_uint32, _value) = (sl_uint32)value;
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

	sl_int32 Variant::getInt32(sl_int32 def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return (sl_int32)(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return (sl_int32)(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return (sl_int32)(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return (sl_int32)(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return (sl_int32)(REF_VAR(float const, _value));
			case VariantType::Double:
				return (sl_int32)(REF_VAR(double const, _value));
			case VariantType::Boolean:
				return (REF_VAR(sl_bool const, _value)) ? 1 : 0;
			case VariantType::String8:
				return REF_VAR(String const, _value).parseInt32(10, def);
			case VariantType::String16:
				return REF_VAR(String16 const, _value).parseInt32(10, def);
			case VariantType::Pointer:
				return (sl_int32)(REF_VAR(const sl_size, _value));
			case VariantType::Sz8:
				{
					sl_int32 ret;
					const sl_char8* str = REF_VAR(sl_char8 const* const , _value);
					sl_reg pos = String::parseInt32(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Sz16:
				{
					sl_int32 ret;
					const sl_char16* str = REF_VAR(sl_char16 const* const , _value);
					sl_reg pos = String16::parseInt32(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Time:
				return (sl_int32)(REF_VAR(Time const, _value).toUnixTime());
			default:
				break;
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

	sl_uint32 Variant::getUint32(sl_uint32 def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return (sl_uint32)(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return (sl_uint32)(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return (sl_uint32)(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return (sl_uint32)(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return (sl_uint32)(REF_VAR(float const, _value));
			case VariantType::Double:
				return (sl_uint32)(REF_VAR(double const, _value));
			case VariantType::Boolean:
				return (REF_VAR(sl_bool const, _value)) ? 1 : 0;
			case VariantType::String8:
				return REF_VAR(String const, _value).parseUint32(10, def);
			case VariantType::String16:
				return REF_VAR(String16 const, _value).parseUint32(10, def);
			case VariantType::Pointer:
				return (sl_uint32)(REF_VAR(sl_size const, _value));
			case VariantType::Sz8:
				{
					sl_uint32 ret;
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseUint32(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Sz16:
				{
					sl_uint32 ret;
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseUint32(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Time:
				return (sl_uint32)(REF_VAR(Time const, _value).toUnixTime());
			default:
				break;
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

	sl_int64 Variant::getInt64(sl_int64 def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return (sl_int64)(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return (sl_int64)(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return (sl_int64)(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return (sl_int64)(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return (sl_int64)(REF_VAR(float const, _value));
			case VariantType::Double:
				return (sl_int64)(REF_VAR(double const, _value));
			case VariantType::Boolean:
				return (REF_VAR(sl_bool const, _value)) ? 1 : 0;
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseInt64(10, def);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseInt64(10, def);
			case VariantType::Pointer:
				return (sl_int64)(REF_VAR(sl_size const, _value));
			case VariantType::Sz8:
				{
					sl_int64 ret;
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseInt64(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Sz16:
				{
					sl_int64 ret;
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseInt64(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Time:
				return REF_VAR(Time const, _value).toUnixTime();
			default:
				break;
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

	sl_uint64 Variant::getUint64(sl_uint64 def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return (sl_uint64)(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return (sl_uint64)(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return (sl_uint64)(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return (sl_uint64)(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return (sl_uint64)(REF_VAR(float const, _value));
			case VariantType::Double:
				return (sl_uint64)(REF_VAR(double const, _value));
			case VariantType::Boolean:
				return (REF_VAR(sl_bool const, _value)) ? 1 : 0;
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseUint64(10, def);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseUint64(10, def);
			case VariantType::Pointer:
				return (sl_uint64)(REF_VAR(sl_size const, _value));
			case VariantType::Sz8:
				{
					sl_uint64 ret;
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseUint64(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Sz16:
				{
					sl_uint64 ret;
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseUint64(10, &ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Time:
				return REF_VAR(Time const, _value).toUnixTime();
			default:
				break;
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

	float Variant::getFloat(float def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return (float)(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return (float)(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return (float)(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return (float)(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return (float)(REF_VAR(float const, _value));
			case VariantType::Double:
				return (float)(REF_VAR(double const, _value));
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseFloat(def);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseFloat(def);
			case VariantType::Sz8:
				{
					float ret;
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseFloat(&ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Sz16:
				{
					float ret;
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseFloat(&ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Time:
				return (float)(REF_VAR(Time const, _value).toUnixTimef());
			default:
				break;
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

	double Variant::getDouble(double def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return (double)(REF_VAR(sl_int32 const, _value));
			case VariantType::Uint32:
				return (double)(REF_VAR(sl_uint32 const, _value));
			case VariantType::Int64:
				return (double)(REF_VAR(sl_int64 const, _value));
			case VariantType::Uint64:
				return (double)(REF_VAR(sl_uint64 const, _value));
			case VariantType::Float:
				return (double)(REF_VAR(float const, _value));
			case VariantType::Double:
				return (double)(REF_VAR(double const, _value));
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseDouble(def);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseDouble(def);
			case VariantType::Sz8:
				{
					double ret;
					const sl_char8* str = REF_VAR(sl_char8 const* const, _value);
					sl_reg pos = String::parseDouble(&ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Sz16:
				{
					double ret;
					const sl_char16* str = REF_VAR(sl_char16 const* const, _value);
					sl_reg pos = String16::parseDouble(&ret, str);
					if (pos != SLIB_PARSE_ERROR && str[pos] == 0) {
						return ret;
					}
				}
				break;
			case VariantType::Time:
				return REF_VAR(Time const, _value).toUnixTimef();
			default:
				break;
		}
		return def;
	}

	void Variant::setDouble(double value) noexcept
	{
		Free(_type, _value);
		_type = VariantType::Double;
		REF_VAR(double, _value) = value;
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
			case VariantType::Null:
				if (_value) {
					return sl_null;
				}
				break;
			default:
				if (isMemory()) {
					CMemory* mem = REF_VAR(CMemory*, _value);
					return String::fromUtf8(mem->getData(), mem->getSize());
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
			default:
				if (isMemory()) {
					CMemory* mem = REF_VAR(CMemory*, _value);
					return String16::fromUtf8(mem->getData(), mem->getSize());
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
			case VariantType::String8:
				return REF_VAR(String const, _value).getData();
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
			case VariantType::String16:
				return REF_VAR(String16 const, _value).getData();
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
		return _type == VariantType::Pointer || _type == VariantType::Sz8 || _type == VariantType::Sz16 || IsReferable(_type);
	}

	void* Variant::getPointer(const void* def) const noexcept
	{
		if (_type == VariantType::Pointer || _type == VariantType::Sz8 || _type == VariantType::Sz16 || IsReferable(_type)) {
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

	sl_bool Variant::isSharedPtr() const noexcept
	{
		return IsSharedPtr(_type);
	}

	SharedPtr<void> Variant::getSharedPtr() const noexcept
	{
		if (IsSharedPtr(_type)) {
			return REF_VAR(SharedPtr<void> const, _value);
		}
		return sl_null;
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

	sl_bool Variant::setElement_NoLock(sl_uint64 index, const Variant& value)
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value).setAt_NoLock((sl_size)index, value);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->setElement(index, value);
			}
		}
		return sl_false;
	}

	sl_bool Variant::setElement(sl_uint64 index, const Variant& value)
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value).setAt((sl_size)index, value);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->setElement(index, value);
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

	sl_bool Variant::removeItem_NoLock(const StringParam& key)
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

	sl_bool Variant::removeItem(const StringParam& key)
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

	sl_bool Variant::enumerateItems_NoLock(const Function<sl_bool(const StringParam& name, const Variant& value)>& callback)
	{
		if (_type == VariantType::Map) {
			VariantMap& map = REF_VAR(VariantMap, _value);
			auto node = map.getFirstNode();
			while (node) {
				if (!(callback(node->key, node->value))) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->enumerateProperties(callback);
			}
		}
		return sl_false;
	}

	sl_bool Variant::enumerateItems(const Function<sl_bool(const StringParam& name, const Variant& value)>& callback)
	{
		if (_type == VariantType::Map) {
			VariantMap& map = REF_VAR(VariantMap, _value);
			MutexLocker lock(map.getLocker());
			auto node = map.getFirstNode();
			while (node) {
				if (!(callback(node->key, node->value))) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->enumerateProperties(callback);
			}
		}
		return sl_false;
	}

	List<String> Variant::getItemKeys_NoLock() const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).getAllKeys_NoLock();
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->getPropertyNames();
			}
		}
		return sl_null;
	}

	List<String> Variant::getItemKeys() const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).getAllKeys();
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->getPropertyNames();
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
		if (isString()) {
			return getString().parseHexString();
		} else {
			return GetObjectT<CMemory, Memory, VariantType::Memory>(*this);
		}
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
					src->enumerateProperties([&dst](const StringParam& name, const Variant& value) {
						dst.put_NoLock(name.toString(), value);
						return sl_true;
					});
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
						src->enumerateProperties([&dst](const StringParam& name, const Variant& value) {
							dst->setProperty(name, value);
							return sl_true;
						});
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

	Memory Variant::serialize() const
	{
		MemoryBuffer buf;
		if (Serialize(&buf, *this)) {
			return buf.merge();
		}
		return sl_null;
	}

	sl_size Variant::deserialize(const void* data, sl_size size)
	{
		SerializeBuffer buf(data, size);
		return Deserialize(&buf, *this);
	}

	sl_size Variant::deserialize(const Memory& mem)
	{
		return deserialize(mem.getData(), mem.getSize());
	}

	sl_compare_result Variant::compare(const Variant& v2) const noexcept
	{
		const Variant& v1 = *this;
		VariantType type = v1._type;
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
				case VariantType::Object:
					return ComparePrimitiveValues(REF_VAR(sl_size const, v1._value), REF_VAR(sl_size const, v2._value));
				default:
					return ComparePrimitiveValues(v1._value, v2._value);
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
		VariantType type = v1._type;
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
				case VariantType::Object:
					return REF_VAR(void const* const, v1._value) == REF_VAR(void const* const, v2._value);
				default:
					return v1._value == v2._value;
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
		VariantType type = _type;
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
	

	sl_bool operator==(const Variant& v1, const Variant& v2) noexcept
	{
		return v1.equals(v2);
	}

	sl_bool operator!=(const Variant& v1, const Variant& v2) noexcept
	{
		return !(v1.equals(v2));
	}

	
	sl_compare_result Compare<Variant>::operator()(const Variant& a, const Variant& b) const noexcept
	{
		return a.compare(b);
	}
	
	sl_bool Equals<Variant>::operator()(const Variant& a, const Variant& b) const noexcept
	{
		return a.equals(b);
	}
	
	sl_size Hash<Variant>::operator()(const Variant& a) const noexcept
	{
		return a.getHashCode();
	}


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

}
