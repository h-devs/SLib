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

#ifndef CHECKHEADER_SLIB_DATA_EXPIRING_VALUE
#define CHECKHEADER_SLIB_DATA_EXPIRING_VALUE

#include "definition.h"

#include "../system/system.h"

namespace slib
{

	template <class T, sl_uint32 EXPIRY_MILLISECONDS>
	class SLIB_EXPORT ExpiringValue
	{
	public:
		ExpiringValue(): lastUpdatedTick(0) {}

		ExpiringValue(const ExpiringValue& other) = default;
		
		ExpiringValue(ExpiringValue&& other) = default;

		template <class... ARGS>
		ExpiringValue(ARGS&&... args): lastUpdatedTick(System::getTickCount64()), value(Forward<ARGS...>(args...)) {}

	public:
		operator T const&() const noexcept
		{
			return value;
		}

		operator T&() noexcept
		{
			return value;
		}

		ExpiringValue& operator=(const ExpiringValue& other) = default;

		ExpiringValue& operator=(ExpiringValue&& other) = default;

		template <class ARG>
		ExpiringValue& operator=(ARG&& arg)
		{
			lastUpdatedTick = System::getTickCount64();
			value = Forward<ARG>(arg);
			return *this;
		}

	public:
		sl_bool isValid() const
		{
			sl_uint64 tick = System::getTickCount64();
			return lastUpdatedTick && lastUpdatedTick <= tick && lastUpdatedTick + EXPIRY_MILLISECONDS >= tick;
		}

		T const& get() const noexcept
		{
			return value;
		}

		T& get() noexcept
		{
			return value;
		}

		void release()
		{
			lastUpdatedTick = 0;
			value = T();
		}

		template <class OUTPUT>
		sl_bool release(OUTPUT& _out) noexcept
		{
			if (isValid()) {
				lastUpdatedTick = 0;
				_out = Move(value);
				return sl_true;
			}
			return sl_false;
		}

	public:
		T value;
		sl_uint64 lastUpdatedTick;

	};

}

#endif
