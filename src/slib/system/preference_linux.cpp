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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_LINUX_DESKTOP

#include "slib/system/preference.h"

#include "slib/system/system.h"
#include "slib/io/file.h"

namespace slib
{

	namespace {
		static String GetFilePath(const String& appName)
		{
			String path = System::getHomeDirectory() + "/.local/.pref";
			if (!(File::exists(path))) {
				File::createDirectories(path);
			}
			return String::concat(path, "/", appName, ".json");
		}
	}

	void Preference::setValue(const StringParam& key, const Json& value)
	{
		if (key.isEmpty()) {
			return;
		}
		String appName = getApplicationKeyName();
		if (appName.isEmpty()) {
			return;
		}
		String path = GetFilePath(appName);
		Json json = Json::parseTextFile(path);
		json.putItem(key.toString(), value);
		File::writeAllTextUTF8(path, json.toJsonString());
	}

	Json Preference::getValue(const StringParam& key)
	{
		if (key.isEmpty()) {
			return sl_null;
		}
		String appName = getApplicationKeyName();
		if (appName.isEmpty()) {
			return sl_null;
		}
		String path = GetFilePath(appName);
		Json json = Json::parseTextFile(path);
		return json[key.toString()];
	}

}

#endif
