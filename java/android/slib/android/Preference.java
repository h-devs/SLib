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

package slib.android;

import android.content.Context;
import android.content.SharedPreferences;

public class Preference {

    public static String getString(Context context, String key) {
        try {
            SharedPreferences sp = getDefaultSharedPreferences(context);
            return sp.getString(key, null);
        } catch (Exception e) {
            Logger.exception(e);
        }
        return null;
    }

    public static void setString(Context context, String key, String value) {
        try {
            SharedPreferences sp = getDefaultSharedPreferences(context);
            SharedPreferences.Editor editor = sp.edit();
            if (value != null) {
                editor.putString(key, value);
            } else {
                editor.remove(key);
            }
            editor.commit();
        } catch (Exception e) {
            Logger.exception(e);
        }
    }

    static String getDefaultSharedPreferencesName(Context context) {
        return context.getPackageName() + "_preferences";
    }

    private static SharedPreferences getDefaultSharedPreferences(Context context) {
        return context.getSharedPreferences(getDefaultSharedPreferencesName(context), Context.MODE_PRIVATE);
    }

}
