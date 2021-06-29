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

#ifndef CHECKHEADER_SLIB_CORE_EVENT
#define CHECKHEADER_SLIB_CORE_EVENT

#include "ref.h"

namespace slib
{

	class SLIB_EXPORT Event : public Referable
	{
		SLIB_DECLARE_OBJECT

	protected:
		Event();

		~Event();

	public:
		static Ref<Event> create(sl_bool flagAutoReset = sl_true);

	public:
		virtual void set() = 0;

		virtual void reset() = 0;

	public:
		// milliseconds. negative means INFINITE
		sl_bool wait(sl_int32 timeout = -1);

	protected:
		virtual sl_bool doWait(sl_int32 timeout) = 0;

	};

#if defined(SLIB_PLATFORM_IS_WINDOWS)
	typedef void* HEvent;
#else
	namespace posix
	{
		class Event;
	}
	typedef posix::Event* HEvent;
#endif

	class GenericEvent : public Event
	{
	protected:
		GenericEvent();

		~GenericEvent();

	public:
		static Ref<GenericEvent> create(sl_bool flagAutoReset = sl_true);
		
		static Ref<GenericEvent> create(HEvent handle);

	public:
		constexpr HEvent getHandle() const
		{
			return m_handle;
		}

		static void closeHandle(HEvent handle);

		void set() override;

		void reset() override;

	protected:
		sl_bool doWait(sl_int32 timeout) override;

	protected:
		HEvent m_handle;

	};

}

#endif
