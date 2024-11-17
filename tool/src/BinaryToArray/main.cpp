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

#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	if (argc < 3) {
		Println("Usage: BinaryToArray [compress|decompress|compress-zstd|decompress-zstd|compress-lzma|decompress-lzma|d] <source-path> <output-path>");
		return -1;
	}
	String fileSource;
	String fileOutput;
	sl_bool flagToBinary = sl_false;
	sl_bool flagCompressZlib = sl_false;
	sl_bool flagCompressZstd = sl_false;
	sl_bool flagCompressLzma = sl_false;
	if (Base::equalsString(argv[1], "compress")) {
		if (argc != 4) {
			Println("Invalid argument count!");
			return -1;
		}
		flagCompressZlib = sl_true;
		fileSource = argv[2];
		fileOutput = argv[3];
	} else if (Base::equalsString(argv[1], "decompress")) {
		if (argc != 4) {
			Println("Invalid argument count!");
			return -1;
		}
		flagCompressZlib = sl_true;
		flagToBinary = sl_true;
		fileSource = argv[2];
		fileOutput = argv[3];
	} else if (Base::equalsString(argv[1], "compress-zstd")) {
		if (argc != 4) {
			Println("Invalid argument count!");
			return -1;
		}
		flagCompressZstd = sl_true;
		fileSource = argv[2];
		fileOutput = argv[3];
	} else if (Base::equalsString(argv[1], "decompress-zstd")) {
		if (argc != 4) {
			Println("Invalid argument count!");
			return -1;
		}
		flagCompressZstd = sl_true;
		flagToBinary = sl_true;
		fileSource = argv[2];
		fileOutput = argv[3];
	} else if (Base::equalsString(argv[1], "compress-lzma")) {
		if (argc != 4) {
			Println("Invalid argument count!");
			return -1;
		}
		flagCompressLzma = sl_true;
		fileSource = argv[2];
		fileOutput = argv[3];
	} else if (Base::equalsString(argv[1], "decompress-lzma")) {
		if (argc != 4) {
			Println("Invalid argument count!");
			return -1;
		}
		flagCompressLzma = sl_true;
		flagToBinary = sl_true;
		fileSource = argv[2];
		fileOutput = argv[3];
	} else if (Base::equalsString(argv[1], "d")) {
		if (argc != 4) {
			Println("Invalid argument count!");
			return -1;
		}
		flagToBinary = sl_true;
		fileSource = argv[2];
		fileOutput = argv[3];
	} else {
		if (argc != 3) {
			Println("Invalid argument count!");
			return -1;
		}
		fileSource = argv[1];
		fileOutput = argv[2];
	}
	if (flagToBinary) {
		String text = File::readAllText(fileSource).toString();
		if (text.isEmpty()) {
			Println("Source content is empty!");
			return -1;
		}
		CList<sl_uint8> buf;
		sl_size len = text.getLength();
		char* str = text.getData();
		sl_uint32 n = 0;
		for (sl_size i = 0; i < len; i++) {
			char ch = str[i];
			if (ch == ',') {
				buf.add_NoLock(n);
				n = 0;
			} else if (SLIB_CHAR_IS_DIGIT(ch)) {
				n = (sl_uint32)(n * 10 + (sl_uint32)(ch - '0'));
				if (n >> 8) {
					Println("Invalid number (bigger than 255)!");
					return -1;
				}
			} else if (!SLIB_CHAR_IS_WHITE_SPACE(ch)) {
				Println("Invalid character: %c", ch);
				return -1;
			}
		}
		buf.add_NoLock(n);
		Memory mem;
		if (flagCompressZlib) {
			mem = Zlib::decompress(buf.getData(), buf.getCount());
			if (mem.isNull()) {
				Println("Failed to decompress!");
				return -1;
			}
		} else if (flagCompressZstd) {
			mem = Zstd::decompress(buf.getData(), buf.getCount());
			if (mem.isNull()) {
				Println("Failed to decompress!");
				return -1;
			}
		} else if (flagCompressLzma) {
			mem = Lzma::decompress(buf.getData(), buf.getCount());
			if (mem.isNull()) {
				Println("Failed to decompress!");
				return -1;
			}
		} else {
			mem = Memory::createStatic(buf.getData(), buf.getCount());
		}
		if (!(File::writeAllBytes(fileOutput, mem))) {
			Println("Failed to write!");
			return -1;
		}
	} else {
		Memory mem = File::readAllBytes(fileSource);
		if (mem.isNull()) {
			Println("Source content is empty!");
			return -1;
		}
		if (flagCompressZlib) {
			mem = Zlib::compress(mem.getData(), mem.getSize());
			if (mem.isNull()) {
				Println("Failed to compress!");
				return -1;
			}
		} else if (flagCompressZstd) {
			mem = Zstd::compress(mem.getData(), mem.getSize(), 22);
			if (mem.isNull()) {
				Println("Failed to compress!");
				return -1;
			}
		} else if (flagCompressLzma) {
			mem = Lzma::compress(mem.getData(), mem.getSize(), 22);
			if (mem.isNull()) {
				Println("Failed to compress!");
				return -1;
			}
		}
		StringBuffer sb;
		sl_size n = mem.getSize();
		sl_uint8* data = (sl_uint8*)(mem.getData());
		for (sl_size i = 0; i < n; i++) {
			sb.add(String::fromUint32(data[i]));
			if (i != n - 1) {
				if ((i & 15) == 15) {
					sb.addStatic(",\r\n");
				} else {
					sb.addStatic(",");
				}
			}
		}
		mem = sb.mergeToMemory();
		if (!(File::writeAllBytes(fileOutput, mem))) {
			Println("Failed to write!");
			return -1;
		}
	}
	return 0;
}
