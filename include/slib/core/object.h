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

#ifndef CHECKHEADER_SLIB_CORE_OBJECT
#define CHECKHEADER_SLIB_CORE_OBJECT

#include "iterator.h"
#include "lockable.h"

namespace slib
{

	class String;

	template <class T>
	class Function;

	template <class T>
	class List;

	typedef CIterator<String, Variant> CPropertyIterator;
	typedef Iterator<String, Variant> PropertyIterator;

	class SLIB_EXPORT Object : public CRef, public Lockable
	{
		SLIB_DECLARE_OBJECT

	public:
		Object() noexcept;

		Object(const Object& other) = delete;

		Object(Object&& other) = delete;

		~Object() noexcept;

	public:
		const Function<void()>& getOnFree();

		// Not thread safe
		void setOnFree(const Function<void()>& callback);

		virtual Variant getProperty(const String& name);

		virtual sl_bool setProperty(const String& name, const Variant& value);

		virtual sl_bool clearProperty(const String& name);

		virtual PropertyIterator getPropertyIterator();

	public:
		sl_bool toJsonString(StringBuffer& buf) override;

		sl_bool toJsonBinary(MemoryBuffer& buf) override;

	public:
		Object& operator=(const Object& other) = delete;

		Object& operator=(Object&& other) = delete;

	protected:
		void free() override;

	private:
		void* m_onFree;
		void* m_properties;

	};

}

#endif

