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
 
#if defined(SLIB_PLATFORM_IS_WIN32)

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
				FreeGlobal(T* _object, sl_bool* _pFreedStatus): object(_object), pFreedStatus(_pFreedStatus)
				{
				}

				~FreeGlobal()
				{
					*pFreedStatus = sl_true;
					object->~T();
				}
			};

			class IFreeable
			{
			public:
				IFreeable();
				virtual ~IFreeable();
			};

			template <class T>
			class FreeLocal : public IFreeable
			{
			public:
				T* object;
				sl_bool* pFreedStatus;

			public:
				FreeLocal(T* _object, sl_bool* _pFreedStatus): object(_object), pFreedStatus(_pFreedStatus)
				{
				}

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
			SLIB_INLINE void FreeObjectOnExit(T* obj, sl_bool* outFreedStatus)
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