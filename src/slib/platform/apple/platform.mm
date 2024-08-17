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

#include "slib/platform.h"

#include "slib/system/system.h"
#include "slib/core/command_line.h"
#include "slib/core/variant.h"

namespace slib
{

#if defined(SLIB_PLATFORM_IS_IOS)
	namespace priv
	{
		UIInterfaceOrientation g_screenOrientation = UIInterfaceOrientationPortrait;
	}
#endif

	NSString* Apple::getNSStringFromString(const StringParam& str, NSString* def)
	{
		if (str.isNull()) {
			return def;
		}
		if (str.is8BitsStringType()) {
			StringData s(str);
			NSString* ret = [[NSString alloc] initWithBytes:s.getData() length:s.getLength() encoding:NSUTF8StringEncoding];
			if (ret != nil) {
				return ret;
			}
		} else {
			StringData16 s(str);
			NSString* ret = [[NSString alloc] initWithCharacters:(unichar*)s.getData() length:s.getLength()];
			if (ret != nil) {
				return ret;
			}
		}
		return def;
	}

	String Apple::getStringFromNSString(NSString* str)
	{
		if (str == nil) {
			return sl_null;
		}
		const char* buf = [str UTF8String];
		return String::fromUtf8(buf);
	}

	String16 Apple::getString16FromNSString(NSString* str)
	{
		if (str == nil) {
			return sl_null;
		}
		sl_size len = (sl_size)([str length]);
		String16 ret = String16::allocate(len);
		if (ret.isNull()) {
			return ret;
		}
		NSRange range;
		range.length = len;
		range.location = 0;
		[str getCharacters:((unichar*)(ret.getData())) range:range];
		return ret;
	}

	String32 Apple::getString32FromNSString(NSString* str)
	{
		if (str == nil) {
			return sl_null;
		}
		const char* buf = [str UTF8String];
		return String32::fromUtf8(buf);
	}

	String Apple::getStringFromNSData(NSData* data)
	{
		if (data != nil) {
			sl_size n = (sl_size)([data length]);
			if (n > 0) {
				String str = String::allocate(n);
				if (str.isNotNull()) {
					char* p = (char*)(str.getData());
					[data enumerateByteRangesUsingBlock:^(const void* bytes, NSRange byteRange, BOOL* stop) {
						Base::copyMemory(p + byteRange.location, bytes, byteRange.length);
					}];
					if (!(p[n - 1])) {
						str.setLength(n - 1);
					}
					return str;
				}
			}
		}
		return sl_null;
	}

	Memory Apple::getMemoryFromNSData(NSData* data)
	{
		if (data != nil) {
			sl_size n = (sl_size)([data length]);
			if (n > 0) {
				Memory mem = Memory::create(n);
				if (mem.isNotNull()) {
					char* p = (char*)(mem.getData());
					[data enumerateByteRangesUsingBlock:^(const void* bytes, NSRange byteRange, BOOL* stop) {
						Base::copyMemory(p + byteRange.location, bytes, byteRange.length);
					}];
					return mem;
				}
			}
		}
		return sl_null;
	}

	NSData* Apple::getNSDataFromMemory(const MemoryView& mem)
	{
		if(mem.size) {
			return [NSData dataWithBytes:mem.data length:mem.size];
		}
		return nil;
	}

	Time Apple::getTimeFromNSDate(NSDate* date)
	{
		if (date == nil) {
			return Time::zero();
		}
		Time time;
		time.setSecondCountF([date timeIntervalSince1970]);
		return time;
	}

	NSDate* Apple::getNSDateFromTime(const Time& time)
	{
		return [NSDate dateWithTimeIntervalSince1970:(time.getSecondCountF())];
	}

	Variant Apple::getVariantFromCFType(CFTypeRef cfValue)
	{
		CFTypeID cfType = CFGetTypeID(cfValue);
		if (cfType == CFBooleanGetTypeID()) {
			return (sl_bool)(CFBooleanGetValue((CFBooleanRef)cfValue) != 0);
		} else if (cfType == CFNumberGetTypeID()) {
			CFNumberRef number = (CFNumberRef)cfValue;
			CFNumberType type = CFNumberGetType(number);
#define CASE_GET_NUMBER(TYPE, VALUE_TYPE) case TYPE: { VALUE_TYPE n = 0; if (CFNumberGetValue(number, type, &n)) { return n; }; break; }
			switch (type) {
				CASE_GET_NUMBER(kCFNumberSInt8Type, SInt8)
				CASE_GET_NUMBER(kCFNumberSInt16Type, SInt16)
				CASE_GET_NUMBER(kCFNumberSInt32Type, SInt32)
				CASE_GET_NUMBER(kCFNumberSInt64Type, SInt64)
				CASE_GET_NUMBER(kCFNumberFloat32Type, Float32)
				CASE_GET_NUMBER(kCFNumberFloat64Type, Float64)
				CASE_GET_NUMBER(kCFNumberCharType, char)
				CASE_GET_NUMBER(kCFNumberShortType, short)
				CASE_GET_NUMBER(kCFNumberIntType, int)
				CASE_GET_NUMBER(kCFNumberLongType, long)
				CASE_GET_NUMBER(kCFNumberLongLongType, long long)
				CASE_GET_NUMBER(kCFNumberFloatType, float)
				CASE_GET_NUMBER(kCFNumberDoubleType, double)
				CASE_GET_NUMBER(kCFNumberCFIndexType, CFIndex)
				CASE_GET_NUMBER(kCFNumberNSIntegerType, NSInteger)
				CASE_GET_NUMBER(kCFNumberCGFloatType, CGFloat)
				default:
					return getStringFromNSString([(__bridge NSNumber*)number stringValue]);
			}
#undef CASE_GET_NUMBER
		} else if (cfType == CFStringGetTypeID()) {
			return Apple::getStringFromNSString((__bridge NSString*)cfValue);
		} else if (cfType == CFDataGetTypeID()) {
			return Apple::getMemoryFromNSData((__bridge NSData*)cfValue);
		} else if (cfType == CFDateGetTypeID()) {
			return Apple::getTimeFromNSDate((__bridge NSDate*)cfValue);
		}
		return Variant();
	}

	String Apple::getFilePathFromNSURL(NSURL* url)
	{
		if (url != nil) {
			String path = Apple::getStringFromNSString([url path]);
			if (path.startsWith("file://")) {
				path = path.substring(7);
			}
			return path;
		}
		return sl_null;
	}

	NSString* Apple::getSystemLocalizedNSString(NSString* str)
	{
#if defined(SLIB_PLATFORM_IS_IOS)
		NSBundle* bundle = [NSBundle bundleForClass:[UIApplication class]];
		if (bundle != nil) {
			return [bundle localizedStringForKey:str value:str table:nil];
		}
#endif
		return str;
	}

	String Apple::runAppleScript(const StringParam& script, sl_bool flagExternalProcess, sl_bool flagErrorOutput)
	{
#ifdef SLIB_PLATFORM_IS_MACOS
		if (flagExternalProcess) {
			StringData s(script);
			if (flagErrorOutput) {
				return System::getCommandOutput("osascript -s o -e " + CommandLine::makeSafeArgument(s));
			} else {
				return System::getCommandOutput("osascript -e " + CommandLine::makeSafeArgument(s));
			}
		} else {
			NSAppleScript* as = [[NSAppleScript alloc] initWithSource:getNSStringFromString(script)];
			if (as != nil) {
				if (flagErrorOutput) {
					NSDictionary* error = nil;
					NSAppleEventDescriptor* desc = [as executeAndReturnError:&error];
					if (error) {
						return getStringFromNSString(error[@"NSAppleScriptErrorMessage"]);
					}
					if (desc != nil) {
						return getStringFromNSString([desc stringValue]);
					}
				} else {
					NSAppleEventDescriptor* desc = [as executeAndReturnError:NULL];
					if (desc != nil) {
						return getStringFromNSString([desc stringValue]);
					}
				}
			}
		}
#endif
		return sl_null;
	}

}

#endif
