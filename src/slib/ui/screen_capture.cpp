/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/screen_capture.h"

#include "slib/media/audio_data.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CaptureScreenInfo)

	CaptureScreenInfo::CaptureScreenInfo()
	{
		screenWidth = 0;
		screenHeight = 0;
		scaleFactor = 1.0f;
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(Screenshot)

	Screenshot::Screenshot()
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CaptureScreenResult)

	CaptureScreenResult::CaptureScreenResult()
	{
		screenIndex = 0;
		status = CaptureScreenStatus::OK;
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CaptureAudioResult)

	CaptureAudioResult::CaptureAudioResult()
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ScreenCaptureParam)

	ScreenCaptureParam::ScreenCaptureParam()
	{
		flagCaptureScreen = sl_true;
		maxWidth = 0; // Maximum value
		maxHeight = 0; // Maximum value
		flagShowCursor = sl_true;

		screenInterval = 0; // Maximum supported frame rate
		flagCaptureAudio = sl_false;
		audioSamplesPerSecond = 16000;
		audioChannelCount = 1;
		audioFramesPerCallback = 0;
		flagExcludeCurrentProcessAudio = sl_false;
	}


	ScreenCapture::ScreenCapture()
	{
		m_flagCaptureScreen = sl_false;
		m_flagCaptureAudio = sl_false;
		m_nAudioChannels = 1;
		m_nAudioFramesPerCallback = 0;
		m_nAudioFramesInCallbackBuffer = 0;
	}

	ScreenCapture::~ScreenCapture()
	{
	}

	Ref<Image> ScreenCapture::takeScreenshot(sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		Screenshot ret;
		if (takeScreenshot(ret, maxWidth, maxHeight)) {
			return ret.image;
		}
		return sl_null;
	}

	Ref<Image> ScreenCapture::takeScreenshotFromCurrentMonitor(sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		Screenshot ret;
		if (takeScreenshotFromCurrentMonitor(ret, maxWidth, maxHeight)) {
			return ret.image;
		}
		return sl_null;
	}

#if !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_WIN32) && !defined(SLIB_UI_IS_GTK)
	sl_bool ScreenCapture::takeScreenshot(Screenshot& _out, sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		return sl_false;
	}

sl_bool ScreenCapture::takeScreenshotFromCurrentMonitor(Screenshot& _out, sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		return sl_false;
	}

	List<Screenshot>  ScreenCapture::takeScreenshotsFromAllMonitors(sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		return sl_null;
	}

	sl_uint32 ScreenCapture::getScreenCount()
	{
		return 0;
	}
#endif

#if !defined(SLIB_UI_IS_MACOS)
	sl_bool ScreenCapture::isEnabled()
	{
#	if defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_GTK)
		return sl_true;
#	else
		return sl_false;
#	endif
	}

	void ScreenCapture::openSystemPreferences()
	{
	}

	void ScreenCapture::requestAccess()
	{
	}
#endif

#if !defined(SLIB_PLATFORM_IS_WIN32)
	void ScreenCapture::switchToCurrentDesktop()
	{
	}
#endif

	void ScreenCapture::_init(const ScreenCaptureParam& param)
	{
		m_flagCaptureScreen = param.flagCaptureScreen;
		m_flagCaptureAudio = param.flagCaptureAudio;
		m_nAudioChannels = param.audioChannelCount;
		m_nAudioFramesPerCallback = param.audioFramesPerCallback;
		m_onCaptureScreen = param.onCaptureScreen;
		m_onCaptureAudio = param.onCaptureAudio;
	}

	Array<sl_int16> ScreenCapture::_getAudioCallbackBuffer(sl_uint32 nSamples)
	{
		Array<sl_int16> buf = m_bufAudioCallback;
		if (buf.getCount() != nSamples) {
			buf = Array<sl_int16>::create(nSamples);
			if (buf.isNull()) {
				return sl_null;
			}
			m_bufAudioCallback = buf;
			m_nAudioFramesInCallbackBuffer = 0;
		}
		return buf;
	}

	void ScreenCapture::_processAudioFrame(AudioData& input, sl_bool flagMute)
	{
		sl_uint32 nFramesInBuffer = m_nAudioFramesInCallbackBuffer;
		if (!nFramesInBuffer) {
			if (flagMute) {
				return;
			}
		}

		sl_uint32 nFrames = (sl_uint32)(input.count);
		if (!nFrames) {
			return;
		}
		if (m_onCaptureAudio.isNull()) {
			return;
		}

		sl_uint32 nFramesPerCallback = m_nAudioFramesPerCallback;
		if (!nFramesPerCallback) {
			CaptureAudioResult result;
			result.data = Move(input);
			m_onCaptureAudio(this, result);
			return;
		}

		sl_uint32 nChannels = m_nAudioChannels;
		sl_uint32 nSamplesPerCallback = nFramesPerCallback * nChannels;

		AudioData audio;
		if (nChannels == 1) {
			audio.format = AudioFormat::Int16_Mono;
		} else {
			audio.format = AudioFormat::Int16_Stereo;
		}

		if (nFramesInBuffer >= nFramesPerCallback) {
			nFramesInBuffer = 0;
		}
		if (!nFramesInBuffer && nFramesPerCallback == nFrames) {
			audio.count = nFrames;
			Array<sl_int16> buf;
			if (audio.format != input.format) {
				buf = _getAudioCallbackBuffer(nSamplesPerCallback);
				if (buf.isNull()) {
					return;
				}
				audio.data = buf.getData();
				audio.copySamplesFrom(input, nFrames);
			} else {
				audio.data = input.data;
			}
			CaptureAudioResult result;
			result.data = audio;
			m_onCaptureAudio(this, result);
			return;
		}
		Array<sl_int16> buf = _getAudioCallbackBuffer(nSamplesPerCallback);
		if (buf.isNull()) {
			return;
		}
		audio.count = nFramesPerCallback;
		sl_int16* pData = buf.getData();
		sl_uint32 iOffset = 0;
		nFramesInBuffer = m_nAudioFramesInCallbackBuffer;
		if (nFramesInBuffer) {
			audio.data = pData + nFramesInBuffer * nChannels;
			if (nFramesInBuffer + nFrames < nFramesPerCallback) {
				audio.copySamplesFrom(input, nFrames);
				m_nAudioFramesInCallbackBuffer = nFramesInBuffer + nFrames;
				return;
			} else {
				sl_uint32 nRemain = nFramesPerCallback - nFramesInBuffer;
				audio.copySamplesFrom(input, nRemain);
				audio.data = pData;
				iOffset = nRemain;
				CaptureAudioResult result;
				result.data = audio;
				m_onCaptureAudio(this, result);
			}
		} else {
			audio.data = pData;
		}
		if (flagMute) {
			m_nAudioFramesInCallbackBuffer = 0;
			return;
		}
		sl_uint32 nCallbacks = (nFrames - iOffset) / nFramesPerCallback;
		for (sl_uint32 i = 0; i < nCallbacks; i++) {
			audio.copySamplesFrom(input, iOffset, nFramesPerCallback);
			iOffset += nFramesPerCallback;
			CaptureAudioResult result;
			result.data = audio;
			m_onCaptureAudio(this, result);
		}
		if (iOffset < nFrames) {
			sl_uint32 nRemain = nFrames - iOffset;
			audio.copySamplesFrom(input, iOffset, nRemain);
			m_nAudioFramesInCallbackBuffer = nRemain;
		} else {
			m_nAudioFramesInCallbackBuffer = 0;
		}
	}

}
