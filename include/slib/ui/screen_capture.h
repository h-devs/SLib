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

#ifndef CHECKHEADER_SLIB_UI_SCREEN_CAPTURE
#define CHECKHEADER_SLIB_UI_SCREEN_CAPTURE

#include "definition.h"

#include "../graphics/image.h"
#include "../media/audio_data.h"
#include "../core/function.h"

namespace slib
{

	class ScreenCapture;

	enum class CaptureScreenStatus
	{
		None = 0,
		OK = 1,
		Idle = 2,
		Stopped = 3,
		Error = 15
	};

	class SLIB_EXPORT CaptureScreenInfo
	{
	public:
		sl_uint32 screenWidth;
		sl_uint32 screenHeight;
		float scaleFactor;

	public:
		CaptureScreenInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CaptureScreenInfo)

	};

	class SLIB_EXPORT Screenshot : public CaptureScreenInfo
	{
	public:
		Ref<Image> image;
		
	public:
		Screenshot();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Screenshot)

	};

	class CaptureScreenResult : public CaptureScreenInfo
	{
	public:
		sl_uint32 screenIndex;
		CaptureScreenStatus status;
		BitmapData data;

	public:
		CaptureScreenResult();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CaptureScreenResult)

	};

	class CaptureAudioResult
	{
	public:
		AudioData data;

	public:
		CaptureAudioResult();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CaptureAudioResult)

	};

	class SLIB_EXPORT ScreenCaptureParam
	{
	public:
		sl_bool flagCaptureScreen;
		sl_uint32 maxWidth;
		sl_uint32 maxHeight;
		sl_bool flagShowCursor;
		sl_uint32 screenInterval; // In milliseconds

		sl_bool flagCaptureAudio;
		sl_uint32 audioSamplesPerSecond;
		sl_uint32 audioChannelCount;
		sl_uint32 audioFramesPerCallback;
		sl_bool flagExcludeCurrentProcessAudio;

		Function<void(ScreenCapture*, CaptureScreenResult&)> onCaptureScreen;
		Function<void(ScreenCapture*, CaptureAudioResult&)> onCaptureAudio;

	public:
		ScreenCaptureParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ScreenCaptureParam)

	};

	class SLIB_EXPORT ScreenCapture : public Object
	{
	protected:
		ScreenCapture();

		~ScreenCapture();

	public:
#ifdef SLIB_PLATFORM_IS_MACOS
		static Ref<ScreenCapture> create(const ScreenCaptureParam& param);
#endif

		static sl_bool takeScreenshot(Screenshot& _out, sl_uint32 maxWidth = 0, sl_uint32 maxHeight = 0);

		static Ref<Image> takeScreenshot(sl_uint32 maxWidth = 0, sl_uint32 maxHeight = 0);

		static sl_bool takeScreenshotFromCurrentMonitor(Screenshot& _out, sl_uint32 maxWidth = 0, sl_uint32 maxHeight = 0);

		static Ref<Image> takeScreenshotFromCurrentMonitor(sl_uint32 maxWidth = 0, sl_uint32 maxHeight = 0);

		static List<Screenshot> takeScreenshotsFromAllMonitors(sl_uint32 maxWidth = 0, sl_uint32 maxHeight = 0);

		static sl_uint32 getScreenCount();

		static sl_bool isEnabled();

		static void openSystemPreferences();

		static void requestAccess();

#ifdef SLIB_PLATFORM_IS_MACOS
		static void resetAccess(const StringParam& appBundleId);
#endif

		static void switchToCurrentDesktop();

	public:
		virtual void release() = 0;

	protected:
		void _init(const ScreenCaptureParam& param);

		Array<sl_int16> _getAudioCallbackBuffer(sl_uint32 nSamples);

		void _processAudioFrame(AudioData& data);

	protected:
		sl_bool m_flagCaptureScreen;
		sl_bool m_flagCaptureAudio;
		sl_uint32 m_nAudioChannels;
		sl_uint32 m_nAudioFramesPerCallback;

		Function<void(ScreenCapture*, CaptureScreenResult&)> m_onCaptureScreen;
		Function<void(ScreenCapture*, CaptureAudioResult&)> m_onCaptureAudio;

		AtomicArray<sl_int16> m_bufAudioCallback;
		sl_uint32 m_nAudioFramesInCallbackBuffer;
	};

}

#endif
