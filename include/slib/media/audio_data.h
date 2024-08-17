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

#ifndef CHECKHEADER_SLIB_MEDIA_AUDIO_DATA
#define CHECKHEADER_SLIB_MEDIA_AUDIO_DATA

#include "audio_format.h"

#include "../core/ref.h"
#include "../core/default_members.h"

namespace slib
{
	class SLIB_EXPORT AudioChannelBuffer
	{
	public:
		void* data;
		sl_size count; // count of samples
		sl_int32 stride; // bytes offset between samples
		Ref<CRef> ref;

	public:
		AudioChannelBuffer();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AudioChannelBuffer)

	};

	class SLIB_EXPORT AudioData
	{
	public:
		sl_size count; // count of audio frames (samples per channel)
		AudioFormat format;

		// main data
		void* data; // samples
		Ref<CRef> ref; // reference for samples

		// additional data for non-interleaved formats
		void* data1; // samples
		Ref<CRef> ref1; // reference for samples

	public:
		AudioData();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AudioData)

	public:
		sl_size getSizeForChannel() const;

		sl_size getTotalSize() const;

		// returns the number of channels
		sl_uint32 getChannelBuffers(AudioChannelBuffer* buffers) const;

		void copySamplesFrom(const AudioData& other, sl_size startFrame, sl_size nFrames) const;

		void copySamplesFrom(const AudioData& other, sl_size nFrames) const;

		void copySamplesFrom(const AudioData& other) const;

		sl_int16 getSample(sl_uint32 sampleIndex, sl_uint32 channelIndex = 0) const;

		sl_int16 getPeakSample(sl_uint32 startSampleIndex, sl_uint32 endSampleIndex, sl_bool flagPositive, sl_uint32 channelIndex = 0) const;

		void seek(sl_reg nFrames);

	};
}

#endif
