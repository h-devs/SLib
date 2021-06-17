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

package slib.android.ui;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Surface;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import java.io.File;
import java.lang.reflect.Field;

import me.leolin.shortcutbadger.ShortcutBadger;
import slib.android.Logger;
import slib.android.SlibActivity;
import slib.android.helper.FileHelper;

public class Util {

	public static boolean isUiThread() {
		Looper looper = Looper.getMainLooper();
		if (looper != null) {
			return looper.getThread() == Thread.currentThread();
		}
		return false;
	}

	public static void dispatchToUiThread(final Runnable runnable) {
		if (handlerMain == null) {
			initMainHandler();
		}
		if (handlerMain != null) {
			handlerMain.post(runnable);
		}
	}

	public static void dispatchToUiThread(final Runnable runnable, int delayMillis) {
		if (handlerMain == null) {
			initMainHandler();
		}
		if (handlerMain != null) {
			if (delayMillis > 0) {
				handlerMain.postDelayed(runnable, delayMillis);
			} else {
				handlerMain.post(runnable);
			}
		}
	}

	public static Point getScreenSize(Activity activity) {
		DisplayMetrics dm = new DisplayMetrics();
		activity.getWindowManager().getDefaultDisplay().getMetrics(dm);
		return new Point(dm.widthPixels, dm.heightPixels);
	}

	public static int getScreenPPI(Activity activity) {
		DisplayMetrics dm = new DisplayMetrics();
		activity.getWindowManager().getDefaultDisplay().getMetrics(dm);
		return dm.densityDpi;
	}

	public static float getScreenDensity(Context context) {
		return context.getResources().getDisplayMetrics().density;
	}

	public static int getScreenOrientation(Activity activity) {
		try {
			DisplayMetrics metrics;
			Display display = activity.getWindowManager().getDefaultDisplay();
			if (display == null) {
				return 0;
			}
			metrics = new DisplayMetrics();
			display.getMetrics(metrics);
			int rotation = display.getRotation();
			int width = metrics.widthPixels;
			int height = metrics.heightPixels;
			if (((rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) && height > width) || ((rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270) && width > height)) {
				switch(rotation) {
					case Surface.ROTATION_0:
						return 0;
					case Surface.ROTATION_90:
						return 90;
					case Surface.ROTATION_180:
						return 180;
					case Surface.ROTATION_270:
						return 270;
					default:
						break;
				}
			} else {
				switch(rotation) {
					case Surface.ROTATION_0:
						return 90;
					case Surface.ROTATION_90:
						return 0;
					case Surface.ROTATION_180:
						return 270;
					case Surface.ROTATION_270:
						return 180;
					default:
						break;
				}
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
		return 0;
	}

	public static void setScreenOrientations(final Activity activity, final boolean flag0, final boolean flag90, final boolean flag180, final boolean flag270) {
		if (!(Util.isUiThread())) {
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					setScreenOrientations(activity, flag0, flag90, flag180, flag270);
				}
			});
			return;
		}
		try {
			int orientation;
			if (flag90 || flag270) {
				if (flag0 || flag180) {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
						orientation = ActivityInfo.SCREEN_ORIENTATION_FULL_USER;
					} else {
						orientation = ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
					}
				} else if (flag90 && flag270) {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
						orientation = ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE;
					} else {
						orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
					}
				} else if (flag270) {
					orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
				} else {
					orientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
				}
			} else {
				if (flag0 && flag180) {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
						orientation = ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT;
					} else {
						orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
					}
				} else if (flag180) {
					orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
				} else {
					orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
				}
			}
			activity.setRequestedOrientation(orientation);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public static void showKeyboard(final Activity activity) {
		if (!(Util.isUiThread())) {
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					showKeyboard(activity);
				}
			});
			return;
		}
		try {
			InputMethodManager imm = (InputMethodManager)(activity.getSystemService(Context.INPUT_METHOD_SERVICE));
			if (imm != null) {
				View view = activity.getCurrentFocus();
				if (view != null) {
					imm.showSoftInput(view, 0);
				}
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public static void dismissKeyboard(final Activity activity) {
		if (!(Util.isUiThread())) {
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					dismissKeyboard(activity);
				}
			});
			return;
		}
		try {
			InputMethodManager imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
			if (imm != null) {
				View view = activity.getCurrentFocus();
				if (view != null) {
					imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
				} else {
					imm.hideSoftInputFromWindow(activity.getWindow().getDecorView().getWindowToken(), 0);
				}
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public static Rect getSafeAreaInsets(Activity activity) {
		if (activity instanceof SlibActivity) {
			Rect rect = ((SlibActivity)activity).getLatestInsets();
			if (rect != null) {
				return rect;
			}
		}
		Rect rect = new Rect(0, 0, 0, 0);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
			try {
				Window window = activity.getWindow();
				if ((window.getAttributes().flags & WindowManager.LayoutParams.FLAG_FULLSCREEN) == 0) {
					if ((window.getAttributes().flags & WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS) != 0 || (window.getDecorView().getSystemUiVisibility() & View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN) != 0) {
						rect.top = getStatusBarHeight(activity);
					}
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return rect;
	}

	public static int getNavigationBarHeight(Context context) {
		try {
			Class<?> c = Class.forName("com.android.internal.R$dimen");
			Object obj = c.newInstance();
			Field field = c.getField("navigation_bar_height");
			int x = Integer.parseInt(field.get(obj).toString());
			return context.getResources().getDimensionPixelSize(x);
		} catch (Exception e) {
			e.printStackTrace();
			return 0;
		}
	}

	public static int getStatusBarHeight(Context context) {
		try {
			Class<?> c = Class.forName("com.android.internal.R$dimen");
			Object obj = c.newInstance();
			Field field = c.getField("status_bar_height");
			int x = Integer.parseInt(field.get(obj).toString());
			return context.getResources().getDimensionPixelSize(x);
		} catch (Exception e) {
			e.printStackTrace();
			return 0;
		}
	}

	public static void setStatusBarStyle(final Activity activity, final boolean flagHidden, final boolean flagDark) {
		if (!(slib.android.ui.Util.isUiThread())) {
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					setStatusBarStyle(activity, flagHidden, flagDark);
				}
			});
			return;
		}
		try {
			Window window = activity.getWindow();
			if (flagHidden) {
				window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
			} else {
				window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
					window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
					window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
					int flagSystemUI = View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
					if (flagDark) {
						flagSystemUI |= View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
					}
					window.getDecorView().setSystemUiVisibility(flagSystemUI);
					window.setStatusBarColor(Color.TRANSPARENT);
				} else {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
						window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public static void setBadgeNumber(final Context context, int count) {
		try {
			ShortcutBadger.applyCount(context, count);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public static void openUrl(final Context context, final String url) {
		if (!(Util.isUiThread())) {
			Util.dispatchToUiThread(new Runnable() {
				@Override
				public void run() {
					openUrl(context, url);
				}
			});
			return;
		}
		try {
			if (url.startsWith("file://")) {
				File file = new File(url.substring(7));
				Uri uri = FileHelper.getUriForFile(context, file);
				if (uri == null) {
					Logger.error("File exposed beyond app: " + file.getAbsolutePath());
					return;
				}
				Intent intent;
				if (url.endsWith("jpg") || url.endsWith("jpeg") || url.endsWith("png")) {
					intent = new Intent(Intent.ACTION_VIEW);
					intent.setDataAndType(uri, "image/*");
				} else {
					intent = new Intent(Intent.ACTION_VIEW, uri);
				}
				intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
				context.startActivity(intent);
			} else {
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
				context.startActivity(intent);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public static void sendFile(final Context context, final String filePath, final String mimeType, final String titleChooser) {
		if (!(Util.isUiThread())) {
			Util.dispatchToUiThread(new Runnable() {
				@Override
				public void run() {
					sendFile(context, filePath, mimeType, titleChooser);
				}
			});
			return;
		}
		try {
			Intent intent = new Intent(Intent.ACTION_SEND);
			intent.setType(mimeType);
			File file = new File(filePath);
			Uri uri = FileHelper.getUriForFile(context, file);
			if (uri == null) {
				Logger.error("File exposed beyond app: " + filePath);
				return;
			}
			intent.putExtra(Intent.EXTRA_STREAM, uri);
			intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
			String title = titleChooser;
			if (title == null || title.length() == 0) {
				title = "Send";
			}
			context.startActivity(Intent.createChooser(intent, title));
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	static private Handler handlerMain = null;

	static private synchronized void initMainHandler() {
		if (handlerMain != null) {
			return;
		}
		handlerMain = new Handler(Looper.getMainLooper());
	}

}
