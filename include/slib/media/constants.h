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

#ifndef CHECKHEADER_SLIB_MEDIA_CONSTANTS
#define CHECKHEADER_SLIB_MEDIA_CONSTANTS

#include "definition.h"

namespace slib
{

	enum class AudioSampleType
	{
		Int8 = 1,
		Uint8 = 2,

		Int16 = 8,
		Uint16 = 9,
		Int16LE = 12,
		Uint16LE = 13,
		Int16BE = 14,
		Uint16BE = 15,

		Float = 32,
		FloatLE = 33,
		FloatBE = 34
	};

	enum class AudioStreamType
	{
		Default = -1,
		VoiceCall = 0,
		System = 1,
		Ring = 2,
		Music = 3,
		Alarm = 4,
		Notification = 5,
		BluetoothSco = 6,
		SystemEnforced = 7,
		DTMF = 8,
		TTS = 9,
		Accessibility = 10
	};

	enum class AudioRecordingPreset
	{
		None = 0,
		Generic = 1,
		Camcorder = 2,
		VoiceRecognition = 3,
		VoiceCommunication = 4,
		Unprocessed = 5
	};

}

#endif
