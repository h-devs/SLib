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

#ifndef CHECKHEADER_SLIB_CORE_NAMED_INSTANCE
#define CHECKHEADER_SLIB_CORE_NAMED_INSTANCE

#include "string.h"
#include "handle_container.h"

namespace slib
{

	namespace priv
	{
		namespace named_instance
		{
			struct HandleType;
			void CloseInstanceHandle(HandleType* handle);
		}
	}
	
	class SLIB_EXPORT NamedInstance
	{
		SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_MEMBERS(NamedInstance, priv::named_instance::HandleType*, m_handle, priv::named_instance::CloseInstanceHandle)

	public:
		NamedInstance(const StringParam& name);

	public:
		static sl_bool exists(const StringParam& name);

	};

	template <>
	class SLIB_EXPORT Atomic<NamedInstance>
	{
		SLIB_DEFINE_ATOMIC_NULLABLE_HANDLE_CONTAINER_MEMBERS(NamedInstance, priv::named_instance::HandleType*, m_handle, priv::named_instance::CloseInstanceHandle)
	};

}

#endif

