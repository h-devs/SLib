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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_USE_JNI

#include "slib/core/java/input_stream.h"

#include "slib/core/java.h"

namespace slib
{

	namespace priv
	{
		namespace java_input_stream
		{

			SLIB_JNI_BEGIN_CLASS(JInputStream, "java/io/InputStream")
				SLIB_JNI_METHOD(read, "read", "([B)I");
				SLIB_JNI_METHOD(close, "close", "()V");
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::java_input_stream;

	namespace java
	{

		sl_int32 InputStream::readStream(jobject stream, jbyteArray array)
		{
			if (stream && array) {
				sl_int32 n = JInputStream::read.callInt(stream, array);
				if (n < 0) {
					n = 0;
				}
				if (Jni::checkException()) {
					n = -1;
					Jni::printException();
					Jni::clearException();
				}
				return n;
			}
			return -1;
		}

		void InputStream::closeStream(jobject stream)
		{
			if (stream) {
				JInputStream::close.call(stream);
				if (Jni::checkException()) {
					Jni::printException();
					Jni::clearException();
				}
			}
		}

	}

}

#endif
