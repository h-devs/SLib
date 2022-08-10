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

#include "slib/crypto/zlib.h"
#include "slib/crypto/crc32.h"

#include "slib/core/memory.h"

#include "zlib/zlib.h"

#undef compress

#define STREAM ((z_stream*)(m_stream))
#define GZIP_HEADER ((gz_header*)(m_gzipHeader))

namespace slib
{

	ZlibCompressor::ZlibCompressor()
	{
		m_flagStarted = sl_false;
	}

	ZlibCompressor::~ZlibCompressor()
	{
		if (m_flagStarted) {
			deflateEnd(STREAM);
		}
	}

	sl_bool ZlibCompressor::isStarted()
	{
		return m_flagStarted;
	}

	sl_bool ZlibCompressor::start(sl_int32 level)
	{
		if (m_flagStarted) {
			return sl_false;
		}
		Base::zeroMemory(STREAM, sizeof(z_stream));
		int iRet = deflateInit2(STREAM, level, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY);
		if (iRet == Z_OK) {
			m_flagStarted = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	DataFilterResult ZlibCompressor::pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
		void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		if (m_flagStarted) {
			z_stream* stream = STREAM;
			stream->next_in = (Bytef*)input;
			stream->avail_in = sizeInputAvailable;
			stream->next_out = (Bytef*)output;
			stream->avail_out = sizeOutputAvailable;
			int iRet = deflate(stream, Z_NO_FLUSH);
			if (iRet >= 0) {
				sizeInputPassed = sizeInputAvailable - stream->avail_in;
				sizeOutputUsed = sizeOutputAvailable - stream->avail_out;
				return DataFilterResult::Continue;
			}
		}
		sizeInputPassed = 0;
		sizeOutputUsed = 0;
		return DataFilterResult::Error;
	}

	DataFilterResult ZlibCompressor::finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		if (m_flagStarted) {
			z_stream* stream = STREAM;
			stream->next_in = sl_null;
			stream->avail_in = 0;
			stream->next_out = (Bytef*)output;
			stream->avail_out = sizeOutputAvailable;
			int iRet = deflate(stream, Z_FINISH);
			if (iRet >= 0) {
				sizeOutputUsed = sizeOutputAvailable - stream->avail_out;
				if (iRet == Z_STREAM_END) {
					return DataFilterResult::Finished;
				} else {
					return DataFilterResult::Continue;
				}
			}
		}
		sizeOutputUsed = 0;
		return DataFilterResult::Error;
	}


	ZlibDecompressor::ZlibDecompressor()
	{
		m_flagStarted = sl_false;
	}

	ZlibDecompressor::~ZlibDecompressor()
	{
		if (m_flagStarted) {
			inflateEnd(STREAM);
		}
	}

	sl_bool ZlibDecompressor::isStarted()
	{
		return m_flagStarted;
	}

	sl_bool ZlibDecompressor::start()
	{
		if (m_flagStarted) {
			return sl_false;
		}
		Base::zeroMemory(STREAM, sizeof(z_stream));
		// Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection
		int iRet = inflateInit2(STREAM, 15 + 32);
		if (iRet == Z_OK) {
			m_flagStarted = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	DataFilterResult ZlibDecompressor::pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
		void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		if (m_flagStarted) {
			z_stream* stream = STREAM;
			stream->next_in = (Bytef*)input;
			stream->avail_in = sizeInputAvailable;
			stream->next_out = (Bytef*)output;
			stream->avail_out = sizeOutputAvailable;
			int iRet = inflate(stream, Z_NO_FLUSH);
			if (iRet >= 0 && iRet != Z_NEED_DICT) {
				sizeInputPassed = sizeInputAvailable - stream->avail_in;
				sizeOutputUsed = sizeOutputAvailable - stream->avail_out;
				if (iRet == Z_STREAM_END) {
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

	DataFilterResult ZlibDecompressor::finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		if (m_flagStarted) {
			z_stream* stream = STREAM;
			stream->next_in = sl_null;
			stream->avail_in = 0;
			stream->next_out = (Bytef*)output;
			stream->avail_out = sizeOutputAvailable;
			int iRet = inflate(stream, Z_FINISH);
			if (iRet >= 0) {
				sizeOutputUsed = sizeOutputAvailable - stream->avail_out;
				if (iRet == Z_STREAM_END) {
					return DataFilterResult::Finished;
				} else {
					if (sizeOutputUsed) {
						return DataFilterResult::Continue;
					}
				}
			}
		}
		sizeOutputUsed = 0;
		return DataFilterResult::Error;
	}


	ZlibRawCompressor::ZlibRawCompressor()
	{
	}

	ZlibRawCompressor::~ZlibRawCompressor()
	{
	}

	sl_bool ZlibRawCompressor::start(sl_uint32 level)
	{
		if (m_flagStarted) {
			return sl_false;
		}
		Base::zeroMemory(STREAM, sizeof(z_stream));
		int iRet = deflateInit2(STREAM, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
		if (iRet == Z_OK) {
			m_flagStarted = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	ZlibRawDecompressor::ZlibRawDecompressor()
	{
	}

	ZlibRawDecompressor::~ZlibRawDecompressor()
	{
	}

	sl_bool ZlibRawDecompressor::start()
	{
		if (m_flagStarted) {
			return sl_false;
		}
		Base::zeroMemory(STREAM, sizeof(z_stream));
		int iRet = inflateInit2(STREAM, -15);
		if (iRet == Z_OK) {
			m_flagStarted = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(GzipParam)

	GzipParam::GzipParam()
	{
		level = 6;
	}
	

	GzipCompressor::GzipCompressor()
	{
	}

	GzipCompressor::~GzipCompressor()
	{
	}

	sl_bool GzipCompressor::start(const GzipParam& param)
	{
		if (m_flagStarted) {
			return sl_false;
		}
		Base::zeroMemory(STREAM, sizeof(z_stream));
		int iRet = deflateInit2(STREAM, param.level, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
		if (iRet == Z_OK) {
			Base::zeroMemory(GZIP_HEADER, sizeof(gz_header));
			m_gzipFileName = param.fileName.toString().toNullTerminated();
			GZIP_HEADER->name = (Bytef*)(m_gzipFileName.getData());
			m_gzipComment = param.comment.toString().toNullTerminated();
			GZIP_HEADER->comment = (Bytef*)(m_gzipComment.getData());
			GZIP_HEADER->os = 255;
			iRet = deflateSetHeader(STREAM, GZIP_HEADER);
			if (iRet == Z_OK) {
				m_flagStarted = sl_true;
				return sl_true;
			}
			deflateEnd(STREAM);
		}
		return sl_false;
	}

	sl_bool GzipCompressor::start(sl_uint32 level)
	{
		GzipParam param;
		param.level = level;
		return start(param);
	}


	Memory Zlib::compress(const void* data, sl_size size, sl_uint32 level)
	{
		ZlibCompressor zlib;
		if (zlib.start(level)) {
			return zlib.passAndFinish(data, size);
		}
		return sl_null;
	}

	Memory Zlib::compressRaw(const void* data, sl_size size, sl_uint32 level)
	{
		ZlibRawCompressor zlib;
		if (zlib.start(level)) {
			return zlib.passAndFinish(data, size);
		}
		return sl_null;
	}

	Memory Zlib::compressGzip(const GzipParam& param, const void* data, sl_size size)
	{
		GzipCompressor zlib;
		if (zlib.start(param)) {
			return zlib.passAndFinish(data, size);
		}
		return sl_null;
	}

	Memory Zlib::compressGzip(const void* data, sl_size size, sl_uint32 level)
	{
		GzipParam param;
		param.level = level;
		return compressGzip(param, data, size);
	}

	Memory Zlib::decompress(const void* data, sl_size size)
	{
		ZlibDecompressor zlib;
		if (zlib.start()) {
			return zlib.passAndFinish(data, size);
		}
		return sl_null;
	}

	Memory Zlib::decompressRaw(const void* data, sl_size size)
	{
		ZlibRawDecompressor zlib;
		if (zlib.start()) {
			return zlib.passAndFinish(data, size);
		}
		return sl_null;
	}

	Memory Zlib::decompressGzip(const void* data, sl_size size)
	{
		return decompress(data, size);
	}


	sl_uint32 Crc32::extend(sl_uint32 crc, const void* _data, sl_size size)
	{
		const char* data = (const char*)_data;
		while (size > 0) {
			sl_uint32 n = 0x10000000;
			if (size < n) {
				n = (sl_uint32)size;
			}
			crc = (sl_uint32)(::z_crc32(crc, (Bytef*)data, n));
			size -= n;
			data += n;
		}
		return crc;
	}

	sl_uint32 Crc32::get(const void* data, sl_size size)
	{
		return extend(0, data, size);
	}

	sl_uint32 Crc32::extend(sl_uint32 crc, const MemoryView& mem)
	{
		return extend(crc, mem.data, mem.size);
	}

	sl_uint32 Crc32::get(const MemoryView& mem)
	{
		return extend(0, mem.data, mem.size);
	}

}
