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

#include "slib/data/lzma.h"

#include "slib/core/memory_buffer.h"

extern "C" {
	#include "lzma/LzmaEnc.h"
	#include "lzma/LzmaDec.h"
}

namespace slib
{

	namespace
	{
		static void* Alloc(void* p, size_t size)
		{
			return Base::createMemory(size);
		}

		static void Free(void* p, void* address)
		{
			Base::freeMemory(address);
		}

		static ISzAlloc g_alloc = { &Alloc, &Free };

		class MemoryInputStream
		{
		public:
			ISeqInStream funcTable;
			sl_uint8* data;
			sl_size size;

		public:
			MemoryInputStream(const void* _data, sl_size _size): data((sl_uint8*)_data), size(_size)
			{
				funcTable.Read = &read;
			}

		public:
			static SRes read(void* _stream, void* data, size_t* size)
			{
				MemoryInputStream* stream = (MemoryInputStream*)_stream;
				size_t curSize = *size;
				if (stream->size < curSize) {
					curSize = stream->size;
				}
				Base::copyMemory(data, stream->data, curSize);
				stream->size -= curSize;
				stream->data += curSize;
				*size = curSize;
				return SZ_OK;
			}
		};

		class MemoryOutputStream
		{
		public:
			ISeqOutStream funcTable;
			MemoryBuffer buffer;

		public:
			MemoryOutputStream()
			{
				funcTable.Write = &write;
			}

		public:
			static size_t write(void* _stream, const void* data, size_t size)
			{
				MemoryOutputStream* stream = (MemoryOutputStream*)_stream;
				if (!size) {
					return 0;
				}
				if (stream->buffer.addNew(data, size)) {
					return size;
				}
				return 0;
			}
		};

	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(LzmaParam)

	LzmaParam::LzmaParam()
	{
		level = 5;
		dictSize = 1 << 24;
		lc = 3;
		lp = 0;
		pb = 2;
		fb = 32;
		flagWriteEndMark = sl_false;
		numThreads = 1;
	}

	Memory Lzma::compress(LzmaParam& param, const void* input, sl_size n)
	{
		CLzmaEncHandle p = LzmaEnc_Create(&g_alloc);
		if (!p) {
			return sl_null;
		}
		Memory ret;
		CLzmaEncProps props;
		LzmaEncProps_Init(&props);
		props.level = (int)(param.level);
		props.dictSize = (UInt32)(param.dictSize);
		props.lc = (int)(param.lc);
		props.lp = (int)(param.lp);
		props.pb = (int)(param.pb);
		props.fb = (int)(param.fb);
		props.writeEndMark = param.flagWriteEndMark ? 1 : 0;
		props.numThreads = (int)(param.numThreads);
		if (LzmaEnc_SetProps(p, &props) == SZ_OK) {
			size_t nProps = 5;
			if (LzmaEnc_WriteProperties(p, param.props, &nProps) == SZ_OK) {
				MemoryInputStream input(input, n);
				MemoryOutputStream output;
				if (LzmaEnc_Encode(p, &(output.funcTable), &(input.funcTable), sl_null, &g_alloc, &g_alloc) == SZ_OK) {
					ret = output.buffer.merge();
				}
			}
		}
		LzmaEnc_Destroy(p, &g_alloc, &g_alloc);
		return ret;
	}


	LzmaDecompressor::LzmaDecompressor()
	{
		m_flagStarted = sl_false;
	}

	LzmaDecompressor::~LzmaDecompressor()
	{
		if (m_flagStarted) {
			LzmaDec_Free((CLzmaDec*)m_decoder, &g_alloc);
		}
	}

	sl_bool LzmaDecompressor::isStarted()
	{
		return m_flagStarted;
	}

	sl_bool LzmaDecompressor::start(const sl_uint8 props[5])
	{
		if (m_flagStarted) {
			return sl_false;
		}
		Base::zeroMemory(m_decoder, sizeof(m_decoder));
		CLzmaDec& decoder = *((CLzmaDec*)m_decoder);
		LzmaDec_Construct(&decoder);
		if (LzmaDec_Allocate(&decoder, (Byte*)props, LZMA_PROPS_SIZE, &g_alloc) != SZ_OK) {
			return sl_false;
		}
		LzmaDec_Init(&decoder);
		m_flagStarted = sl_true;
		return sl_true;
	}

	DataConvertResult LzmaDecompressor::pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed, void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		if (m_flagStarted) {
			CLzmaDec& decoder = *((CLzmaDec*)m_decoder);
			SizeT srcLen = sizeInputAvailable;
			SizeT dstLen = sizeOutputAvailable;
			ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
			if (LzmaDec_DecodeToBuf(&decoder, (Byte*)output, &dstLen, (Byte*)input, &srcLen, LZMA_FINISH_ANY, &status) == SZ_OK) {
				sizeInputPassed = (sl_uint32)srcLen;
				sizeOutputUsed = (sl_uint32)dstLen;
				if (sizeOutputUsed < sizeOutputAvailable) {
					return DataConvertResult::Finished;
				}
				switch (status) {
					case LZMA_STATUS_FINISHED_WITH_MARK:
					case LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK:
						return DataConvertResult::Finished;
					case LZMA_STATUS_NEEDS_MORE_INPUT:
					case LZMA_STATUS_NOT_FINISHED:
						return DataConvertResult::Continue;
					default:
						break;
				}
			}
		}
		sizeInputPassed = 0;
		sizeOutputUsed = 0;
		return DataConvertResult::Error;
	}

	DataConvertResult LzmaDecompressor::finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		if (m_flagStarted) {
			CLzmaDec& decoder = *((CLzmaDec*)m_decoder);
			SizeT srcLen = 0;
			SizeT dstLen = sizeOutputAvailable;
			ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
			if (LzmaDec_DecodeToBuf(&decoder, (Byte*)output, &dstLen, sl_null, &srcLen, LZMA_FINISH_ANY, &status) == SZ_OK) {
				sizeOutputUsed = (sl_uint32)dstLen;
				if (sizeOutputUsed < sizeOutputAvailable) {
					return DataConvertResult::Finished;
				}
				switch (status) {
					case LZMA_STATUS_FINISHED_WITH_MARK:
					case LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK:
						return DataConvertResult::Finished;
					case LZMA_STATUS_NOT_FINISHED:
						return DataConvertResult::Continue;
					default:
						break;
				}
			}
		}
		sizeOutputUsed = 0;
		return DataConvertResult::Error;
	}

	Memory Lzma::decompress(const sl_uint8 props[5], const void* data, sl_size size)
	{
		LzmaDecompressor decoder;
		if (decoder.start(props)) {
			return decoder.passAndFinish(data, size);
		}
		return sl_null;
	}

}
