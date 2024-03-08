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

#ifndef CHECKHEADER_SLIB_DATA_CONTAINER
#define CHECKHEADER_SLIB_DATA_CONTAINER

#include "json.h"

namespace slib
{

	class SLIB_EXPORT DataContainer
	{
	public:
		const void* data;
		sl_uint32 size;

	private:
		Ref<CRef> ref;
		Memory mem;
		String string;
		Json json;
		sl_bool flagNotJson;

	public:
		DataContainer() noexcept;

		DataContainer(const void* data, sl_size size, CRef* ref = sl_null) noexcept;

		template <class T>
		DataContainer(T&& value): data(sl_null), size(0)
		{
			setContent(Forward<T>(value));
		}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DataContainer)

	public:
		sl_bool isEmpty() const noexcept
		{
			return !size;
		}

		sl_bool isNotEmpty() const
		{
			return size > 0;
		}

		void clear();

		void setContent(const void* data, sl_uint32 size, CRef* ref = sl_null);

		void setContent(const Variant& var);

		Memory getMemory() const noexcept;

		void setMemory(const Memory& mem);

		String getString() const noexcept;

		void setString(const String& str);

		Json getJson() const noexcept;

		void setJson(const Json& json);

		void setJson(const Json& json, const Memory& mem);

	};

}

#endif
