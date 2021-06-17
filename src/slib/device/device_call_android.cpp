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

#include "slib/device/definition.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/device/device.h"

#include "slib/core/safe_static.h"
#include "slib/core/platform.h"

namespace slib
{

	namespace priv
	{
		namespace device
		{

			SLIB_JNI_BEGIN_CLASS(JPhoneCall, "slib/android/device/PhoneCall")
				SLIB_JNI_STATIC_METHOD(openDial, "openDial", "(Landroid/app/Activity;Ljava/lang/String;)V");
				SLIB_JNI_STATIC_METHOD(callPhone, "callPhone", "(Landroid/app/Activity;Ljava/lang/String;)V");
				SLIB_JNI_STATIC_METHOD(callPhoneWithSim, "callPhone", "(Landroid/app/Activity;Ljava/lang/String;I)V");
				SLIB_JNI_STATIC_METHOD(answerCall, "answerCall", "(Ljava/lang/String;)V");
				SLIB_JNI_STATIC_METHOD(endCall, "endCall", "(Ljava/lang/String;)V");
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JContact, "slib/android/device/Contact")
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

				SLIB_JNI_STATIC_METHOD(getAllContacts, "getAllContacts", "(Landroid/app/Activity;)[Lslib/android/device/Contact;");
				SLIB_JNI_STATIC_METHOD(addContact, "addContact", "(Landroid/app/Activity;Lslib/android/device/Contact;)Z");
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
