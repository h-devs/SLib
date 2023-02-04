/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/core/preference.h"

#include "slib/platform.h"
#include "slib/platform/android/context.h"
#include "slib/platform/android/preference.h"

namespace slib
{

	namespace {

		static JniLocal<jobject> GetSharedPreference() noexcept
		{
			jobject context = Android::getCurrentContext();
			if (context) {
				return android::Context::getSharedPreferences(context,
					android::Context::getPackageName(context) + "__preferences",
					0 // Context.MODE_PRIVATE
					);
			}
			return sl_null;
		}

		static JniLocal<jobject> GetSharedPreferenceEditor() noexcept
		{
			JniLocal<jobject> pref = GetSharedPreference();
			if (pref) {
				return android::SharedPreferences::getEditor(pref);
			}
			return sl_null;
		}

	}

	// From Java code: slib.android.Preference.getString
	Json Preference::getValue(const StringParam& key)
	{
		JniLocal<jobject> pref = GetSharedPreference();
		if (pref.isNotNull()) {
			String value = android::SharedPreferences::getString(pref, key, sl_null);
			return Json::parse(value);
		}
		return sl_null;
	}

	// From Java code: slib.android.Preference.setString
	void Preference::setValue(const StringParam& key, const Json& value)
	{
		JniLocal<jobject> editor = GetSharedPreferenceEditor();
		if (editor.isNotNull()) {
			if (value.isNotNull()) {
				android::SharedPreferencesEditor::putString(editor, key, value.toJsonString());
			} else {
				android::SharedPreferencesEditor::remove(editor, key);
			}
			android::SharedPreferencesEditor::apply(editor);
		}
	}

}

#endif
