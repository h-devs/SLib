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

#include "slib/platform/definition.h"

#if defined(SLIB_PLATFORM_IS_APPLE)

#include "slib/platform/apple/iokit.h"

namespace slib
{
	namespace apple
	{

		Variant IOKit::getRegistryEntryPropertyValue(CFTypeRef cfValue)
		{
			if (CFGetTypeID(cfValue) == CFDataGetTypeID()) {
				return Apple::getStringFromNSData((__bridge NSData*)cfValue);
			}
			return Apple::getVariantFromCFType(cfValue);
		}

		Variant IOKit::getRegistryEntryProperty(io_registry_entry_t entry, CFStringRef key)
		{
			CFTypeRef cfValue = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
			if (cfValue) {
				Variant ret = getRegistryEntryPropertyValue(cfValue);
				CFRelease(cfValue);
				return ret;
			}
			return Variant();
		}

		sl_bool IOKit::isRegistryEntryMatchingClass(io_registry_entry_t service, const char* className)
		{
			io_name_t _name = {0};
			IOObjectGetClass(service, _name);
			return Base::equalsString(_name, className);
		}

		io_registry_entry_t IOKit::findRegistryEntry(io_registry_entry_t parent, const io_name_t plane, const char* className)
		{
			io_registry_entry_t ret = 0;
			io_iterator_t iter;
			kern_return_t kr = IORegistryEntryCreateIterator(parent, plane, 0, &iter);
			if (kr == KERN_SUCCESS) {
				for (;;) {
					io_registry_entry_t entry = IOIteratorNext(iter);
					if (!entry) {
						break;
					}
					if (isRegistryEntryMatchingClass(entry, className)) {
						ret = entry;
						break;
					}
					ret = findRegistryEntry(entry, plane, className);
					IOObjectRelease(entry);
					if (ret) {
						break;
					}
				}
				IOObjectRelease(iter);
			}
			return ret;
		}

		void IOKit::findServices(const char* className, const Function<sl_bool(io_service_t)>& callback)
		{
			io_iterator_t iter;
			kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(className), &iter);
			if (kr == KERN_SUCCESS) {
				for (;;) {
					io_service_t service = IOIteratorNext(iter);
					if (!service) {
						break;
					}
					sl_bool flagContinue = callback(service);
					IOObjectRelease(service);
					if (!flagContinue) {
						break;
					}
				}
				IOObjectRelease(iter);
			}
		}

		io_service_t IOKit::findService(const char* className)
		{
			return IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(className));
		}

		Variant IOKit::getServiceProperty(const char* className, CFStringRef key)
		{
			Variant ret;
			io_service_t service = findService(className);
			if (service) {
				ret = getRegistryEntryProperty(service, key);
				IOObjectRelease(service);
			}
			return ret;
		}

	}
}

#endif
