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

#include "slib/media/codec_opus.h"

#include "slib/core/time_counter.h"
#include "slib/core/log.h"
#include "slib/core/scoped_buffer.h"

#include "opus/opus.h"

//#define OPUS_RESET_INTERVAL 10000

namespace slib
{
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(OpusEncoderParam)

	OpusEncoderParam::OpusEncoderParam()
	{
		samplesPerSecond = 16000;
		channelCount = 1;
		bitsPerSecond = 8000;
		type = OpusEncoderType::Voice;
	}


	SLIB_DEFINE_OBJECT(OpusEncoder, AudioEncoder)

	OpusEncoder::OpusEncoder()
	{
	}

	OpusEncoder::~OpusEncoder()
	{
	}

	sl_bool OpusEncoder::isValidSamplingRate(sl_uint32 nSamplesPerSecond)
	{
		if (nSamplesPerSecond == 8000
			||nSamplesPerSecond == 12000
			|| nSamplesPerSecond == 16000
			|| nSamplesPerSecond == 24000
			|| nSamplesPerSecond == 48000) {
			return sl_true;
		} else {
			return sl_false;
		}
	}

	namespace priv
	{
		namespace opus
		{

			class EncoderImpl : public OpusEncoder
			{
			public:
				sl_size m_sizeEncoder;
				::OpusEncoder* m_encoder;
			
#ifdef OPUS_RESET_INTERVAL
				OpusEncoder* m_encoderBackup;
				TimeCounter m_timeStartReset;
#endif
	
				sl_bool m_flagResetBitrate;
				
			public:
				EncoderImpl()
				{
					m_encoder = sl_null;
					m_sizeEncoder = 0;
					m_flagResetBitrate = sl_false;
				}

				~EncoderImpl()
				{
					Base::freeMemory(m_encoder);
#ifdef OPUS_RESET_INTERVAL
					Base::freeMemory(m_encoderBackup);
#endif
				}

			public:
				static void logError(String str)
				{
					LogError("AudioOpusEncoder", str);
				}

				static Ref<EncoderImpl> create(const OpusEncoderParam& param)
				{
					if (!isValidSamplingRate(param.samplesPerSecond)) {
						logError("Encoding sampling rate must be one of 8000, 12000, 16000, 24000, 48000");
						return sl_null;
					}
					if (param.channelCount != 1 && param.channelCount != 2) {
						logError("Encoding channel must be 1 or 2");
						return sl_null;
					}
					
					int sizeEncoder = opus_encoder_get_size((opus_int32)(param.channelCount));
					if (sizeEncoder <= 0) {
						return sl_null;
					}
					
					::OpusEncoder* encoder = (::OpusEncoder*)(Base::createMemory(sizeEncoder));
					if (encoder) {
#ifdef OPUS_RESET_INTERVAL
						::OpusEncoder* encoderBackup = (::OpusEncoder*)(Base::createMemory(sizeEncoder));
						if (encoderBackup) {
#endif
							int app = OPUS_APPLICATION_VOIP;
							if (param.type != OpusEncoderType::Voice) {
								app = OPUS_APPLICATION_AUDIO;
							}
							int error = opus_encoder_init(encoder, (opus_int32)(param.samplesPerSecond), (opus_int32)(param.channelCount), app);
							if (error == OPUS_OK) {
								
								if (param.type == OpusEncoderType::Voice) {
									opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
								} else if (param.type == OpusEncoderType::Music) {
									opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
								} else {
									opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_AUTO));
								}
								
								Ref<EncoderImpl> ret = new EncoderImpl();

								if (ret.isNotNull()) {
									
									ret->m_sizeEncoder = sizeEncoder;
									ret->m_encoder = encoder;
							
#ifdef OPUS_RESET_INTERVAL
									ret->m_encoderBackup = encoderBackup;
									Base::copyMemory(encoderBackup, encoder, sizeEncoder);
									ret->m_timeStartReset.reset();
#endif
							
									ret->m_nSamplesPerSecond = param.samplesPerSecond;
									ret->m_nChannels = param.channelCount;
									
									ret->setBitrate(param.bitsPerSecond);
									
									return ret;
								}
							}
#ifdef OPUS_RESET_INTERVAL
							Base::freeMemory(encoderBackup);
						}
#endif
						Base::freeMemory(encoder);
					}
					return sl_null;
				}
				
				Memory encode(const AudioData& input) override
				{
					sl_uint32 lenMinFrame = m_nSamplesPerSecond / 400; // 2.5 ms
					if (input.count % lenMinFrame == 0) {
						sl_uint32 nMinFrame = (sl_uint32)(input.count / lenMinFrame);
						// one frame is (2.5, 5, 10, 20, 40 or 60 ms) of audio data
						if (nMinFrame == 1 || nMinFrame == 2 || nMinFrame == 4 || nMinFrame == 8 || nMinFrame == 16 || nMinFrame == 24) {
							
							AudioData audio;
							audio.count = input.count;
							
							sl_bool flagFloat = AudioFormatHelper::isFloat(input.format);
							
							if (flagFloat) {
								if (m_nChannels == 2) {
									audio.format = AudioFormat::Float_Stereo;
								} else {
									audio.format = AudioFormat::Float_Mono;
								}
							} else {
								if (m_nChannels == 2) {
									audio.format = AudioFormat::Int16_Stereo;
								} else {
									audio.format = AudioFormat::Int16_Mono;
								}
							}
							
							if (audio.format == input.format) {
								if (flagFloat) {
									if (((sl_size)(input.data) & 3) == 0) {
										audio.data = input.data;
									}
								} else {
									if (((sl_size)(input.data) & 1) == 0) {
										audio.data = input.data;
									}
								}
							}
							
							/*
								maximum sampling rate is 48000
								maximum frame length is 48000/400*24=2880 bytes
								maximum frame size is 2880*2*4=23040 bytes (5760 dwords)
							*/
							sl_uint32 _samples[5760];
							if (!(audio.data)) {
								audio.data = _samples;
								audio.copySamplesFrom(input);
							}
							
							ObjectLocker lock(this);
							if (m_flagResetBitrate) {
								sl_uint32 bitrate = getBitrate();
								opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(bitrate));
								m_flagResetBitrate = sl_false;
							}
#ifdef OPUS_RESET_INTERVAL
							if (m_timeStartReset.getElapsedMilliseconds() > OPUS_RESET_INTERVAL || ret <= 0) {
								Base::copyMemory(m_encoder, m_encoderBackup, m_sizeEncoder);
								m_timeStartReset.reset();
							}
#endif

							sl_uint8 output[4000]; // opus recommends 4000 bytes for output buffer
							int ret;
							if (flagFloat) {
								ret = opus_encode_float(m_encoder, (float*)(audio.data), (int)(audio.count), (unsigned char*)output, sizeof(output));
							} else {
								ret = opus_encode(m_encoder, (opus_int16*)(audio.data), (int)(audio.count), (unsigned char*)output, sizeof(output));
							}
							if (ret > 0) {
								return Memory::create(output, ret);
							}
						}
					}
					return sl_null;
				}

				void setBitrate(sl_uint32 _bitrate) override
				{
					sl_uint32 bitrate = _bitrate;
					if (bitrate < 500) {
						bitrate = 500;
					}
					if (bitrate > 512000) {
						bitrate = 512000;
					}
					m_flagResetBitrate = sl_true;
					AudioEncoder::setBitrate(bitrate);
				}
		
			};

		}
	}

	Ref<OpusEncoder> OpusEncoder::create(const OpusEncoderParam& param)
	{
		return priv::opus::EncoderImpl::create(param);
	}

	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(OpusDecoderParam)

	OpusDecoderParam::OpusDecoderParam()
	{
		samplesPerSecond = 16000;
		channelCount = 1;
	}


	SLIB_DEFINE_OBJECT(OpusDecoder, AudioDecoder)

	OpusDecoder::OpusDecoder()
	{
	}

	OpusDecoder::~OpusDecoder()
	{
	}

	sl_bool OpusDecoder::isValidSamplingRate(sl_uint32 nSamplesPerSecond)
	{
		return OpusEncoder::isValidSamplingRate(nSamplesPerSecond);
	}

	namespace priv
	{
		namespace opus
		{

			class DecoderImpl : public OpusDecoder
			{
			public:
				::OpusDecoder* m_decoder;

			public:
				DecoderImpl()
				{
					m_decoder = sl_null;
				}
				
				~DecoderImpl()
				{
					opus_decoder_destroy(m_decoder);
				}

			public:
				static void logError(String str)
				{
					LogError("AudioOpusDecoder", str);
				}

				static Ref<DecoderImpl> create(const OpusDecoderParam& param)
				{
					if (!isValidSamplingRate(param.samplesPerSecond)) {
						logError("Decoding sampling rate must be one of 8000, 12000, 16000, 24000, 48000");
						return sl_null;
					}
					if (param.channelCount != 1 && param.channelCount != 2) {
						logError("Decoding channel must be 1 or 2");
						return sl_null;
					}
					int error;
					::OpusDecoder* decoder = opus_decoder_create((opus_int32)(param.samplesPerSecond), (opus_int32)(param.channelCount), &error);
					if (! decoder) {
						return sl_null;
					}
					Ref<DecoderImpl> ret = new DecoderImpl();
					if (ret.isNotNull()) {
						ret->m_decoder = decoder;
						ret->m_nSamplesPerSecond = param.samplesPerSecond;
						ret->m_nChannels = param.channelCount;
					} else {
						opus_decoder_destroy(decoder);
					}
					return ret;
				}

				sl_uint32 decode(const void* input, sl_uint32 sizeInput, const AudioData& output) override
				{
					AudioData audio;
					audio.count = output.count;
					
					sl_bool flagFloat = AudioFormatHelper::isFloat(output.format);
					
					if (flagFloat) {
						if (m_nChannels == 2) {
							audio.format = AudioFormat::Float_Stereo;
						} else {
							audio.format = AudioFormat::Float_Mono;
						}
					} else {
						if (m_nChannels == 2) {
							audio.format = AudioFormat::Int16_Stereo;
						} else {
							audio.format = AudioFormat::Int16_Mono;
						}
					}
					
					if (audio.format == output.format) {
						if (flagFloat) {
							if (((sl_size)(output.data) & 3) == 0) {
								audio.data = output.data;
							}
						} else {
							if (((sl_size)(output.data) & 1) == 0) {
								audio.data = output.data;
							}
						}
					}
					
					SLIB_SCOPED_BUFFER(sl_uint8, 23040, _samples, audio.data ? 0 : audio.count);
					if (!(audio.data)) {
						audio.data = _samples;
					}
					int ret = 0;
					{
						ObjectLocker lock(this);
						if (flagFloat) {
							ret = opus_decode_float(m_decoder, (unsigned char*)input, (int)sizeInput, (float*)(audio.data), (int)(audio.count), 0);
						} else {
							ret = opus_decode(m_decoder, (unsigned char*)input, (int)sizeInput, (opus_int16*)(audio.data), (int)(audio.count), 0);
						}
					}
					if (ret > 0) {
						if (audio.data != output.data) {
							output.copySamplesFrom(audio, ret);
						}
						return ret;
					}
					return 0;
				}
			};

		}
	}

	Ref<OpusDecoder> OpusDecoder::create(const OpusDecoderParam& param)
	{
		return priv::opus::DecoderImpl::create(param);
	}

}

