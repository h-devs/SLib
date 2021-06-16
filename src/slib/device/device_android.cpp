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

#include "slib/device/definition.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/device/device.h"

#include "slib/core/safe_static.h"
#include "slib/core/android/platform.h"
#include "slib/core/android/context.h"
#include "slib/core/android/preference.h"
#include "slib/core/android/window_manager.h"
#include "slib/core/android/display.h"
#include "slib/core/java/object.h"
#include "slib/core/java/list.h"
#include "slib/core/java/uuid.h"

#define MAX_SIM_SLOT_COUNT 8

namespace slib
{

	namespace priv
	{
		namespace device
		{

			SLIB_JNI_BEGIN_CLASS(JAudioManager, "android/media/AudioManager")
				SLIB_JNI_METHOD(getMode, "getMode", "()I")
				SLIB_JNI_METHOD(setMode, "setMode", "(I)V");
				SLIB_JNI_METHOD(getRingerMode, "getRingerMode", "()I")
				SLIB_JNI_METHOD(setRingerMode, "setRingerMode", "(I)V");
				SLIB_JNI_METHOD(getStreamVolume, "getStreamVolume", "(I)I")
				SLIB_JNI_METHOD(setStreamVolume, "setStreamVolume", "(III)V");
				SLIB_JNI_METHOD(getStreamMaxVolume, "getStreamMaxVolume", "(I)I")
				SLIB_JNI_METHOD(isMicrophoneMute, "isMicrophoneMute", "()Z")
				SLIB_JNI_METHOD(setMicrophoneMute, "setMicrophoneMute", "(Z)V")
				SLIB_JNI_METHOD(isSpeakerphoneOn, "isSpeakerphoneOn", "()Z")
				SLIB_JNI_METHOD(setSpeakerphoneOn, "setSpeakerphoneOn", "(Z)V")
				SLIB_JNI_METHOD(isBluetoothScoOn, "isBluetoothScoOn", "()Z")
				SLIB_JNI_METHOD(setBluetoothScoOn, "setBluetoothScoOn", "(Z)V")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JVibrator, "android/os/Vibrator")
				SLIB_JNI_METHOD(vibrate, "vibrate", "(J)V")
				SLIB_JNI_METHOD(cancel, "cancel", "()V")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JTelephonyManager, "android/telephony/TelephonyManager")
				SLIB_JNI_METHOD(getPhoneCount, "getPhoneCount", "()I")
				SLIB_JNI_METHOD(getImei, "getImei", "(I)Ljava/lang/String;")
				SLIB_JNI_METHOD(getDeviceId, "getDeviceId", "()Ljava/lang/String;")					
				SLIB_JNI_METHOD(getDeviceIdWithSlotIndex, "getDeviceId", "(I)Ljava/lang/String;")					
				SLIB_JNI_METHOD(getLine1Number, "getLine1Number", "()Ljava/lang/String;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JTelephonySubscriptionManager, "android/telephony/SubscriptionManager")
				SLIB_JNI_METHOD(getActiveSubscriptionInfoForSimSlotIndex, "getActiveSubscriptionInfoForSimSlotIndex", "(I)Landroid/telephony/SubscriptionInfo;")
				SLIB_JNI_METHOD(getActiveSubscriptionInfoList, "getActiveSubscriptionInfoList", "()Ljava/util/List;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JTelephonySubscriptionInfo, "android/telephony/SubscriptionInfo")
				SLIB_JNI_METHOD(getNumber, "getNumber", "()Ljava/lang/String;")
			SLIB_JNI_END_CLASS



			SLIB_JNI_BEGIN_CLASS(JDevice, "slib/platform/android/device/Device")
				SLIB_JNI_STATIC_METHOD(openURL, "openURL", "(Landroid/app/Activity;Ljava/lang/String;)V");
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JPhoneCall, "slib/platform/android/device/PhoneCall")
				SLIB_JNI_STATIC_METHOD(openDial, "openDial", "(Landroid/app/Activity;Ljava/lang/String;)V");
				SLIB_JNI_STATIC_METHOD(callPhone, "callPhone", "(Landroid/app/Activity;Ljava/lang/String;)V");
				SLIB_JNI_STATIC_METHOD(callPhoneWithSim, "callPhone", "(Landroid/app/Activity;Ljava/lang/String;I)V");
				SLIB_JNI_STATIC_METHOD(answerCall, "answerCall", "(Ljava/lang/String;)V");
				SLIB_JNI_STATIC_METHOD(endCall, "endCall", "(Ljava/lang/String;)V");
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JContact, "slib/platform/android/device/Contact")
				SLIB_JNI_NEW(init, "()V");
				SLIB_JNI_STRING_FIELD(namePrefix);
				SLIB_JNI_STRING_FIELD(givenName);
				SLIB_JNI_STRING_FIELD(middleName);
				SLIB_JNI_STRING_FIELD(familyName);
				SLIB_JNI_STRING_FIELD(nameSuffix);
				SLIB_JNI_STRING_FIELD(displayName);
				SLIB_JNI_STRING_FIELD(nickname);
				SLIB_JNI_STRING_FIELD(phoneNumbers);
				SLIB_JNI_STRING_FIELD(emails);

				SLIB_JNI_STATIC_METHOD(getAllContacts, "getAllContacts", "(Landroid/app/Activity;)[Lslib/platform/android/device/Contact;");
				SLIB_JNI_STATIC_METHOD(addContact, "addContact", "(Landroid/app/Activity;Lslib/platform/android/device/Contact;)Z");
			SLIB_JNI_END_CLASS

			SLIB_GLOBAL_ZERO_INITIALIZED(Atomic<PhoneCallCallback>, g_callbackOnIncomingCall)

			SLIB_JNI_BEGIN_CLASS_SECTION(JPhoneCall)
				SLIB_JNI_NATIVE_IMPL(nativeOnIncomingCall, "nativeOnIncomingCall", "(Ljava/lang/String;Ljava/lang/String;)V", void, jstring callId, jstring phoneNumber)
				{
					g_callbackOnIncomingCall(Jni::getString(callId), Jni::getString(phoneNumber));
				}
			SLIB_JNI_END_CLASS_SECTION

			SLIB_GLOBAL_ZERO_INITIALIZED(Atomic<PhoneCallCallback>, g_callbackOnOutgoingCall)

			SLIB_JNI_BEGIN_CLASS_SECTION(JPhoneCall)
				SLIB_JNI_NATIVE_IMPL(nativeOnOutgoingCall, "nativeOnOutgoingCall", "(Ljava/lang/String;Ljava/lang/String;)V", void, jstring callId, jstring phoneNumber)
				{
					g_callbackOnOutgoingCall(Jni::getString(callId), Jni::getString(phoneNumber));
				}
			SLIB_JNI_END_CLASS_SECTION

			SLIB_GLOBAL_ZERO_INITIALIZED(Atomic<PhoneCallCallback>, g_callbackOnEndCall)

			SLIB_JNI_BEGIN_CLASS_SECTION(JPhoneCall)
				SLIB_JNI_NATIVE_IMPL(nativeOnEndCall, "nativeOnEndCall", "(Ljava/lang/String;Ljava/lang/String;)V", void, jstring callId, jstring phoneNumber)
				{
					g_callbackOnEndCall(Jni::getString(callId), Jni::getString(phoneNumber));
				}
			SLIB_JNI_END_CLASS_SECTION

		}
	}

	using namespace priv::device;

	DeviceAudioMode Device::getAudioMode()
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			return (DeviceAudioMode)(JAudioManager::getMode.callInt(manager));
		}
		return DeviceAudioMode::Default;
	}

	void Device::setAudioMode(DeviceAudioMode mode)
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			JAudioManager::setMode.call(manager, (jint)mode);
		}
	}

	DeviceRingerMode Device::getRingerMode()
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			return (DeviceRingerMode)(JAudioManager::getRingerMode.callInt(manager));
		}
		return DeviceRingerMode::Normal;
	}

	void Device::setRingerMode(DeviceRingerMode mode)
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			JAudioManager::setRingerMode.call(manager, (jint)mode);
		}
	}

	float Device::getVolume(AudioStreamType stream)
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			int vol = (int)(JAudioManager::getStreamVolume.callInt(manager, (jint)stream));
			int max = (int)(JAudioManager::getStreamMaxVolume.callInt(manager, (jint)stream));
			if (max) {
				return (float)vol / (float)max;
			}
		}
		return 0;
	}

	void Device::setVolume(float volume, AudioStreamType stream, const DeviceSetVolumeFlags& flags)
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			int max = (int)(JAudioManager::getStreamMaxVolume.callInt(manager, (jint)stream));
			int vol = (int)(volume * (float)max);
			JAudioManager::setStreamVolume.call(manager, (jint)stream, (jint)vol, (jint)(flags.value));
		}
	}

	sl_bool Device::isMicrophoneMute()
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			return JAudioManager::isMicrophoneMute.callBoolean(manager);
		}
		return sl_false;
	}

	void Device::setMicrophoneMute(sl_bool flag)
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			JAudioManager::setMicrophoneMute.call(manager, (jboolean)(flag ? 1 : 0));
		}
	}

	sl_bool Device::isSpeakerOn()
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			return JAudioManager::isSpeakerOn.callBoolean(manager);
		}
		return sl_false;
	}

	void Device::setSpeakerOn(sl_bool flag)
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			JAudioManager::setSpeakerOn.call(manager, (jboolean)(flag ? 1 : 0));
		}
	}

	sl_bool Device::isBluetoothScoOn()
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			return JAudioManager::isBluetoothScoOn.callBoolean(manager);
		}
		return sl_false;
	}

	void Device::setBluetoothScoOn(sl_bool flag)
	{
		JniLocal<jobject> manager = android::Context::getAudioManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			JAudioManager::setBluetoothScoOn.call(manager, (jboolean)(flag ? 1 : 0));
		}
	}

	// From Java code: slib.android.Device.vibrate
	void Device::vibrate(sl_uint32 millisec)
	{
		JniLocal<jobject> vibrator = android::Context::getVibrator(Android::getCurrentContext());
		if (vibrator.isNotNull()) {
			if (millisec) {
				JVibrator::vibrate.call(vibrator, (jlong)millisec);
			} else {
				JVibrator::cancel.call(vibrator);
			}
		}
	}

	// Require permission: android.permission.READ_PHONE_STATE
	// From Java code: slib.android.Device.getSimSlotsCount
	sl_uint32 Device::getSimSlotsCount()
	{
		AndroidSdkVersion version = Android::getSdkVersion();
		if (version >= AndroidSdkVersion::M) {
			JniLocal<jobject> manager = android::Context::getTelephonyManager(Android::getCurrentContext());
			if (manager.isNotNull()) {
				return (sl_uint32)(JTelephonyManager::getPhoneCount.callInt(manager));
			}
		} else if (version > AndroidSdkVersion::LOLLIPOP_MR1) {
			JniLocal<jobject> manager = android::Context::getTelephonySubscriptionManager(Android::getCurrentContext());
			if (manager.isNotNull()) {
				sl_uint32 n = 0;
				for (sl_uint32 i = 0; i < MAX_SIM_SLOT_COUNT; i++) {
					JniLocal<jobject> info = JTelephonySubscriptionManager::getActiveSubscriptionInfoForSimSlotIndex.callObject(manager, (jint)i);
					if (info.isNotNull()) {
						n = i + 1;
					}
				}
				return n;
			}
		} else {
			return 1;
		}
		return 0;
	}

	// Require permission: android.permission.READ_PHONE_STATE
	// From Java code: slib.android.Device.getIMEIs
	List<String> Device::getIMEIs()
	{
		JniLocal<jobject> manager = android::Context::getTelephonyManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			if (Android::getSdkVersion() >= AndroidSdkVersion::M) {
				List<String> ret;
				for (int i = 0; i < MAX_SIM_SLOT_COUNT; i++) {
					String value;
					if (Android::getSdkVersion() >= AndroidSdkVersion::O) {
						value = JTelephonyManager::getImei.callString(manager, (jint)i);
					} else {
						value = JTelephonyManager::getDeviceIdWithSlotIndex.callString(manager, (jint)i);
					}
					if (value.isNotEmpty()) {
						ret.add_NoLock(Move(value));
					} else {
						break;
					}
				}
				return ret;
			} else {
				String value = JTelephonyManager::getDeviceId.callString(manager);
				if (value.isNotEmpty()) {
					return List<String>::createFromElement(Move(value));
				}
			}
		}
		return sl_null;
	}

	// Require permission: android.permission.READ_PHONE_STATE
	// From Java code: slib.android.Device.getIMEI
	String Device::getIMEI(sl_uint32 indexSlot)
	{
		JniLocal<jobject> manager = android::Context::getTelephonyManager(Android::getCurrentContext());
		if (manager.isNotNull()) {
			if (Android::getSdkVersion() >= AndroidSdkVersion::O) {
				return JTelephonyManager::getImei.callString(manager, (jint)indexSlot);
			} else if (Android::getSdkVersion() >= AndroidSdkVersion::M) {
				return JTelephonyManager::getDeviceIdWithSlotIndex.callString(manager, (jint)indexSlot);
			} else {
				if (!indexSlot) {
					return JTelephonyManager::getDeviceId.callString(manager);
				}
			}
		}
		return sl_null;
	}

	// Require permission: android.permission.READ_PHONE_STATE
	// From Java code: slib.android.Device.getPhoneNumbers
	List<String> Device::getPhoneNumbers()
	{
		if (Android::getSdkVersion() >= AndroidSdkVersion::LOLLIPOP_MR1) {
			JniLocal<jobject> manager = android::Context::getTelephonySubscriptionManager(Android::getCurrentContext());
			if (manager.isNotNull()) {
				JniLocal<jobject> list = JTelephonySubscriptionManager::getActiveSubscriptionInfoList.callObject(manager);
				if (list.isNotNull()) {
					sl_int32 n = java::List::size(list);
					if (n > 0) {
						List<String> ret;
						for (sl_int32 i = 0; i < n; i++) {
							JniLocal<jobject> element = java::List::get(list, i);
							if (element.isNotNull()) {
								String number = JTelephonySubscriptionInfo::getNumber.callString(element);
								if (number.isNotEmpty()) {
									ret.add_NoLock(Move(number));
								}
							}
						}
						return ret;
					}
				}
			}
		} else {
			JniLocal<jobject> manager = android::Context::getTelephonyManager(Android::getCurrentContext());
			if (manager.isNotNull()) {
				String number = JTelephonyManager::getLine1Number.callString(manager);
				if (number.isNotEmpty()) {
					return List<String>::createFromElement(Move(number));
				}
			}
		}
		return sl_null;
	}

	// Require permission: android.permission.READ_PHONE_STATE
	// From Java code: slib.android.Device.getPhoneNumber
	String Device::getPhoneNumber(sl_uint32 indexSlot)
	{
		if (Android::getSdkVersion() >= AndroidSdkVersion::LOLLIPOP_MR1) {
			JniLocal<jobject> manager = android::Context::getTelephonySubscriptionManager(Android::getCurrentContext());
			if (manager.isNotNull()) {
				JniLocal<jobject> info = JTelephonySubscriptionManager::getActiveSubscriptionInfoForSimSlotIndex.callObject(manager, (jint)indexSlot);
				if (info.isNotNull()) {
					return JTelephonySubscriptionInfo::getNumber.callString(info);
				}
			}
		} else if (!indexSlot) {
			JniLocal<jobject> manager = android::Context::getTelephonyManager(Android::getCurrentContext());
			if (manager.isNotNull()) {
				return JTelephonyManager::getLine1Number.callString(manager);
			}
		}
		return sl_null;
	}

	// From Java code: slib.android.Device.getDeviceId
	String Device::getDeviceId()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jobject> prefs = android::Context::getSharedPreferences(context, "device_id_prefs", 0);
			if (prefs.isNotNull()) {
				String value = android::SharedPreferences::getString(prefs, "DeviceId", sl_null);
				if (value.isNotNull()) {
					return value;
				}
				JniLocal<jobject> uuid = java::UUID::randomUUID();
				value = java::Object::toString(uuid);
				if (value.isNotEmpty()) {
					JniLocal<jobject> editor = android::SharedPreferences::getEditor();
					if (editor.isNotNull()) {
						android::SharedPreferencesEditor::putString(editor, value);
						android::SharedPreferencesEditor::apply(editor);\
						return value;
					}
				}
			}
		}
		return sl_null;
	}

	String Device::getDeviceName()
	{
		return Android::getDeviceName();
	}

	String Device::getSystemVersion()
	{
		return Android::getSystemRelease();
	}

	String Device::getSystemName()
	{
		return "Android " + Android::getSystemRelease();
	}

	// From Java code: slib.android.Device.getScreenSize
	Sizei Device::getScreenSize()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			if (android::Activity::isActivity(context)) {
				JniLocal<jobject> manager = android::Activity::getWindowManager(context);
				if (manager.isNotNull()) {
					JniLocal<jobject> display = android::WindowManager::getDefaultDisplay(manager);
					if (display.isNotNull()) {
						JniLocal<jobject> metrics = android::Display::getMetrics(display);
						if (display.isNotNull()) {
							return Sizei(android::DisplayMetrics::getWidthPixels(metrics), android::DisplayMetrics::getHeightPixels(metrics));
						}
					}
				}
			}
		}
		return Sizei::zero();
	}

	double Device::getScreenPPI()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			if (android::Activity::isActivity(context)) {
				JniLocal<jobject> manager = android::Activity::getWindowManager(context);
				if (manager.isNotNull()) {
					JniLocal<jobject> display = android::WindowManager::getDefaultDisplay(manager);
					if (display.isNotNull()) {
						JniLocal<jobject> metrics = android::Display::getMetrics(display);
						if (display.isNotNull()) {
							return android::DisplayMetrics::getDensityDpi(metrics);
						}
					}
				}
			}
		}
		return Sizei::zero();
	}

	void Device::openUrl(const StringParam& _url) {
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jstring> jurl = Jni::getJniString(_url);
			JDevice::openURL.call(sl_null, context, jurl.get());
		}
	}

	void Device::openDial(const String& phoneNumber)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jstring> tel = Jni::getJniString(phoneNumber);
			JPhoneCall::openDial.call(sl_null, context, tel.get());
		}
	}

	void Device::callPhone(const String& phoneNumber)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jstring> tel = Jni::getJniString(phoneNumber);
			JPhoneCall::callPhone.call(sl_null, context, tel.get());
		}
	}

	void Device::callPhone(const String& phoneNumber, sl_uint32 indexSIM)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jstring> tel = Jni::getJniString(phoneNumber);
			JPhoneCall::callPhoneWithSim.call(sl_null, context, tel.get(), (jint)indexSIM);
		}
	}

	void Device::answerCall(const String& callId)
	{
		JniLocal<jstring> s = Jni::getJniString(callId);
		JPhoneCall::answerCall.call(sl_null, s.get());
	}

	void Device::endCall(const String& callId)
	{
		JniLocal<jstring> s = Jni::getJniString(callId);
		JPhoneCall::endCall.call(sl_null, s.get());
	}

	void Device::addOnIncomingCall(const PhoneCallCallback& callback)
	{
		g_callbackOnIncomingCall.add(callback);
	}

	void Device::removeOnIncomingCall(const PhoneCallCallback& callback)
	{
		g_callbackOnIncomingCall.remove(callback);
	}

	void Device::addOnOutgoingCall(const PhoneCallCallback& callback)
	{
		g_callbackOnOutgoingCall.add(callback);
	}

	void Device::removeOnOutgoingCall(const PhoneCallCallback& callback)
	{
		g_callbackOnOutgoingCall.remove(callback);
	}

	void Device::addOnEndCall(const PhoneCallCallback& callback)
	{
		g_callbackOnEndCall.add(callback);
	}

	void Device::removeOnEndCall(const PhoneCallCallback& callback)
	{
		g_callbackOnEndCall.remove(callback);
	}

	List<Contact> Device::getAllContacts()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jobjectArray> arr = (jobjectArray)(JContact::getAllContacts.callObject(sl_null, context));
			if (arr.isNotNull()) {
				sl_uint32 n = Jni::getArrayLength(arr.get());
				List<Contact> ret;
				for (sl_uint32 i = 0; i < n; i++) {
					JniLocal<jobject> obj = Jni::getObjectArrayElement(arr.get(), i);
					if (obj.isNotNull()) {
						Contact c;
						c.namePrefix = JContact::namePrefix.get(obj.get());
						c.givenName = JContact::givenName.get(obj.get());
						c.middleName = JContact::middleName.get(obj.get());
						c.familyName = JContact::familyName.get(obj.get());
						c.nameSuffix = JContact::nameSuffix.get(obj.get());
						c.displayName = JContact::displayName.get(obj.get());
						c.nickname = JContact::nickname.get(obj.get());
						{
							ListElements<String> list(JContact::phoneNumbers.get(obj.get()).split(","));
							for (sl_size k = 0; k < list.count; k++) {
								String& e = list[k];
								sl_reg t = e.indexOf(':');
								if (t >= 0) {
									String label = e.substring(0, t);
									String value = e.substring(t + 1);
									c.phoneNumbers.add_NoLock(label, value);
								} else {
									c.phoneNumbers.add_NoLock(String::getEmpty(), e);
								}
							}
						}
						{
							ListElements<String> list(JContact::emails.get(obj.get()).split(","));
							for (sl_size k = 0; k < list.count; k++) {
								String& e = list[k];
								sl_reg t = e.indexOf(':');
								if (t >= 0) {
									String label = e.substring(0, t);
									String value = e.substring(t + 1);
									c.emails.add_NoLock(label, value);
								} else {
									c.emails.add_NoLock(String::getEmpty(), e);
								}
							}
						}
						ret.add_NoLock(c);
					}
				}
				return ret;
			}
		}
		return sl_null;
	}

	/*
	sl_bool Device::addContact(const Contact& contact)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jobject> jcontact = JContact::init.newObject(sl_null);
			if (jcontact.isNotNull()) {
				JContact::namePrefix.set(jcontact.get(), contact.namePrefix);
				JContact::givenName.set(jcontact.get(), contact.givenName);
				JContact::middleName.set(jcontact.get(), contact.middleName);
				JContact::familyName.set(jcontact.get(), contact.familyName);
				JContact::nameSuffix.set(jcontact.get(), contact.nameSuffix);
				JContact::nickname.set(jcontact.get(), contact.nickname);
				JContact::phoneNumbers.set(jcontact.get(), String::join(contact.phoneNumbers, ","));
				JContact::emails.set(jcontact.get(), String::join(contact.emails, ","));
				return JContact::addContact.callBoolean(sl_null, context, jcontact.get());
			}
		}
		return sl_false;
	}
	*/

}

#endif
