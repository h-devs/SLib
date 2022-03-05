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

#include "flags.h"

namespace slib
{

	SLIB_DEFINE_FLAGS(ObjectOperator, {
		Unknown = 0,
		UnaryMinus = 1, // -
		LogicalNot = 2, // !
		BitwiseNot = 3, // ~
		Multiply = 11, // *
		Divide = 12, // /
		Remainder = 13, // %
		Add = 21, // +
		Subtract = 22, // -
		ShiftLeft = 31, // <<
		ShiftRight = 32, // >>
		Compare = 41,
		Equals = 51,
		BitwiseAnd = 61, // &
		BitwiseXor = 71, // ^
		BitwiseOr = 81, // |
	})

}