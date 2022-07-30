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

package slib.android.device;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Vibrator;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import java.util.UUID;
import java.util.Vector;

import slib.android.Logger;

public class Device {

	@SuppressLint("MissingPermission")
	public static void vibrate(Context context, int durationMillis) {
		try {
			Vibrator v = (Vibrator)(context.getSystemService(Context.VIBRATOR_SERVICE));
			if (v != null) {
				if (durationMillis > 0) {
					v.vibrate(durationMillis);
				} else {
					v.cancel();
				}
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
	}

	private static final int MAX_SIM_SLOT_COUNT = 8;

	@SuppressLint("MissingPermission")
	public static int getSimSlotCount(Context context) {
		try {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
				TelephonyManager tm = (TelephonyManager) (context.getSystemService(Context.TELEPHONY_SERVICE));
				if (tm != null) {
					return tm.getPhoneCount();
				}
			} else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
				SubscriptionManager sm = (SubscriptionManager)(context.getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE));
				if (sm != null) {
					int n = 0;
					for (int i = 0; i < MAX_SIM_SLOT_COUNT; i++) {
						try {
							SubscriptionInfo info = sm.getActiveSubscriptionInfoForSimSlotIndex(i);
							if (info != null) {
								n = i + 1;
							}
						} catch (Exception e) {
							Logger.exception(e);
						}
					}
					return n;
				}
			} else {
				return 1;
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
		return 0;
	}

	@SuppressLint("MissingPermission")
	public static String[] getIMEIs(Context context) {
		try {
			TelephonyManager tm = (TelephonyManager)(context.getSystemService(Context.TELEPHONY_SERVICE));
			if (tm != null) {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
					Vector<String> numbers = new Vector<String>();
					for (int i = 0; i < MAX_SIM_SLOT_COUNT; i++) {
						String number = null;
						try {
							if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
								number = tm.getImei(i);
							} else {
								number = tm.getDeviceId(i);
							}
						} catch (Exception e) {
						}
						if (number != null && number.length() > 0) {
							numbers.add(number);
						} else {
							break;
						}
					}
					if (numbers.size() > 0) {
						return numbers.toArray(new String[] {});
					}
				}
				String number = tm.getDeviceId();
				if (number != null && number.length() > 0) {
					return new String[]{ number };
				}
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
		return new String[] {};
	}

	@SuppressLint("MissingPermission")
	public static String getIMEI(Context context, int slot) {
		try {
			TelephonyManager tm = (TelephonyManager)(context.getSystemService(Context.TELEPHONY_SERVICE));
			if (tm != null) {
				try {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
						return tm.getImei(slot);
					} else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
						return tm.getDeviceId(slot);
					}
				} catch (Exception e) {
					Logger.exception(e);
				}
				if (slot == 0) {
					return tm.getDeviceId();
				}
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
		return null;
	}

	@SuppressLint("MissingPermission")
	public static String[] getPhoneNumbers(Context context) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
			try {
				Vector<String> numbers = new Vector<String>();
				SubscriptionManager sm = (SubscriptionManager)(context.getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE));
				if (sm != null) {
					for (SubscriptionInfo info : sm.getActiveSubscriptionInfoList()) {
						numbers.add(info.getNumber());
					}
				}
				if (!(numbers.isEmpty())) {
					return numbers.toArray(new String[] {});
				}
			} catch (Throwable e) {
				Logger.exception(e);
			}
		}
		try {
			TelephonyManager tm = (TelephonyManager) (context.getSystemService(Context.TELEPHONY_SERVICE));
			if (tm != null) {
				return new String[] { tm.getLine1Number() };
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
		return new String[] {};
	}

	@SuppressLint("MissingPermission")
	public static String getPhoneNumber(Context context, int slot) {
		try {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
				SubscriptionManager sm = (SubscriptionManager)(context.getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE));
				if (sm != null) {
					SubscriptionInfo info = sm.getActiveSubscriptionInfoForSimSlotIndex(slot);
					if (info != null) {
						return info.getNumber();
					}
				}
			}
			if (slot == 0) {
				try {
					TelephonyManager tm = (TelephonyManager) (context.getSystemService(Context.TELEPHONY_SERVICE));
					if (tm != null) {
						return tm.getLine1Number();
					}
				} catch (Throwable e) {
					Logger.exception(e);
				}
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
		return null;
	}

	public static String getDeviceId(Context context) {
		try {
			final String KEY_DEVICEID = "DeviceId";
			SharedPreferences prefs = context.getSharedPreferences("device_id_prefs", 0);
			String value = prefs.getString(KEY_DEVICEID, null);
			if (value == null) {
				value = UUID.randomUUID().toString();
				prefs.edit().putString(KEY_DEVICEID, value).apply();
			}
			return value;
		} catch (Exception e) {
			Logger.exception(e);
		}
		return null;
	}

	public static String getDeviceName() {
		String manufacturer = Build.MANUFACTURER;
		String model = Build.MODEL;
		if (model.startsWith(manufacturer)) {
			return model;
		} else {
			return manufacturer + " " + model;
		}
	}

}
