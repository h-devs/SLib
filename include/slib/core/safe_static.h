/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_SAFE_STATIC
#define CHECKHEADER_SLIB_CORE_SAFE_STATIC

#include "spin_lock.h"

#include <new>

/****************************************************************
 C++ 11 introduces thread-safe static local variable initialization.
 But we need customized implementation to check the storage-status on
 exit especially for Win32 applications, and also to support the option
 `-fno-threadsafe-statics` (GCC) or `/Zc:threadSafeInit-` (VC).
 And this is very important to support Windows XP/2003 Server, because
 older Windows systems can't support thread-local-storage when loading
 the dll using `LoadLibrary()`.
 ****************************************************************/

#define PRIV_SLIB_SAFE_GLOBAL_DESTRUCTOR(TYPE, NAME) \
		static sl_bool _static_freeflag_##NAME = sl_false; \
		static slib::priv::safe_static::FreeGlobal<TYPE> _safe_static_free_##NAME(&NAME, &_static_freeflag_##NAME);

#define PRIV_SLIB_SAFE_LOCAL_STATIC_DESTRUCTOR(NAME) \
		slib::priv::safe_static::FreeObjectOnExit(&NAME, &_static_freeflag_##NAME);

#define PRIV_SLIB_ZERO_LOCAL_STATIC_DESTRUCTOR(NAME) \
		static sl_int32 _static_safeflag_##NAME = 0; \
		static sl_bool _static_freeflag_##NAME = sl_false; \
		if (!_static_safeflag_##NAME) { \
			SLIB_STATIC_SPINLOCK(locker); \
			locker.lock(); \
			if (!_static_safeflag_##NAME) { \
				slib::priv::safe_static::FreeObjectOnExit(&NAME, &_static_freeflag_##NAME); \
				_static_safeflag_##NAME = 1; \
			} \
			locker.unlock(); \
		}

#define SLIB_SAFE_STATIC_CHECK_FREED(NAME) _static_freeflag_##NAME

#define SLIB_SAFE_LOCAL_STATIC(TYPE, NAME, ...) \
	SLIB_ALIGN(8) static sl_uint8 _static_safemem_##NAME[sizeof(TYPE)]; \
	static sl_int32 _static_safeflag_##NAME = 0; \
	static sl_bool _static_freeflag_##NAME = sl_false; \
	static TYPE& NAME = *(reinterpret_cast<TYPE*>(_static_safemem_##NAME)); \
	if (!_static_safeflag_##NAME) { \
		SLIB_STATIC_SPINLOCK(locker); \
		locker.lock(); \
		if (!_static_safeflag_##NAME) { \
			new (&NAME) TYPE(__VA_ARGS__); \
			PRIV_SLIB_SAFE_LOCAL_STATIC_DESTRUCTOR(NAME) \
			_static_safeflag_##NAME = 1; \
		} \
		locker.unlock(); \
	}

#define SLIB_SAFE_STATIC_GETTER(TYPE, FUNC, ...) \
	static TYPE* FUNC() { \
		SLIB_SAFE_LOCAL_STATIC(TYPE, ret, ##__VA_ARGS__) \
		if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) { \
			return sl_null; \
		} \
		return &ret; \
	}

#define SLIB_GLOBAL_ZERO_INITIALIZED(TYPE, NAME) \
	SLIB_ALIGN(8) static char g_static_safemem_##NAME[sizeof(TYPE)] = {0}; \
	static TYPE& NAME = *(reinterpret_cast<TYPE*>(g_static_safemem_##NAME)); \
	PRIV_SLIB_SAFE_GLOBAL_DESTRUCTOR(TYPE, NAME)

#define SLIB_THREAD_LOCAL_ZERO_INITIALIZED(TYPE, NAME) \
	SLIB_ALIGN(8) static char g_static_safemem_##NAME[sizeof(TYPE)] = {0}; \
	static thread_local TYPE& NAME = *(reinterpret_cast<TYPE*>(g_static_safemem_##NAME)); \
	PRIV_SLIB_SAFE_GLOBAL_DESTRUCTOR(TYPE, NAME)

#define SLIB_LOCAL_STATIC_ZERO_INITIALIZED(TYPE, NAME) \
	SLIB_ALIGN(8) static char _static_safemem_##NAME[sizeof(TYPE)] = {0}; \
	TYPE& NAME = *(reinterpret_cast<TYPE*>(_static_safemem_##NAME)); \
	PRIV_SLIB_ZERO_LOCAL_STATIC_DESTRUCTOR(NAME)


namespace slib
{

	namespace priv
	{
		namespace safe_static
		{

			template <class T>
			class FreeGlobal
			{
			public:
				T* object;
				sl_bool* pFreedStatus;

			public:
				SLIB_CONSTEXPR FreeGlobal(T* _object, sl_bool* _pFreedStatus): object(_object), pFreedStatus(_pFreedStatus) {}

				~FreeGlobal()
				{
					*pFreedStatus = sl_true;
					object->~T();
				}
			};

			class IFreeable
			{
			public:
				SLIB_CONSTEXPR IFreeable() {}

				virtual ~IFreeable() {}

			};

			template <class T>
			class FreeLocal : public IFreeable
			{
			public:
				T* object;
				sl_bool* pFreedStatus;

			public:
				SLIB_CONSTEXPR FreeLocal(T* _object, sl_bool* _pFreedStatus): object(_object), pFreedStatus(_pFreedStatus) {}

				~FreeLocal()
				{
					if (pFreedStatus) {
						*pFreedStatus = sl_true;
					}
					object->~T();
				}
			};

			void FreeObjectOnExitImpl(IFreeable* obj);

			template <typename T>
			void FreeObjectOnExit(T* obj, sl_bool* outFreedStatus)
			{
				priv::safe_static::IFreeable* d = new priv::safe_static::FreeLocal<T>(obj, outFreedStatus);
				if (d) {
					priv::safe_static::FreeObjectOnExitImpl(d);
				}
			}

		}
	}

}

#endif

