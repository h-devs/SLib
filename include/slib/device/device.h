/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DEVICE_DEVICE
#define CHECKHEADER_SLIB_DEVICE_DEVICE

#include "constants.h"
#include "../core/string.h"
#include "../core/function.h"
#include "../math/size.h"
#include "../media/constants.h"

namespace slib
{

	typedef Function<void(const String& callId, const String& phoneNumber)> PhoneCallCallback;

	class Contact;

	class VideoControllerInfo
	{
	public:
		String name;
		sl_uint64 memorySize;

	public:
		VideoControllerInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(VideoControllerInfo)

	};

	class SoundDeviceInfo
	{
	public:
		String name;
		String manufacturer;
		String pnpDeviceId;

	public:
		SoundDeviceInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SoundDeviceInfo)

	};

	class SLIB_EXPORT Device
	{
	public:
		// Works on iOS
		static void setAudioCategory(DeviceAudioCategory category);

		static DeviceAudioMode getAudioMode();

		static void setAudioMode(DeviceAudioMode mode);

		static DeviceRingerMode getRingerMode();

		static void setRingerMode(DeviceRingerMode mode);

		static float getVolume(AudioStreamType stream = AudioStreamType::Default);

		static void setVolume(AudioStreamType stream, float volume, const DeviceSetVolumeFlags& flags = 0);

		static void setVolume(float volume, const DeviceSetVolumeFlags& flags = 0);

		static sl_bool isMute(AudioStreamType stream = AudioStreamType::Default);

		static void setMute(AudioStreamType stream, sl_bool flagMute = sl_true, const DeviceSetVolumeFlags& flags = 0);

		static void setMute(sl_bool flagMute = sl_true, const DeviceSetVolumeFlags& flags = 0);

		static float getMicrophoneVolume();

		static void setMicrophoneVolume(float volume);

		static sl_bool isMicrophoneMute();

		static void setMicrophoneMute(sl_bool flag = sl_true);

		static sl_bool isSpeakerphoneOn();

		static void setSpeakerphoneOn(sl_bool flag = sl_true);

		static sl_bool isBluetoothScoOn();

		static void setBluetoothScoOn(sl_bool flag = sl_true);

		static void vibrate(sl_uint32 durationMillis = 500);


		// Works on Android
		static sl_uint32 getSimSlotCount();

		// Works on Android
		static List<String> getIMEIs();

		// Works on Android
		static String getIMEI(sl_uint32 indexSlot = 0);

		// Works on Android
		static List<String> getPhoneNumbers();

		// Works on Android
		static String getPhoneNumber(sl_uint32 indexSlot = 0);


		static String getDeviceId();

		static double getScreenPPI();

		static SizeI getScreenSize();

		static sl_uint32 getScreenWidth();

		static sl_uint32 getScreenHeight();


		static String getManufacturer();

		static String getModel();

		static String getBoardSerialNumber();


		static List<VideoControllerInfo> getVideoControllers();

		static List<SoundDeviceInfo> getSoundDevices();


		static void openDial(const String& phoneNumber);

		static void callPhone(const String& phoneNumber);

		static void callPhone(const String& phoneNumber, sl_uint32 indexSIM);


		static void answerCall(const String& callId);

		static void endCall(const String& callId);


		static void addOnIncomingCall(const PhoneCallCallback& callback);

		static void removeOnIncomingCall(const PhoneCallCallback& callback);

		static void addOnOutgoingCall(const PhoneCallCallback& callback);

		static void removeOnOutgoingCall(const PhoneCallCallback& callback);

		static void addOnEndCall(const PhoneCallCallback& callback);

		static void removeOnEndCall(const PhoneCallCallback& callback);


		static List<Contact> getAllContacts();

	};

}

#endif
