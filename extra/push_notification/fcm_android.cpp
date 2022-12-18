/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include <slib/core/definition.h>

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "fcm.h"

#include <slib/data/json.h>
#include <slib/core/safe_static.h>
#include <slib/ui/platform.h>

namespace slib
{

	namespace priv
	{
		namespace fcm
		{

			void OnToken(JNIEnv* env, jobject _this, jstring token);

			void OnMessageReceived(JNIEnv* env, jobject _this, jstring title, jstring content, jobjectArray data, jboolean flagClicked, jboolean flagBackground);

			SLIB_JNI_BEGIN_CLASS(JFCM, "slib/android/fcm/FCM")
				SLIB_JNI_STATIC_METHOD(initialize, "initialize", "(Landroid/app/Activity;)V");
				SLIB_JNI_NATIVE(onToken, "nativeOnToken", "(Ljava/lang/String;)V", OnToken);
				SLIB_JNI_NATIVE(onReceive, "nativeOnMessageReceived", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;ZZ)V", OnMessageReceived);
			SLIB_JNI_END_CLASS

			void OnToken(JNIEnv* env, jobject _this, jstring _token)
			{
				Ref<FCM> instance = FCM::getInstance();
				if (instance.isNotNull()) {
					String token = Jni::getString(_token);
					instance->dispatchRefreshToken(token);
				}
			}

			void OnMessageReceived(JNIEnv* env, jobject _this, jstring title, jstring content, jobjectArray data, jboolean flagClicked, jboolean flagBackground)
			{
				Ref<FCM> instance = FCM::getInstance();
				if (instance.isNull()) {
					return;
				}
				JsonMap _data;
				if (data) {
					sl_uint32 n = Jni::getArrayLength(data);
					if (n > 0) {
						for (sl_uint32 i = 0; i + 1 < n; i += 2) {
							String key = Jni::getStringArrayElement(data, i);
							String strValue = Jni::getStringArrayElement(data, i + 1);
							Json value;
							if (strValue.isNotEmpty()) {
								JsonParseParam p;
								p.flagLogError = sl_false;
								value = Json::parse(strValue, p);
								if (value.isNull()) {
									value = strValue;
								}
							} else {
								value = strValue;
							}
							_data.add_NoLock(key, value);
						}
					}
				}

				String _title = Jni::getString(title);
				String _content = Jni::getString(content);

				PushNotificationMessage message;
				message.title = _title;
				message.content = _content;
				message.data = _data;
				message.flagClicked = flagClicked;
				message.flagBackground = flagBackground;

				instance->dispatchReceiveMessage(message);
			}

		}
	}

	using namespace priv::fcm;

	Ref<FCM> FCM::getInstance()
	{
		SLIB_SAFE_LOCAL_STATIC(Ref<FCM>, instance, new FCM)
		return instance;
	}

	void FCM::onStart()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JFCM::initialize.call(sl_null, context);
		}
	}

}

#endif
