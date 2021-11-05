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

#include "slib/core/memory_buffer.h"

#define DEFAULT_STACK_SIZE 4096

#define PREPARE_CHUNK(BUF_NAME, SIZE_NAME, SIZE) \
	char* BUF_NAME; \
	sl_size SIZE_NAME = SIZE; \
	char stack__##BUF_NAME[DEFAULT_STACK_SIZE]; \
	Memory mem__##BUF_NAME = Memory::create(SIZE_NAME); \
	if (mem__##BUF_NAME.isNotNull()) { \
		BUF_NAME = (char*)(mem__##BUF_NAME.getData()); \
	} else { \
		BUF_NAME = stack__##BUF_NAME; \
		SIZE_NAME = DEFAULT_STACK_SIZE; \
	}

namespace slib
{

	namespace priv
	{
		namespace compress
		{

			static DataFilterResult Pass(IDataFilter* filter, const void* input, sl_size size, MemoryBuffer& output, void* chunk, sl_size sizeChunk)
			{
				for (;;) {
					sl_size sizeInputPassed;
					sl_size sizeOutputUsed;
					DataFilterResult result = filter->pass(input, size, sizeInputPassed, chunk, sizeChunk, sizeOutputUsed);
					input = (char*)input + sizeInputPassed;
					size -= sizeInputPassed;
					if (sizeOutputUsed) {
						if (!(output.addNew(chunk, sizeOutputUsed))) {
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
						if (!(output.addNew(chunk, sizeOutputUsed))) {
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
		return pass32(input, (sl_uint32)sizeInputAvailable, *((sl_uint32*)&sizeInputPassed), output, (sl_uint32)sizeOutputAvailable, *((sl_uint32*)&sizeOutputUsed));
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

	sl_size IDataFilter::getRecommendedInputSize()
	{
		return 0x20000;
	}

	sl_size IDataFilter::getRecommendedOutputSize()
	{
		return 0x20000;
	}

	DataFilterResult IDataFilter::pass(const void* input, sl_size size, MemoryBuffer& output)
	{
		if (!size) {
			return DataFilterResult::Error;
		}
		PREPARE_CHUNK(chunk, sizeChunk, getRecommendedOutputSize())
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
		PREPARE_CHUNK(chunk, sizeChunk, getRecommendedOutputSize())
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
		m_sizeInput = 0;
		m_sizeOutput = 0;
		m_flagFinishing = sl_false;
		m_flagFinished = sl_false;
	}

	FilterBufferIO::~FilterBufferIO()
	{
	}

	DataFilterResult FilterBufferIO::_processWriteResult(sl_reg nWrite)
	{
		if (nWrite > 0) {
			m_dataOutput = (char*)m_dataOutput + nWrite;
			m_sizeOutput -= nWrite;
			if (m_sizeOutput) {
				return DataFilterResult::Continue;
			} else {
				return DataFilterResult::Finished;
			}
		} else {
			if (nWrite == SLIB_IO_WOULD_BLOCK) {
				return DataFilterResult::WouldBlock;
			} else {
				return DataFilterResult::Error;
			}
		}
	}

	DataFilterResult FilterBufferIO::pass(IDataFilter* filter, const void* input, sl_size sizeInput, sl_size& sizeInputPassed, IWriter* writer)
	{
		sizeInputPassed = 0;
		if (m_sizeOutput) {
			sl_reg nWrite = writer->write(m_dataOutput, m_sizeOutput);
			DataFilterResult result = _processWriteResult(nWrite);
			if (result != DataFilterResult::Finished) {
				return result;
			}
		}
		if (m_flagFinished) {
			return DataFilterResult::Finished;
		}
		if (sizeInput) {
			if (!(_resetOutputBuffer(filter))) {
				return DataFilterResult::Error;
			}
			sl_size n = m_bufOutput.getSize();
			for (;;) {
				sl_size k1, k2;
				DataFilterResult resultPass = filter->pass(input, sizeInput, k1, m_dataOutput, n, k2);
				input = (char*)input + k1;
				sizeInput -= k1;
				sizeInputPassed += k1;
				if (resultPass == DataFilterResult::Finished) {
					m_flagFinished = sl_true;
				} else if (resultPass != DataFilterResult::Continue) {
					return resultPass;
				}
				if (k2) {
					m_sizeOutput = k2;
					sl_reg nWrite = writer->write(m_dataOutput, k2);
					DataFilterResult result = _processWriteResult(nWrite);
					if (result != DataFilterResult::Finished) {
						return result;
					}
				}
				if (resultPass == DataFilterResult::Finished) {
					return DataFilterResult::Finished;
				}
				if (!sizeInput) {
					break;
				}
			}
		}
		if (!m_flagFinishing) {
			return DataFilterResult::Continue;
		}
		if (!(_resetOutputBuffer(filter))) {
			return DataFilterResult::Error;
		}
		sl_size n = m_bufOutput.getSize();
		for (;;) {
			sl_size k;
			DataFilterResult resultPass = filter->finish(m_dataOutput, n, k);
			if (resultPass == DataFilterResult::Finished) {
				m_flagFinished = sl_true;
			} else if (resultPass == DataFilterResult::Error) {
				return DataFilterResult::Error;
			}
			if (k) {
				m_sizeOutput = k;
				sl_reg nWrite = writer->write(m_dataOutput, k);
				DataFilterResult result = _processWriteResult(nWrite);
				if (result != DataFilterResult::Finished) {
					return result;
				}
			}
			if (resultPass == DataFilterResult::Finished) {
				break;
			}
		}
		return DataFilterResult::Finished;
	}

	DataFilterResult FilterBufferIO::pass(IDataFilter* filter, IReader* reader, void* output, sl_size sizeOutput, sl_size& sizeOutputUsed)
	{
		sizeOutputUsed = 0;
		if (m_flagFinished) {
			return DataFilterResult::Finished;
		}
		for (;;) {
			if (m_flagFinishing) {
				if (sizeOutput) {
					DataFilterResult result = filter->finish(output, sizeOutput, sizeOutputUsed);
					if (result == DataFilterResult::Finished) {
						m_flagFinished = sl_true;
					}
					return result;
				} else {
					return DataFilterResult::Continue;
				}
			}
			if (!m_sizeInput) {
				if (!(_resetInputBuffer(filter))) {
					return DataFilterResult::Error;
				}
				sl_size n = m_bufInput.getSize();
				sl_reg nRead = reader->read(m_dataInput, n);
				if (nRead < 0) {
					if (nRead == SLIB_IO_WOULD_BLOCK) {
						return DataFilterResult::WouldBlock;
					} else {
						return DataFilterResult::Error;
					}
				}
				if (nRead == SLIB_IO_ENDED) {
					m_flagFinishing = sl_true;
					continue;
				} else {
					m_sizeInput = nRead;
				}
			}
			sl_size k1, k2;
			DataFilterResult result = filter->pass(m_dataInput, m_sizeInput, k1, output, sizeOutput, k2);
			if (result == DataFilterResult::Finished) {
				m_flagFinished = sl_true;
			}
			if (k1 || k2) {
				m_dataInput = (char*)m_dataInput + k1;
				m_sizeInput -= k1;
				output = (char*)output + k2;
				sizeOutput -= k2;
				sizeOutputUsed += k2;
				if (result != DataFilterResult::Continue) {
					return result;
				}
			} else {
				return result;
			}
		}
	}

	DataFilterResult FilterBufferIO::pass(IDataFilter* filter, IReader* reader, IWriter* writer)
	{
		for (;;) {
			if (m_flagFinishing || m_sizeOutput) {
				sl_size k;
				return pass(filter, sl_null, 0, k, writer);
			}
			if (!m_sizeInput) {
				if (!(_resetInputBuffer(filter))) {
					return DataFilterResult::Error;
				}
				sl_size n = m_bufInput.getSize();
				sl_reg nRead = reader->read(m_dataInput, n);
				if (nRead < 0) {
					if (nRead == SLIB_IO_WOULD_BLOCK) {
						return DataFilterResult::WouldBlock;
					} else {
						return DataFilterResult::Error;
					}
				}
				if (nRead == SLIB_IO_ENDED) {
					m_flagFinishing = sl_true;
					continue;
				} else {
					m_sizeInput = nRead;
				}
			}
			sl_size k;
			DataFilterResult result = pass(filter, m_dataInput, m_sizeInput, k, writer);
			if (result == DataFilterResult::Finished) {
				m_flagFinished = sl_true;
			}
			m_dataInput = (char*)m_dataInput + k;
			m_sizeInput -= k;
			if (result != DataFilterResult::Continue) {
				return result;
			}
		}
	}

	sl_bool FilterBufferIO::isFinishing()
	{
		return m_flagFinishing;
	}

	void FilterBufferIO::setFinishing()
	{
		m_flagFinishing = sl_true;
	}

	sl_bool FilterBufferIO::_resetInputBuffer(IDataFilter* filter)
	{
		if (m_bufInput.isNull()) {
			m_bufInput = Memory::create(filter->getRecommendedInputSize());
			if (m_bufInput.isNull()) {
				return sl_false;
			}
		}
		m_dataInput = m_bufInput.getData();
		m_sizeInput = 0;
		return sl_true;
	}

	sl_bool FilterBufferIO::_resetOutputBuffer(IDataFilter* filter)
	{
		if (m_bufOutput.isNull()) {
			m_bufOutput = Memory::create(filter->getRecommendedOutputSize());
			if (m_bufOutput.isNull()) {
				return sl_false;
			}
		}
		m_dataOutput = m_bufOutput.getData();
		m_sizeOutput = 0;
		return sl_true;
	}
	
}
