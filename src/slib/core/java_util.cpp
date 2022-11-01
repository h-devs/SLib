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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_USE_JNI

#include "slib/core/java/list.h"
#include "slib/core/java/locale.h"
#include "slib/core/java/uuid.h"

namespace slib
{

	namespace priv
	{
		namespace java_util
		{

			SLIB_JNI_BEGIN_CLASS(JList, "java/util/List")
				SLIB_JNI_METHOD(size, "size", "()I")
				SLIB_JNI_METHOD(contains, "contains", "(Ljava/lang/Object;)Z")
				SLIB_JNI_METHOD(iterator, "iterator", "()Ljava/util/Iterator;")
				SLIB_JNI_METHOD(getAt, "get", "(I)Ljava/lang/Object;")
				SLIB_JNI_METHOD(set, "set", "(ILjava/lang/Object;)Ljava/lang/Object;")
				SLIB_JNI_METHOD(add, "add", "(Ljava/lang/Object;)Z")
				SLIB_JNI_METHOD(addAt, "add", "(ILjava/lang/Object;)V")
				SLIB_JNI_METHOD(remove, "remove", "(Ljava/lang/Object;)Z")
				SLIB_JNI_METHOD(removeAt, "remove", "(I)Ljava/lang/Object;")
				SLIB_JNI_METHOD(clear, "clear", "()V")
				SLIB_JNI_METHOD(indexOf, "indexOf", "(Ljava/lang/Object;)I")
				SLIB_JNI_METHOD(lastIndexOf, "lastIndexOf", "(Ljava/lang/Object;)I")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JLocale, "java/util/Locale")
				SLIB_JNI_STATIC_METHOD(getDefault, "getDefault", "()Ljava/util/Locale;")
				SLIB_JNI_METHOD(getLanguage, "getLanguage", "()Ljava/lang/String;")
				SLIB_JNI_METHOD(getCountry, "getCountry", "()Ljava/lang/String;")
				SLIB_JNI_METHOD(getScript, "getScript", "()Ljava/lang/String;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JUUID, "java/util/UUID")
				SLIB_JNI_STATIC_METHOD(randomUUID, "randomUUID", "()Ljava/util/UUID;")
			SLIB_JNI_END_CLASS
		}
	}

	using namespace priv::java_util;

	namespace java
	{

		sl_int32 List::size(jobject list) noexcept
		{
			return (sl_int32)(JList::size.callInt(list));
		}

		sl_bool List::contains(jobject list, jobject element) noexcept
		{
			return JList::contains.callBoolean(list, element);
		}

		JniLocal<jobject> List::iterator(jobject list) noexcept
		{
			return JList::iterator.callObject(list);
		}

		JniLocal<jobject> List::get(jobject list, sl_int32 index) noexcept
		{
			return JList::getAt.callObject(list, (jint)index);
		}

		JniLocal<jobject> List::set(jobject list, sl_int32 index, jobject element) noexcept
		{
			return JList::set.callObject(list, (jint)index, element);
		}

		sl_bool List::add(jobject list, jobject element) noexcept
		{
			return JList::add.callBoolean(list, element);
		}

		void List::add(jobject list, sl_int32 index, jobject element) noexcept
		{
			JList::addAt.call(list, (jint)index, element);
		}

		sl_bool List::remove(jobject list, jobject element) noexcept
		{
			return JList::remove.callBoolean(list, element);
		}

		JniLocal<jobject> List::remove(jobject list, sl_int32 index) noexcept
		{
			return JList::removeAt.callObject(list, (jint)index);
		}

		void List::clear(jobject list) noexcept
		{
			JList::clear.call(list);
		}

		sl_int32 List::indexOf(jobject list, jobject element) noexcept
		{
			return (sl_int32)(JList::indexOf.callInt(list, element));
		}

		sl_int32 List::lastIndexOf(jobject list, jobject element) noexcept
		{
			return (sl_int32)(JList::lastIndexOf.callInt(list, element));
		}


		JniLocal<jobject> Locale::getDefault() noexcept
		{
			return JLocale::getDefault.callObject(sl_null);
		}

		String Locale::getLanguage(jobject thiz) noexcept
		{
			return JLocale::getLanguage.callString(thiz);
		}

		String Locale::getCountry(jobject thiz) noexcept
		{
			return JLocale::getCountry.callString(thiz);
		}

		String Locale::getScript(jobject thiz) noexcept
		{
			return JLocale::getScript.callString(thiz);
		}


		JniLocal<jobject> UUID::randomUUID() noexcept
		{
			return JUUID::randomUUID.callObject(sl_null);
		}

	}

}

#endif
