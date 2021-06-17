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
#include "slib/core/java/file.h"

#include "slib/core/memory_output.h"

namespace slib
{

	namespace priv
	{
		namespace java_io
		{

			SLIB_JNI_BEGIN_CLASS(JInputStream, "java/io/InputStream")
				SLIB_JNI_METHOD(read, "read", "([B)I")
				SLIB_JNI_METHOD(close, "close", "()V")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JFile, "java/io/File")
				SLIB_JNI_METHOD(getAbsolutePath, "getAbsolutePath", "()Ljava/lang/String;")
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::java_io;

	namespace java
	{

		sl_int32 InputStream::readStream(jobject stream, jbyteArray array) noexcept
		{
			JniPreserveExceptionScope scope;
			if (stream && array) {
				sl_int32 n = JInputStream::read.callInt(stream, array);
				if (n < 0) {
					n = 0;
				}
				if (Jni::checkExceptionAndPrintClear()) {
					n = -1;
				}
				return n;
			}
			return -1;
		}

		void InputStream::closeStream(jobject stream) noexcept
		{
			if (stream) {
				JInputStream::close.call(stream);
			}
		}

		Memory InputStream::readAllBytes(jobject stream) noexcept
		{
			JniPreserveExceptionScope scope;
			if (stream) {
				JniLocal<jbyteArray> arr = Jni::newByteArray(512);
				if (Jni::checkExceptionAndPrintClear()) {
					return sl_null;
				}
				jbyte buf[512];
				if (arr.isNotNull()) {
					MemoryOutput writer;
					while (1) {
						sl_int32 n = JInputStream::read.callInt(stream, arr.get());
						if (Jni::checkExceptionAndPrintClear()) {
							break;
						}
						if (n > 0) {
							Jni::getByteArrayRegion(arr, 0, n, buf);
							writer.write(buf, n);
						} else {
							break;
						}
					}
					JInputStream::close.call(stream);
					Jni::checkExceptionAndPrintClear();
					return writer.getData();
				}
			}
			return sl_null;
		}


		String File::getAbsolutePath(jobject thiz) noexcept
		{
			return JFile::getAbsolutePath.callString(thiz);
		}

	}

}

#endif
