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

package slib.android.app;

import android.Manifest;
import android.app.Activity;
import android.app.role.RoleManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.telecom.TelecomManager;

import java.util.Vector;

import slib.android.Logger;
import slib.android.SlibActivity;
import slib.android.helper.Permissions;
import slib.android.ui.Util;

public class Application {


	private static String[] getPermissions(int permissions) {

		Vector<String> list = new Vector<String>();

		if ((permissions & 1) != 0) {
			list.add(Manifest.permission.CAMERA);
		}

		if ((permissions & (1<<1)) != 0) {
			list.add(Manifest.permission.RECORD_AUDIO);
		}

		if ((permissions & (1<<2)) != 0) {
			list.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
		}
		if ((permissions & (1<<3)) != 0) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
				list.add(Manifest.permission.READ_EXTERNAL_STORAGE);
			}
		}

		if ((permissions & (1<<4)) != 0) {
			list.add(Manifest.permission.READ_PHONE_STATE);
		}
		if ((permissions & (1<<5)) != 0) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				list.add(Manifest.permission.READ_PHONE_NUMBERS);
			}
		}
		if ((permissions & (1<<6)) != 0) {
			list.add(Manifest.permission.CALL_PHONE);
		}
		if ((permissions & (1<<7)) != 0) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				list.add(Manifest.permission.ANSWER_PHONE_CALLS);
			}
		}
		if ((permissions & (1<<8)) != 0) {
			list.add(Manifest.permission.ADD_VOICEMAIL);
		}
		if ((permissions & (1<<9)) != 0) {
			list.add(Manifest.permission.USE_SIP);
		}

		if ((permissions & (1<<10)) != 0) {
			list.add(Manifest.permission.SEND_SMS);
		}
		if ((permissions & (1<<11)) != 0) {
			list.add(Manifest.permission.RECEIVE_SMS);
		}
		if ((permissions & (1<<12)) != 0) {
			list.add(Manifest.permission.READ_SMS);
		}
		if ((permissions & (1<<13)) != 0) {
			list.add(Manifest.permission.RECEIVE_WAP_PUSH);
		}
		if ((permissions & (1<<14)) != 0) {
			list.add(Manifest.permission.RECEIVE_MMS);
		}

		if ((permissions & (1<<15)) != 0) {
			list.add(Manifest.permission.READ_CONTACTS);
		}
		if ((permissions & (1<<16)) != 0) {
			list.add(Manifest.permission.WRITE_CONTACTS);
		}
		if ((permissions & (1<<17)) != 0) {
			list.add(Manifest.permission.GET_ACCOUNTS);
		}

		if ((permissions & (1<<18)) != 0) {
			list.add(Manifest.permission.ACCESS_FINE_LOCATION);
		}
		if ((permissions & (1<<19)) != 0) {
			list.add(Manifest.permission.ACCESS_COARSE_LOCATION);
		}

		if ((permissions & (1<<20)) != 0) {
			list.add(Manifest.permission.READ_CALENDAR);
		}
		if ((permissions & (1<<21)) != 0) {
			list.add(Manifest.permission.WRITE_CALENDAR);
		}

		if ((permissions & (1<<22)) != 0) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
				list.add(Manifest.permission.READ_CALL_LOG);
			}
		}
		if ((permissions & (1<<23)) != 0) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
				list.add(Manifest.permission.WRITE_CALL_LOG);
			}
		}
		if ((permissions & (1<<24)) != 0) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
				list.add(Manifest.permission.PROCESS_OUTGOING_CALLS);
			}
		}

		if ((permissions & (1<<25)) != 0) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
				list.add(Manifest.permission.BODY_SENSORS);
			}
		}

		return list.toArray(new String[] {});
	}

	public static boolean checkPermissions(final Activity activity, final int permissions) {
		try {
			String[] list = getPermissions(permissions);
			if (list.length > 0) {
				return Permissions.checkPermissions(activity, list);
			} else {
				return true;
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
		return false;
	}

	public static void grantPermissions(final Activity activity, final int permissions) {
		if (!(Util.isUiThread())) {
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					grantPermissions(activity, permissions);
				}
			});
			return;
		}
		try {
			String[] list = getPermissions(permissions);
			if (list.length > 0) {
				if (Permissions.grantPermissions(activity, list, SlibActivity.REQUEST_PERMISSIONS)) {
					return;
				}
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
		nativeOnCallbackGrantPermissions();
	}

	private static native void nativeOnCallbackGrantPermissions();

	public static void onRequestPermissionsResult(Activity activity) {
		nativeOnCallbackGrantPermissions();
	}

	private static String getRole(int role) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
			switch (role) {
				case 0:
					return RoleManager.ROLE_HOME;
				case 1:
					return RoleManager.ROLE_BROWSER;
				case 2:
					return RoleManager.ROLE_DIALER;
				case 3:
					return RoleManager.ROLE_SMS;
				case 4:
					return RoleManager.ROLE_EMERGENCY;
				case 5:
					return RoleManager.ROLE_CALL_REDIRECTION;
				case 6:
					return RoleManager.ROLE_CALL_SCREENING;
				case 7:
					return RoleManager.ROLE_ASSISTANT;
			}
		}
		return null;
	}

	public static boolean isRoleHeld(final Activity activity, final int role) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
			try {
				RoleManager manager = (RoleManager) (activity.getSystemService(Context.ROLE_SERVICE));
				if (manager != null) {
					String strRole = getRole(role);
					if (strRole != null) {
						return manager.isRoleHeld(strRole);
					}
				}
			} catch (Exception e) {
				Logger.exception(e);
			}
		}
		return false;
	}

	public static void requestRole(final Activity activity, final int role) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
			if (!(Util.isUiThread())) {
				activity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						requestRole(activity, role);
					}
				});
				return;
			}
			try {
				RoleManager manager = (RoleManager) (activity.getSystemService(Context.ROLE_SERVICE));
				if (manager != null) {
					String strRole = getRole(role);
					if (strRole == null) {
						return;
					}
					Intent intent = manager.createRequestRoleIntent(strRole);
					activity.startActivityForResult(intent, SlibActivity.REQUEST_ROLE);
				}
			} catch (Exception e) {
				Logger.exception(e);
			}
		}
	}

	private static native void nativeOnCallbackRequestRole();

	public static void onRequestRoleResult(Activity activity) {
		nativeOnCallbackRequestRole();
	}

	public static void openDefaultAppsSetting(final Activity activity) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
			if (!(Util.isUiThread())) {
				activity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						openDefaultAppsSetting(activity);
					}
				});
				return;
			}
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					try {
						activity.startActivity(new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS));
					} catch (Exception e) {
						Logger.exception(e);
					}
				}
			});
		}
	}

	public static boolean isSupportedDefaultCallingApp() {
		return Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
	}

	public static boolean isDefaultCallingApp(Activity activity) {
		try {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
				TelecomManager manger = (TelecomManager) (activity.getSystemService(Context.TELECOM_SERVICE));
				if (manger != null) {
					String current = manger.getDefaultDialerPackage();
					if (current != null && current.equals(activity.getPackageName())) {
						return true;
					}
				}
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
		return false;
	}

	private static native void nativeOnCallbackSetDefaultCallingApp();

	public static void onSetDefaultCallingAppResult(Activity activity) {
		nativeOnCallbackSetDefaultCallingApp();
	}

	private static boolean mFlagInitedSetDefaultCallingAppListener = false;

	public static void setDefaultCallingApp(final Activity activity) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			if (!(Util.isUiThread())) {
				activity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						setDefaultCallingApp(activity);
					}
				});
				return;
			}
			if (!mFlagInitedSetDefaultCallingAppListener) {
				mFlagInitedSetDefaultCallingAppListener = true;
				SlibActivity.addActivityResultListener(SlibActivity.REQUEST_ACTIVITY_SET_DEFAULT_CALLING_APP, new SlibActivity.ActivityResultListener() {
					@Override
					public void onActivityResult(Activity activity, int requestCode, int resultCode, Intent data) {
						try {
							onSetDefaultCallingAppResult(activity);
						} catch (Exception e) {
							Logger.exception(e);
						}
					}
				});
			}
			try {
				final Intent intent = new Intent(TelecomManager.ACTION_CHANGE_DEFAULT_DIALER);
				intent.putExtra(TelecomManager.EXTRA_CHANGE_DEFAULT_DIALER_PACKAGE_NAME, activity.getPackageName());
				activity.startActivityForResult(intent, SlibActivity.REQUEST_ACTIVITY_SET_DEFAULT_CALLING_APP);
			} catch (Exception e) {
				Logger.exception(e);
			}
		}

	}

	public static boolean isSystemOverlayEnabled(Activity activity) {
		try {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
				return Settings.canDrawOverlays(activity);
			} else {
				return Permissions.checkPermission(activity, Manifest.permission.SYSTEM_ALERT_WINDOW);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
		return false;
	}

	public static void openSystemOverlaySetting(final Activity activity) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			if (!(Util.isUiThread())) {
				activity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						openSystemOverlaySetting(activity);
					}
				});
				return;
			}
			try {
				Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
				intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				intent.setData(Uri.parse("package:" + activity.getPackageName()));
				activity.startActivity(intent);
			} catch (Exception e) {
				Logger.exception(e);
			}
		}
	}

}
