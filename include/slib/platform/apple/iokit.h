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

#ifndef CHECKHEADER_SLIB_PLATFORM_APPLE_IOKIT
#define CHECKHEADER_SLIB_PLATFORM_APPLE_IOKIT

#include "platform.h"

#include "slib/core/variant.h"

namespace slib
{
	namespace apple
	{

		class SLIB_EXPORT IOKit
		{
		public:
			static Variant getRegistryEntryPropertyValue(CFTypeRef cfValue);

			static Variant getRegistryEntryProperty(io_registry_entry_t entry, CFStringRef key);

			static sl_bool isRegistryEntryMatchingClass(io_registry_entry_t service, const char* className);

			static io_registry_entry_t findRegistryEntry(io_registry_entry_t parent, const io_name_t plane, const char* className);

			static void findServices(const char* className, const Function<sl_bool(io_service_t)>& callback);

			static io_service_t findService(const char* className);

			static Variant getServiceProperty(const char* className, CFStringRef key);

		};

	}
}

#endif
