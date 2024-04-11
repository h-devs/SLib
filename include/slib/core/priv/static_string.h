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

#ifndef CHECKHEADER_SLIB_CORE_STATIC_STRING
#define CHECKHEADER_SLIB_CORE_STATIC_STRING

#define SLIB_STATIC_STRING(name, str) \
	static slib::StringContainer _static_string_container_##name = {(sl_char8*)str, sizeof(str)-1, 0, 0, -1}; \
	static slib::StringContainer* _static_string_##name = &_static_string_container_##name; \
	static const slib::String& name = *(reinterpret_cast<slib::String*>(&_static_string_##name));

#define SLIB_RETURN_STRING(str) { SLIB_STATIC_STRING(strRetTemp, str) return strRetTemp; }

#define SLIB_STATIC_STRING16(name, str) \
		static slib::StringContainer16 _static_string_container_##name = {(sl_char16*)SLIB_UNICODE(str), (sizeof(SLIB_UNICODE(str))/2)-1, 0, 0, -1}; \
		static slib::StringContainer16* _static_string_##name = &_static_string_container_##name; \
		static const slib::String16& name = *(reinterpret_cast<slib::String16*>(&_static_string_##name));

#define SLIB_RETURN_STRING16(str) { SLIB_STATIC_STRING16(strRetTemp16, str) return strRetTemp16; }

#define SLIB_STATIC_STRING32(name, str) \
		static slib::StringContainer32 _static_string_container_##name = {(sl_char32*)SLIB_UNICODE32(str), (sizeof(SLIB_UNICODE32(str))/4)-1, 0, 0, -1}; \
		static slib::StringContainer32* _static_string_##name = &_static_string_container_##name; \
		static const slib::String32& name = *(reinterpret_cast<slib::String32*>(&_static_string_##name));

#define SLIB_RETURN_STRING32(str) { SLIB_STATIC_STRING32(strRetTemp32, str) return strRetTemp32; }

#endif
