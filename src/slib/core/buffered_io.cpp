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

#include "slib/core/buffered_io.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(BufferedReader, Object)

	BufferedReader::BufferedReader(): m_reader(sl_null), m_closable(sl_null), m_posInBuf(0), m_sizeRead(0), m_dataBuf(sl_null), m_sizeBuf(0)
	{
	}

	BufferedReader::~BufferedReader()
	{
	}

	Ref<BufferedReader> BufferedReader::create(const Ptrx<IReader, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_null;
		}
		Ptrx<IReader, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_null;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_null;
		}

		Ref<BufferedReader> ret = new BufferedReader;
		if (ret.isNotNull()) {
			ret->_init(obj, buf);
			return ret;
		}
		return sl_null;
	}

	sl_reg BufferedReader::read(void* buf, sl_size size)
	{
		if (!size) {
			return 0;
		}
		IReader* reader = m_reader;
		if (!reader) {
			return -1;
		}
		sl_size nAvailable = m_sizeRead - m_posInBuf;
		if (!nAvailable) {
			if (size >= m_sizeBuf) {
				return reader->read(buf, size);
			}
			m_posInBuf = 0;
			sl_reg nRead = reader->read(m_dataBuf, m_sizeBuf);
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

	void BufferedReader::close()
	{
		if (m_closable) {
			m_closable->close();
		}
		m_reader = sl_null;
		m_closable = sl_null;
		m_ref.setNull();
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


	SLIB_DEFINE_OBJECT(BufferedSeekableReader, Object)

	BufferedSeekableReader::BufferedSeekableReader() : m_reader(sl_null), m_seekable(sl_null), m_closable(sl_null), m_posCurrent(0), m_sizeTotal(0), m_posInternal(0), m_dataBuf(sl_null), m_sizeBuf(0), m_sizeRead(0), m_posBuf(0)
	{
	}

	BufferedSeekableReader::~BufferedSeekableReader()
	{
	}

	Ref<BufferedSeekableReader> BufferedSeekableReader::create(const Ptrx<IReader, ISeekable, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_null;
		}
		Ptrx<IReader, ISeekable, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_null;
		}
		ISeekable* seeker = obj;
		if (!seeker) {
			return sl_null;
		}
		sl_uint64 size = seeker->getSize();
		if (!size) {
			return sl_null;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_null;
		}

		Ref<BufferedSeekableReader> ret = new BufferedSeekableReader;
		if (ret.isNotNull()) {
			ret->_init(obj, size, buf);
			return ret;
		}
		return sl_null;
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
		return -1;
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

	sl_reg BufferedSeekableReader::_readInternal(sl_uint64 pos, void* buf, sl_size size)
	{
		if (_seekInternal(pos)) {
			sl_uint64 n = m_sizeTotal - pos;
			if (size > n) {
				size = (sl_size)n;
			}
			if (!size) {
				return 0;
			}
			IReader* reader = m_reader;
			if (reader) {
				sl_reg nRead = reader->read(buf, size);
				if (nRead > 0) {
					m_posInternal += nRead;
				}
				return nRead;
			}
		}
		return -1;
	}

	sl_reg BufferedSeekableReader::_fillBuf(sl_uint64 pos, sl_size size)
	{
		m_posBuf = pos;
		sl_reg nRead = _readInternal(pos, m_dataBuf, size);
		if (nRead > 0) {
			m_sizeRead = nRead;
		} else {
			m_sizeRead = 0;
		}
		return nRead;
	}

	sl_reg BufferedSeekableReader::_fillBuf(sl_uint64 pos)
	{
		return _fillBuf(pos, m_sizeBuf);
	}

	sl_reg BufferedSeekableReader::_readFillingBuf(sl_uint64 pos, void* buf, sl_size size)
	{
		sl_reg nRead = _fillBuf(pos);
		if (nRead > 0) {
			return _readInBuf(buf, size);
		}
		return nRead;
	}

	sl_reg BufferedSeekableReader::read(void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (!size) {
			return 0;
		}
		if (m_posCurrent >= m_sizeTotal) {
			return -1;
		}
		if (!m_sizeRead) {
			return _readFillingBuf(m_posCurrent, buf, size);
		}
		sl_reg nRead = _readInBuf(buf, size);
		if (nRead > 0) {
			return nRead;
		}
		if (m_posCurrent >= m_posBuf) {
			return _readFillingBuf(m_posCurrent, buf, size);
		}
		sl_uint64 _offset = m_posBuf - m_posCurrent;
		if (_offset >= m_sizeBuf) {
			return _readFillingBuf(m_posCurrent, buf, size);
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
			nRead = _fillBuf(m_posBuf - m_sizeBuf);
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
			nRead = _fillBuf(0, pos);
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

	sl_uint64 BufferedSeekableReader::getPosition()
	{
		return m_posCurrent;
	}

	sl_uint64 BufferedSeekableReader::getSize()
	{
		return m_sizeTotal;
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

}
