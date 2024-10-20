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

#include "slib/data/zstd.h"

#include "slib/core/memory.h"

#include "zstd/zstd.h"

#define CSTREAM ((ZSTD_CCtx*)(m_stream))
#define DSTREAM ((ZSTD_DCtx*)(m_stream))

namespace slib
{

	ZstdCompressor::ZstdCompressor()
	{
		m_stream = sl_null;
	}

	ZstdCompressor::~ZstdCompressor()
	{
		if (m_stream) {
			ZSTD_freeCCtx(CSTREAM);
		}
	}

	sl_bool ZstdCompressor::isStarted()
	{
		return m_stream != sl_null;
	}

	sl_bool ZstdCompressor::start(sl_int32 level)
	{
		if (m_stream) {
			return sl_false;
		}
		m_stream = ZSTD_createCCtx();
		if (!m_stream) {
			return sl_false;
		}
		ZSTD_CCtx_setParameter(CSTREAM, ZSTD_c_compressionLevel, (int)level);
		return sl_true;
	}

	DataConvertResult ZstdCompressor::pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
		void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			ZSTD_inBuffer in;
			in.src = input;
			in.pos = 0;
			in.size = (size_t)sizeInputAvailable;
			ZSTD_outBuffer out;
			out.dst = output;
			out.pos = 0;
			out.size = (size_t)sizeOutputAvailable;
			size_t ret = ZSTD_compressStream2(CSTREAM, &out, &in, ZSTD_e_continue);
			if (!(ZSTD_isError(ret))) {
				sizeInputPassed = (sl_size)(in.pos);
				sizeOutputUsed = (sl_size)(out.pos);
				return DataConvertResult::Continue;
			}
		}
		sizeInputPassed = 0;
		sizeOutputUsed = 0;
		return DataConvertResult::Error;
	}

	DataConvertResult ZstdCompressor::finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			ZSTD_inBuffer in;
			in.src = sl_null;
			in.pos = 0;
			in.size = 0;
			ZSTD_outBuffer out;
			out.dst = output;
			out.pos = 0;
			out.size = (size_t)sizeOutputAvailable;
			size_t ret = ZSTD_compressStream2(CSTREAM, &out, &in, ZSTD_e_end);
			if (!(ZSTD_isError(ret))) {
				sizeOutputUsed = (sl_size)(out.pos);
				if (ret) {
					return DataConvertResult::Continue;
				} else {
					return DataConvertResult::Finished;
				}
			}
		}
		sizeOutputUsed = 0;
		return DataConvertResult::Error;
	}

	sl_size ZstdCompressor::getRecommendedInputSize()
	{
		return (sl_size)(ZSTD_CStreamInSize());
	}

	sl_size ZstdCompressor::getRecommendedOutputSize()
	{
		return (sl_size)(ZSTD_CStreamOutSize());
	}


	ZstdDecompressor::ZstdDecompressor()
	{
		m_stream = sl_null;
	}

	ZstdDecompressor::~ZstdDecompressor()
	{
		if (m_stream) {
			ZSTD_freeDCtx(DSTREAM);
		}
	}

	sl_bool ZstdDecompressor::isStarted()
	{
		return m_stream != sl_null;
	}

	sl_bool ZstdDecompressor::start()
	{
		if (m_stream) {
			return sl_false;
		}
		m_stream = ZSTD_createDCtx();
		return m_stream != sl_null;
	}

	DataConvertResult ZstdDecompressor::pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
		void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			ZSTD_inBuffer in;
			in.src = input;
			in.pos = 0;
			in.size = (size_t)sizeInputAvailable;
			ZSTD_outBuffer out;
			out.dst = output;
			out.pos = 0;
			out.size = (size_t)sizeOutputAvailable;
			size_t ret = ZSTD_decompressStream(DSTREAM, &out, &in);
			if (!(ZSTD_isError(ret))) {
				sizeInputPassed = (sl_size)(in.pos);
				sizeOutputUsed = (sl_size)(out.pos);
				if (ret) {
					return DataConvertResult::Continue;
				} else {
					return DataConvertResult::Finished;
				}
			}
		}
		sizeInputPassed = 0;
		sizeOutputUsed = 0;
		return DataConvertResult::Error;
	}

	DataConvertResult ZstdDecompressor::finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
		if (m_stream) {
			ZSTD_inBuffer in;
			in.src = sl_null;
			in.pos = 0;
			in.size = 0;
			ZSTD_outBuffer out;
			out.dst = output;
			out.pos = 0;
			out.size = (size_t)sizeOutputAvailable;
			size_t ret = ZSTD_decompressStream(DSTREAM, &out, &in);
			if (!(ZSTD_isError(ret))) {
				sizeOutputUsed = (sl_size)(out.pos);
				if (ret) {
					if (sizeOutputUsed) {
						return DataConvertResult::Continue;
					}
				} else {
					return DataConvertResult::Finished;
				}
			}
		}
		sizeOutputUsed = 0;
		return DataConvertResult::Error;
	}

	sl_size ZstdDecompressor::getRecommendedInputSize()
	{
		return (sl_size)(ZSTD_DStreamInSize());
	}

	sl_size ZstdDecompressor::getRecommendedOutputSize()
	{
		return (sl_size)(ZSTD_DStreamOutSize());
	}


	sl_int32 Zstd::getMaximumLevel()
	{
		return (sl_int32)(ZSTD_maxCLevel());
	}

	sl_int32 Zstd::getMinimumLevel()
	{
		return (sl_int32)(ZSTD_minCLevel());
	}

	Memory Zstd::compress(const void* data, sl_size size, sl_int32 level)
	{
		ZstdCompressor compressor;
		if (compressor.start(level)) {
			return compressor.passAndFinish(data, size);
		}
		return sl_null;
	}

	Memory Zstd::decompress(const void* data, sl_size size)
	{
		ZstdDecompressor decompressor;
		if (decompressor.start()) {
			return decompressor.passAndFinish(data, size);
		}
		return sl_null;
	}

}
