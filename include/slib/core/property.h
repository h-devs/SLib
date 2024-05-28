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

#define SLIB_PROPERTY_EX(TYPE, NAME, GETTER, SETTER) \
protected: \
	TYPE _m_property_##NAME; \
public: \
	typename slib::PropertyTypeHelper<TYPE>::RetType GETTER() const { return _m_property_##NAME; } \
	void SETTER(typename slib::PropertyTypeHelper<TYPE>::ArgType v) { _m_property_##NAME = v; }

#define SLIB_DECLARE_PROPERTY_EX(TYPE, NAME, GETTER, SETTER) \
protected: \
	TYPE _m_property_##NAME; \
public: \
	typename slib::PropertyTypeHelper<TYPE>::RetType GETTER() const; \
	void SETTER(typename slib::PropertyTypeHelper<TYPE>::ArgType v);

#define SLIB_DEFINE_PROPERTY_EX(CLASS, TYPE, NAME, GETTER, SETTER) \
	typename slib::PropertyTypeHelper<TYPE>::RetType CLASS::GETTER() const { return _m_property_##NAME; } \
	void CLASS::SETTER(typename slib::PropertyTypeHelper<TYPE>::ArgType v) { _m_property_##NAME = v; }

#define SLIB_DECLARE_STATIC_PROPERTY_EX(TYPE, GETTER, SETTER) \
public: \
	static typename slib::PropertyTypeHelper<TYPE>::RetType GETTER(); \
	static void SETTER(typename slib::PropertyTypeHelper<TYPE>::ArgType v);

#define SLIB_DEFINE_STATIC_PROPERTY_EX(CLASS, TYPE, NAME, GETTER, SETTER) \
	namespace { static TYPE _g_property_##CLASS##_##NAME; } \
	typename slib::PropertyTypeHelper<TYPE>::RetType CLASS::GETTER() { return _g_property_##CLASS##_##NAME; } \
	void CLASS::SETTER(typename slib::PropertyTypeHelper<TYPE>::ArgType v) { _g_property_##CLASS##_##NAME = v; }

#define SLIB_PROPERTY(TYPE, NAME) SLIB_PROPERTY_EX(TYPE, NAME, get##NAME, set##NAME)
#define SLIB_DECLARE_PROPERTY(TYPE, NAME) SLIB_DECLARE_PROPERTY_EX(TYPE, NAME, get##NAME, set##NAME)
#define SLIB_DEFINE_PROPERTY(CLASS, TYPE, NAME) SLIB_DEFINE_PROPERTY_EX(CLASS, TYPE, NAME, get##NAME, set##NAME)
#define SLIB_DECLARE_STATIC_PROPERTY(TYPE, NAME) SLIB_DECLARE_STATIC_PROPERTY_EX(TYPE, get##NAME, set##NAME)
#define SLIB_DEFINE_STATIC_PROPERTY(CLASS, TYPE, NAME) SLIB_DEFINE_STATIC_PROPERTY_EX(CLASS, TYPE, NAME, get##NAME, set##NAME)

#define SLIB_BOOLEAN_PROPERTY(NAME) SLIB_PROPERTY_EX(sl_bool, NAME, is##NAME, set##NAME)
#define SLIB_DECLARE_BOOLEAN_PROPERTY(NAME) SLIB_DECLARE_PROPERTY_EX(sl_bool, NAME, is##NAME, set##NAME)
#define SLIB_DEFINE_BOOLEAN_PROPERTY(CLASS, NAME) SLIB_DEFINE_PROPERTY_EX(CLASS, sl_bool, NAME, is##NAME, set##NAME)
#define SLIB_DECLARE_STATIC_BOOLEAN_PROPERTY(NAME) SLIB_DECLARE_STATIC_PROPERTY_EX(sl_bool, is##NAME, set##NAME)
#define SLIB_DEFINE_STATIC_BOOLEAN_PROPERTY(CLASS, NAME) SLIB_DEFINE_STATIC_PROPERTY_EX(CLASS, sl_bool, NAME, is##NAME, set##NAME)

#endif
