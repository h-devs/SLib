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

#include "slib/media/definition.h"

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/media/audio_device.h"

#include "slib/system/system.h"

#import <AVFoundation/AVFoundation.h>

namespace slib
{

	sl_bool AudioRecorder::isEnabled()
	{
		if (@available(macos 10.14, *)) {
			AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
			return status == AVAuthorizationStatusNotDetermined || status == AVAuthorizationStatusAuthorized;
		} else {
			return sl_true;
		}
	}

	void AudioRecorder::requestAccess(const Function<void(sl_bool flagGranted)>& callback)
	{
		if (@available(macos 10.14, *)) {
			Function<void(sl_bool flagGranted)> _callback = callback;
			[AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
				_callback(granted ? sl_true : sl_false);
			}];
		} else {
			callback(sl_true);
		}
	}

	void AudioRecorder::resetAccess(const StringParam& appBundleId)
	{
		if (@available(macos 10.14, *)) {
			System::execute(String::concat(StringView::literal("tccutil reset Microphone "), appBundleId));
		}
	}

}

#endif
