/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY_IO
#define CHECKHEADER_SLIB_CORE_MEMORY_IO

#include "definition.h"

#include "io.h"

namespace slib
{
	
	class SLIB_EXPORT MemoryIO : public IO
	{
		SLIB_DECLARE_OBJECT
		
	public:
		MemoryIO(sl_size size = 0, sl_bool flagResizable = sl_true);
		
		MemoryIO(const void* data, sl_size size, sl_bool flagResizable = sl_true);

		MemoryIO(const Memory& mem);
	
		~MemoryIO();
	
	public:
		void initialize(const void* data, sl_size size, sl_bool flagResizable = sl_true);
	
		void initialize(sl_size size = 0, sl_bool flagResizable = sl_true);
		
		void initialize(const Memory& mem);

		sl_size getOffset();

		sl_size getLength();

		char* getBuffer();
	
		void close() override;
	
		sl_reg read(void* buf, sl_size size) override;
	
		sl_reg write(const void* buf, sl_size size) override;

		using IO::getPosition;
		sl_bool getPosition(sl_uint64& outPos) override;

		using IO::getSize;
		sl_bool getSize(sl_uint64& outSize) override;

		sl_bool seek(sl_int64 offset, SeekPosition pos = SeekPosition::Current) override;
	
		sl_bool setSize(sl_uint64 size) override;
	
		sl_bool isResizable();

		sl_int64 find(const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_int64 endPosition = -1);

		sl_int64 findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_int64 endPosition = -1);

	protected:
		void _initialize(const void* data, sl_size size, sl_bool flagResizable);

		void _initialize(sl_size size, sl_bool flagResizable);
		
		void _initialize(const Memory& mem);
		
		void _free();
		
	protected:
		void* m_buf;
		sl_size m_size;
		sl_size m_offset;
		sl_bool m_flagResizable;
	
	};
	
	class SLIB_EXPORT MemoryReader : public Object, public IReader, public ISeekable, public SeekableReaderBase<MemoryReader>
	{
		SLIB_DECLARE_OBJECT
		
	public:
		MemoryReader(const Memory& mem);
	
		MemoryReader(const void* buf, sl_size size);

		~MemoryReader();
	
	public:
		void initialize(const Memory& mem);

		void initialize(const void* buf, sl_size size);
	
		sl_size getOffset();

		sl_size getLength();

		char* getBuffer();
		
		sl_reg read(void* buf, sl_size size) override;

		using ISeekable::getPosition;
		sl_bool getPosition(sl_uint64& outPos) override;

		using ISeekable::getSize;
		sl_bool getSize(sl_uint64& outSize) override;

		sl_bool seek(sl_int64 offset, SeekPosition pos) override;

		sl_int64 find(const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_int64 endPosition = -1);

		sl_int64 findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_int64 endPosition = -1);

	protected:
		const void* m_buf;
		sl_size m_size;
		sl_size m_offset;
		Memory m_mem;
	
	};
	
	class SLIB_EXPORT MemoryWriter : public Object, public IWriter, public ISeekable
	{
		SLIB_DECLARE_OBJECT
		
	public:
		// write-only/appending memory
		MemoryWriter();

		MemoryWriter(const Memory& mem);

		MemoryWriter(void* buf, sl_size size);

		~MemoryWriter();
	
	public:
		void initialize();

		void initialize(const Memory& mem);

		void initialize(void* buf, sl_size size);
		
		sl_reg write(const void* buf, sl_size size) override;

		sl_reg write(const Memory& mem);
	
		sl_bool seek(sl_int64 offset, SeekPosition pos) override;
	
		Memory getData();
	
		MemoryBuffer& getMemoryBuffer();
	
		sl_size getOffset();
	
		sl_size getLength();
	
		char* getBuffer();
	
		using ISeekable::getPosition;
		sl_bool getPosition(sl_uint64& outPos) override;

		using ISeekable::getSize;
		sl_bool getSize(sl_uint64& outSize) override;
		
	protected:
		void* m_buf;
		sl_size m_size;
		sl_size m_offset;
		Memory m_mem;
		MemoryBuffer m_buffer;
	
	};
	
}

#endif
