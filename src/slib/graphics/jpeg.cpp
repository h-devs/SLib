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

#include "slib/graphics/jpeg.h"

#include "slib/io/memory_reader.h"
#include "slib/io/memory_output.h"
#include "slib/io/file.h"
#include "slib/core/mio.h"
#include "slib/math/math.h"
#include "slib/graphics/image.h"
#include "slib/graphics/yuv.h"

#define HUFFMAN_FAST_BITS SLIB_JPEG_HUFFMAN_FAST_BITS

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegMarker)

	JpegMarker::JpegMarker():
		code(JpegMarkerCode::None),
		size(0)
	{
	}


	JpegHuffmanReader::JpegHuffmanReader(JpegFile* file, IReader* reader) : m_file(file), m_reader(reader)
	{
		restart();
	}

	JpegHuffmanReader::~JpegHuffmanReader()
	{
	}

	sl_uint32 JpegHuffmanReader::read(sl_uint32 len)
	{
		prepare(len);
		return pop(len);
	}

	sl_uint32 JpegHuffmanReader::get(sl_uint32 len)
	{
		return m_buf >> (32 - len);
	}

	void JpegHuffmanReader::remove(sl_uint32 len)
	{
		m_buf <<= len;
		m_len -= len;
	}

	sl_uint32 JpegHuffmanReader::pop(sl_uint32 len)
	{
		sl_uint32 v = get(len);
		remove(len);
		return v;
	}

	sl_int32 JpegHuffmanReader::extendReceive(sl_uint32 len)
	{
		prepare(len);
		sl_int32 sign = ((sl_int32)m_buf) >> 31;
		sl_int32 k = pop(len);
		return k - (((sl_int32)(1 << len) - 1) & (~sign));
	}

	void JpegHuffmanReader::prepare(sl_uint32 length)
	{
		if (m_len >= length) {
			return;
		}
		while (m_len <= 24) {
			sl_uint8 data;
			if (m_flagEnd) {
				data = 0;
			} else {
				data = m_reader->readUint8();
				if (data == 0xff) {
					sl_uint8 c;
					do {
						c = m_reader->readUint8();
					} while (c == 0xff);
					if (c) {
						m_file->setReadMarkerCode((JpegMarkerCode)c);
						m_flagEnd = sl_true;
						return;
					}
				}
			}
			m_buf |= (data << (24 - m_len));
			m_len += 8;
		}
	}

	sl_bool JpegHuffmanReader::decodeBlock(
		sl_int16 _out[64],
		JpegComponent& component,
		JpegHuffmanTable& dc_huffman_table,
		JpegHuffmanTable& ac_huffman_table)
	{
		sl_uint8 t;
		if (!(decode(&t, dc_huffman_table))) {
			return sl_false;
		}
		Base::zeroMemory(_out, 128);
		sl_int32 diff = t ? extendReceive(t) : 0;
		sl_int32 dc = component.dc_prediction + diff;
		component.dc_prediction = dc;
		_out[0] = (sl_int16)(dc);
		// decode AC components
		sl_uint32 k = 1;
		do {
			prepare(16);
			sl_int32 c = get(HUFFMAN_FAST_BITS);
			sl_int32 r = ac_huffman_table.fast_ac[c];
			if (r) { // fast-AC path
				k += (r >> 4) & 15; // run
				sl_uint32 s = r & 15; // combined length
				remove(s);
				_out[k] = (sl_int16)(r >> 8);
				k++;
			} else {
				sl_uint8 rs;
				if (!(decode(&rs, ac_huffman_table))) {
					return sl_false;
				}
				sl_uint32 s = rs & 15;
				r = rs >> 4;
				if (s) {
					k += r;
					_out[k] = (sl_int16)(extendReceive(s));
					k++;
				} else {
					if (rs != 0xf0) {
						break;
					}
					k += 16;
				}
			}
		} while (k < 64);
		return sl_true;
	}

	sl_bool JpegHuffmanReader::decode(sl_uint8* _out, JpegHuffmanTable& table)
	{
		prepare(16);
		sl_uint32 c = get(HUFFMAN_FAST_BITS);
		sl_uint32 k = table.fast[c];
		if (k < 255) {
			sl_uint32 size = table.size[k];
			if (size > m_len) {
				return sl_false;
			}
			remove(size);
			*_out = table.values[k];
			return sl_true;
		}
		sl_uint32 temp = get(16);
		for (k = HUFFMAN_FAST_BITS + 1; ; k++) {
			if (temp < table.max_code[k]) {
				break;
			}
		}
		if (k == 17) {
			remove(16);
			return sl_false;
		}
		if (k > m_len) {
			return sl_false;
		}
		// convert the huffman code to the symbol id
		c = pop(k) + table.delta[k];
		if (c >= 256) {
			return sl_false;
		}
		*_out = table.values[c];
		return sl_true;
	}

	sl_bool JpegHuffmanReader::isEnd()
	{
		return m_flagEnd;
	}

	void JpegHuffmanReader::restart()
	{
		m_buf = 0;
		m_len = 0;
		m_flagEnd = sl_false;
	}


	namespace {
		
		class EncodeContext
		{
		public:
			sl_uint8 category[65535];
			JpegHuffmanEncodeItem bitcode[65535];
			sl_bool flagInit;

		public:
			void initialize()
			{
				if (flagInit) {
					return;
				}
				sl_int32 lower = 1;
				sl_int32 upper = 2;
				for (sl_int32 cat = 1; cat <= 15; cat++) {
					sl_int32 v;
					for (v = lower; v < upper; v++) {
						category[32767 + v] = cat;
						bitcode[32767 + v].size = cat;
						bitcode[32767 + v].code = v;
					}
					for (v = -(upper - 1); v <= -lower; v++) {
						category[32767 + v] = cat;
						bitcode[32767 + v].size = cat;
						bitcode[32767 + v].code = upper - 1 + v;
					}
					lower <<= 1;
					upper <<= 1;
				}
				flagInit = sl_true;
			}

		};

		static EncodeContext g_encodeContext = { 0 };

	}

	JpegHuffmanWriter::JpegHuffmanWriter(JpegFile* file, IWriter* writer) : m_file(file), m_writer(writer)
	{
		g_encodeContext.initialize();
		restart();
	}

	JpegHuffmanWriter::~JpegHuffmanWriter()
	{
	}

	sl_bool JpegHuffmanWriter::encodeBlock(
		sl_int16 data[64],
		JpegComponent& component,
		JpegHuffmanTable& dc_huffman_table,
		JpegHuffmanTable& ac_huffman_table)
	{
		JpegHuffmanEncodeItem endOfBlock = ac_huffman_table.getEncodeItem(0);
		JpegHuffmanEncodeItem zero16 = ac_huffman_table.getEncodeItem(0xF0);

		sl_int32 diff = data[0] - component.dc_Wprediction;
		component.dc_Wprediction = data[0];
		//Encode DC
		if (diff) {
			sl_int32 pos = 32767 + diff;
			writeBits(dc_huffman_table.getEncodeItem(g_encodeContext.category[pos]));
			writeBits(g_encodeContext.bitcode[pos]);
		} else {
			writeBits(dc_huffman_table.getEncodeItem(0)); // Diff might be 0
		}
		//Encode ACs
		sl_int16 posNotZero = 63; // first element in reverse order which is not zero
		while (posNotZero) {
			if (data[posNotZero]) {
				break;
			}
			posNotZero--;
		}
		if (!posNotZero) {
			writeBits(endOfBlock);
			return sl_true;
		}
		sl_int16 i = 1;
		while (i <= posNotZero) {
			sl_int16 value;
			sl_uint32 nZeros = 0;
			while (i <= posNotZero) {
				value = data[i];
				if (value) {
					break;
				} else {
					nZeros++;
					i++;
				}
			}
			if (nZeros >= 16) {
				sl_int32 n = nZeros >> 4;
				for (sl_int16 i = 0; i < n; i++) {
					writeBits(zero16);
				}
				nZeros &= 15;
			}
			sl_int32 pos = 32767 + data[i];
			writeBits(ac_huffman_table.getEncodeItem((nZeros << 4) + g_encodeContext.category[pos]));
			writeBits(g_encodeContext.bitcode[pos]);
			i++;
		}
		if (posNotZero != 63) {
			writeBits(endOfBlock);
		}
		return sl_true;
	}

	void JpegHuffmanWriter::writeBits(const JpegHuffmanEncodeItem& bs)
	{
		sl_uint16 value = bs.code;
		sl_uint8 pos = bs.size;
		while (pos) {
			pos--;
			if (value & (1 << pos)) {
				m_buf |= (1 << m_len);
			}
			if (m_len) {
				m_len--;
			} else {
				if (m_buf == 0xFF) {
					m_writer->writeInt8((sl_uint8)0xFF);
					m_writer->writeInt8(0);
				} else {
					m_writer->writeInt8(m_buf);
				}
				restart();
			}
		}
	}

	void JpegHuffmanWriter::flush()
	{
		JpegHuffmanEncodeItem item;
		item.size = m_len + 1;
		item.code = (sl_int16)((1 << (m_len + 1)) - 1);
		writeBits(item);
	}

	void JpegHuffmanWriter::restart()
	{
		m_buf = 0;
		m_len = 7;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegAdobeSegment)

	JpegAdobeSegment::JpegAdobeSegment():
		version(0),
		flags0(0),
		flags1(0),
		color_transform(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegQuantizationTable)

	JpegQuantizationTable::JpegQuantizationTable(): flagDefined(sl_false)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegHuffmanTable)

	JpegHuffmanTable::JpegHuffmanTable(): flagDefined(sl_false)
	{
	}

	sl_bool JpegHuffmanTable::build()
	{
		JpegHuffmanTable& table = *this;
		sl_uint32 i, j;
		sl_uint32 k = 0;
		for (i = 0; i < 16; i++) {
			for (j = 0; j < table.bits[i]; j++) {
				table.size[k++] = (sl_uint8)(i + 1);
			}
		}
		table.size[k] = 0;
		table.count = k;

		sl_uint32 code = 0;
		k = 0;
		for (j = 1; j <= 16; j++) {
			table.delta[j] = k - code;
			if (table.size[k] == j) {
				do {
					table.code[k++] = (sl_uint16)(code++);
				} while (table.size[k] == j);
				if (code > (sl_uint32)(1 << j)) {
					return sl_false;
				}
			}
			table.max_code[j] = code << (16 - j);
			code <<= 1;
		}
		table.max_code[17] = 0xffffffff;

		Base::resetMemory(table.fast, 1 << HUFFMAN_FAST_BITS, 255);
		for (i = 0; i < k; i++) {
			sl_uint8 s = table.size[i];
			if (s <= HUFFMAN_FAST_BITS) {
				sl_uint32 c = table.code[i] << (HUFFMAN_FAST_BITS - s);
				sl_uint32 m = 1 << (HUFFMAN_FAST_BITS - s);
				for (j = 0; j < m; j++) {
					table.fast[c + j] = (sl_uint8)i;
				}
			}
		}
		return sl_true;
	}

	void JpegHuffmanTable::buildFastAC()
	{
		JpegHuffmanTable& table = *this;
		sl_uint32 n = 1 << HUFFMAN_FAST_BITS;
		for (sl_uint32 i = 0; i < n; i++) {
			sl_uint8 v = table.fast[i];
			table.fast_ac[i] = 0;
			if (v < 255) {
				sl_int32 rs = table.values[v];
				sl_int32 run = (rs >> 4) & 15;
				sl_int32 magbits = rs & 15;
				sl_int32 len = table.size[v];
				if (magbits && len + magbits <= HUFFMAN_FAST_BITS) {
					sl_int32 k = ((i << len) & (n - 1)) >> (HUFFMAN_FAST_BITS - magbits);
					sl_int32 m = 1 << (magbits - 1);
					if (k < m) {
						k += ((sl_uint32)(0xffffffff) << magbits) + 1;
					}
					if (k >= -128 && k <= 127) {
						table.fast_ac[i] = (sl_int16)((k << 8) + (run << 4) + (len + magbits));
					}
				}
			}
		}
	}

	void JpegHuffmanTable::buildEncodeItems()
	{
		for (sl_uint8 p = 0; p < count; p++) {
			sl_uint8 v = values[p];
			encode_code[v] = code[p];
			encode_size[v] = size[p];
		}
	}

	JpegHuffmanEncodeItem JpegHuffmanTable::getEncodeItem(sl_int16 index)
	{
		return {encode_code[index], encode_size[index]};
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegComponent)

	JpegComponent::JpegComponent():
		id(0),
		horizontal_sample_factor(0),
		vertical_sample_factor(0),
		quant_table_no(0),
		dc_prediction(0),
		dc_Wprediction(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegFrameHeader)

	JpegFrameHeader::JpegFrameHeader():
		flagBaseline(sl_false),
		flagProgressive(sl_false),
		flagArithmetic(sl_false),
		precision(0),
		width(0),
		height(0),
		nComponents(0),

		horizontal_sample_factor_max(0),
		vertical_sample_factor_max(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegScanHeader)

	JpegScanHeader::JpegScanHeader() :
		nComponents(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JpegFile)

	JpegFile::JpegFile()
	{
		flagJFIF = sl_false;
		restart_interval = 0;

		flagJFIF = sl_false;

		m_flagReadFully = sl_true;

		m_flagReadMarkerCode = sl_false;
		m_lastMarkerCode = JpegMarkerCode::None;
		m_nRestartCountDown = 0x7fffffff;
	}

	void JpegFile::setReadFully(sl_bool flag)
	{
		m_flagReadFully = flag;
	}

	sl_bool JpegFile::setReader(const Ptrx<IReader, ISeekable>& reader)
	{
		return m_reader.setReader(reader);
	}

	namespace {

		static sl_bool IsSOF(JpegMarkerCode code)
		{
			switch (code) {
				case JpegMarkerCode::SOF0:
				case JpegMarkerCode::SOF1:
				case JpegMarkerCode::SOF2:
				case JpegMarkerCode::SOF9:
				case JpegMarkerCode::SOF10:
				case JpegMarkerCode::SOF3:
				case JpegMarkerCode::SOF5:
				case JpegMarkerCode::SOF6:
				case JpegMarkerCode::SOF7:
				case JpegMarkerCode::JPG:
				case JpegMarkerCode::SOF11:
				case JpegMarkerCode::SOF13:
				case JpegMarkerCode::SOF14:
				case JpegMarkerCode::SOF15:
					return sl_true;
				default:
					break;
			}
			return sl_false;
		}

		static sl_bool IsSupportedSOF(JpegMarkerCode code)
		{
			switch (code) {
				case JpegMarkerCode::SOF0:
				case JpegMarkerCode::SOF1:
				case JpegMarkerCode::SOF2:
				case JpegMarkerCode::SOF9:
				case JpegMarkerCode::SOF10:
					return sl_true;
				default:
					break;
			}
			return sl_false;
		}

	}

	sl_bool JpegFile::readHeader()
	{
		if (readMarkerCode() != JpegMarkerCode::SOI) {
			return sl_false;
		}
		for (;;) {
			JpegMarker marker; 
			if (readMarker(marker)) {
				if (IsSOF(marker.code)) {
					if (IsSupportedSOF(marker.code)) {
						return sl_true;
					}
					break;
				}
			} else {
				break;
			}
		}
		return sl_false;
	}

	sl_bool JpegFile::readContent()
	{
		for (;;) {
			JpegMarker marker;
			if (readMarker(marker)) {
				if (marker.code == JpegMarkerCode::EOI) {
					break;
				}
				if (marker.code == JpegMarkerCode::SOS) {
					if (onReachedScanData.isNotNull()) {
						onReachedScanData();
					}
					if (!(readScanData())) {
						return sl_false;
					}
				}
			} else {
				return sl_false;
			}
		}
		return sl_true;
	}

	JpegMarkerCode JpegFile::readMarkerCode()
	{
		if (m_flagReadMarkerCode) {
			m_flagReadMarkerCode = sl_false;
			return m_lastMarkerCode;
		}
		sl_uint8 code;
		if (!(m_reader.readUint8(&code))) {
			return JpegMarkerCode::None;
		}
		if (code != 0xff) {
			return JpegMarkerCode::None;
		}
		while (m_reader.readUint8(&code)) {
			if (code != 0xff) {
				m_lastMarkerCode = (JpegMarkerCode)code;
				return (JpegMarkerCode)code;
			}
		}
		return JpegMarkerCode::None;
	}

	void JpegFile::setReadMarkerCode(JpegMarkerCode code)
	{
		m_lastMarkerCode = code;
		m_flagReadMarkerCode = sl_true;
	}

	JpegMarkerCode JpegFile::getLastMarkerCode()
	{
		return m_lastMarkerCode;
	}

	sl_bool JpegFile::readMarker(JpegMarker& marker)
	{
		JpegMarkerCode code = readMarkerCode();
		if (code == JpegMarkerCode::SOI || code == JpegMarkerCode::EOI) {
			marker.code = code;
			return sl_true;
		}
		sl_uint16 size;
		if (!(m_reader.readUint16(&size, EndianType::Big))) {
			return sl_false;
		}
		if (size < 2) {
			return sl_false;
		}
		marker.code = code;
		marker.size = size;
		size -= 2;
		if (m_flagReadFully) {
			Memory mem = m_reader.readToMemory(size);
			if (mem.isNull()) {
				return sl_false;
			}
			marker.content = mem;
			MemoryReader reader(mem);
			if (readMarkerContent(&reader, marker, size)) {
				if (markers.add_NoLock(marker)) {
					return sl_true;
				}
			}
		} else {
			sl_uint64 posStart = m_reader.getPosition();
			if (readMarkerContent(&m_reader, marker, size)) {
				sl_uint64 posCurrent = m_reader.getPosition();
				sl_uint64 posEnd = posStart + size - 2;
				if (posCurrent >= posStart && posCurrent <= posEnd) {
					if (posCurrent < posEnd) {
						if (!(m_reader.skip(posEnd - posCurrent))) {
							return sl_false;
						}
					}
					if (markers.add_NoLock(marker)) {
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool JpegFile::readMarkerContent(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		switch (marker.code) {
		case JpegMarkerCode::APP0:
			return readJFIF(reader, marker, size);
		case JpegMarkerCode::APP14:
			return readAdobeSegment(reader, marker, size);
		case JpegMarkerCode::DQT:
			return readDQT(reader, marker, size);
		case JpegMarkerCode::SOF0:
		case JpegMarkerCode::SOF1:
		case JpegMarkerCode::SOF2:
		case JpegMarkerCode::SOF9:
		case JpegMarkerCode::SOF10:
			return readSOF(reader, marker, size);
		case JpegMarkerCode::DHT:
			return readDHT(reader, marker, size);
		case JpegMarkerCode::SOS:
			return readSOS(reader, marker, size);
		case JpegMarkerCode::DRI:
			return readDRI(reader, marker, size);
		default:
			break;
		}
		return sl_true;
	}

	sl_bool JpegFile::readJFIF(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		char sig[5];
		if (reader->readFully(sig, 5) != 5) {
			return sl_false;
		}
		if (sig[0] == 'J' && sig[1] == 'F' && sig[2] == 'I' && sig[3] == 'F' && sig[4] == 0) {
			flagJFIF = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool JpegFile::readAdobeSegment(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		char sig[6];
		if (reader->readFully(sig, 6) != 6) {
			return sl_false;
		}
		if (sig[0] == 'A' && sig[1] == 'd' && sig[2] == 'o' && sig[3] == 'b' && sig[4] == 'e' && sig[5] == 0) {
			if (!(reader->readUint8(&(adobe_segment.version)))) {
				return sl_false;
			}
			if (!(reader->readUint16(&(adobe_segment.flags0), EndianType::Big))) {
				return sl_false;
			}
			if (!(reader->readUint16(&(adobe_segment.flags1), EndianType::Big))) {
				return sl_false;
			}
			if (!(reader->readUint8(&(adobe_segment.color_transform)))) {
				return sl_false;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool JpegFile::readDQT(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		sl_uint32 nRead = 0;
		while (nRead < size) {
			sl_uint8 t;
			if (!(reader->readUint8(&t))) {
				return sl_false;
			}
			nRead++;
			sl_uint8 flag16 = t >> 4;
			if (flag16 > 1) {
				return sl_false;
			}
			sl_uint8 index = t & 15;
			if (index > SLIB_JPEG_QUANTIZATION_TABLES_COUNT) {
				return sl_false;
			}
			JpegQuantizationTable& table = quantization_table[index];
			sl_uint8 buf[128];
			sl_uint32 n = flag16 ? 128 : 64;
			if (reader->readFully(buf, n) != n) {
				return sl_false;
			}
			nRead += n;
			table.flag_16bit = flag16;
			table.index = index;
			for (sl_uint32 i = 0; i < 64; i++) {
				table.quant[i] = flag16 ? MIO::readUint16BE(buf + (i << 1)) : buf[i];
			}
			table.flagDefined = sl_true;
		}
		return sl_true;
	}

	sl_bool JpegFile::readSOF(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		switch (marker.code) {
		case JpegMarkerCode::SOF0:
			frame_header.flagBaseline = sl_true;
			break;
		case JpegMarkerCode::SOF1:
			break;
		case JpegMarkerCode::SOF2:
			frame_header.flagProgressive = sl_true;
			break;
		case JpegMarkerCode::SOF9:
			frame_header.flagArithmetic = sl_true;
			break;
		case JpegMarkerCode::SOF10:
			frame_header.flagArithmetic = sl_true;
			frame_header.flagProgressive = sl_true;
			break;
		default:
			// Not supported
			return sl_false;
		}

		if (!(reader->readUint8(&(frame_header.precision)))) {
			return sl_false;
		}
		if (frame_header.precision != 8) {
			// Not supported
			return sl_false;
		}
		if (!(reader->readUint16(&(frame_header.height), EndianType::Big))) {
			return sl_false;
		}
		if (!(frame_header.height)) {
			return sl_false;
		}
		if (!(reader->readUint16(&(frame_header.width), EndianType::Big))) {
			return sl_false;
		}
		if (!(frame_header.width)) {
			return sl_false;
		}
		if (!(reader->readUint8(&(frame_header.nComponents)))) {
			return sl_false;
		}
		if (frame_header.nComponents != 1 && frame_header.nComponents != 3 && frame_header.nComponents != 4) {
			// Not supported
			return sl_false;
		}

		sl_uint32 i;
		for (i = 0; i < frame_header.nComponents; i++) {
			JpegComponent& comp = frame_header.components[i];
			if (!(reader->readUint8(&(comp.id)))) {
				return sl_false;
			}
			sl_uint8 t;
			if (!(reader->readUint8(&t))) {
				return sl_false;
			}
			comp.horizontal_sample_factor = t >> 4;
			if (comp.horizontal_sample_factor <= 0 || comp.horizontal_sample_factor > 4) {
				return sl_false;
			}
			comp.vertical_sample_factor = t & 15;
			if (comp.vertical_sample_factor <= 0 || comp.vertical_sample_factor > 4) {
				return sl_false;
			}
			if (!(reader->readUint8(&(comp.quant_table_no)))) {
				return sl_false;
			}
			if (comp.quant_table_no > 3) {
				return sl_false;
			}

			if (comp.horizontal_sample_factor > frame_header.horizontal_sample_factor_max) {
				frame_header.horizontal_sample_factor_max = comp.horizontal_sample_factor;
			}
			if (comp.vertical_sample_factor > frame_header.vertical_sample_factor_max) {
				frame_header.vertical_sample_factor_max = comp.vertical_sample_factor;
			}
		}
		return sl_true;
	}

	sl_bool JpegFile::readDHT(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		sl_uint32 nRead = 0;
		while (nRead < size) {
			sl_uint8 t;
			if (!(reader->readUint8(&t))) {
				return sl_false;
			}
			nRead++;
			sl_uint8 flagAC = t >> 4;
			if (flagAC > 1) {
				return sl_false;
			}
			sl_uint8 index = t & 15;
			if (index > SLIB_JPEG_HUFFMAN_TABLES_COUNT) {
				return sl_false;
			}

			JpegHuffmanTable* pTable;
			if (flagAC) {
				pTable = ac_huffman_tables + index;
			} else {
				pTable = dc_huffman_tables + index;
			}
			JpegHuffmanTable& table = *pTable;
			table.flagAC = flagAC;

			if (reader->readFully(table.bits, 16) != 16) {
				return sl_false;
			}
			nRead += 16;
			sl_uint32 nTotal = 0;
			for (sl_uint32 i = 0; i < 16; i++) {
				nTotal += table.bits[i];
			}
			if (nTotal > 256) {
				return sl_false;
			}
			if (reader->readFully(table.values, nTotal) != nTotal) {
				return sl_false;
			}
			nRead += nTotal;

			table.build();
			table.buildEncodeItems();
			if (flagAC) {
				table.buildFastAC();
			}

			table.flagDefined = sl_true;
		}
		return sl_true;
	}

	sl_bool JpegFile::readSOS(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		if (!(reader->readUint8(&(scan_header.nComponents)))) {
			return sl_false;
		}
		if (scan_header.nComponents > 4) {
			return sl_false;
		}
		for (sl_uint8 i = 0; i < scan_header.nComponents; i++) {
			JpegScanComponent& comp = scan_header.components[i];
			if (!(reader->readUint8(&(comp.id)))) {
				return sl_false;
			}
			sl_uint8 t;
			if (!(reader->readUint8(&t))) {
				return sl_false;
			}
			comp.dc_huffman_table_no = t >> 4;
			if (comp.dc_huffman_table_no > SLIB_JPEG_HUFFMAN_TABLES_COUNT) {
				return sl_false;
			}
			comp.ac_huffman_table_no = t & 15;
			if (comp.ac_huffman_table_no > SLIB_JPEG_HUFFMAN_TABLES_COUNT) {
				return sl_false;
			}

			comp.index = 4;
			for (sl_uint8 k = 0; k < frame_header.nComponents; k++) {
				if (comp.id == frame_header.components[k].id) {
					comp.index = k;
					break;
				}
			}
			if (comp.index >= 4) {
				return sl_false;
			}
		}
		if (!(reader->readUint8(&(scan_header.spec_start)))) {
			return sl_false;
		}
		if (!(reader->readUint8(&(scan_header.spec_end)))) {
			return sl_false;
		}
		sl_uint8 t;
		if (!(reader->readUint8(&t))) {
			return sl_false;
		}
		scan_header.succ_high = t >> 4;
		scan_header.succ_low = t & 15;
		return sl_true;
	}

	sl_bool JpegFile::readDRI(IReader* reader, JpegMarker& marker, sl_uint32 size)
	{
		if (size != 2) {
			return sl_false;
		}
		if (!(reader->readUint16(&restart_interval, Endian::Big))) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool JpegFile::readScanData()
	{
		if (frame_header.flagArithmetic || frame_header.flagProgressive) {
			// Not Supported
			return sl_false;
		}

		JpegHuffmanReader reader(this, &m_reader);
		restartDecoder(&reader);

		sl_uint8 h_max = frame_header.horizontal_sample_factor_max;
		sl_uint8 v_max = frame_header.vertical_sample_factor_max;
		sl_uint32 mcu_w = h_max << 3;
		sl_uint32 mcu_h = v_max << 3;
		sl_uint32 mcu_count_x = (frame_header.width + mcu_w - 1) / mcu_w;
		sl_uint32 mcu_count_y = (frame_header.height + mcu_h - 1) / mcu_h;

		sl_int16 data[64];
		sl_int16 dataz[64];
		sl_uint8 colors[64];
		for (sl_uint32 mcu_y = 0; mcu_y < mcu_count_y; mcu_y++) {
			for (sl_uint32 mcu_x = 0; mcu_x < mcu_count_x; mcu_x++) {
				for (sl_uint8 iComp = 0; iComp < scan_header.nComponents; iComp++) {
					sl_uint8 index = scan_header.components[iComp].index;
					JpegComponent& comp = frame_header.components[index];
					sl_uint8 ac_huffman_table_no = scan_header.components[iComp].ac_huffman_table_no;
					if (!(ac_huffman_tables[ac_huffman_table_no].flagDefined)) {
						return sl_false;
					}
					sl_uint8 dc_huffman_table_no = scan_header.components[iComp].dc_huffman_table_no;
					if (!(dc_huffman_tables[dc_huffman_table_no].flagDefined)) {
						return sl_false;
					}
					if (!(quantization_table[comp.quant_table_no].flagDefined)) {
						return sl_false;
					}
					sl_uint32 tx0 = mcu_x * comp.horizontal_sample_factor;
					sl_uint32 ty0 = mcu_y * comp.vertical_sample_factor;
					for (sl_uint8 ty = 0; ty < comp.vertical_sample_factor; ty++) {
						for (sl_uint8 tx = 0; tx < comp.horizontal_sample_factor; tx++) {
							if (!(reader.decodeBlock(data, comp, dc_huffman_tables[dc_huffman_table_no], ac_huffman_tables[ac_huffman_table_no]))) {
								return sl_false;
							}
							Result result = Result::OK;
							if (onDecodeHuffmanBlock.isNotNull()) {
								result = onDecodeHuffmanBlock(iComp, data, comp, dc_huffman_tables[dc_huffman_table_no], ac_huffman_tables[ac_huffman_table_no]);
							}
							if (result == Result::Finish) {
								return sl_true;
							} else if (result == Result::OK) {
								dequantizeBlock(data, quantization_table[comp.quant_table_no]);
								dezigzag(data, dataz);
								idctBlock(dataz, colors);
								onLoadBlock(tx0 + tx, ty0 + ty, iComp, colors);
							}
						}
					}
				}
				if (!(controlRestartInterval(&reader))) {
					return sl_false;
				}
				if (getLastMarkerCode() == JpegMarkerCode::EOI) {
					setReadMarkerCode(JpegMarkerCode::EOI);
					return sl_true;
				}
			}
		}
		return sl_true;
	}

	sl_bool JpegFile::controlRestartInterval(JpegHuffmanReader* reader)
	{
		m_nRestartCountDown--;
		if (m_nRestartCountDown > 0) {
			return sl_true;
		} else {
			if (reader->isEnd()) {
				JpegMarkerCode code = readMarkerCode();
				if (code >= JpegMarkerCode::RST0 && code <= JpegMarkerCode::RST7) {
					restartDecoder(reader);
					return sl_true;
				} else if (code == JpegMarkerCode::EOI) {
					return sl_true;
				}
			}
			return sl_false;
		}
	}

	void JpegFile::restartDecoder(JpegHuffmanReader* reader)
	{
		reader->restart();
		for (sl_uint32 i = 0; i < 4; i++) {
			frame_header.components[i].dc_prediction = 0;
			frame_header.components[i].dc_Wprediction = 0;
		}
		m_nRestartCountDown = restart_interval ? restart_interval : 0x7fffffff;
		if (!onDecodeRestartControl.isNull()) {
			onDecodeRestartControl(m_nRestartCountDown);
		}
	}

	void JpegFile::zigzag(sl_int16 input[64], sl_int16 output[64])
	{
		static const sl_uint8 table[64] = {
			0,  1,  5,  6,  14, 15, 27, 28,
			2,  4,  7,  13, 16, 26, 29, 42,
			3,  8,  12, 17, 25, 30, 41, 43,
			9,  11, 18, 24, 31, 40, 44, 53,
			10, 19, 23, 32, 39, 45, 52, 54,
			20, 22, 33, 38, 46, 51, 55, 60,
			21, 34, 37, 47, 50, 56, 59, 61,
			35, 36, 48, 49, 57, 58, 62, 63
		};
		for (sl_uint32 i = 0; i < 64; i++) {
			output[table[i]] = input[i];
		}
	}

	void JpegFile::dezigzag(sl_int16 input[64], sl_int16 output[64])
	{
		static const sl_uint8 table[64 + 15] = {
			0,  1,  8, 16,  9,  2,  3, 10,
			17, 24, 32, 25, 18, 11,  4,  5,
			12, 19, 26, 33, 40, 48, 41, 34,
			27, 20, 13,  6,  7, 14, 21, 28,
			35, 42, 49, 56, 57, 50, 43, 36,
			29, 22, 15, 23, 30, 37, 44, 51,
			58, 59, 52, 45, 38, 31, 39, 46,
			53, 60, 61, 54, 47, 55, 62, 63,
			// let corrupt input sample past end
			63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63
		};
		for (sl_uint32 i = 0; i < 64; i++) {
			output[table[i]] = input[i];
		}
	}

	void JpegFile::quantizeBlock(sl_int16 data[64], JpegQuantizationTable& table)
	{
		for (sl_uint32 i = 0; i < 64; i++) {
			data[i] /= (sl_int16)(table.quant[i]);
		}
	}

	void JpegFile::dequantizeBlock(sl_int16 data[64], JpegQuantizationTable& table)
	{
		for (sl_uint32 i = 0; i < 64; i++) {
			data[i] *= (sl_int16)(table.quant[i]);
		}
	}

#define F2I_ROUND(x)  ((sl_int32)(((x) * 4096 + 0.5)))
#define FSH(x)  ((sl_int32)((x) * 4096))

#define IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \
		sl_int32 t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
		p2 = s2; \
		p3 = s6; \
		p1 = (p2+p3) * F2I_ROUND(0.5411961f); \
		t2 = p1 + p3*F2I_ROUND(-1.847759065f); \
		t3 = p1 + p2*F2I_ROUND( 0.765366865f); \
		p2 = s0; \
		p3 = s4; \
		t0 = FSH(p2+p3); \
		t1 = FSH(p2-p3); \
		x0 = t0+t3; \
		x3 = t0-t3; \
		x1 = t1+t2; \
		x2 = t1-t2; \
		t0 = s7; \
		t1 = s5; \
		t2 = s3; \
		t3 = s1; \
		p3 = t0+t2; \
		p4 = t1+t3; \
		p1 = t0+t3; \
		p2 = t1+t2; \
		p5 = (p3+p4)*F2I_ROUND( 1.175875602f); \
		t0 = t0*F2I_ROUND( 0.298631336f); \
		t1 = t1*F2I_ROUND( 2.053119869f); \
		t2 = t2*F2I_ROUND( 3.072711026f); \
		t3 = t3*F2I_ROUND( 1.501321110f); \
		p1 = p5 + p1*F2I_ROUND(-0.899976223f); \
		p2 = p5 + p2*F2I_ROUND(-2.562915447f); \
		p3 = p3*F2I_ROUND(-1.961570560f); \
		p4 = p4*F2I_ROUND(-0.390180644f); \
		t3 += p1+p4; \
		t2 += p2+p3; \
		t1 += p2+p4; \
		t0 += p1+p3;

	void JpegFile::idctBlock(sl_int16 d[64], sl_uint8 _out[64])
	{
		sl_int32 val[64];
		sl_uint32 i;
		sl_uint8 *o = _out;
		sl_int32* v = val;

		for (i = 0; i < 8; i++) {
			if (!(d[8]) && !(d[16]) && !(d[24]) && !(d[32]) && !(d[40]) && !(d[48]) && !(d[56])) {
				v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = (sl_uint32)(d[0]) << 2;
			} else {
				IDCT_1D(d[0], d[8], d[16], d[24], d[32], d[40], d[48], d[56])
				x0 += 512; x1 += 512; x2 += 512; x3 += 512;
				v[0] = (x0 + t3) >> 10;
				v[56] = (x0 - t3) >> 10;
				v[8] = (x1 + t2) >> 10;
				v[48] = (x1 - t2) >> 10;
				v[16] = (x2 + t1) >> 10;
				v[40] = (x2 - t1) >> 10;
				v[24] = (x3 + t0) >> 10;
				v[32] = (x3 - t0) >> 10;
			}
			d++;
			v++;
		}
		v = val;
		for (i = 0; i < 8; i++) {
			IDCT_1D(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7])
			x0 += 65536 + (128 << 17);
			x1 += 65536 + (128 << 17);
			x2 += 65536 + (128 << 17);
			x3 += 65536 + (128 << 17);
			o[0] = Math::clamp0_255((x0 + t3) >> 17);
			o[7] = Math::clamp0_255((x0 - t3) >> 17);
			o[1] = Math::clamp0_255((x1 + t2) >> 17);
			o[6] = Math::clamp0_255((x1 - t2) >> 17);
			o[2] = Math::clamp0_255((x2 + t1) >> 17);
			o[5] = Math::clamp0_255((x2 - t1) >> 17);
			o[3] = Math::clamp0_255((x3 + t0) >> 17);
			o[4] = Math::clamp0_255((x3 - t0) >> 17);
			v += 8;
			o += 8;
		}
	}

	Ref<Image> Jpeg::load(const Ptrx<IReader, ISeekable>& reader)
	{
		JpegFile file;

		file.setReader(reader);

		if (file.readHeader()) {

			sl_uint32 width = file.frame_header.width;
			sl_uint32 height = file.frame_header.height;

			if (width && height) {

				sl_uint32 mcu_w = file.frame_header.horizontal_sample_factor_max << 3;
				sl_uint32 mcu_h = file.frame_header.vertical_sample_factor_max << 3;
				sl_uint32 nx = (width + mcu_w - 1) / mcu_w * mcu_w;
				sl_uint32 ny = (height + mcu_h - 1) / mcu_h * mcu_h;

				Ref<Image> image = Image::create(nx, ny);
				if (image.isNotNull()) {

					ImageDesc desc;
					image->getDesc(desc);
					Color* colors = desc.colors;
					sl_reg stride = desc.stride;

					file.onLoadBlock = [&file, stride, colors](sl_uint32 ix, sl_uint32 iy, sl_uint8 index, sl_uint8* src) {
						JpegComponent& comp = file.frame_header.components[index];
						sl_uint32 fx = file.frame_header.horizontal_sample_factor_max;
						sl_uint32 fy = file.frame_header.vertical_sample_factor_max;
						sl_reg pitch = stride << 2;
						if (comp.horizontal_sample_factor == fx && comp.vertical_sample_factor == fy) {
							Color* p = colors + (ix << 3) + (iy << 3) * stride;
							sl_uint8* dst = ((sl_uint8*)p) + index;
							for (sl_uint32 y = 0; y < 8; y++) {
								sl_uint8* d = dst;
								sl_uint8* s = src;
								for (sl_uint32 x = 0; x < 8; x++) {
									*d = *s;
									d += 4;
									s++;
								}
								dst += pitch;
								src += 8;
							}
						} else if (comp.horizontal_sample_factor == 1 && comp.vertical_sample_factor == 1) {
							Color* p = colors + (ix << 3) * fx + (iy << 3) * fy * stride;
							sl_uint8* dst = ((sl_uint8*)p) + index;
							for (sl_uint32 y = 0; y < 8; y++) {
								sl_uint8* d = dst;
								sl_uint8* s = src;
								for (sl_uint32 x = 0; x < 8; x++) {
									sl_uint8* dst_sub = d;
									sl_uint8 v = *s;
									for (sl_uint32 ty = 0; ty < fy; ty++) {
										sl_uint8* d_sub = dst_sub;
										for (sl_uint32 tx = 0; tx < fx; tx++) {
											*d_sub = v;
											d_sub += 4;
										}
										dst_sub += pitch;
									}
									d += (fx << 2);
									s++;
								}
								dst += pitch * fy;
								src += 8;
							}
						}
					};
					if (file.readContent()) {
						for (sl_uint32 y = 0; y < height; y++) {
							Color* c = colors;
							for (sl_uint32 x = 0; x < width; x++) {
								YUV::convertYUVToRGB(c->r, c->g, c->b, c->r, c->g, c->b);
								c->a = 255;
								c++;
							}
							colors += stride;
						}
						return image->sub(0, 0, width, height);
					}
				}
			}
		}
		return sl_null;
	}

	Ref<Image> Jpeg::loadFromMemory(const void* mem, sl_size size)
	{
		MemoryReader reader(mem, size);
		return load(&reader);
	}

	Ref<Image> Jpeg::loadFromMemory(const MemoryView& mem)
	{
		return loadFromMemory(mem.data, mem.size);
	}

	Ref<Image> Jpeg::loadFromFile(const StringParam& path)
	{
		SeekableReader<File> file = File::openForRead(path);
		if (file.isOpened()) {
			return load(&file);
		}
		return sl_null;
	}

	sl_uint32 Jpeg::getBlockCount(const Ptrx<IReader, ISeekable>& reader, sl_uint32 _colorIndex)
	{
		sl_uint32 n = 0;
		if (loadHuffmanBlocks(reader, [&n, _colorIndex](sl_uint32 colorIndex, sl_int16*, sl_bool&) {
			if (colorIndex == _colorIndex) {
				n++;
			}
		})) {
			return n;
		}
		return 0;
	}

	sl_bool Jpeg::loadHuffmanBlocks(const Ptrx<IReader, ISeekable>& reader, const Function<void(sl_uint32 colorIndex, sl_int16 data[64], sl_bool& outFlagFinish)>& onLoadBlock)
	{
		JpegFile file;
		file.setReader(reader);
		if (file.readHeader()) {
			file.onDecodeHuffmanBlock = [onLoadBlock](sl_uint32 colorIndex, sl_int16 data[64], JpegComponent& component, JpegHuffmanTable& dc_huffman_table, JpegHuffmanTable& ac_huffman_table) {
				sl_bool flagFinish = sl_false;
				onLoadBlock(colorIndex, data, flagFinish);
				if (flagFinish) {
					return JpegFile::Result::Finish;
				} else {
					return JpegFile::Result::Ignore;
				}
			};
			if (file.readContent()) {
				return sl_true;
			}
		}
		return sl_false;
	}

	Memory Jpeg::modifyHuffmanBlocks(const Ptr<IReader, ISeekable>& reader, const Function<void(sl_uint32 colorIndex, sl_int16 data[64])>& onLoadBlock)
	{
		JpegFile file;
		file.setReader(reader);

		if (file.readHeader()) {

			MemoryOutput writer;
			JpegHuffmanWriter huffWriter(&file, &writer);
			sl_int32 nRestartIndex = 0;

			file.onDecodeHuffmanBlock = [&huffWriter, &file, onLoadBlock](sl_uint32 colorIndex, sl_int16 data[64], JpegComponent& comp, JpegHuffmanTable& dc_huffman_table, JpegHuffmanTable& ac_huffman_table) {
				onLoadBlock(colorIndex, data);
				huffWriter.encodeBlock(data, comp, dc_huffman_table, ac_huffman_table); // encode and write data;
				return JpegFile::Result::Ignore;
			};

			file.onDecodeRestartControl = [&file, &writer, &huffWriter, &nRestartIndex](sl_int32& count) {
				if (nRestartIndex > 0) {
					sl_uint8 value = Math::abs(nRestartIndex - 1) % 8;
					huffWriter.flush();
					// RST Marker
					writer.writeUint8(0xFF);
					writer.writeUint8((sl_uint8)(JpegMarkerCode::RST0) + value);
				}
				nRestartIndex++;
				huffWriter.restart();
			};

			file.onReachedScanData = [&file, &writer]() {
				sl_size headerSize = (sl_size)(file.m_reader.getPosition());
				if (file.m_reader.getSeekable()->seekToBegin()) {
					Memory buf = file.m_reader.readToMemory(headerSize);
					if (buf.getSize() == headerSize) {
						writer.write(buf);
					}
				}
			};

			if (file.readContent()) {
				huffWriter.flush();
				writer.writeUint8(0xFF);
				writer.writeUint8((sl_uint8)(JpegMarkerCode::EOI)); //EOI
				return writer.getData();
			}
		}
		return sl_null;
	}

}
