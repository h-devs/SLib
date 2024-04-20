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

#ifndef CHECKHEADER_SLIB_CORE_PROPERTY
#define CHECKHEADER_SLIB_CORE_PROPERTY

#include "cpp_helper.h"
#include "primitive_wrapper.h"

namespace slib
{

	template <class T> struct PropertyTypeHelper
	{
		typedef T const& ArgType;
		typedef T const& RetType;
	};

	template <class T>
	class Atomic;

	template <class T>
	struct PropertyTypeHelper< Atomic<T> >
	{
		typedef typename PropertyTypeHelper<T>::ArgType ArgType;
		typedef typename RemoveConstReference< typename PropertyTypeHelper<T>::RetType >::Type RetType;
	};


	template <class... TYPES>
	class Ref;

	template <class T>
	class WeakRef;

	template <class T>
	struct PropertyTypeHelper< WeakRef<T> >
	{
		typedef Ref<T> const& ArgType;
		typedef Ref<T> RetType;
	};

}

#define SLIB_PROPERTY(TYPE, NAME) \
protected: \
	TYPE _m_property_##NAME; \
public: \
	typename slib::PropertyTypeHelper<TYPE>::RetType get##NAME() const { return _m_property_##NAME; } \
	void set##NAME(typename slib::PropertyTypeHelper<TYPE>::ArgType v) { _m_property_##NAME = v; } \

#define SLIB_BOOLEAN_PROPERTY(NAME) \
protected: \
	sl_bool _m_property_##NAME; \
public: \
	sl_bool is##NAME() const { return _m_property_##NAME; } \
	void set##NAME(sl_bool v) { _m_property_##NAME = v; }

#endif
