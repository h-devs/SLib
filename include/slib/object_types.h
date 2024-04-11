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

#ifndef CHECKHEADER_SLIB_OBJECT_TYPES
#define CHECKHEADER_SLIB_OBJECT_TYPES

namespace slib
{
	namespace object_types
	{

		namespace packages
		{
			enum {
				Core = 0xff010000,
				Data = 0xff020000,
				Math = 0xff030000,
				System = 0xff040000,
				IO = 0xff050000,
				Crypto = 0xff060000,
				Network = 0xff070000,
				Graphics = 0xff080000,
				Render = 0xff090000,
				Device = 0xff0a0000,
				Ui = 0xff0b0000,
				Db = 0xff0c0000,
				Media = 0xff0d0000,
				Geo = 0xff0e0000,
				Doc = 0xff0f0000,
				Storage = 0xff100000,
				Script = 0xff110000
			};
		}

	}
}

#endif