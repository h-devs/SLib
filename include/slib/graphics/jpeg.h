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

#ifndef CHECKHEADER_SLIB_GRAPHICS_JPEG
#define CHECKHEADER_SLIB_GRAPHICS_JPEG

#include "definition.h"

#include "../core/skippable_reader.h"
#include "../core/function.h"

#define SLIB_JPEG_QUANTIZATION_TABLES_COUNT 4
#define SLIB_JPEG_HUFFMAN_TABLES_COUNT 4
#define SLIB_JPEG_HUFFMAN_FAST_BITS 9

namespace slib
{

	enum class JpegMarkerCode
	{
		None = 0x100,

		SOF0 = 0xc0, // Start Of Frame, Baseline
		SOF1 = 0xc1, // Extended sequential, Huffman
		SOF2 = 0xc2, // Progressive, Huffman
		SOF3 = 0xc3, // Lossless, Huffman

		SOF5 = 0xc5, // Differential sequential, Huffman
		SOF6 = 0xc6, // Differential progressive, Huffman
		SOF7 = 0xc7, // Differential lossless, Huffman

		JPG = 0xc8, // Reserved for JPEG extensions
		SOF9 = 0xc9, // Extended sequential, arithmetic
		SOF10 = 0xca, // Progressive, arithmetic
		SOF11 = 0xcb, // Lossless, arithmetic

		SOF13 = 0xcd, // Differential sequential, arithmetic
		SOF14 = 0xce, // Differential progressive, arithmetic
		SOF15 = 0xcf, // Differential lossless, arithmetic

		DHT = 0xc4, // Define Huffman Table

		DAC = 0xcc,

		RST0 = 0xd0,
		RST1 = 0xd1,
		RST2 = 0xd2,
		RST3 = 0xd3,
		RST4 = 0xd4,
		RST5 = 0xd5,
		RST6 = 0xd6,
		RST7 = 0xd7,

		SOI = 0xd8, // Start Of Image
		EOI = 0xd9, // End Of Image
		SOS = 0xda, // Start of Scan
		DQT = 0xdb, // Define Quantization Table
		DNL = 0xdc,
		DRI = 0xdd, // Define Restart Interval
		DHP = 0xde,
		EXP = 0xdf,

		APP0 = 0xe0, // JFIF
		APP1 = 0xe1,
		APP2 = 0xe2,
		APP3 = 0xe3,
		APP4 = 0xe4,
		APP5 = 0xe5,
		APP6 = 0xe6,
		APP7 = 0xe7,
		APP8 = 0xe8,
		APP9 = 0xe9,
		APP10 = 0xea,
		APP11 = 0xeb,
		APP12 = 0xec,
		APP13 = 0xed,
		APP14 = 0xee, // Adobe
		APP15 = 0xef,

		JPG0 = 0xf0,
		JPG8 = 0xf8,
		JPG13 = 0xfd,
		COM = 0xfe,

		TEM = 0x01

	};

	class SLIB_EXPORT JpegAdobeSegment
	{
	public:
		sl_uint8 version;
		sl_uint16 flags0;
		sl_uint16 flags1;
		sl_uint8 color_transform;

	public:
		JpegAdobeSegment();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegAdobeSegment)

	};

	class SLIB_EXPORT JpegQuantizationTable
	{
	public:
		sl_bool flagDefined;
		sl_bool flag_16bit;
		sl_uint8 index;
		sl_uint16 quant[64];

	public:
		JpegQuantizationTable();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegQuantizationTable)

	};

	struct SLIB_EXPORT JpegHuffmanEncodeItem
	{
		sl_uint16 code;
		sl_uint8 size;
	};

	class SLIB_EXPORT JpegHuffmanTable
	{
	public:
		sl_bool flagDefined;
		sl_bool flagAC; // AC/DC
		sl_uint8 index;
		sl_uint8 bits[16]; // bits[k]: number of symbols with codes of length k+1 bits
		sl_uint8 values[256];

		sl_uint8 count;
		sl_uint16 code[257];
		sl_uint8 size[257];
		sl_uint32 max_code[18];
		sl_int32 delta[17];

		sl_uint8 fast[1 << SLIB_JPEG_HUFFMAN_FAST_BITS];
		sl_int16 fast_ac[1 << SLIB_JPEG_HUFFMAN_FAST_BITS];

		sl_uint16 encode_code[256];
		sl_uint8 encode_size[256];

	public:
		JpegHuffmanTable();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegHuffmanTable)

	public:
		sl_bool build();

		void buildFastAC();

		void buildEncodeItems();

		JpegHuffmanEncodeItem getEncodeItem(sl_int16 index);

	};

	class SLIB_EXPORT JpegComponent
	{
	public:
		sl_uint8 id;
		sl_uint8 horizontal_sample_factor;
		sl_uint8 vertical_sample_factor;
		sl_uint8 quant_table_no;

		sl_int32 dc_prediction;
		sl_int32 dc_Wprediction; //for writing

	public:
		JpegComponent();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegComponent)

	};

	class SLIB_EXPORT JpegFrameHeader
	{
	public:
		sl_bool flagBaseline;
		sl_bool flagProgressive;
		sl_bool flagArithmetic;

		sl_uint8 precision;
		sl_uint16 width;
		sl_uint16 height;
		sl_uint8 nComponents;
		JpegComponent components[4];

		sl_uint8 horizontal_sample_factor_max;
		sl_uint8 vertical_sample_factor_max;

	public:
		JpegFrameHeader();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegFrameHeader)

	};

	struct SLIB_EXPORT JpegScanComponent
	{
		sl_uint8 id;
		sl_uint8 ac_huffman_table_no;
		sl_uint8 dc_huffman_table_no;

		sl_uint8 index; // component index in frame header
	};

	class SLIB_EXPORT JpegScanHeader
	{
	public:
		sl_uint8 nComponents;
		JpegScanComponent components[4];
		sl_uint8 spec_start;
		sl_uint8 spec_end;
		sl_uint8 succ_high;
		sl_uint8 succ_low;

	public:
		JpegScanHeader();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegScanHeader)

	};

	class SLIB_EXPORT JpegMarker
	{
	public:
		JpegMarkerCode code;
		sl_uint16 size;
		Memory content;

	public:
		JpegMarker();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegMarker)

	};

	class JpegFile;

	class JpegHuffmanReader
	{
	public:
		JpegHuffmanReader(JpegFile* file, IReader* reader);

		~JpegHuffmanReader();

		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(JpegHuffmanReader)

	public:
		sl_bool decodeBlock(
			sl_int16 _out[64],
			JpegComponent& component,
			JpegHuffmanTable& dc_huffman_table,
			JpegHuffmanTable& ac_huffman_table);

		sl_bool decode(sl_uint8* _out, JpegHuffmanTable& table);

		sl_bool isEnd();

		void restart();

	private:
		sl_uint32 read(sl_uint32 len);

		sl_uint32 get(sl_uint32 len);

		void remove(sl_uint32 len);

		sl_uint32 pop(sl_uint32 len);

		sl_int32 extendReceive(sl_uint32 len);

		void prepare(sl_uint32 length);

	private:
		JpegFile* m_file;
		IReader* m_reader;

		sl_uint32 m_buf;
		sl_uint32 m_len;
		sl_bool m_flagEnd;

	};

	class SLIB_EXPORT JpegHuffmanWriter
	{
	public:
		JpegHuffmanWriter(JpegFile* file, IWriter* writer);

		~JpegHuffmanWriter();

		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(JpegHuffmanWriter)

	public:
		sl_bool encodeBlock(
			sl_int16 _in[64],
			JpegComponent& component,
			JpegHuffmanTable& dc_huffman_table,
			JpegHuffmanTable& ac_huffman_table
		);

		void writeBits(const JpegHuffmanEncodeItem& item);

		void flush();

		void restart();

	private:
		JpegFile* m_file;
		IWriter* m_writer;

		sl_uint32 m_buf;
		sl_uint32 m_len;

	};

	class SLIB_EXPORT JpegFile
	{
	public:
		List<JpegMarker> markers;
		
		// JFIF Marker
		sl_bool flagJFIF;

		// SOF
		JpegFrameHeader frame_header;

		// SOS
		JpegScanHeader scan_header;

		// Adobe Segment Marker
		JpegAdobeSegment adobe_segment;

		// DQT
		JpegQuantizationTable quantization_table[SLIB_JPEG_QUANTIZATION_TABLES_COUNT];

		// DHT
		JpegHuffmanTable ac_huffman_tables[SLIB_JPEG_HUFFMAN_TABLES_COUNT];
		JpegHuffmanTable dc_huffman_tables[SLIB_JPEG_HUFFMAN_TABLES_COUNT];

		// DRI
		sl_uint16 restart_interval;

		// x, y: block number
		Function<sl_bool(sl_int16 data[64], JpegComponent& component, JpegHuffmanTable& dc_huffman_table, JpegHuffmanTable& ac_huffman_table)> onDecodeHuffmanBlock;
		Function<void(sl_int32& count)> onDecodeRestartControl;
		Function<void(sl_uint32 x, sl_uint32 y, sl_uint8 color_index, sl_uint8 data[64])> onLoadBlock;
		Function<void()> onReachedScanData;
		Function<sl_bool()> onFinishJob;
		SkippableReader m_reader;

	public:
		JpegFile();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JpegFile)

	public:
		void setReadFully(sl_bool flag);

		sl_bool setReader(const Ptrx<IReader, ISeekable>& reader);

		sl_bool readHeader();

		sl_bool readContent();

		JpegMarkerCode readMarkerCode();

		void setReadMarkerCode(JpegMarkerCode code);

		JpegMarkerCode getLastMarkerCode();

		sl_bool readMarker(JpegMarker& _out);

		sl_bool readMarkerContent(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readJFIF(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readAdobeSegment(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readDQT(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readSOF(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readDHT(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readSOS(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readDRI(IReader* reader, JpegMarker& marker, sl_uint32 size);

		sl_bool readScanData();

		sl_bool controlRestartInterval(JpegHuffmanReader* reader);

		void restartDecoder(JpegHuffmanReader* reader);

		static void zigzag(sl_int16 input[64], sl_int16 output[64]);

		static void dezigzag(sl_int16 input[64], sl_int16 output[64]);

		static void quantizeBlock(sl_int16 data[64], JpegQuantizationTable& table);

		static void dequantizeBlock(sl_int16 data[64], JpegQuantizationTable& table);

		static void idctBlock(sl_int16 input[64], sl_uint8 output[64]);

	private:
		sl_bool m_flagReadFully;

		sl_bool m_flagReadMarkerCode;
		JpegMarkerCode m_lastMarkerCode;

		sl_int32 m_nRestartCountDown;

	};

	class Image;

	class Jpeg
	{
	public:
		static Ref<Image> load(const Ptrx<IReader, ISeekable>& reader);

		static Ref<Image> loadFromMemory(const void* mem, sl_size size);

		static Ref<Image> loadFromMemory(const Memory& mem);

		static Ref<Image> loadFromFile(const StringParam& path);
		
		static sl_bool loadHuffmanBlocks(const Ptrx<IReader, ISeekable>& reader, const Function<sl_bool(sl_int16 data[64])>& onLoadBlock);
		
		static Memory modifyHuffmanBlocks(const Ptr<IReader, ISeekable>& reader, const Function<void(sl_int16 data[64])>& onLoadBlock);

	};

}

#endif