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

#ifndef CHECKHEADER_SLIB_MATH_FFT
#define CHECKHEADER_SLIB_MATH_FFT

/*
	Fast Fourier Transform
*/

#include "complex.h"

namespace slib
{

	class SLIB_EXPORT FFT
	{
	public:
		// N: count of input/output data, power of 2 ( N = 2^O )
		FFT(sl_uint32 N);

		~FFT();

	public:
		void transform(Complex* data) const;

		void inverse(Complex* data) const;

	protected:
		void _init(sl_uint32 N);

	protected:
		sl_uint32 m_count;
		sl_uint32* m_ip; // work area for bit reversal
		sl_real* m_w; // cos/sin table

	};


	class SLIB_EXPORT DCT
	{
	public:
		// N: count of input/output data, power of 2 ( N = 2^O )
		DCT(sl_uint32 N);

		~DCT();

	public:
		void transform(sl_real* data) const;

		void inverse(sl_real* data) const;

	protected:
		void _init(sl_uint32 N);

	protected:
		sl_uint32 m_count;
		sl_uint32* m_ip; // work area for bit reversal
		sl_real* m_w; // cos/sin table

	};

}

#endif