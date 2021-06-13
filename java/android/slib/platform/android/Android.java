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

package slib.platform.android;

import android.app.Activity;

public class Android {

	public static void onCreateActivity(Activity activity)
	{
		nativeOnCreateActivity(activity);
	}
	
	public static void onResumeActivity(Activity activity)
	{
		nativeOnResumeActivity(activity);
	}
	
	public static void onPauseActivity(Activity activity)
	{
		nativeOnPauseActivity(activity);
	}
	
	public static void onDestroyActivity(Activity activity)
	{
		nativeOnDestroyActivity(activity);
	}
	
	public static boolean onBack(Activity activity)
	{
		return nativeOnBack(activity);
	}

	public static void onConfigurationChanged(Activity activity) {
		nativeOnConfigurationChanged(activity);
	}

	public static void onChangeWindowInsets(Activity activity) {
		nativeOnChangeWindowInsets(activity);
	}

	public static void onOpenUrl(Activity activity, String url) {
		nativeOnOpenUrl(activity, url);
	}

	private static native void nativeOnCreateActivity(Activity activity);
	private static native void nativeOnResumeActivity(Activity activity);
	private static native void nativeOnPauseActivity(Activity activity);
	private static native void nativeOnDestroyActivity(Activity activity);
	private static native boolean nativeOnBack(Activity activity);
	private static native void nativeOnConfigurationChanged(Activity activity);
	private static native void nativeOnChangeWindowInsets(Activity activity);
	private static native void nativeOnOpenUrl(Activity activity, String url);

}