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

#include "slib/core/bson.h"

#include "slib/core/memory_buffer.h"

namespace slib
{

	namespace priv
	{
		namespace bson
		{

			Json Parse(const void* data, sl_size size, sl_bool* pOutFlagError)
			{
				return sl_null;
			}

			void Build(MemoryBuffer& buf, const Json& json)
			{
			}

			Memory Build(const Json& json)
			{
				return sl_null;
			}

		}
	}

	using namespace priv::bson;
	
	Json Bson::parse(const void* data, sl_size size, sl_bool* pOutFlagError)
	{
		return Parse(data, size, pOutFlagError);
	}

	Json Bson::parse(const Memory& mem, sl_bool* pOutFlagError)
	{
		return parse(mem.getData(), mem.getSize(), pOutFlagError);
	}

	Memory Bson::build(const Json& json)
	{
		return Build(json);
	}

}
