/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include <slib/core.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	if (argc != 5) {
		return -1;
	}
	String fileSource = argv[1];
	String fileOutput = argv[2];
	String textFind = argv[3];
	String textReplace = argv[4];
	Memory mem = File::readAllBytes(fileSource);
	if (mem.isNull()) {
		return -1;
	}
	String str = String((sl_char8*)(mem.getData()), mem.getSize());
	str = str.replaceAll(textFind, textReplace);
	if (File::writeAllBytes(fileOutput, Memory::createStatic(str.getData(), str.getLength()))) {
		return 0;
	} else {
		return -1;		
	}
}
