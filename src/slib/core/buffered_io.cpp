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

	BufferedReader::BufferedReader(): m_reader(sl_null), m_closable(sl_null), m_pos(0), m_count(0)
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
		IReader* reader = obj;
		if (!reader) {
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
		sl_size nAvailable = m_count - m_pos;
		if (!nAvailable) {
			if (size >= m_bufSize) {
				return reader->read(buf, size);
			}
			m_pos = 0;
			sl_reg nRead = reader->read(m_bufData, m_bufSize);
			if (nRead <= 0) {
				m_count = 0;
				return nRead;
			}
			m_count = nRead;
			nAvailable = nRead;
		}
		if (size > nAvailable) {
			size = nAvailable;
		}
		Base::copyMemory(buf, m_bufData + m_pos, size);
		m_pos += size;
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
		m_bufData = (sl_uint8*)(buf.getData());
		m_bufSize = buf.getSize();
	}

}
