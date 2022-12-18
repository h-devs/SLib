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

#include "slib/data/compress.h"
#include "slib/data/convert_io.h"

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

			static DataConvertResult Pass(IDataConverter* converter, const void* input, sl_size size, MemoryBuffer& output, void* chunk, sl_size sizeChunk)
			{
				for (;;) {
					sl_size sizeInputPassed;
					sl_size sizeOutputUsed;
					DataConvertResult result = converter->pass(input, size, sizeInputPassed, chunk, sizeChunk, sizeOutputUsed);
					input = (char*)input + sizeInputPassed;
					size -= sizeInputPassed;
					if (sizeOutputUsed) {
						if (!(output.addNew(chunk, sizeOutputUsed))) {
							return DataConvertResult::Error;
						}
					}
					if (result != DataConvertResult::Continue) {
						return result;
					}
					if (!size) {
						break;
					}
				}
				return DataConvertResult::Continue;
			}

			static DataConvertResult Finish(IDataConverter* converter, MemoryBuffer& output, void* chunk, sl_size sizeChunk)
			{
				for (;;) {
					sl_size sizeOutputUsed;
					DataConvertResult result = converter->finish(chunk, sizeChunk, sizeOutputUsed);
					if (sizeOutputUsed) {
						if (!(output.addNew(chunk, sizeOutputUsed))) {
							return DataConvertResult::Error;
						}
					}
					if (result != DataConvertResult::Continue) {
						return result;
					}
				}
			}

		}
	}

	using namespace priv::compress;

	DataConvertResult IDataConverter::pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
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
			DataConvertResult result = pass32(input, (sl_uint32)n1, k1, output, (sl_uint32)n2, k2);
			sizeInputPassed += k1;
			sizeOutputUsed += k2;
			if (result != DataConvertResult::Continue) {
				return result;
			}
			input = (char*)input + k1;
			sizeInputAvailable -= k1;
			output = (char*)output + k2;
			sizeOutputAvailable -= k2;
		}
		return DataConvertResult::Continue;
#else
		return pass32(input, (sl_uint32)sizeInputAvailable, *((sl_uint32*)&sizeInputPassed), output, (sl_uint32)sizeOutputAvailable, *((sl_uint32*)&sizeOutputUsed));
#endif
	}

	DataConvertResult IDataConverter::finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed)
	{
#ifdef SLIB_ARCH_IS_64BIT
		sizeOutputUsed = 0;
		while (sizeOutputAvailable) {
			sl_size n = sizeOutputAvailable;
			if (n > 0x40000000) {
				n = 0x40000000;
			}
			sl_uint32 k;
			DataConvertResult result = finish32(output, (sl_uint32)n, k);
			sizeOutputUsed += k;
			if (result != DataConvertResult::Continue) {
				return result;
			}
			output = (char*)output + k;
			sizeOutputAvailable -= k;
		}
		return DataConvertResult::Continue;
#else
		return finish32(output, (sl_uint32)sizeOutputAvailable, *((sl_uint32*)&sizeOutputUsed));
#endif
	}

	DataConvertResult IDataConverter::pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
		void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		sl_size k1, k2;
		DataConvertResult result = pass(input, sizeInputAvailable, k1, output, sizeOutputAvailable, k2);
		sizeInputPassed = (sl_uint32)k1;
		sizeOutputUsed = (sl_uint32)k2;
		return result;
	}

	DataConvertResult IDataConverter::finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed)
	{
		sl_size k;
		DataConvertResult result = finish(output, sizeOutputAvailable, k);
		sizeOutputUsed = (sl_uint32)k;
		return result;
	}

	sl_size IDataConverter::getRecommendedInputSize()
	{
		return 0x20000;
	}

	sl_size IDataConverter::getRecommendedOutputSize()
	{
		return 0x20000;
	}

	DataConvertResult IDataConverter::pass(const void* input, sl_size size, MemoryBuffer& output)
	{
		if (!size) {
			return DataConvertResult::Error;
		}
		PREPARE_CHUNK(chunk, sizeChunk, getRecommendedOutputSize())
		return Pass(this, input, size, output, chunk, sizeChunk);
	}

	Memory IDataConverter::pass(const void* input, sl_size size)
	{
		if (!size) {
			return sl_null;
		}
		MemoryBuffer buf;
		if (pass(input, size, buf) != DataConvertResult::Error) {
			return buf.merge();
		}
		return sl_null;
	}

	DataConvertResult IDataConverter::passAndFinish(const void* input, sl_size size, MemoryBuffer& output)
	{
		if (!size) {
			return DataConvertResult::Error;
		}
		PREPARE_CHUNK(chunk, sizeChunk, getRecommendedOutputSize())
		DataConvertResult result = Pass(this, input, size, output, chunk, sizeChunk);
		if (result == DataConvertResult::Continue) {
			return Finish(this, output, chunk, sizeChunk);
		} else {
			return result;
		}
	}

	Memory IDataConverter::passAndFinish(const void* input, sl_size size)
	{
		if (!size) {
			return sl_null;
		}
		MemoryBuffer buf;
		if (passAndFinish(input, size, buf) == DataConvertResult::Finished) {
			return buf.merge();
		}
		return sl_null;
	}


	DataConvertIO::DataConvertIO()
	{
		m_sizeInput = 0;
		m_sizeOutput = 0;
		m_flagFinishing = sl_false;
		m_flagFinished = sl_false;
	}

	DataConvertIO::~DataConvertIO()
	{
	}

	DataConvertResult DataConvertIO::_processWriteResult(sl_reg nWrite)
	{
		if (nWrite > 0) {
			m_dataOutput = (char*)m_dataOutput + nWrite;
			m_sizeOutput -= nWrite;
			if (m_sizeOutput) {
				return DataConvertResult::Continue;
			} else {
				return DataConvertResult::Finished;
			}
		} else {
			if (nWrite == SLIB_IO_WOULD_BLOCK) {
				return DataConvertResult::WouldBlock;
			} else {
				return DataConvertResult::Error;
			}
		}
	}

	DataConvertResult DataConvertIO::pass(IDataConverter* converter, const void* input, sl_size sizeInput, sl_size& sizeInputPassed, IWriter* writer)
	{
		sizeInputPassed = 0;
		if (m_sizeOutput) {
			sl_reg nWrite = writer->write(m_dataOutput, m_sizeOutput);
			DataConvertResult result = _processWriteResult(nWrite);
			if (result != DataConvertResult::Finished) {
				return result;
			}
		}
		if (m_flagFinished) {
			return DataConvertResult::Finished;
		}
		if (sizeInput) {
			if (!(_resetOutputBuffer(converter))) {
				return DataConvertResult::Error;
			}
			sl_size n = m_bufOutput.getSize();
			for (;;) {
				sl_size k1, k2;
				DataConvertResult resultPass = converter->pass(input, sizeInput, k1, m_dataOutput, n, k2);
				input = (char*)input + k1;
				sizeInput -= k1;
				sizeInputPassed += k1;
				if (resultPass == DataConvertResult::Finished) {
					m_flagFinished = sl_true;
				} else if (resultPass != DataConvertResult::Continue) {
					return resultPass;
				}
				if (k2) {
					m_sizeOutput = k2;
					sl_reg nWrite = writer->write(m_dataOutput, k2);
					DataConvertResult result = _processWriteResult(nWrite);
					if (result != DataConvertResult::Finished) {
						return result;
					}
				}
				if (resultPass == DataConvertResult::Finished) {
					return DataConvertResult::Finished;
				}
				if (!sizeInput) {
					break;
				}
			}
		}
		if (!m_flagFinishing) {
			return DataConvertResult::Continue;
		}
		if (!(_resetOutputBuffer(converter))) {
			return DataConvertResult::Error;
		}
		sl_size n = m_bufOutput.getSize();
		for (;;) {
			sl_size k;
			DataConvertResult resultPass = converter->finish(m_dataOutput, n, k);
			if (resultPass == DataConvertResult::Finished) {
				m_flagFinished = sl_true;
			} else if (resultPass == DataConvertResult::Error) {
				return DataConvertResult::Error;
			}
			if (k) {
				m_sizeOutput = k;
				sl_reg nWrite = writer->write(m_dataOutput, k);
				DataConvertResult result = _processWriteResult(nWrite);
				if (result != DataConvertResult::Finished) {
					return result;
				}
			}
			if (resultPass == DataConvertResult::Finished) {
				break;
			}
		}
		return DataConvertResult::Finished;
	}

	DataConvertResult DataConvertIO::pass(IDataConverter* converter, IReader* reader, void* output, sl_size sizeOutput, sl_size& sizeOutputUsed)
	{
		sizeOutputUsed = 0;
		if (m_flagFinished) {
			return DataConvertResult::Finished;
		}
		for (;;) {
			if (m_flagFinishing) {
				if (sizeOutput) {
					DataConvertResult result = converter->finish(output, sizeOutput, sizeOutputUsed);
					if (result == DataConvertResult::Finished) {
						m_flagFinished = sl_true;
					}
					return result;
				} else {
					return DataConvertResult::Continue;
				}
			}
			if (!m_sizeInput) {
				if (!(_resetInputBuffer(converter))) {
					return DataConvertResult::Error;
				}
				sl_size n = m_bufInput.getSize();
				sl_reg nRead = reader->read(m_dataInput, n);
				if (nRead < 0) {
					if (nRead == SLIB_IO_WOULD_BLOCK) {
						return DataConvertResult::WouldBlock;
					} else {
						return DataConvertResult::Error;
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
			DataConvertResult result = converter->pass(m_dataInput, m_sizeInput, k1, output, sizeOutput, k2);
			if (result == DataConvertResult::Finished) {
				m_flagFinished = sl_true;
			}
			if (k1 || k2) {
				m_dataInput = (char*)m_dataInput + k1;
				m_sizeInput -= k1;
				output = (char*)output + k2;
				sizeOutput -= k2;
				sizeOutputUsed += k2;
				if (result != DataConvertResult::Continue) {
					return result;
				}
			} else {
				return result;
			}
		}
	}

	DataConvertResult DataConvertIO::pass(IDataConverter* converter, IReader* reader, IWriter* writer)
	{
		for (;;) {
			if (m_flagFinishing || m_sizeOutput) {
				sl_size k;
				return pass(converter, sl_null, 0, k, writer);
			}
			if (!m_sizeInput) {
				if (!(_resetInputBuffer(converter))) {
					return DataConvertResult::Error;
				}
				sl_size n = m_bufInput.getSize();
				sl_reg nRead = reader->read(m_dataInput, n);
				if (nRead < 0) {
					if (nRead == SLIB_IO_WOULD_BLOCK) {
						return DataConvertResult::WouldBlock;
					} else {
						return DataConvertResult::Error;
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
			DataConvertResult result = pass(converter, m_dataInput, m_sizeInput, k, writer);
			if (result == DataConvertResult::Finished) {
				m_flagFinished = sl_true;
			}
			m_dataInput = (char*)m_dataInput + k;
			m_sizeInput -= k;
			if (result != DataConvertResult::Continue) {
				return result;
			}
		}
	}

	sl_bool DataConvertIO::isFinishing()
	{
		return m_flagFinishing;
	}

	void DataConvertIO::setFinishing()
	{
		m_flagFinishing = sl_true;
	}

	sl_bool DataConvertIO::_resetInputBuffer(IDataConverter* converter)
	{
		if (m_bufInput.isNull()) {
			m_bufInput = Memory::create(converter->getRecommendedInputSize());
			if (m_bufInput.isNull()) {
				return sl_false;
			}
		}
		m_dataInput = m_bufInput.getData();
		m_sizeInput = 0;
		return sl_true;
	}

	sl_bool DataConvertIO::_resetOutputBuffer(IDataConverter* converter)
	{
		if (m_bufOutput.isNull()) {
			m_bufOutput = Memory::create(converter->getRecommendedOutputSize());
			if (m_bufOutput.isNull()) {
				return sl_false;
			}
		}
		m_dataOutput = m_bufOutput.getData();
		m_sizeOutput = 0;
		return sl_true;
	}

}
