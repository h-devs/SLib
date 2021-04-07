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

#ifndef CHECKHEADER_SLIB_CORE_TIME_ZONE
#define CHECKHEADER_SLIB_CORE_TIME_ZONE

#include "ref.h"

namespace slib
{
	
	class CTimeZone: public Referable
	{
		SLIB_DECLARE_OBJECT
		
	public:
		CTimeZone() noexcept;
		
		~CTimeZone() noexcept;
		
	public:
		// In seconds
		sl_int64 getOffset();
		
		// In seconds
		virtual sl_int64 getOffset(const Time& time) = 0;
		
	};
	
	class GenericTimeZone : public CTimeZone
	{
		SLIB_DECLARE_OBJECT
		
	public:
		GenericTimeZone(sl_int64 offsetSeconds = 0);
		
		~GenericTimeZone();
		
	public:
		using CTimeZone::getOffset;
		
		sl_int64 getOffset(const Time& time) override;
		
	protected:
		sl_int64 m_offset;
		
	};
	
	
	template <>
	class SLIB_EXPORT Atomic<TimeZone>
	{
		SLIB_ATOMIC_REF_WRAPPER(CTimeZone)
		
	public:
		sl_bool isLocal() const noexcept;
		
		sl_bool isUTC() const noexcept;
		
		// In seconds
		sl_int64 getOffset() const;
		
		// In seconds
		sl_int64 getOffset(const Time& time) const;
		
	public:
		AtomicRef<CTimeZone> ref;
		
	};
	
	typedef Atomic<TimeZone> AtomicTimeZone;
	
	class TimeZone
	{
		SLIB_REF_WRAPPER(TimeZone, CTimeZone)
		
	public:
		static const TimeZone& Local;
		
		static const TimeZone& UTC() noexcept;
		
		static TimeZone create(sl_int64 offset) noexcept;
		
	public:
		sl_bool isLocal() const noexcept;
		
		sl_bool isUTC() const noexcept;

		// In seconds
		sl_int64 getOffset() const;
		
		// In seconds
		sl_int64 getOffset(const Time& time) const;

	public:
		Ref<CTimeZone> ref;
		
	};
	
}

#endif
