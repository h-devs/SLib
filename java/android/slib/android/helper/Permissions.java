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

package slib.android.helper;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Process;

import java.util.Vector;

import slib.android.Logger;

public class Permissions {

	public static boolean requestPermissions(Activity activity, String[] permissions, int requestCode) {
		if (Build.VERSION.SDK_INT >= 23) {
			try {
				if (permissions.length == 0) {
					return false;
				}
				activity.requestPermissions(permissions, requestCode);
				return true;
			} catch (Exception e) {
				Logger.exception(e);
			}
		}
		return false;
	}

	public static boolean requestPermission(Activity activity, String permission, int requestCode) {
		return requestPermissions(activity, new String[] {permission}, requestCode);
	}

	public static boolean checkPermission(Context context, String permission) {
		return context.checkPermission(permission, Process.myPid(), Process.myUid()) == PackageManager.PERMISSION_GRANTED;
	}

	public static boolean checkPermissions(Context context, String[] permissions) {
		for (int i = 0; i < permissions.length; i++) {
			if (!(checkPermission(context, permissions[i]))) {
				return false;
			}
		}
		return true;
	}

	public static boolean grantPermissions(Activity activity, String[] permissions, int requestCode) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			Vector<String> list = new Vector<String>();
			for (int i = 0; i < permissions.length; i++) {
				if (!(checkPermission(activity, permissions[i]))) {
					list.add(permissions[i]);
				}
			}
			if (list.size() == 0) {
				return false;
			}
			return requestPermissions(activity, list.toArray(new String[] {}), requestCode);
		} else {
			return false;
		}
	}

	public static boolean grantPermission(Activity activity, String permission, int requestCode) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			if (checkPermission(activity, permission)) {
				return false;
			} else {
				return requestPermission(activity, permission, requestCode);
			}
		} else {
			return false;
		}
	}

	public static boolean hasFeature(Context context, String feature) {
		return context.getPackageManager().hasSystemFeature(feature);
	}

}
