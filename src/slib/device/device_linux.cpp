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

#if defined(SLIB_UI_IS_GTK)

#include "slib/device/device.h"

#include "slib/system/system.h"
#include "slib/io/file.h"
#include "slib/core/stringx.h"
#include "slib/core/safe_static.h"
#include "slib/dl/linux/gdk.h"
#include "slib/dl/linux/gtk.h"

namespace slib
{

	namespace
	{
		static String GetAudioDefaultSink()
		{
			String s = System::getCommandOutput("pactl info");
			sl_reg index = s.indexOf(StringView::literal("Default Sink:"));
			if (index >= 0) {
				index += 13;
				sl_reg index2 = Stringx::indexOfLine(s, index);
				if (index2 >= 0) {
					return s.substring(index, index2).trim();
				}
			}
			return sl_null;
		}

		static String GetAudioDefaultSource()
		{
			String s = System::getCommandOutput("pactl info");
			sl_reg index = s.indexOf(StringView::literal("Default Source:"));
			if (index >= 0) {
				index += 15;
				sl_reg index2 = Stringx::indexOfLine(s, index);
				if (index2 >= 0) {
					return s.substring(index, index2).trim();
				}
			}
			return sl_null;
		}

		static float GetAudioVolume(const StringView& output, const StringView& name)
		{
			char* str = output.getData();
			String sName = StringView::literal("Name: ") + name;
			sl_reg index = output.indexOf(sName);
			if (index > 0) {
				index += sName.getLength();
				index = output.indexOf(StringView::literal("Volume:"), index);
				if (index > 0) {
					index += 7;
					index = Stringx::indexOfNotWhitespace(output, index);
					sl_uint32 v = 0;
					sl_uint32 n = 0;
					while (index > 0) {
						sl_reg index2 = Stringx::indexOfWhitespace(output, index + 1);
						if (index2 < 0) {
							break;
						}
						char c = str[index2];
						if (c == '\r' || c == '\n') {
							break;
						}
						if (str[index2 - 1] == '%') {
							sl_uint32 m;
							if (StringView(str + index, index2 - 1 - index).parseUint32(&m)) {
								v += m;
								n++;
							}
						}
						index = Stringx::indexOfNotWhitespace(output, index2 + 1);
					}
					if (n) {
						return (float)(v / n) / 100.0f;
					}
				}
			}
			return 0;
		}

		static sl_bool IsAudioMute(const StringView& output, const StringView& name)
		{
			String sName = StringView::literal("Name: ") + name;
			sl_reg index = output.indexOf(sName);
			if (index > 0) {
				index += sName.getLength();
				index = output.indexOf(StringView::literal("Mute:"), index);
				if (index > 0) {
					index += 5;
					sl_reg index2 = Stringx::indexOfLine(output, index);
					if (index2 > 0) {
						if (StringView(output.getData() + index, index2 - index).trim() == StringView::literal("yes")) {
							return sl_true;
						}
					}
				}
			}
			return sl_false;
		}
	}

	float Device::getVolume(AudioStreamType stream)
	{
		String defaultSink = GetAudioDefaultSink();
		if (defaultSink.isNotEmpty()) {
			String s = System::getCommandOutput("pactl list sinks");
			return GetAudioVolume(s, defaultSink);
		}
		return 0;
	}

	void Device::setVolume(AudioStreamType stream, float volume, const DeviceSetVolumeFlags& flags)
	{
		System::execute(String::concat(StringView::literal("pactl set-sink-volume @DEFAULT_SINK@ "), String::fromInt32((sl_int32)(volume * 100)), StringView::literal("%")));
	}

	sl_bool Device::isMute(AudioStreamType stream)
	{
		String defaultSink = GetAudioDefaultSink();
		if (defaultSink.isNotEmpty()) {
			String s = System::getCommandOutput("pactl list sinks");
			return IsAudioMute(s, defaultSink);
		}
		return sl_false;
	}

	void Device::setMute(AudioStreamType stream, sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
		if (flagMute) {
			System::execute(StringView::literal("pactl set-sink-mute @DEFAULT_SINK@ 1"));
		} else {
			System::execute(StringView::literal("pactl set-sink-mute @DEFAULT_SINK@ 0"));
		}
	}

	float Device::getMicrophoneVolume()
	{
		String defaultSource = GetAudioDefaultSource();
		if (defaultSource.isNotEmpty()) {
			String s = System::getCommandOutput("pactl list sources");
			return GetAudioVolume(s, defaultSource);
		}
		return 0;
	}

	void Device::setMicrophoneVolume(float volume)
	{
		System::execute(String::concat(StringView::literal("pactl set-source-volume @DEFAULT_SOURCE@ "), String::fromInt32((sl_int32)(volume * 100)), StringView::literal("%")));
	}

	sl_bool Device::isMicrophoneMute()
	{
		String defaultSource = GetAudioDefaultSource();
		if (defaultSource.isNotEmpty()) {
			String s = System::getCommandOutput("pactl list sources");
			return IsAudioMute(s, defaultSource);
		}
		return sl_false;
	}

	void Device::setMicrophoneMute(sl_bool flag)
	{
		if (flag) {
			System::execute(StringView::literal("pactl set-source-mute @DEFAULT_SOURCE@ 1"));
		} else {
			System::execute(StringView::literal("pactl set-source-mute @DEFAULT_SOURCE@ 0"));
		}
	}

	double Device::getScreenPPI()
	{
		gtk_init_check(NULL, NULL);
		GdkScreen* screen = gdk_screen_get_default();
		if (screen) {
			return gdk_screen_get_resolution(screen);
		}
		return 96;
	}

	SizeI Device::getScreenSize()
	{
		gtk_init_check(NULL, NULL);
		GdkScreen* screen = gdk_screen_get_default();
		if (screen) {
			SizeI ret;
			ret.x = (int)(gdk_screen_get_width(screen));
			ret.y = (int)(gdk_screen_get_height(screen));
			return ret;
		}
		return SizeI::zero();
	}

	String Device::getManufacturer()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, File::readAllTextUTF8("/sys/devices/virtual/dmi/id/board_vendor").trim());
		return ret;
	}

	String Device::getModel()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, File::readAllTextUTF8("/sys/devices/virtual/dmi/id/product_name").trim())
		return ret;
	}

	// Requires root privilege
	String Device::getBoardSerialNumber() 
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, File::readAllTextUTF8("/sys/devices/virtual/dmi/id/chassis_serial").trim())
		return ret;
	}

}

#endif
