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

#include "slib/crypto/compress.h"
#include "slib/crypto/filter_buffer_io.h"

#define DEFAULT_CHUNK_SIZE 0x20000
#define DEFAULT_STACK_SIZE 4096

#define PREPARE_CHUNK(BUF_NAME, SIZE_NAME) \
	char* BUF_NAME; \
	sl_size SIZE_NAME; \
	char stack__##BUF_NAME[DEFAULT_STACK_SIZE]; \
	Memory mem__##BUF_NAME = Memory::create(DEFAULT_CHUNK_SIZE); \
	if (mem__##BUF_NAME.isNotNull()) { \
		BUF_NAME = (char*)(mem__##BUF_NAME.getData()); \
		SIZE_NAME = DEFAULT_CHUNK_SIZE; \
	} else { \
		BUF_NAME = stack__##BUF_NAME; \
		SIZE_NAME = DEFAULT_STACK_SIZE; \
	}

#define PREPARE_FILTER_BUFFER_IO_CHUNK(BUF_NAME, SIZE_NAME) \
	char* BUF_NAME; \
	sl_size SIZE_NAME; \
	char stack__##BUF_NAME[DEFAULT_STACK_SIZE]; \
	if (m_chunkData) { \
		BUF_NAME = m_chunkData; \
		SIZE_NAME = DEFAULT_CHUNK_SIZE; \
	} else { \
		m_chunk = Memory::create(DEFAULT_CHUNK_SIZE); \
		if (m_chunk.isNotNull()) { \
			m_chunkData = (char*)(m_chunk.getData()); \
			BUF_NAME = m_chunkData; \
			SIZE_NAME = DEFAULT_CHUNK_SIZE; \
		} else { \
			BUF_NAME = stack__##BUF_NAME; \
			SIZE_NAME = DEFAULT_STACK_SIZE; \
		} \
	}

namespace slib
{

	namespace priv
	{
		namespace compress
		{

			static sl_bool PrepareMemoryData(MemoryData& mem)
			{
				if (mem.ref.isNull()) {
					mem = Memory::create(mem.data, mem.size);
					return mem.ref.isNotNull();
				} else {
					return sl_true;
				}
			}

			static DataFilterResult Pass(IDataFilter* filter, const void* input, sl_size size, MemoryBuffer& output, void* chunk, sl_size sizeChunk)
			{
				for (;;) {
					sl_size sizeInputPassed;
					sl_size sizeOutputUsed;
					DataFilterResult result = filter->pass(input, size, sizeInputPassed, chunk, sizeChunk, sizeOutputUsed);
					input = (char*)input + sizeInputPassed;
					size -= sizeInputPassed;
					if (sizeOutputUsed) {
						Memory mem = Memory::create(chunk, sizeOutputUsed);
						if (mem.isNull()) {
							return DataFilterResult::Error;
						}
						if (!(output.add(Move(mem)))) {
							return DataFilterResult::Error;
						}
					}
					if (result != DataFilterResult::Continue) {
						return result;
					}
					if (!size) {
						break;
					}
				}
				return DataFilterResult::Continue;
			}

			static DataFilterResult Finish(IDataFilter* filter, MemoryBuffer& output, void* chunk, sl_size sizeChunk)
			{
				for (;;) {
					sl_size sizeOutputUsed;
					DataFilterResult result = filter->finish(chunk, sizeChunk, sizeOutputUsed);
					if (sizeOutputUsed) {
						Memory mem = Memory::create(chunk, sizeOutputUsed);
						if (mem.isNull()) {
							return DataFilterResult::Error;
						}
						if (!(output.add(Move(mem)))) {
							return DataFilterResult::Error;
						}
					}
					if (result != DataFilterResult::Continue) {
						return result;
					}
				}
			}

		}
	}

	using namespace priv::compress;

	DataFilterResult IDataFilter::pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
		void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
#ifdef SLIB_ARCH_IS_64BIT
		sizeInputPassed = 0;
		sizeOutputUsed = 0;
		while (sizeInputAvailable && sizeOutputAvailable) {
			sl_size n1 = sizeInputAvailable;
			if (n1 > 0x40000000) {
				n1 = 0x40000000;
			}
			sl_size n2 = sizeOutputAvailable;
			if (n2 > 0x40000000) {
				n2 = 0x40000000;
			}
			sl_uint32 k1, k2;
			DataFilterResult result = pass32(input, (sl_uint32)n1, k1, output, (sl_uint32)n2, k2);
			sizeInputPassed += k1;
			sizeOutputUsed += k2;
			if (result != DataFilterResult::Continue) {
				return result;
			}
			input = (char*)input + k1;
			sizeInputAvailable -= k1;
			output = (char*)output + k2;
			sizeOutputAvailable -= k2;
		}
		return DataFilterResult::Continue;
#else
		return pass32(input, (sl_uint32)sizeInputAvailable, *((sl_uint32*)&sizeInputPassed), output, (sl_uint32)sizeOutputAvailable, *((sl_uint32*)&sizeOutputAvailable));
#endif
	}

	DataFilterResult IDataFilter::finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
#ifdef SLIB_ARCH_IS_64BIT
		sizeOutputUsed = 0;
		while (sizeOutputAvailable) {
			sl_size n = sizeOutputAvailable;
			if (n > 0x40000000) {
				n = 0x40000000;
			}
			sl_uint32 k;
			DataFilterResult result = finish32(output, (sl_uint32)n, k);
			sizeOutputUsed += k;
			if (result != DataFilterResult::Continue) {
				return result;
			}
			output = (char*)output + k;
			sizeOutputAvailable -= k;
		}
		return DataFilterResult::Continue;
#else
		return finish32(output, (sl_uint32)sizeOutputAvailable, *((sl_uint32*)&sizeOutputUsed));
#endif
	}

	DataFilterResult IDataFilter::pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
		void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		sl_size k1, k2;
		DataFilterResult result = pass(input, sizeInputAvailable, k1, output, sizeOutputAvailable, k2);
		sizeInputPassed = (sl_uint32)k1;
		sizeOutputUsed = (sl_uint32)k2;
		return result;
	}

	DataFilterResult IDataFilter::finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		sl_size k;
		DataFilterResult result = finish(output, sizeOutputAvailable, k);
		sizeOutputUsed = (sl_uint32)k;
		return result;
	}

	DataFilterResult IDataFilter::pass(const void* input, sl_size size, MemoryBuffer& output)
	{
		if (!size) {
			return DataFilterResult::Error;
		}
		PREPARE_CHUNK(chunk, sizeChunk)
		return Pass(this, input, size, output, chunk, sizeChunk);
	}

	Memory IDataFilter::pass(const void* input, sl_size size)
	{
		if (!size) {
			return sl_null;
		}
		MemoryBuffer buf;
		if (pass(input, size, buf) != DataFilterResult::Error) {
			return buf.merge();
		}
		return sl_null;
	}

	DataFilterResult IDataFilter::passAndFinish(const void* input, sl_size size, MemoryBuffer& output)
	{
		if (!size) {
			return DataFilterResult::Error;
		}
		PREPARE_CHUNK(chunk, sizeChunk)
		DataFilterResult result = Pass(this, input, size, output, chunk, sizeChunk);
		if (result == DataFilterResult::Continue) {
			return Finish(this, output, chunk, sizeChunk);
		} else {
			return result;
		}
	}

	Memory IDataFilter::passAndFinish(const void* input, sl_size size)
	{
		if (!size) {
			return sl_null;
		}
		MemoryBuffer buf;
		if (passAndFinish(input, size, buf) == DataFilterResult::Finished) {
			return buf.merge();
		}
		return sl_null;
	}


	FilterBufferIO::FilterBufferIO()
	{
		m_chunkData = sl_null;
		m_flagFinishing = sl_false;
		m_flagFinished = sl_false;
	}

	FilterBufferIO::~FilterBufferIO()
	{
	}

	DataFilterResult FilterBufferIO::_addInput(const void* data, sl_size size, DataFilterResult result)
	{
		if (size) {
			Memory mem = Memory::create(data, size);
			if (mem.isNotNull()) {
				if (m_bufInput.add(Move(mem))) {
					return result;
				}
			}
			return DataFilterResult::Error;
		} else {
			return result;
		}
	}

	DataFilterResult FilterBufferIO::_doPass(IDataFilter* filter, MemoryData& sliceInput, void*& output, sl_size& sizeOutputBuffer, sl_size& sizeOutputWritten)
	{
		sl_size sizeInputPassed;
		sl_size sizeOutputUsed;
		DataFilterResult result = filter->pass(sliceInput.data, sliceInput.size, sizeInputPassed, output, sizeOutputBuffer, sizeOutputUsed);
		if (result != DataFilterResult::Error) {
			sliceInput.data = (char*)(sliceInput.data) + sizeInputPassed;
			sliceInput.size -= sizeInputPassed;
			if (sizeOutputUsed) {
				output = (char*)output + sizeOutputUsed;
				sizeOutputBuffer -= sizeOutputUsed;
				sizeOutputWritten += sizeOutputUsed;
			}
			if (result == DataFilterResult::Finished) {
				m_flagFinished = sl_true;
				return DataFilterResult::Finished;
			}
			if (sliceInput.size) {
				if (PrepareMemoryData(sliceInput)) {
					if (m_bufInput.pushFront(sliceInput)) {
						return DataFilterResult::Continue;
					}
				}
				return DataFilterResult::Error;
			} else {
				return DataFilterResult::Finished;
			}
		} else {
			return DataFilterResult::Error;
		}
	}

	DataFilterResult FilterBufferIO::pass(IDataFilter* filter, const void* input, sl_size sizeInput, void* output, sl_size sizeOutputBuffer, sl_size& sizeOutputWritten)
	{
		sizeOutputWritten = 0;
		if (m_flagFinished) {
			return DataFilterResult::Finished;
		}
		if (m_bufInput.getSize()) {
			MemoryData slice;
			while (m_bufInput.pop(slice)) {
				DataFilterResult result = _doPass(filter, slice, output, sizeOutputBuffer, sizeOutputWritten);
				if (m_flagFinished) {
					return result;
				}
				if (result != DataFilterResult::Finished) {
					return _addInput(input, sizeInput, result);
				}
			}
		}
		if (sizeInput) {
			MemoryData slice(input, sizeInput);
			DataFilterResult result = _doPass(filter, slice, output, sizeOutputBuffer, sizeOutputWritten);
			if (m_flagFinished) {
				return result;
			}
			if (result != DataFilterResult::Finished) {
				return result;
			}
		}
		if (!m_flagFinishing) {
			return DataFilterResult::Continue;
		}
		sl_size sizeOutputUsed;
		DataFilterResult result = filter->finish(output, sizeOutputBuffer, sizeOutputUsed);
		if (result == DataFilterResult::Finished) {
			m_flagFinished = sl_true;
		} else if (result == DataFilterResult::Error) {
			return DataFilterResult::Error;
		}
		if (sizeOutputUsed) {
			output = (char*)output + sizeOutputUsed;
			sizeOutputBuffer -= sizeOutputUsed;
			sizeOutputWritten += sizeOutputUsed;
		}
		return result;
	}

	DataFilterResult FilterBufferIO::_processWriteResult(MemoryData& slice, sl_reg nWrite)
	{
		if (nWrite > 0) {
			if ((sl_size)nWrite < slice.size) {
				slice.data = (char*)(slice.data) + nWrite;
				slice.size -= nWrite;
				if (PrepareMemoryData(slice)) {
					if (m_bufOutput.pushFront(slice)) {
						return DataFilterResult::Continue;
					}
				}
				return DataFilterResult::Error;
			} else {
				return DataFilterResult::Finished;
			}
		} else {
			if (nWrite == SLIB_IO_WOULD_BLOCK) {
				if (PrepareMemoryData(slice)) {
					if (m_bufOutput.pushFront(slice)) {
						return DataFilterResult::WouldBlock;
					}
				}
			}
			return DataFilterResult::Error;
		}
	}

	DataFilterResult FilterBufferIO::_doPass(IDataFilter* filter, MemoryData& sliceInput, IWriter* writer)
	{
		PREPARE_FILTER_BUFFER_IO_CHUNK(output, sizeOutput)
		for (;;) {
			sl_size sizeInputPassed;
			sl_size sizeOutputUsed;
			DataFilterResult resultPass = filter->pass(sliceInput.data, sliceInput.size, sizeInputPassed, output, sizeOutput, sizeOutputUsed);
			if (resultPass != DataFilterResult::Error) {
				sliceInput.data = (char*)(sliceInput.data) + sizeInputPassed;
				sliceInput.size -= sizeInputPassed;
				if (resultPass == DataFilterResult::Finished) {
					m_flagFinished = sl_true;
				}
				if (sizeOutputUsed) {
					sl_reg nWrite = writer->write(output, sizeOutputUsed);
					MemoryData sliceOutput(output, sizeOutputUsed);
					DataFilterResult result = _processWriteResult(sliceOutput, nWrite);
					if (result != DataFilterResult::Finished) {
						if (resultPass == DataFilterResult::Finished) {
							return result;
						}
						if (sliceInput.size) {
							if (PrepareMemoryData(sliceInput)) {
								if (m_bufInput.pushFront(sliceInput)) {
									return result;
								}
							}
							return DataFilterResult::Error;
						} else {
							return result;
						}
					}
				} else {
					if (!sizeInputPassed) {
						return DataFilterResult::Error;
					}
				}
				if (resultPass == DataFilterResult::Finished) {
					break;
				}
			} else {
				return DataFilterResult::Error;
			}
			if (!(sliceInput.size)) {
				break;
			}
		}
		return DataFilterResult::Finished;
	}

	DataFilterResult FilterBufferIO::pass(IDataFilter* filter, const void* input, sl_size sizeInput, IWriter* writer)
	{
		if (m_bufOutput.getSize()) {
			MemoryData slice;
			while (m_bufOutput.pop(slice)) {
				sl_reg nWrite = writer->write(slice.data, slice.size);
				DataFilterResult result = _processWriteResult(slice, nWrite);
				if (result != DataFilterResult::Finished) {
					return _addInput(input, sizeInput, result);
				}
			}
		}
		if (m_flagFinished) {
			return DataFilterResult::Finished;
		}
		if (m_bufInput.getSize()) {
			MemoryData slice;
			while (m_bufInput.pop(slice)) {
				DataFilterResult result = _doPass(filter, slice, writer);
				if (m_flagFinished) {
					return result;
				}
				if (result != DataFilterResult::Finished) {
					return _addInput(input, sizeInput, result);
				}
			}
		}
		if (sizeInput) {
			MemoryData slice(input, sizeInput);
			DataFilterResult result = _doPass(filter, slice, writer);
			if (m_flagFinished) {
				return result;
			}
			if (result != DataFilterResult::Finished) {
				return result;
			}
		}
		if (!m_flagFinishing) {
			return DataFilterResult::Continue;
		}
		PREPARE_FILTER_BUFFER_IO_CHUNK(output, sizeOutput)
		for (;;) {
			sl_size sizeOutputUsed;
			DataFilterResult result = filter->finish(output, sizeOutput, sizeOutputUsed);
			if (result == DataFilterResult::Finished) {
				m_flagFinished = sl_true;
			} else if (result == DataFilterResult::Error) {
				return DataFilterResult::Error;
			}
			if (sizeOutputUsed) {
				sl_reg nWrite = writer->write(output, sizeOutputUsed);
				MemoryData sliceOutput(output, sizeOutputUsed);
				DataFilterResult resultWrite = _processWriteResult(sliceOutput, nWrite);
				if (resultWrite != DataFilterResult::Finished) {
					return resultWrite;
				}
			}
			if (result == DataFilterResult::Finished) {
				break;
			}
		}
		return DataFilterResult::Finished;
	}

	DataFilterResult FilterBufferIO::_step(IDataFilter* filter, IReader* reader, void* output, sl_size size, sl_size& sizeUsed)
	{
		if (m_flagFinishing || m_bufInput.getSize()) {
			return pass(filter, sl_null, 0, output, size, sizeUsed);
		}
		char* buf;
		sl_size sizeBuf;
		PREPARE_FILTER_BUFFER_IO_CHUNK(chunk, sizeChunk)
		if (size >= (sizeChunk << 1)) {
			size >>= 1;
			buf = (char*)(output) + size;
			sizeBuf = size;
		} else {
			buf = chunk;
			sizeBuf = sizeChunk;
		}
		sl_reg nRead = reader->read(buf, sizeBuf);
		if (nRead > 0) {
			return pass(filter, buf, nRead, output, size, sizeUsed);
		} else {
			if (nRead == SLIB_IO_ENDED) {
				m_flagFinishing = sl_true;
				return pass(filter, sl_null, 0, output, size, sizeUsed);
			} else if (nRead == SLIB_IO_WOULD_BLOCK) {
				sizeUsed = 0;
				return DataFilterResult::WouldBlock;
			} else {
				sizeUsed = 0;
				return DataFilterResult::Error;
			}
		}
	}

	DataFilterResult FilterBufferIO::pass(IDataFilter* filter, IReader* reader, void* _output, sl_size size, sl_size& sizeUsed)
	{
		char* output = (char*)_output;
		sizeUsed = 0;
		for (;;) {
			sl_size nWritten;
			DataFilterResult result = _step(filter, reader, output, size, nWritten);
			sizeUsed += nWritten;
			if (result == DataFilterResult::Continue) {
				if (nWritten >= size) {
					return DataFilterResult::Continue;
				}
				output += nWritten;
				size -= nWritten;
			} else {
				return result;
			}
		}
	}

	DataFilterResult FilterBufferIO::_step(IDataFilter* filter, IReader* reader, IWriter* writer)
	{
		if (m_flagFinishing || m_bufOutput.getSize() || m_bufInput.getSize()) {
			return pass(filter, sl_null, 0, writer);
		}
		PREPARE_FILTER_BUFFER_IO_CHUNK(buf, sizeBuf)
		sl_reg nRead = reader->read(buf, sizeBuf);
		if (nRead > 0) {
			return pass(filter, buf, nRead, writer);
		} else {
			if (nRead == SLIB_IO_ENDED) {
				m_flagFinishing = sl_true;
				return pass(filter, sl_null, 0, writer);
			} else if (nRead == SLIB_IO_WOULD_BLOCK) {
				return DataFilterResult::WouldBlock;
			} else {
				return DataFilterResult::Error;
			}
		}
	}

	DataFilterResult FilterBufferIO::pass(IDataFilter* filter, IReader* reader, IWriter* writer)
	{
		for (;;) {
			DataFilterResult result = _step(filter, reader, writer);
			if (result != DataFilterResult::Continue) {
				return result;
			}
		}
	}

	sl_size FilterBufferIO::getInputBufferSize()
	{
		return m_bufInput.getSize();
	}

	sl_bool FilterBufferIO::isFinishing()
	{
		return m_flagFinishing;
	}

	void FilterBufferIO::setFinishing()
	{
		m_flagFinishing = sl_true;
	}

}
