/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DATA_ZLIB
#define CHECKHEADER_SLIB_DATA_ZLIB

#include "compress.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT ZlibCompressor : public ICompressor
	{
	public:
		ZlibCompressor();

		~ZlibCompressor();

	public:
		sl_bool isStarted();

		// level = 0-9
		sl_bool start(sl_int32 level = 6);

		DataConvertResult pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
			void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed) override;

		DataConvertResult finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed) override;

	protected:
		sl_uint8 m_stream[128]; // bigger than sizeof(z_stream)
		sl_bool m_flagStarted;
	};

	class SLIB_EXPORT ZlibDecompressor : public IDecompressor
	{
	public:
		ZlibDecompressor();

		~ZlibDecompressor();

	public:
		sl_bool isStarted();

		sl_bool start();

		DataConvertResult pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
			void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed) override;

		DataConvertResult finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed) override;

	protected:
		sl_uint8 m_stream[128]; // bigger than sizeof(z_stream)
		sl_bool m_flagStarted;
	};

	class SLIB_EXPORT ZlibRawCompressor : public ZlibCompressor
	{
	public:
		ZlibRawCompressor();

		~ZlibRawCompressor();

	public:
		// level = 0-9
		sl_bool start(sl_uint32 level = 6);

	};

	class SLIB_EXPORT ZlibRawDecompressor : public ZlibDecompressor
	{
	public:
		ZlibRawDecompressor();

		~ZlibRawDecompressor();

	public:
		sl_bool start();

	};

	class SLIB_EXPORT GzipParam
	{
	public:
		StringParam fileName;
		StringParam comment;
		sl_uint32 level;

	public:
		GzipParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(GzipParam)

	};

	class SLIB_EXPORT GzipCompressor : public ZlibCompressor
	{
	public:
		GzipCompressor();

		~GzipCompressor();

	public:
		// level = 0-9
		sl_bool start(const GzipParam& param);

		// level = 0-9
		sl_bool start(sl_uint32 level = 6);

	public:
		sl_uint8 m_gzipHeader[128];
		String m_gzipFileName;
		String m_gzipComment;
	};

	typedef ZlibDecompressor GzipDecompressor;


	class SLIB_EXPORT Zlib
	{
	public:
		static Memory compress(const void* data, sl_size size, sl_uint32 level = 6);

		static Memory compressRaw(const void* data, sl_size size, sl_uint32 level = 6);

		static Memory compressGzip(const GzipParam& param, const void* data, sl_size size);

		static Memory compressGzip(const void* data, sl_size size, sl_uint32 level = 6);


		static Memory decompress(const void* data, sl_size size);

		static Memory decompressRaw(const void* data, sl_size size);

		static Memory decompressGzip(const void* data, sl_size size);

	};

}

#endif
