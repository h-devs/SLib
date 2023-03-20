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

#include "slib/media/audio_data.h"

#include "slib/media/audio_util.h"
#include "slib/core/base.h"
#include "slib/core/endian.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioChannelBuffer)

	AudioChannelBuffer::AudioChannelBuffer() : data(0), count(0), stride(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AudioData)

	AudioData::AudioData()
	{
		count = 0;
		format = AudioFormat::None;

		data = sl_null;
		data1 = sl_null;
	}

	sl_size AudioData::getSizeForChannel() const
	{
		if (format == AudioFormat::None) {
			return 0;
		}
		return ((AudioFormatHelper::getBitsPerSample(format) * count) + 7) >> 3;
	}

	sl_size AudioData::getTotalSize() const
	{
		if (format == AudioFormat::None) {
			return 0;
		}
		if (AudioFormatHelper::isNonInterleaved(format)) {
			return getSizeForChannel() * AudioFormatHelper::getChannelCount(format);
		} else {
			return ((AudioFormatHelper::getBitsPerSample(format) * AudioFormatHelper::getChannelCount(format) * count) + 7) >> 3;
		}
	}

	sl_uint32 AudioData::getChannelBuffers(AudioChannelBuffer* buffers) const
	{
		sl_uint32 nChannels = AudioFormatHelper::getChannelCount(format);
		if (nChannels == 1) {
			if (buffers) {
				buffers[0].count = count;
				buffers[0].stride = AudioFormatHelper::getBytesPerSample(format);
				buffers[0].data = data;
				buffers[0].ref = ref;
			}
			return 1;
		} else if (nChannels == 2) {
			if (buffers) {
				if (AudioFormatHelper::isNonInterleaved(format)) {
					buffers[0].count = count;
					buffers[0].stride = AudioFormatHelper::getBytesPerSample(format);
					buffers[0].data = data;
					buffers[0].ref = ref;
					if (data1) {
						buffers[1].count = count;
						buffers[1].stride = AudioFormatHelper::getBytesPerSample(format);
						buffers[1].data = data1;
						buffers[1].ref = ref1;
					} else {
						buffers[1].count = count;
						buffers[1].stride = AudioFormatHelper::getBytesPerSample(format);
						buffers[1].data = (sl_uint8*)data + getSizeForChannel();
						buffers[1].ref = ref;
					}
				} else {
					buffers[0].count = count;
					buffers[0].stride = AudioFormatHelper::getBytesPerSample(format) * 2;
					buffers[0].data = data;
					buffers[0].ref = ref;

					buffers[1].count = count;
					buffers[1].stride = AudioFormatHelper::getBytesPerSample(format) * 2;
					buffers[1].data = (sl_uint8*)data + AudioFormatHelper::getBytesPerSample(format);
					buffers[1].ref = ref;
				}
			}
			return 2;
		}
		return 0;
	}

	namespace {

		class AUDIO_INT8_PROC
		{
		public:
			SLIB_INLINE static sl_uint8 readSample(sl_uint8*& p)
			{
				sl_uint8 v = *p;
				p++;
				return v;
			}

			SLIB_INLINE static void writeSample(sl_uint8*& p, sl_uint8 v)
			{
				*p = v;
				p++;
			}
		};

		class AUDIO_INT16LE_PROC
		{
		public:
			SLIB_INLINE static sl_uint16 readSample(sl_uint8*& p)
			{
				sl_uint16 v = p[0] | (((sl_uint16)(p[1])) << 8);
				p += 2;
				return v;
			}

			SLIB_INLINE static void writeSample(sl_uint8*& p, sl_uint16 v)
			{
				p[0] = (sl_uint8)(v);
				p[1] = (sl_uint8)(v >> 8);
				p += 2;
			}
		};

		class AUDIO_INT16BE_PROC
		{
		public:
			SLIB_INLINE static sl_uint16 readSample(sl_uint8*& p)
			{
				sl_uint16 v = p[1] | (((sl_uint16)(p[0])) << 8);
				p += 2;
				return v;
			}

			SLIB_INLINE static void writeSample(sl_uint8*& p, sl_uint16 v)
			{
				p[1] = (sl_uint8)(v);
				p[0] = (sl_uint8)(v >> 8);
				p += 2;
			}
		};

		class AUDIO_FLOAT_LE_PROC
		{
		public:
			SLIB_INLINE static float readSample(sl_uint8*& p)
			{
				sl_uint32 n = p[0] | (((sl_uint32)(p[1])) << 8) | (((sl_uint32)(p[2])) << 16) | (((sl_uint32)(p[3])) << 24);
				p += 4;
				return *((float*)((void*)(&n)));
			}

			SLIB_INLINE static void writeSample(sl_uint8*& p, float v)
			{
				sl_uint32& n = *((sl_uint32*)((void*)(&v)));
				p[0] = (sl_uint8)(n);
				p[1] = (sl_uint8)(n >> 8);
				p[2] = (sl_uint8)(n >> 16);
				p[3] = (sl_uint8)(n >> 24);
				p += 4;
			}
		};

		class AUDIO_FLOAT_BE_PROC
		{
		public:
			SLIB_INLINE static float readSample(sl_uint8*& p)
			{
				sl_uint32 n = p[3] | (((sl_uint32)(p[2])) << 8) | (((sl_uint32)(p[1])) << 16) | (((sl_uint32)(p[0])) << 24);
				p += 4;
				return *((float*)((void*)(&n)));
			}

			SLIB_INLINE static void writeSample(sl_uint8*& p, float v)
			{
				sl_uint32& n = *((sl_uint32*)((void*)(&v)));
				p[3] = (sl_uint8)(n);
				p[2] = (sl_uint8)(n >> 8);
				p[1] = (sl_uint8)(n >> 16);
				p[0] = (sl_uint8)(n >> 24);
				p += 4;
			}
		};

		template <class IN_PROC, class IN_TYPE, class OUT_PROC, class OUT_TYPE>
		static void CopySamples_Step2(sl_size count, AudioFormat format_in, sl_uint8* data_in, sl_uint8* data_in1, AudioFormat format_out, sl_uint8* data_out, sl_uint8* data_out1)
		{
			IN_TYPE _in;
			IN_TYPE _in1;
			OUT_TYPE _out;
			OUT_TYPE _out1;
			sl_uint32 nChannels_in = AudioFormatHelper::getChannelCount(format_in);
			sl_uint32 nChannels_out = AudioFormatHelper::getChannelCount(format_out);
			if (nChannels_in == 1) {
				if (nChannels_out == 1) {
					for (sl_size i = 0; i < count; i++) {
						_in = IN_PROC::readSample(data_in);
						AudioUtil::convertSample(_in, _out);
						OUT_PROC::writeSample(data_out, _out);
					}
				} else if (nChannels_out == 2) {
					if (AudioFormatHelper::isNonInterleaved(format_out)) {
						for (sl_size i = 0; i < count; i++) {
							_in = IN_PROC::readSample(data_in);
							AudioUtil::convertSample(_in, _out);
							OUT_PROC::writeSample(data_out, _out);
							OUT_PROC::writeSample(data_out1, _out);
						}
					} else {
						for (sl_size i = 0; i < count; i++) {
							_in = IN_PROC::readSample(data_in);
							AudioUtil::convertSample(_in, _out);
							OUT_PROC::writeSample(data_out, _out);
							OUT_PROC::writeSample(data_out, _out);
						}
					}
				}
			} else if (nChannels_in == 2) {
				if (AudioFormatHelper::isNonInterleaved(format_in)) {
					if (nChannels_out == 1) {
						for (sl_size i = 0; i < count; i++) {
							_in = IN_PROC::readSample(data_in);
							_in1 = IN_PROC::readSample(data_in1);
							AudioUtil::mixSamples(_in, _in1, _in);
							AudioUtil::convertSample(_in, _out);
							OUT_PROC::writeSample(data_out, _out);
						}
					} else if (nChannels_out == 2) {
						if (AudioFormatHelper::isNonInterleaved(format_out)) {
							for (sl_size i = 0; i < count; i++) {
								_in = IN_PROC::readSample(data_in);
								AudioUtil::convertSample(_in, _out);
								_in1 = IN_PROC::readSample(data_in1);
								AudioUtil::convertSample(_in1, _out1);
								OUT_PROC::writeSample(data_out, _out);
								OUT_PROC::writeSample(data_out1, _out1);
							}
						} else {
							for (sl_size i = 0; i < count; i++) {
								_in = IN_PROC::readSample(data_in);
								AudioUtil::convertSample(_in, _out);
								_in1 = IN_PROC::readSample(data_in1);
								AudioUtil::convertSample(_in1, _out1);
								OUT_PROC::writeSample(data_out, _out);
								OUT_PROC::writeSample(data_out, _out1);
							}
						}
					}
				} else {
					if (nChannels_out == 1) {
						for (sl_size i = 0; i < count; i++) {
							_in = IN_PROC::readSample(data_in);
							_in1 = IN_PROC::readSample(data_in);
							AudioUtil::mixSamples(_in, _in1, _in);
							AudioUtil::convertSample(_in, _out);
							OUT_PROC::writeSample(data_out, _out);
						}
					} else if (nChannels_out == 2) {
						if (AudioFormatHelper::isNonInterleaved(format_out)) {
							for (sl_size i = 0; i < count; i++) {
								_in = IN_PROC::readSample(data_in);
								AudioUtil::convertSample(_in, _out);
								_in1 = IN_PROC::readSample(data_in);
								AudioUtil::convertSample(_in1, _out1);
								OUT_PROC::writeSample(data_out, _out);
								OUT_PROC::writeSample(data_out1, _out1);
							}
						} else {
							for (sl_size i = 0; i < count; i++) {
								_in = IN_PROC::readSample(data_in);
								AudioUtil::convertSample(_in, _out);
								_in1 = IN_PROC::readSample(data_in);
								AudioUtil::convertSample(_in1, _out1);
								OUT_PROC::writeSample(data_out, _out);
								OUT_PROC::writeSample(data_out, _out1);
							}
						}
					}
				}
			}
		}


		template<class IN_PROC, class IN_TYPE>
		static void CopySamples_Step1(sl_size count, AudioFormat format_in, sl_uint8* data_in, sl_uint8* data_in1, AudioFormat format_out, sl_uint8* data_out, sl_uint8* data_out1)
		{
			switch (AudioFormatHelper::getSampleType(format_out)) {
				case AudioSampleType::Int8:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT8_PROC, sl_int8>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Uint8:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT8_PROC, sl_uint8>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Int16:
					if (Endian::isLE()) {
						CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16LE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					} else {
						CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16BE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					}
					break;
				case AudioSampleType::Int16LE:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16LE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Int16BE:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16BE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Uint16:
					if (Endian::isLE()) {
						CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16LE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					} else {
						CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16BE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					}
					break;
				case AudioSampleType::Uint16LE:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16LE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Uint16BE:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_INT16BE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Float:
					if (Endian::isLE()) {
						CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_FLOAT_LE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					} else {
						CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_FLOAT_BE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					}
					break;
				case AudioSampleType::FloatLE:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_FLOAT_LE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::FloatBE:
					CopySamples_Step2<IN_PROC, IN_TYPE, AUDIO_FLOAT_BE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
			}
		}

		static void CopySamples(sl_size count, AudioFormat format_in, sl_uint8* data_in, sl_uint8* data_in1, AudioFormat format_out, sl_uint8* data_out, sl_uint8* data_out1)
		{
			switch (AudioFormatHelper::getSampleType(format_in)) {
				case AudioSampleType::Int8:
					CopySamples_Step1<AUDIO_INT8_PROC, sl_int8>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Uint8:
					CopySamples_Step1<AUDIO_INT8_PROC, sl_uint8>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Int16:
					if (Endian::isLE()) {
						CopySamples_Step1<AUDIO_INT16LE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					} else {
						CopySamples_Step1<AUDIO_INT16BE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					}
					break;
				case AudioSampleType::Int16LE:
					CopySamples_Step1<AUDIO_INT16LE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Int16BE:
					CopySamples_Step1<AUDIO_INT16BE_PROC, sl_int16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Uint16:
					if (Endian::isLE()) {
						CopySamples_Step1<AUDIO_INT16LE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					} else {
						CopySamples_Step1<AUDIO_INT16BE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					}
					break;
				case AudioSampleType::Uint16LE:
					CopySamples_Step1<AUDIO_INT16LE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Uint16BE:
					CopySamples_Step1<AUDIO_INT16BE_PROC, sl_uint16>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::Float:
					if (Endian::isLE()) {
						CopySamples_Step1<AUDIO_FLOAT_LE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					} else {
						CopySamples_Step1<AUDIO_FLOAT_BE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					}
					break;
				case AudioSampleType::FloatLE:
					CopySamples_Step1<AUDIO_FLOAT_LE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
				case AudioSampleType::FloatBE:
					CopySamples_Step1<AUDIO_FLOAT_BE_PROC, float>(count, format_in, data_in, data_in1, format_out, data_out, data_out1);
					break;
			}
		}

	}

	void AudioData::copySamplesFrom(const AudioData& other, sl_size countSamples) const
	{
		if (format == AudioFormat::None) {
			return;
		}
		if (other.format == AudioFormat::None) {
			return;
		}
		if (countSamples > count) {
			countSamples = count;
		}
		if (countSamples > other.count) {
			countSamples = other.count;
		}
		if (countSamples == 0) {
			return;
		}

		sl_uint8* data_out = (sl_uint8*)data;
		sl_uint8* data_out1 = (sl_uint8*)data1;
		if (AudioFormatHelper::isNonInterleaved(format) && !data_out1) {
			data_out1 = data_out + getSizeForChannel();
		}

		sl_uint8* data_in = (sl_uint8*)(other.data);
		sl_uint8* data_in1 = (sl_uint8*)(other.data1);
		if (AudioFormatHelper::isNonInterleaved(other.format) && !data_in1) {
			data_in1 = data_in + other.getSizeForChannel();
		}

		if (format == other.format) {
			if (AudioFormatHelper::isNonInterleaved(format)) {
				sl_size n = (countSamples * AudioFormatHelper::getBitsPerSample(format) + 7) >> 3;
				Base::copyMemory(data_out, data_in, n);
				Base::copyMemory(data_out1, data_in1, n);
			} else {
				sl_size n = (countSamples * AudioFormatHelper::getBitsPerSample(format) * AudioFormatHelper::getChannelCount(format) + 7) >> 3;
				Base::copyMemory(data_out, data_in, n);
			}
			return;
		}

		CopySamples(countSamples, other.format, data_in, data_in1, format, data_out, data_out1);
	}

	void AudioData::copySamplesFrom(const AudioData& other) const
	{
		copySamplesFrom(other, count);
	}

	sl_int16 AudioData::getSample(sl_uint32 sampleIndex, sl_uint32 channelIndex)
	{
		AudioSampleType type = AudioFormatHelper::getSampleType(format);
		sl_uint32 nChannels = AudioFormatHelper::getChannelCount(format);
		sl_uint32 index = sampleIndex * nChannels + channelIndex;
		switch (type) {
			case AudioSampleType::Int8:
				return ((sl_int8*)data)[index];
			case AudioSampleType::Uint8:
				return ((sl_uint8*)data)[index];
			case AudioSampleType::Int16:
			case AudioSampleType::Uint16:
				return ((sl_uint16*)data)[index];
			case AudioSampleType::Int16LE:
			case AudioSampleType::Uint16LE:
				return Endian::swap16BE(((sl_uint16*)data)[index]);
			case AudioSampleType::Int16BE:
			case AudioSampleType::Uint16BE:
				return Endian::swap16LE(((sl_uint16*)data)[index]);
			case AudioSampleType::Float:
				{
					float f = ((float*)data)[index];
					sl_int16 o;
					AudioUtil::convertSample(f, o);
					return o;
				}
			case AudioSampleType::FloatLE:
				{
					float f = Endian::swapFloatBE(((float*)data)[index]);
					sl_int16 o;
					AudioUtil::convertSample(f, o);
					return o;
				}
			case AudioSampleType::FloatBE:
				{
					float f = Endian::swapFloatLE(((float*)data)[index]);
					sl_int16 o;
					AudioUtil::convertSample(f, o);
					return o;
				}
		}
		return 0;
	}

	sl_int16 AudioData::getPeakSample(sl_uint32 startSampleIndex, sl_uint32 endSampleIndex, sl_bool flagPositive, sl_uint32 channelIndex = 0)
	{
		sl_int16 peakValue = 0;
		for (sl_uint32 index = startSampleIndex; index < endSampleIndex; index++) {
			sl_int16 val = getSample(index, channelIndex);
			if (flagPositive) {
				if (peakValue < val) {
					peakValue = val;
				}
			} else {
				if (val < peakValue) {
					peakValue = val;
				}
			}
		}
		return peakValue;
	}

}
