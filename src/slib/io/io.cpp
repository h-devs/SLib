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

#include "slib/io/io.h"
#include "slib/io/memory_io.h"
#include "slib/io/memory_reader.h"
#include "slib/io/memory_writer.h"
#include "slib/io/memory_output.h"
#include "slib/io/buffered_reader.h"
#include "slib/io/buffered_writer.h"
#include "slib/io/buffered_seekable_reader.h"
#include "slib/io/skippable_reader.h"

#include "slib/io/priv/util.h"
#include "slib/io/priv/impl.h"
#include "slib/core/mio.h"
#include "slib/data/serialize/io.h"

namespace slib
{

	namespace {

		SLIB_INLINE static sl_bool CheckLittleEndianRuntime() noexcept
		{
			sl_uint32 n = 0x12345678;
			return *(sl_uint8*)(&n) == 0x78;
		}

	}

	sl_bool Endian::checkLittleEndianRuntime() noexcept
	{
		static volatile sl_bool flagInit = sl_true;
		static volatile sl_bool value = sl_true;
		if (flagInit) {
			value = CheckLittleEndianRuntime();;
			flagInit = sl_false;
		}
		return value;
	}


	SLIB_DEFINE_IREADER_MEMBERS(IReader,)

	sl_int32 IReader::read32(void* buf, sl_uint32 size, sl_int32 timeout)
	{
		return (sl_int32)(read(buf, size, timeout));
	}

	sl_reg IReader::read(void* buf, sl_size size, sl_int32 timeout)
	{
		return ReaderHelper::readWithRead32(this, buf, size, timeout);
	}


	SLIB_DEFINE_IWRITER_MEMBERS(IWriter,)

	sl_int32 IWriter::write32(const void* buf, sl_uint32 size, sl_int32 timeout)
	{
		return (sl_int32)(write(buf, size, timeout));
	}

	sl_reg IWriter::write(const void* buf, sl_size size, sl_int32 timeout)
	{
		return WriterHelper::writeWithWrite32(this, buf, size, timeout);
	}


	sl_int32 IBlockReader::readAt32(sl_uint64 offset, void* buf, sl_uint32 size, sl_int32 timeout)
	{
		return (sl_int32)(readAt(offset, buf, size, timeout));
	}

	sl_reg IBlockReader::readAt(sl_uint64 offset, void* buf, sl_size size, sl_int32 timeout)
	{
		return BlockReaderHelper::readAtWithReadAt32(this, offset, buf, size, timeout);
	}

	sl_reg IBlockReader::readFullyAt(sl_uint64 offset, void* buf, sl_size size, sl_int32 timeout)
	{
		return BlockReaderHelper::readFullyAt(this, offset, buf, size, timeout);
	}


	sl_int32 IBlockWriter::writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size, sl_int32 timeout)
	{
		return (sl_int32)(writeAt(offset, buf, size, timeout));
	}

	sl_reg IBlockWriter::writeAt(sl_uint64 offset, const void* buf, sl_size size, sl_int32 timeout)
	{
		return BlockWriterHelper::writeAtWithWriteAt32(this, offset, buf, size, timeout);
	}

	sl_reg IBlockWriter::writeFullyAt(sl_uint64 offset, const void* buf, sl_size size, sl_int32 timeout)
	{
		return BlockWriterHelper::writeFullyAt(this, offset, buf, size, timeout);
	}


	SLIB_DEFINE_ISIZE_MEMBERS(ISizeProvider,)


	SLIB_DEFINE_ISEEKABLE_MEMBERS(ISeekable,)

	sl_bool ISeekable::isEnd(sl_bool& outFlag)
	{
		sl_uint64 pos, size;
		if (getPosition(pos) && getSize(size)) {
			outFlag = pos >= size;
			return sl_true;
		}
		return sl_false;
	}


	SLIB_DEFINE_SEEKABLE_READER_MEMBERS(SeekableReaderBase,)


	SLIB_DEFINE_SEEKABLE_READER_MEMBERS(IOBase,)
	SLIB_DEFINE_SEEKABLE_WRITER_MEMBERS(IOBase,)


	MemoryIO::MemoryIO()
	{
		_initialize();
	}

	MemoryIO::MemoryIO(sl_size size)
	{
		_initialize(size);
	}

	MemoryIO::MemoryIO(void* data, sl_size size)
	{
		_initialize(data, size);
	}

	MemoryIO::MemoryIO(const Memory& mem)
	{
		_initialize(mem);
	}

	MemoryIO::~MemoryIO()
	{
	}

	void MemoryIO::_initialize()
	{
		m_buf = sl_null;
		m_size = 0;
		m_offset = 0;
		m_flagResizable = sl_true;
	}

	void MemoryIO::_initialize(sl_size size)
	{
		if (size) {
			Memory data = Memory::createResizable(size);
			if (data.isNotNull()) {
				m_buf = data.getData();
				m_size = size;
				m_offset = 0;
				m_flagResizable = sl_true;
				m_data = Move(data);
				return;
			}
		}
		_initialize();
	}

	void MemoryIO::_initialize(void* data, sl_size size)
	{
		m_flagResizable = sl_false;
		m_offset = 0;
		if (data && size) {
			m_buf = data;
			m_size = size;
		} else {
			m_buf = sl_null;
			m_size = 0;
		}
	}

	void MemoryIO::_initialize(const Memory& data)
	{
		m_offset = 0;
		if (data.isNotNull()) {
			m_buf = data.getData();
			m_size = data.getSize();
			m_flagResizable = data.isResizable();
			m_data = data;
		} else {
			m_buf = sl_null;
			m_size = 0;
			m_flagResizable = sl_false;
		}
	}

	sl_bool MemoryIO::_growCapacity(sl_size size)
	{
		if (!m_flagResizable) {
			return sl_false;
		}
		sl_size n = m_data.getSize();
		if (size < n) {
			m_size = size;
			return sl_true;
		}
		if (n < 16) {
			n = 16;
		} else {
			n = n + (n >> 1);
		}
		if (n < size) {
			n = size;
		}
		if (m_data.setSize(n)) {
			m_buf = m_data.getData();
			m_size = size;
			return sl_true;
		}
		return sl_false;
	}

	void MemoryIO::initialize()
	{
		m_data.setNull();
		_initialize();
	}

	void MemoryIO::initialize(sl_size size)
	{
		m_data.setNull();
		_initialize(size);
	}

	void MemoryIO::initialize(void* data, sl_size size)
	{
		m_data.setNull();
		_initialize(data, size);
	}

	void MemoryIO::initialize(const Memory& data)
	{
		m_data.setNull();
		_initialize(data);
	}

	void MemoryIO::close()
	{
		m_data.setNull();
		m_buf = sl_null;
		m_size = 0;
		m_offset = 0;
		m_flagResizable = sl_false;
	}

	sl_reg MemoryIO::read(void* buf, sl_size size, sl_int32 timeout)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (m_offset >= m_size) {
			return SLIB_IO_ENDED;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			size = limit;
		}
		if (size > 0) {
			Base::copyMemory(buf, (char*)m_buf + m_offset, size);
			m_offset += size;
		}
		return size;
	}

	sl_reg MemoryIO::write(const void* buf, sl_size size, sl_int32 timeout)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			if (m_flagResizable) {
				sl_size limitMax = SLIB_SIZE_MAX - m_offset;
				if (size > limitMax) {
					size = limitMax;
				}
				if (!(_growCapacity(m_offset + size))) {
					size = limit;
				}
			} else {
				size = limit;
			}
		}
		if (size > 0) {
			Base::copyMemory((char*)m_buf + m_offset, buf, size);
			m_offset += size;
		}
		return size;
	}

	sl_bool MemoryIO::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 p = m_offset;
		if (pos == SeekPosition::Begin) {
			p = 0;
		} else if (pos == SeekPosition::End) {
			p = m_size;
		}
		p += offset;
		if (p > m_size) {
			return sl_false;
		}
		m_offset = (sl_size)p;
		return sl_true;
	}

	sl_bool MemoryIO::getPosition(sl_uint64& outPos)
	{
		outPos = m_offset;
		return sl_true;
	}

	sl_bool MemoryIO::getSize(sl_uint64& outSize)
	{
		outSize = m_size;
		return sl_true;
	}

	sl_bool MemoryIO::setSize(sl_uint64 _size)
	{
		sl_size size = (sl_size)_size;
		if (!m_flagResizable) {
			return sl_false;
		}
		if (m_data.isNull()) {
			if (m_buf) {
				return sl_false;
			}
		}
		if (size < m_data.getSize()) {
			m_size = size;
			if (m_offset > size) {
				m_offset = size;
			}
			return sl_true;
		}
		if (m_data.setSize(size)) {
			m_buf = m_data.getData();
			m_size = size;
			return sl_true;
		}
		return sl_false;
	}

	sl_size MemoryIO::getPosition()
	{
		return m_offset;
	}

	sl_size MemoryIO::getSize()
	{
		return m_size;
	}

	sl_uint8* MemoryIO::getBuffer()
	{
		return (sl_uint8*)m_buf;
	}

	sl_bool MemoryIO::isResizable()
	{
		return m_flagResizable;
	}

	sl_bool MemoryIO::setResizable(sl_bool flag)
	{
		if (m_data.isNull()) {
			if (m_buf) {
				return sl_false;
			}
		}
		m_flagResizable = flag;
		return sl_true;
	}

	Memory MemoryIO::getData()
	{
		if (m_data.isNotNull()) {
			return m_data.sub(0, m_size);
		} else {
			return Memory::createStatic(m_buf, m_size);
		}
	}

	namespace {
		static sl_bool FixFindMemoryPosition(sl_size& outStartPos, sl_size& outEndPos, sl_size size, sl_int64 startPos, sl_int64 endPos)
		{
			if (startPos < 0) {
				outStartPos = 0;
			} else if ((sl_uint64)startPos >= size) {
				return sl_false;
			} else {
				outStartPos = (sl_size)startPos;
			}
			if (!endPos) {
				return sl_false;
			} else if (endPos< 0) {
				outEndPos = size;
			} else if ((sl_size)endPos > size) {
				outEndPos = size;
			} else {
				outEndPos = (sl_size)endPos;
			}
			if (startPos >= endPos) {
				return sl_false;
			}
			return sl_true;
		}
	}

	sl_int64 MemoryIO::find(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemory(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}

	sl_int64 MemoryIO::findBackward(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemoryBackward(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}


	SLIB_DEFINE_SEEKABLE_READER_MEMBERS(MemoryReader,)

	MemoryReader::MemoryReader(const Memory& mem)
	{
		initialize(mem);
	}

	MemoryReader::MemoryReader(const void* buf, sl_size size)
	{
		initialize(buf, size);
	}

	MemoryReader::~MemoryReader()
	{
	}

	void MemoryReader::initialize(const Memory& mem)
	{
		m_mem = mem;
		m_buf = mem.getData();
		m_size = mem.getSize();
		m_offset = 0;
	}

	void MemoryReader::initialize(const void* buf, sl_size size)
	{
		if (buf && size) {
			m_buf = buf;
			m_size = size;
		} else {
			m_buf = sl_null;
			m_size = 0;
		}
		m_offset = 0;
	}

	sl_reg MemoryReader::read(void* buf, sl_size size, sl_int32 timeout)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (m_offset >= m_size) {
			return SLIB_IO_ENDED;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			size = limit;
		}
		if (size > 0) {
			Base::copyMemory(buf, (char*)m_buf + m_offset, size);
			m_offset += size;
		}
		return size;
	}

	sl_bool MemoryReader::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 p = m_offset;
		if (pos == SeekPosition::Begin) {
			p = 0;
		} else if (pos == SeekPosition::End) {
			p = m_size;
		}
		p += offset;
		if (p > m_size) {
			return sl_false;
		}
		m_offset = (sl_size)p;
		return sl_true;
	}

	sl_bool MemoryReader::getPosition(sl_uint64& outPos)
	{
		outPos = m_offset;
		return sl_true;
	}

	sl_bool MemoryReader::getSize(sl_uint64& outSize)
	{
		outSize = m_size;
		return sl_true;
	}

	sl_size MemoryReader::getPosition()
	{
		return m_offset;
	}

	sl_size MemoryReader::getSize()
	{
		return m_size;
	}

	sl_size MemoryReader::getRemainedSize()
	{
		if (m_size > m_offset) {
			return m_size - m_offset;
		} else {
			return 0;
		}
	}

	sl_reg MemoryReader::skip(sl_size size)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (m_offset >= m_size) {
			return SLIB_IO_ENDED;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			size = limit;
		}
		if (size > 0) {
			m_offset += size;
		}
		return size;
	}

	sl_uint8* MemoryReader::getBuffer()
	{
		return (sl_uint8*)m_buf;
	}

	sl_int64 MemoryReader::find(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemory(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}

	sl_int64 MemoryReader::findBackward(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemoryBackward(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}

	sl_bool MemoryReader::readInt8(sl_int8* output)
	{
		if (m_offset < m_size) {
			if (output) {
				*output = ((sl_int8*)m_buf)[m_offset];
			}
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int8 MemoryReader::readInt8(sl_int8 def)
	{
		if (m_offset < m_size) {
			sl_int8 ret = ((sl_int8*)m_buf)[m_offset];
			m_offset++;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint8(sl_uint8* output)
	{
		if (m_offset < m_size) {
			if (output) {
				*output = ((sl_uint8*)m_buf)[m_offset];
			}
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint8 MemoryReader::readUint8(sl_uint8 def)
	{
		if (m_offset < m_size) {
			sl_uint8 ret = ((sl_int8*)m_buf)[m_offset];
			m_offset++;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readInt16(sl_int16* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readInt16((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int16 MemoryReader::readInt16(sl_int16 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			sl_int16 ret = MIO::readInt16((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint16(sl_uint16* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readUint16((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint16 MemoryReader::readUint16(sl_uint16 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			sl_uint16 ret = MIO::readUint16((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readInt32(sl_int32* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readInt32((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int32 MemoryReader::readInt32(sl_int32 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			sl_int32 ret = MIO::readInt32((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint32(sl_uint32* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readUint32((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint32 MemoryReader::readUint32(sl_uint32 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			sl_uint32 ret = MIO::readUint32((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readInt64(sl_int64* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readInt64((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int64 MemoryReader::readInt64(sl_int64 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			sl_int64 ret = MIO::readInt64((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint64(sl_uint64* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readUint64((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint64 MemoryReader::readUint64(sl_uint64 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			sl_uint64 ret = MIO::readUint64((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readFloat(float* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readFloat((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	float MemoryReader::readFloat(float def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			float ret = MIO::readFloat((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readDouble(double* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readDouble((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	double MemoryReader::readDouble(double def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			double ret = MIO::readDouble((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}


	SLIB_DEFINE_SEEKABLE_WRITER_MEMBERS(MemoryWriter,)

	MemoryWriter::MemoryWriter(const Memory& mem)
	{
		initialize(mem);
	}

	MemoryWriter::MemoryWriter(void* buf, sl_size size)
	{
		initialize(buf, size);
	}

	MemoryWriter::~MemoryWriter()
	{
	}

	void MemoryWriter::initialize(const Memory& mem)
	{
		m_mem = mem;
		m_buf = mem.getData();
		m_size = mem.getSize();
		m_offset = 0;
	}

	void MemoryWriter::initialize(void* buf, sl_size size)
	{
		if (buf && size) {
			m_buf = buf;
			m_size = size;
		} else {
			m_buf = sl_null;
			m_size = 0;
		}
		m_offset = 0;
	}

	sl_reg MemoryWriter::write(const void* buf, sl_size size, sl_int32 timeout)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (m_offset >= m_size) {
			return SLIB_IO_ENDED;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			size = limit;
		}
		if (size > 0) {
			Base::copyMemory((char*)m_buf + m_offset, buf, size);
			m_offset += size;
		}
		return size;
	}

	sl_reg MemoryWriter::write(const MemoryView& mem)
	{
		return write(mem.data, mem.size);
	}

	sl_bool MemoryWriter::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 p = m_offset;
		if (pos == SeekPosition::Begin) {
			p = 0;
		} else if (pos == SeekPosition::End) {
			p = m_size;
		}
		p += offset;
		if (p > m_size) {
			return sl_false;
		}
		m_offset = (sl_size)p;
		return sl_true;
	}

	sl_bool MemoryWriter::getPosition(sl_uint64& outPos)
	{
		outPos = m_offset;
		return sl_true;
	}

	sl_bool MemoryWriter::getSize(sl_uint64& outSize)
	{
		outSize = m_size;
		return sl_true;
	}

	sl_size MemoryWriter::getPosition()
	{
		return m_offset;
	}

	sl_size MemoryWriter::getSize()
	{
		return m_size;
	}

	sl_uint8* MemoryWriter::getBuffer()
	{
		return (sl_uint8*)m_buf;
	}

	sl_bool MemoryWriter::writeInt8(sl_int8 value)
	{
		if (m_offset < m_size) {
			((sl_int8*)m_buf)[m_offset] = value;
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint8(sl_uint8 value)
	{
		if (m_offset < m_size) {
			((sl_uint8*)m_buf)[m_offset] = value;
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeInt16(sl_int16 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			MIO::writeInt16((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint16(sl_uint16 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			MIO::writeUint16((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeInt32(sl_int32 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			MIO::writeInt32((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint32(sl_uint32 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			MIO::writeUint32((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeInt64(sl_int64 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			MIO::writeInt64((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint64(sl_uint64 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			MIO::writeUint64((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeFloat(float value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			MIO::writeFloat((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeDouble(double value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			MIO::writeDouble((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}


	MemoryOutput::MemoryOutput()
	{
	}

	MemoryOutput::~MemoryOutput()
	{
	}

	sl_reg MemoryOutput::write(const void* buf, sl_size size, sl_int32 timeout)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (size <= 64) {
			if (m_buffer.addElements_NoLock((sl_uint8*)buf, size)) {
				return size;
			}
		} else {
			if (flush()) {
				Memory mem = Memory::create(buf, size);
				if (mem.isNotNull()) {
					if (m_queue.add(mem)) {
						return size;
					}
				}
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_reg MemoryOutput::write(const Memory& mem)
	{
		if (mem.isNull()) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (flush()) {
			if (m_queue.add(mem)) {
				return mem.getSize();
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_reg MemoryOutput::write(Memory&& mem)
	{
		if (mem.isNull()) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (flush()) {
			if (m_queue.add(Move(mem))) {
				return mem.getSize();
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_bool MemoryOutput::flush()
	{
		sl_size n = m_buffer.getCount();
		if (!n) {
			return sl_true;
		}
		MemoryData mem(m_buffer.getData(), n, Move(m_buffer.ref));
		if (m_queue.add(mem)) {
			return sl_true;
		}
		m_buffer.ref = Move(Ref< CList<sl_uint8> >::cast(mem.ref));
		return sl_false;
	}

	sl_size MemoryOutput::getSize()
	{
		return m_queue.getSize() + m_buffer.getCount();
	}

	Memory MemoryOutput::merge()
	{
		flush();
		return m_queue.merge();
	}

	void MemoryOutput::clear()
	{
		m_queue.clear();
		m_buffer.setNull();
	}

	void MemoryOutput::link(MemoryBuffer& mem)
	{
		flush();
		m_queue.link(mem);
	}

	void MemoryOutput::link(MemoryOutput& other)
	{
		flush();
		other.flush();
		m_queue.link(other.m_queue);
	}

	sl_bool MemoryOutput::writeInt8(sl_int8 value)
	{
		return m_buffer.add_NoLock(value);
	}

	sl_bool MemoryOutput::writeUint8(sl_uint8 value)
	{
		return m_buffer.add_NoLock(value);
	}

	sl_bool MemoryOutput::writeInt16(sl_int16 value, EndianType endian)
	{
		sl_uint8 v[2];
		MIO::writeInt16(v, value, endian);
		return m_buffer.addElements_NoLock(v, 2);
	}

	sl_bool MemoryOutput::writeUint16(sl_uint16 value, EndianType endian)
	{
		sl_uint8 v[2];
		MIO::writeUint16(v, value, endian);
		return m_buffer.addElements_NoLock(v, 2);
	}

	sl_bool MemoryOutput::writeInt32(sl_int32 value, EndianType endian)
	{
		sl_uint8 v[4];
		MIO::writeInt32(v, value, endian);
		return m_buffer.addElements_NoLock(v, 4);
	}

	sl_bool MemoryOutput::writeUint32(sl_uint32 value, EndianType endian)
	{
		sl_uint8 v[4];
		MIO::writeUint32(v, value, endian);
		return m_buffer.addElements_NoLock(v, 4);
	}

	sl_bool MemoryOutput::writeInt64(sl_int64 value, EndianType endian)
	{
		sl_uint8 v[8];
		MIO::writeInt64(v, value, endian);
		return m_buffer.addElements_NoLock(v, 8);
	}

	sl_bool MemoryOutput::writeUint64(sl_uint64 value, EndianType endian)
	{
		sl_uint8 v[8];
		MIO::writeUint64(v, value, endian);
		return m_buffer.addElements_NoLock(v, 8);
	}

	sl_bool MemoryOutput::writeFloat(float value, EndianType endian)
	{
		sl_uint8 v[4];
		MIO::writeFloat(v, value, endian);
		return m_buffer.addElements_NoLock(v, 4);
	}

	sl_bool MemoryOutput::writeDouble(double value, EndianType endian)
	{
		sl_uint8 v[8];
		MIO::writeDouble(v, value, endian);
		return m_buffer.addElements_NoLock(v, 8);
	}


	BufferedReader::BufferedReader(): m_reader(sl_null), m_closable(sl_null), m_posInBuf(0), m_sizeRead(0), m_dataBuf(sl_null), m_sizeBuf(0)
	{
	}

	BufferedReader::~BufferedReader()
	{
	}

	sl_bool BufferedReader::open(const Ptrx<IReader, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_false;
		}
		Ptrx<IReader, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_false;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_false;
		}

		_init(obj, buf);
		return sl_true;
	}

	void BufferedReader::close()
	{
		if (m_closable) {
			m_closable->close();
		}
		m_reader = sl_null;
		m_closable = sl_null;
		m_ref.setNull();
	}

	sl_reg BufferedReader::read(void* buf, sl_size size, sl_int32 timeout)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		IReader* reader = m_reader;
		if (!reader) {
			return SLIB_IO_ERROR;
		}
		sl_size nAvailable = m_sizeRead - m_posInBuf;
		if (!nAvailable) {
			if (size >= m_sizeBuf) {
				return reader->read(buf, size, timeout);
			}
			m_posInBuf = 0;
			sl_reg nRead = reader->read(m_dataBuf, m_sizeBuf, timeout);
			if (nRead <= 0) {
				m_sizeRead = 0;
				return nRead;
			}
			m_sizeRead = nRead;
			nAvailable = nRead;
		}
		if (size > nAvailable) {
			size = nAvailable;
		}
		Base::copyMemory(buf, m_dataBuf + m_posInBuf, size);
		m_posInBuf += size;
		return size;
	}

	sl_bool BufferedReader::readInt8(sl_int8* output)
	{
		if (m_posInBuf < m_sizeRead) {
			if (output) {
				*output = m_dataBuf[m_posInBuf];
			}
			m_posInBuf++;
			return sl_true;
		} else {
			return IReader::readInt8(output);
		}
	}

	sl_int8 BufferedReader::readInt8(sl_int8 def)
	{
		if (m_posInBuf < m_sizeRead) {
			sl_int8 ret = m_dataBuf[m_posInBuf];
			m_posInBuf++;
			return ret;
		} else {
			return IReader::readInt8(def);
		}
	}

	sl_bool BufferedReader::readUint8(sl_uint8* output)
	{
		if (m_posInBuf < m_sizeRead) {
			if (output) {
				*output = m_dataBuf[m_posInBuf];
			}
			m_posInBuf++;
			return sl_true;
		} else {
			return IReader::readUint8(output);
		}
	}

	sl_uint8 BufferedReader::readUint8(sl_uint8 def)
	{
		if (m_posInBuf < m_sizeRead) {
			sl_uint8 ret = m_dataBuf[m_posInBuf];
			m_posInBuf++;
			return ret;
		} else {
			return IReader::readUint8(def);
		}
	}

	sl_bool BufferedReader::readInt16(sl_int16* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readInt16(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readInt16(output, endian);
		}
	}

	sl_int16 BufferedReader::readInt16(sl_int16 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			sl_int16 ret = MIO::readInt16(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readInt16(def, endian);
		}
	}

	sl_bool BufferedReader::readUint16(sl_uint16* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readUint16(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readUint16(output, endian);
		}
	}

	sl_uint16 BufferedReader::readUint16(sl_uint16 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			sl_uint16 ret = MIO::readUint16(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readUint16(def, endian);
		}
	}

	sl_bool BufferedReader::readInt32(sl_int32* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readInt32(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readInt32(output, endian);
		}
	}

	sl_int32 BufferedReader::readInt32(sl_int32 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			sl_int32 ret = MIO::readInt32(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readInt32(def, endian);
		}
	}

	sl_bool BufferedReader::readUint32(sl_uint32* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readUint32(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readUint32(output, endian);
		}
	}

	sl_uint32 BufferedReader::readUint32(sl_uint32 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			sl_uint32 ret = MIO::readUint32(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readUint32(def, endian);
		}
	}

	sl_bool BufferedReader::readInt64(sl_int64* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readInt64(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readInt64(output, endian);
		}
	}

	sl_int64 BufferedReader::readInt64(sl_int64 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			sl_int64 ret = MIO::readInt64(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readInt64(def, endian);
		}
	}

	sl_bool BufferedReader::readUint64(sl_uint64* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readUint64(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readUint64(output, endian);
		}
	}

	sl_uint64 BufferedReader::readUint64(sl_uint64 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			sl_uint64 ret = MIO::readUint64(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readUint64(def, endian);
		}
	}

	sl_bool BufferedReader::readFloat(float* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readFloat(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readFloat(output, endian);
		}
	}

	float BufferedReader::readFloat(float def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			float ret = MIO::readFloat(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readFloat(def, endian);
		}
	}

	sl_bool BufferedReader::readDouble(double* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readDouble(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readDouble(output, endian);
		}
	}

	double BufferedReader::readDouble(double def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			double ret = MIO::readDouble(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readDouble(def, endian);
		}
	}

	void BufferedReader::_init(const Ptrx<IReader, IClosable>& reader, const Memory& buf)
	{
		m_ref = reader.ref;
		m_reader = reader;
		m_closable = reader;

		m_buf = buf;
		m_dataBuf = (sl_uint8*)(buf.getData());
		m_sizeBuf = buf.getSize();
	}


	BufferedWriter::BufferedWriter(): m_writer(sl_null), m_closable(sl_null), m_dataBuf(sl_null), m_sizeBuf(0), m_sizeWritten(0)
	{
	}

	BufferedWriter::~BufferedWriter()
	{
		flush();
	}

	sl_bool BufferedWriter::open(const Ptrx<IWriter, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_false;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_false;
		}
		Ptrx<IWriter, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_false;
		}
		_init(obj, buf);
		return sl_true;
	}

	sl_bool BufferedWriter::isOpened()
	{
		return m_writer != sl_null;
	}

	void BufferedWriter::close()
	{
		if (!m_writer) {
			return;
		}
		flush();
		if (m_closable) {
			m_closable->close();
		}
		m_writer = sl_null;
		m_closable = sl_null;
		m_ref.setNull();
	}

	sl_reg BufferedWriter::write(const void* buf, sl_size size, sl_int32 timeout)
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		IWriter* writer = m_writer;
		if (!writer) {
			return SLIB_IO_ERROR;
		}
		if (size <= m_sizeBuf - m_sizeWritten) {
			Base::copyMemory(m_dataBuf + m_sizeWritten, buf, size);
			m_sizeWritten += size;
			return size;
		} else {
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			if (flush(timeout)) {
				return writer->write(buf, size, GetTimeoutFromTick(tickEnd));
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_bool BufferedWriter::flush(sl_int32 timeout)
	{
		sl_size size = m_sizeWritten;
		if (!size) {
			return sl_true;
		}
		IWriter* writer = m_writer;
		if (!writer) {
			return sl_false;
		}
		sl_reg n = writer->writeFully(m_dataBuf, size, timeout);
		if (n == size) {
			m_sizeWritten = 0;
			return sl_true;
		}
		if (n <= 0) {
			return sl_false;
		}
		size -= n;
		Base::moveMemory(m_dataBuf, m_dataBuf + n, size);
		m_sizeWritten = size;
		return sl_false;
	}

	sl_bool BufferedWriter::writeInt8(sl_int8 value)
	{
		if (m_sizeWritten < m_sizeBuf) {
			m_dataBuf[m_sizeWritten] = value;
			m_sizeWritten++;
			return sl_true;
		} else {
			return IWriter::writeInt8(value);
		}
	}

	sl_bool BufferedWriter::writeUint8(sl_uint8 value)
	{
		if (m_sizeWritten < m_sizeBuf) {
			m_dataBuf[m_sizeWritten] = value;
			m_sizeWritten++;
			return sl_true;
		} else {
			return IWriter::writeUint8(value);
		}
	}

	sl_bool BufferedWriter::writeInt16(sl_int16 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 2;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeInt16(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeInt16(value, endian);
		}
	}

	sl_bool BufferedWriter::writeUint16(sl_uint16 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 2;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeUint16(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeUint16(value, endian);
		}
	}

	sl_bool BufferedWriter::writeInt32(sl_int32 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 4;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeInt32(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeInt32(value, endian);
		}
	}

	sl_bool BufferedWriter::writeUint32(sl_uint32 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 4;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeUint32(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeUint32(value, endian);
		}
	}

	sl_bool BufferedWriter::writeInt64(sl_int64 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 8;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeInt64(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeInt64(value, endian);
		}
	}

	sl_bool BufferedWriter::writeUint64(sl_uint64 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 8;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeUint64(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeUint64(value, endian);
		}
	}

	sl_bool BufferedWriter::writeFloat(float value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 4;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeFloat(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeFloat(value, endian);
		}
	}

	sl_bool BufferedWriter::writeDouble(double value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 8;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeDouble(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeDouble(value, endian);
		}
	}

	void BufferedWriter::_init(const Ptrx<IWriter, IClosable>& writer, const Memory& buf)
	{
		m_ref = writer.ref;
		m_writer = writer;
		m_closable = writer;

		m_buf = buf;
		m_dataBuf = (sl_uint8*)(buf.getData());
		m_sizeBuf = buf.getSize();
		m_sizeWritten = 0;
	}


	SLIB_DEFINE_SEEKABLE_READER_MEMBERS(BufferedSeekableReader,)

	BufferedSeekableReader::BufferedSeekableReader() : m_reader(sl_null), m_seekable(sl_null), m_closable(sl_null), m_posCurrent(0), m_sizeTotal(0), m_posInternal(0), m_dataBuf(sl_null), m_sizeBuf(0), m_sizeRead(0), m_posBuf(0)
	{
	}

	BufferedSeekableReader::~BufferedSeekableReader()
	{
	}

	sl_bool BufferedSeekableReader::open(const Ptrx<IReader, ISeekable, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_false;
		}
		Ptrx<IReader, ISeekable, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_false;
		}
		ISeekable* seeker = obj;
		if (!seeker) {
			return sl_false;
		}
		sl_uint64 size = seeker->getSize();
		if (!size) {
			return sl_false;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_false;
		}

		_init(obj, size, buf);
		return sl_true;
	}

	sl_bool BufferedSeekableReader::isOpened()
	{
		return m_reader != sl_null;
	}

	void BufferedSeekableReader::_init(const Ptrx<IReader, ISeekable, IClosable>& reader, sl_uint64 size, const Memory& buf)
	{
		m_ref = reader.ref;
		m_reader = reader;
		m_seekable = reader;
		m_closable = reader;

		m_sizeTotal = size;

		m_buf = buf;
		m_dataBuf = (sl_uint8*)(buf.getData());
		m_sizeBuf = buf.getSize();
	}

	sl_reg BufferedSeekableReader::_readInBuf(void* buf, sl_size size)
	{
		if (m_posCurrent >= m_posBuf) {
			sl_uint64 _offset = m_posCurrent - m_posBuf;
			if (_offset < m_sizeRead) {
				sl_size offset = (sl_size)_offset;
				sl_size nAvailable = m_sizeRead - offset;
				if (size > nAvailable) {
					size = nAvailable;
				}
				Base::copyMemory(buf, m_dataBuf + offset, size);
				m_posCurrent += size;
				return size;
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_bool BufferedSeekableReader::_seekInternal(sl_uint64 pos)
	{
		if (pos == m_posInternal) {
			return sl_true;
		}
		ISeekable* seeker = m_seekable;
		if (seeker) {
			if (seeker->seek(pos, SeekPosition::Begin)) {
				m_posInternal = pos;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_reg BufferedSeekableReader::_readInternal(sl_uint64 pos, void* buf, sl_size size, sl_int32 timeout)
	{
		if (_seekInternal(pos)) {
			sl_uint64 n = m_sizeTotal - pos;
			if (size > n) {
				size = (sl_size)n;
			}
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			IReader* reader = m_reader;
			if (reader) {
				sl_reg nRead = reader->readFully(buf, size, timeout);
				if (nRead > 0) {
					m_posInternal += nRead;
				}
				return nRead;
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_reg BufferedSeekableReader::_fillBuf(sl_uint64 pos, sl_size size, sl_int32 timeout)
	{
		m_posBuf = pos;
		sl_reg nRead = _readInternal(pos, m_dataBuf, size, timeout);
		if (nRead > 0) {
			m_sizeRead = nRead;
		} else {
			m_sizeRead = 0;
		}
		return nRead;
	}

	sl_reg BufferedSeekableReader::_fillBuf2(sl_uint64 pos, sl_int32 timeout)
	{
		return _fillBuf(pos, m_sizeBuf, timeout);
	}

	sl_reg BufferedSeekableReader::_readFillingBuf(sl_uint64 pos, void* buf, sl_size size, sl_int32 timeout)
	{
		sl_reg nRead = _fillBuf2(pos, timeout);
		if (nRead > 0) {
			return _readInBuf(buf, size);
		}
		return nRead;
	}

	sl_bool BufferedSeekableReader::readInt8(sl_int8* _out)
	{
		return readUint8((sl_uint8*)((void*)_out));
	}

	sl_bool BufferedSeekableReader::peekInt8(sl_int8*_out)
	{
		return peekUint8((sl_uint8*)((void*)_out));
	}

	sl_bool BufferedSeekableReader::readUint8(sl_uint8* _out)
	{
		if (m_posCurrent >= m_sizeTotal) {
			return sl_false;
		}
		if (m_posCurrent >= m_posBuf && m_posCurrent < m_posBuf + m_sizeRead) {
			*_out = m_dataBuf[(sl_size)(m_posCurrent - m_posBuf)];
			m_posCurrent++;
			return sl_true;
		}
		return readFully(_out, 1);
	}

	sl_bool BufferedSeekableReader::peekUint8(sl_uint8* _out)
	{
		if (m_posCurrent >= m_sizeTotal) {
			return sl_false;
		}
		if (m_posCurrent >= m_posBuf && m_posCurrent < m_posBuf + m_sizeRead) {
			*_out = m_dataBuf[(sl_size)(m_posCurrent - m_posBuf)];
			return sl_true;
		}
		if (readFully(_out, 1) == 1) {
			seek(-1, SeekPosition::Current);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_reg BufferedSeekableReader::read(void*& buf, sl_int32 timeout)
	{
		if (m_posCurrent >= m_sizeTotal) {
			return SLIB_IO_ENDED;
		}
		if (m_posCurrent >= m_posBuf) {
			sl_uint64 _offset = m_posCurrent - m_posBuf;
			if (_offset < m_sizeRead) {
				sl_size offset = (sl_size)_offset;
				sl_size sizeRemain = m_sizeRead - offset;
				m_posCurrent += sizeRemain;
				buf = m_dataBuf + offset;
				return sizeRemain;
			}
		}
		sl_reg nRead = _fillBuf2(m_posCurrent, timeout);
		if (nRead > 0) {
			buf = m_dataBuf;
			m_posCurrent += nRead;
		}
		return nRead;
	}

	sl_reg BufferedSeekableReader::read(void* _buf, sl_size size, sl_int32 timeout)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		if (m_posCurrent >= m_sizeTotal) {
			return SLIB_IO_ENDED;
		}
		if (!m_sizeRead) {
			return _readFillingBuf(m_posCurrent, buf, size, timeout);
		}
		sl_reg nRead = _readInBuf(buf, size);
		if (nRead > 0) {
			return nRead;
		}
		if (m_posCurrent >= m_posBuf) {
			return _readFillingBuf(m_posCurrent, buf, size, timeout);
		}
		sl_uint64 _offset = m_posBuf - m_posCurrent;
		if (_offset >= m_sizeBuf) {
			return _readFillingBuf(m_posCurrent, buf, size, timeout);
		}
		sl_size offset = (sl_size)_offset;
		sl_size sizeTailData;
		if (offset < size) {
			sizeTailData = size - offset;
			if (sizeTailData > m_sizeRead) {
				sizeTailData = m_sizeRead;
			}
			Base::copyMemory(buf + offset, m_dataBuf, sizeTailData);
			size = offset;
		} else {
			sizeTailData = 0;
		}
		if (m_posBuf >= m_sizeBuf) {
			nRead = _fillBuf2(m_posBuf - m_sizeBuf, timeout);
			if (nRead <= 0) {
				return nRead;
			}
		} else {
			sl_size pos = (sl_size)m_posBuf;
			sl_size n = pos + m_sizeRead;
			if (n > m_sizeBuf) {
				n = m_sizeBuf;
			}
			n -= pos;
			Base::moveMemory(m_dataBuf + pos, m_dataBuf, n);
			nRead = _fillBuf(0, pos, timeout);
			if (nRead == pos) {
				m_sizeRead += n;
			}
		}
		nRead = _readInBuf(buf, size);
		if (nRead == size) {
			m_posCurrent += sizeTailData;
			return size + sizeTailData;
		}
		return nRead;
	}

	sl_bool BufferedSeekableReader::getPosition(sl_uint64& outPos)
	{
		outPos = m_posCurrent;
		return sl_true;
	}

	sl_bool BufferedSeekableReader::getSize(sl_uint64& outSize)
	{
		outSize = m_sizeTotal;
		return sl_true;
	}

	sl_bool BufferedSeekableReader::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 posNew;
		switch (pos) {
		case SeekPosition::Begin:
			if (offset < 0) {
				return sl_false;
			}
			if ((sl_uint64)offset > m_sizeTotal) {
				return sl_false;
			}
			m_posCurrent = offset;
			break;
		case SeekPosition::End:
			if (offset > 0) {
				return sl_false;
			}
			if ((sl_uint64)(-offset) > m_sizeTotal) {
				return sl_false;
			}
			posNew = m_sizeTotal + offset;
			break;
		case SeekPosition::Current:
			{
				sl_uint64 posCurrent = m_posCurrent;
				if (offset > 0) {
					if ((sl_uint64)offset > m_sizeTotal - posCurrent) {
						return sl_false;
					}
				} else if (offset < 0) {
					if ((sl_uint64)(-offset) > posCurrent) {
						return sl_false;
					}
				} else {
					return sl_true;
				}
				m_posCurrent = posCurrent + offset;
				break;
			}
		}
		return sl_true;
	}

	void BufferedSeekableReader::close()
	{
		if (m_closable) {
			m_closable->close();
		}
		m_reader = sl_null;
		m_seekable = sl_null;
		m_closable = sl_null;
		m_ref.setNull();
	}


	SkippableReader::SkippableReader(): m_reader(sl_null), m_seekable(sl_null), m_pos(0)
	{
	}

	SkippableReader::SkippableReader(const Ptrx<IReader, ISeekable>& reader): m_ref(reader.ref), m_reader(reader), m_seekable(reader), m_pos(0)
	{
	}

	SkippableReader::~SkippableReader()
	{
	}

	sl_bool SkippableReader::setReader(const Ptrx<IReader, ISeekable>& reader)
	{
		m_ref = reader.ref;
		m_reader = reader;
		m_seekable = reader;
		return m_reader != sl_null;
	}

	sl_reg SkippableReader::read(void* buf, sl_size size, sl_int32 timeout)
	{
		sl_reg iRead = m_reader->read(buf, size, timeout);
		if (iRead > 0) {
			m_pos += iRead;
		}
		return iRead;
	}

	sl_int32 SkippableReader::read32(void* buf, sl_uint32 size, sl_int32 timeout)
	{
		sl_int32 iRead = m_reader->read32(buf, size, timeout);
		if (iRead > 0) {
			m_pos += iRead;
		}
		return iRead;
	}

	sl_uint64 SkippableReader::skip(sl_uint64 size)
	{
		sl_uint64 nSkip = IOUtil::skip(Pointerx<IReader, ISeekable>(m_reader, m_seekable), size);
		if (nSkip) {
			m_pos += nSkip;
		}
		return nSkip;
	}

	sl_uint64 SkippableReader::getPosition()
	{
		if (m_seekable) {
			return m_seekable->getPosition();
		} else {
			return m_pos;
		}
	}


	sl_uint64 IOUtil::skip(const Pointerx<IReader, ISeekable>& _reader, sl_uint64 size)
	{
		if (!size) {
			return 0;
		}
		ISeekable* seekable = _reader;
		if (seekable) {
			if (seekable->seek(size, SeekPosition::Current)) {
				return size;
			}
			sl_uint64 pos = seekable->getPosition();
			sl_uint64 total = seekable->getSize();
			if (pos >= total) {
				return 0;
			}
			sl_uint64 remain = total - pos;
			if (size > remain) {
				size = remain;
			}
			if (seekable->seek(size, SeekPosition::Current)) {
				return size;
			}
			return 0;
		} else {
			IReader* reader = _reader;
			if (reader) {
				char buf[1024];
				sl_uint64 nRead = 0;
				while (nRead < size) {
					sl_uint64 nRemain = size - nRead;
					sl_uint32 n = sizeof(buf);
					if (n > nRemain) {
						n = (sl_uint32)nRemain;
					}
					sl_reg m = reader->read(buf, n);
					if (m > 0) {
						nRead += m;
					} else {
						return nRead;
					}
				}
				return nRead;
			}
		}
		return 0;
	}

	sl_int64 IOUtil::find(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind)
	{
		return SeekableReaderHelper::find((IReader*)reader, (ISeekable*)reader, pattern, nPattern, startPosition, sizeFind);
	}

	sl_int64 IOUtil::findBackward(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind)
	{
		return SeekableReaderHelper::findBackward((IReader*)reader, (ISeekable*)reader, pattern, nPattern, startPosition, sizeFind);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(SerializeBuffer)

	SerializeBuffer::SerializeBuffer(const void* buf, sl_size size) noexcept
	{
		current = begin = (sl_uint8*)buf;
		end = begin ? begin + size : sl_null;
	}

	SerializeBuffer::SerializeBuffer(const MemoryView& mem) noexcept: SerializeBuffer(mem.data, mem.size)
	{
	}

	SerializeBuffer::SerializeBuffer(MemoryData&& data) noexcept: SerializeBuffer(data.data, data.size, Move(data.ref))
	{
	}

	SerializeBuffer::SerializeBuffer(const Memory& mem) noexcept: SerializeBuffer(MemoryView(mem))
	{
	}

	SerializeBuffer::SerializeBuffer(Memory&& mem) noexcept: SerializeBuffer(MemoryData(Move(mem)))
	{
	}

	sl_bool SerializeBuffer::read(sl_uint8& _out) noexcept
	{
		if (current < end) {
			_out = *(current++);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool SerializeBuffer::write(sl_uint8 value) noexcept
	{
		if (current < end) {
			*(current++) = value;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_size SerializeBuffer::read(void* buf, sl_size size) noexcept
	{
		if (size && current < end) {
			if (current + size > end) {
				size = end - current;
			}
			Base::copyMemory(buf, current, size);
			current += size;
			return size;
		}
		return 0;
	}

	sl_size SerializeBuffer::write(const void* buf, sl_size size) noexcept
	{
		if (size && current < end) {
			if (current + size > end) {
				size = end - current;
			}
			Base::copyMemory(current, buf, size);
			current += size;
			return size;
		}
		return 0;
	}

	sl_size SerializeBuffer::write(const MemoryView& mem) noexcept
	{
		return write(mem.data, mem.size);
	}

#define DEFINE_SERIALIZE_BUFFER_READWRITE_INT8(TYPE, SUFFIX) \
	sl_bool SerializeBuffer::read##SUFFIX(TYPE& _out) noexcept \
	{ \
		if (current < end) { \
			_out = *(current++); \
			return sl_true; \
		} \
		return sl_false; \
	} \
	\
	sl_bool SerializeBuffer::write##SUFFIX(TYPE value) noexcept \
	{ \
		if (current < end) { \
			*(current++) = value; \
			return sl_true; \
		} \
		return sl_false; \
	}

	DEFINE_SERIALIZE_BUFFER_READWRITE_INT8(sl_uint8, Uint8)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT8(sl_int8, Int8)

#define DEFINE_SERIALIZE_BUFFER_READWRITE_INT(TYPE, SUFFIX) \
	sl_bool SerializeBuffer::read##SUFFIX(TYPE& _out) noexcept \
	{ \
		if (current + sizeof(TYPE) <= end) { \
			_out = MIO::read##SUFFIX(current); \
			current += sizeof(TYPE); \
			return sl_true; \
		} \
		return sl_false; \
	} \
	\
	sl_bool SerializeBuffer::write##SUFFIX(TYPE value) noexcept \
	{ \
		if (current + sizeof(TYPE) <= end) { \
			MIO::write##SUFFIX(current, value); \
			current += sizeof(TYPE); \
			return sl_true; \
		} \
		return sl_false; \
	}

	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_uint16, Uint16BE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_uint16, Uint16LE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_int16, Int16BE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_int16, Int16LE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_uint32, Uint32BE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_uint32, Uint32LE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_int32, Int32BE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_int32, Int32LE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_uint64, Uint64BE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_uint64, Uint64LE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_int64, Int64BE)
	DEFINE_SERIALIZE_BUFFER_READWRITE_INT(sl_int64, Int64LE)

	sl_bool SerializeBuffer::readSection(void* buf, sl_size size) noexcept
	{
		if (!size) {
			return sl_true;
		}
		if (current + size <= end) {
			Base::copyMemory(buf, current, size);
			current += size;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool SerializeBuffer::skip(sl_size size) noexcept
	{
		if (!size) {
			return sl_true;
		}
		if (current + size <= end) {
			current += size;
			return sl_true;
		}
		return sl_false;
	}


	SerializeOutput::~SerializeOutput()
	{
		if (begin) {
			delete[] begin;
		}
	}

	sl_bool SerializeOutput::write(sl_uint8 value) noexcept
	{
		if (_growSmallSize(1)) {
			begin[offset] = value;
			offset++;
			return sl_true;
		}
		return sl_false;
	}

	sl_size SerializeOutput::write(const void* buf, sl_size sizeAdd) noexcept
	{
		if (!sizeAdd) {
			return 0;
		}
		sl_size sizeNew = offset + sizeAdd;
		if (sizeNew <= size) {
			Base::copyMemory(begin + offset, buf, sizeAdd);
			offset += sizeAdd;
			return sizeAdd;
		} else {
			if (sizeAdd < 64) {
				if (!(_growSize(offset + 64))) {
					return 0;
				}
			} else {
				if (!(_growSize(sizeNew))) {
					return 0;
				}
			}
			Base::copyMemory(begin + offset, buf, sizeAdd);
			offset += sizeAdd;
			return sizeAdd;
		}
	}

	sl_size SerializeOutput::write(const MemoryView& mem) noexcept
	{
		return write(mem.data, mem.size);
	}

	void* SerializeOutput::allocate(sl_size sizeAdd) noexcept
	{
		sl_size sizeNew = offset + sizeAdd;
		if (sizeNew > size) {
			if (sizeAdd < 64) {
				if (!(_growSize(offset + 64))) {
					return sl_null;
				}
			} else {
				if (!(_growSize(sizeNew))) {
					return sl_null;
				}
			}
		}
		void* ret = begin + offset;
		offset += sizeAdd;
		return ret;
	}

#define DEFINE_SERIALIZE_OUTPUT_WRITE_INT8(TYPE, SUFFIX) \
	sl_bool SerializeOutput::write##SUFFIX(TYPE value) noexcept \
	{ \
		return write(value); \
	}

	DEFINE_SERIALIZE_OUTPUT_WRITE_INT8(sl_uint8, Uint8)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT8(sl_int8, Int8)

#define DEFINE_SERIALIZE_OUTPUT_WRITE_INT(TYPE, SUFFIX) \
	sl_bool SerializeOutput::write##SUFFIX(TYPE value) noexcept \
	{ \
		if (_growSmallSize(sizeof(TYPE))) { \
			MIO::write##SUFFIX(begin + offset, value); \
			offset += sizeof(TYPE); \
			return sl_true; \
		} \
		return sl_false; \
	}

	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_uint16, Uint16BE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_uint16, Uint16LE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_int16, Int16BE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_int16, Int16LE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_uint32, Uint32BE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_uint32, Uint32LE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_int32, Int32BE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_int32, Int32LE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_uint64, Uint64BE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_uint64, Uint64LE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_int64, Int64BE)
	DEFINE_SERIALIZE_OUTPUT_WRITE_INT(sl_int64, Int64LE)

	Memory SerializeOutput::releaseToMemory() noexcept
	{
		Memory ret = Memory::createNoCopy(begin, offset);
		if (ret.isNotNull()) {
			begin = sl_null;
			offset = 0;
			size = 0;
			return ret;
		}
		return sl_null;
	}

	sl_bool SerializeOutput::_growSmallSize(sl_size addSize) noexcept
	{
		if (offset + addSize <= size) {
			return sl_true;
		} else {
			return _growSize(offset + 64);
		}
	}

	sl_bool SerializeOutput::_growSize(sl_size newSize) noexcept
	{
		if (begin) {
			sl_size n = size + (size >> 4);
			if (newSize < n) {
				newSize = n;
			}
			sl_uint8* data = (sl_uint8*)(Base::reallocMemory(begin, newSize));
			if (data) {
				begin = data;
				size = newSize;
				return sl_true;
			}
		} else {
			sl_uint8* data = (sl_uint8*)(Base::createMemory(newSize));
			if (data) {
				begin = data;
				size = newSize;
				return sl_true;
			}
		}
		return sl_false;
	}


	sl_bool SerializeByte(IWriter* writer, sl_uint8 value) noexcept
	{
		return writer->writeUint8(value);
	}

	sl_bool SerializeByte(MemoryBuffer* buf, sl_uint8 value) noexcept
	{
		return buf->addNew(&value, 1);
	}

	sl_bool SerializeByte(SerializeBuffer* buf, sl_uint8 value) noexcept
	{
		return buf->write(value);
	}

	sl_bool SerializeByte(SerializeOutput* output, sl_uint8 value) noexcept
	{
		return output->write(value);
	}

	sl_bool SerializeRaw(IWriter* writer, const void* data, sl_size size) noexcept
	{
		return writer->writeFully(data, size) == size;
	}

	sl_bool SerializeRaw(MemoryBuffer* buf, const void* data, sl_size size) noexcept
	{
		return buf->addNew(data, size);
	}

	sl_bool SerializeRaw(SerializeBuffer* buf, const void* data, sl_size size) noexcept
	{
		return buf->write(data, size) == size;
	}

	sl_bool SerializeRaw(SerializeOutput* output, const void* data, sl_size size) noexcept
	{
		return output->write(data, size) == size;
	}

	sl_bool SerializeRaw(sl_uint8** _buf, const void* data, sl_size size) noexcept
	{
		sl_uint8*& buf = *_buf;
		Base::copyMemory(buf, data, size);
		buf += size;
		return sl_true;
	}

	sl_bool SerializeRaw(sl_uint8*& buf, const void* data, sl_size size) noexcept
	{
		Base::copyMemory(buf, data, size);
		buf += size;
		return sl_true;
	}

	sl_bool SerializeRaw(IWriter* writer, const MemoryData& data) noexcept
	{
		return writer->writeFully(data.data, data.size) == data.size;
	}

	sl_bool SerializeRaw(MemoryBuffer* buf, const MemoryData& data) noexcept
	{
		return buf->add(data);
	}

	sl_bool SerializeRaw(MemoryBuffer* buf, MemoryData&& data) noexcept
	{
		return buf->add(Move(data));
	}

	sl_bool SerializeRaw(SerializeBuffer* buf, const MemoryData& data) noexcept
	{
		return buf->write(data.data, data.size) == data.size;
	}

	sl_bool SerializeRaw(SerializeOutput* output, const MemoryData& data) noexcept
	{
		return output->write(data.data, data.size) == data.size;
	}

	sl_bool SerializeRaw(sl_uint8** _buf, const MemoryData& data) noexcept
	{
		sl_uint8*& buf = *_buf;
		Base::copyMemory(buf, data.data, data.size);
		buf += data.size;
		return sl_true;
	}

	sl_bool SerializeRaw(sl_uint8*& buf, const MemoryData& data) noexcept
	{
		Base::copyMemory(buf, data.data, data.size);
		buf += data.size;
		return sl_true;
	}

	sl_bool SerializeStatic(IWriter* writer, const void* data, sl_size size) noexcept
	{
		return writer->writeFully(data, size) == size;
	}

	sl_bool SerializeStatic(MemoryBuffer* buf, const void* data, sl_size size) noexcept
	{
		return buf->addStatic(data, size);
	}

	sl_bool SerializeStatic(SerializeBuffer* buf, const void* data, sl_size size) noexcept
	{
		return buf->write(data, size) == size;
	}

	sl_bool SerializeStatic(SerializeOutput* output, const void* data, sl_size size) noexcept
	{
		return output->write(data, size) == size;
	}

	sl_bool SerializeStatic(sl_uint8** _buf, const void* data, sl_size size) noexcept
	{
		sl_uint8*& buf = *_buf;
		Base::copyMemory(buf, data, size);
		buf += size;
		return sl_true;
	}

	sl_bool SerializeStatic(sl_uint8*& buf, const void* data, sl_size size) noexcept
	{
		Base::copyMemory(buf, data, size);
		buf += size;
		return sl_true;
	}

	sl_bool DeserializeByte(IReader* reader, sl_uint8& value) noexcept
	{
		return reader->readUint8(&value);
	}

	sl_bool DeserializeByte(SerializeBuffer* buf, sl_uint8& _out) noexcept
	{
		return buf->read(_out);
	}

	sl_bool DeserializeRaw(IReader* reader, void* data, sl_size size) noexcept
	{
		return reader->readFully(data, size) == size;
	}

	sl_bool DeserializeRaw(SerializeBuffer* buf, void* data, sl_size size) noexcept
	{
		return buf->read(data, size) == size;
	}

	sl_bool DeserializeRaw(sl_uint8** _buf, void* data, sl_size size) noexcept
	{
		sl_uint8*& buf = *_buf;
		Base::copyMemory(data, buf, size);
		buf += size;
		return sl_true;
	}

	sl_bool DeserializeRaw(sl_uint8 const** _buf, void* data, sl_size size) noexcept
	{
		sl_uint8 const*& buf = *_buf;
		Base::copyMemory(data, buf, size);
		buf += size;
		return sl_true;
	}

	sl_bool DeserializeRaw(sl_uint8*& buf, void* data, sl_size size) noexcept
	{
		Base::copyMemory(data, buf, size);
		buf += size;
		return sl_true;
	}

	sl_bool DeserializeRaw(sl_uint8 const*& buf, void* data, sl_size size) noexcept
	{
		Base::copyMemory(data, buf, size);
		buf += size;
		return sl_true;
	}

}
