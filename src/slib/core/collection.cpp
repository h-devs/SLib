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

#include "slib/core/collection.h"

#include "slib/core/variant.h"
#include "slib/core/string_buffer.h"
#include "slib/core/serialize.h"

namespace slib
{

	SLIB_DEFINE_ROOT_OBJECT(Collection)

	Collection::Collection()
	{
	}

	Collection::~Collection()
	{
	}

	sl_uint64 Collection::getElementCount()
	{
		return 0;
	}

	Variant Collection::getElement(sl_uint64 index)
	{
		return Variant();
	}

	sl_bool Collection::setElement(sl_uint64 index, const Variant& item)
	{
		return sl_false;
	}

	sl_bool Collection::addElement(const Variant& item)
	{
		return sl_false;
	}

	String Collection::toString()
	{
		StringBuffer buf;
		if (toJsonString(buf)) {
			return buf.merge();
		}
		return sl_null;
	}

	sl_bool Collection::toJsonString(StringBuffer& buf)
	{
		sl_uint64 n = getElementCount();
		if (!(buf.addStatic("["))) {
			return sl_false;
		}
		for (sl_uint64 i = 0; i < n; i++) {
			Variant v = getElement(i);
			if (i) {
				if (!(buf.addStatic(", "))) {
					return sl_false;
				}
			}
			if (!(v.toJsonString(buf))) {
				return sl_false;
			}
		}
		if (!(buf.addStatic("]"))) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool Collection::toJsonBinary(MemoryBuffer& buf)
	{
		if (!(SerializeByte(&buf, (sl_uint8)(VariantType::Collection)))) {
			return sl_false;
		}
		sl_uint64 n = getElementCount();
		if (!(CVLI::serialize(&buf, n))) {
			return sl_false;
		}
		for (sl_uint64 i = 0; i < n; i++) {
			if (!(Serialize(&buf, getElement(i)))) {
				return sl_false;
			}
		}
		return sl_true;
	}

}
