/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/crypto/brotli.h"

#include "slib/core/memory.h"

#include "brotli/encode.h"
#include "brotli/decode.h"

#define CSTREAM ((BrotliEncoderState*)(m_stream))
#define DSTREAM ((BrotliDecoderState*)(m_stream))

namespace slib
{

	BrotliCompressor::BrotliCompressor()
	{
		m_stream = sl_null;
	}

	BrotliCompressor::~BrotliCompressor()
	{
		if (m_stream) {
			BrotliEncoderDestroyInstance(CSTREAM);
		}
	}

	sl_bool BrotliCompressor::isStarted()
	{
		return m_stream != sl_null;
	}

	sl_bool BrotliCompressor::start(sl_int32 level, sl_bool flagText)
	{
		if (m_stream) {
			return sl_false;
		}
		m_stream = BrotliEncoderCreateInstance(sl_null, sl_null, sl_null);
		if (!m_stream) {
			return sl_false;
		}
		if (level < 0) {
			level = 0;
		}
		BrotliEncoderSetParameter(CSTREAM, BROTLI_PARAM_QUALITY, level);
		if (flagText) {
			BrotliEncoderSetParameter(CSTREAM, BROTLI_PARAM_MODE, BROTLI_MODE_TEXT);
		}
		return sl_true;
	}

	DataFilterResult BrotliCompressor::pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
		void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			const uint8_t* nextIn = (uint8_t*)input;
			size_t sizeIn = (size_t)sizeInputAvailable;
			uint8_t* nextOut = (uint8_t*)output;
			size_t sizeOut = (size_t)sizeOutputAvailable;
			if (BrotliEncoderCompressStream(CSTREAM, BrotliEncoderOperation::BROTLI_OPERATION_PROCESS, &sizeIn, &nextIn, &sizeOut, &nextOut, sl_null)) {
				sizeInputPassed = (sl_size)(sizeInputAvailable - sizeIn);
				sizeOutputUsed = (sl_size)(sizeOutputAvailable - sizeOut);
				return DataFilterResult::Continue;
			}
		}
		sizeInputPassed = 0;
		sizeOutputUsed = 0;
		return DataFilterResult::Error;
	}

	DataFilterResult BrotliCompressor::finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			const uint8_t* nextIn = sl_null;
			size_t sizeIn = 0;
			uint8_t* nextOut = (uint8_t*)output;
			size_t sizeOut = (size_t)sizeOutputAvailable;
			if (BrotliEncoderCompressStream(CSTREAM, BrotliEncoderOperation::BROTLI_OPERATION_FINISH, &sizeIn, &nextIn, &sizeOut, &nextOut, sl_null)) {
				sizeOutputUsed = (sl_size)(sizeOutputAvailable - sizeOut);
				if (BrotliEncoderIsFinished(CSTREAM)) {
					return DataFilterResult::Finished;
				} else {
					return DataFilterResult::Continue;
				}
			}
		}
		sizeOutputUsed = 0;
		return DataFilterResult::Error;
	}


	BrotliDecompressor::BrotliDecompressor()
	{
		m_stream = sl_null;
	}

	BrotliDecompressor::~BrotliDecompressor()
	{
		if (m_stream) {
			BrotliDecoderDestroyInstance(DSTREAM);
		}
	}

	sl_bool BrotliDecompressor::isStarted()
	{
		return m_stream != sl_null;
	}

	sl_bool BrotliDecompressor::start()
	{
		if (m_stream) {
			return sl_false;
		}
		m_stream = BrotliDecoderCreateInstance(sl_null, sl_null, sl_null);
		return m_stream != sl_null;
	}

	DataFilterResult BrotliDecompressor::pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
		void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			const uint8_t* nextIn = (uint8_t*)input;
			size_t sizeIn = (size_t)sizeInputAvailable;
			uint8_t* nextOut = (uint8_t*)output;
			size_t sizeOut = (size_t)sizeOutputAvailable;
			BrotliDecoderResult result = BrotliDecoderDecompressStream(DSTREAM, &sizeIn, &nextIn, &sizeOut, &nextOut, sl_null);
			if (result != BROTLI_DECODER_RESULT_ERROR) {
				sizeInputPassed = (sl_size)(sizeInputAvailable - sizeIn);
				sizeOutputUsed = (sl_size)(sizeOutputAvailable - sizeOut);
				if (result == BROTLI_DECODER_RESULT_SUCCESS) {
					return DataFilterResult::Finished;
				} else {
					return DataFilterResult::Continue;
				}
			}
		}
		sizeInputPassed = 0;
		sizeOutputUsed = 0;
		return DataFilterResult::Error;
	}

	DataFilterResult BrotliDecompressor::finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			const uint8_t* nextIn = sl_null;
			size_t sizeIn = 0;
			uint8_t* nextOut = (uint8_t*)output;
			size_t sizeOut = (size_t)sizeOutputAvailable;
			BrotliDecoderResult result = BrotliDecoderDecompressStream(DSTREAM, &sizeIn, &nextIn, &sizeOut, &nextOut, sl_null);
			if (result != BROTLI_DECODER_RESULT_ERROR) {
				sizeOutputUsed = (sl_size)(sizeOutputAvailable - sizeOut);
				if (result == BROTLI_DECODER_RESULT_SUCCESS) {
					return DataFilterResult::Finished;
				} else if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
					return DataFilterResult::Continue;
				}
			}
		}
		sizeOutputUsed = 0;
		return DataFilterResult::Error;
	}


	Memory Brotli::compress(const void* data, sl_size size, sl_int32 level, sl_bool flagText)
	{
		BrotliCompressor compressor;
		if (compressor.start(level, flagText)) {
			return compressor.passAndFinish(data, size);
		}
		return sl_null;
	}

	Memory Brotli::decompress(const void* data, sl_size size)
	{
		BrotliDecompressor decompressor;
		if (decompressor.start()) {
			return decompressor.passAndFinish(data, size);
		}
		return sl_null;
	}


}
