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

#include "slib/media/wav.h"

#include "slib/core/scoped_buffer.h"
#include "slib/io/file.h"
#include "slib/io/memory_reader.h"

namespace slib
{
	WavFile::WavFile()
	{
	}

	WavFile::~WavFile()
	{
	}

	sl_bool WavFile::loadWavFile(const StringParam& path, AudioData& out)
	{
		Reader<File> file = File::openForRead(path);
		if (file.isOpened()) {
			sl_uint32 header = file.readUint32();

			// Header == "RIFF"
			if (header != 0x46464952) {
				return sl_false;
			}

			sl_uint32 chunkSize = file.readUint32();
			header = file.readUint32();
			// Header == "WAVE"
			if (header != 0x45564157) {
				return sl_false;
			}
			header = file.readUint32();
			// Header == "fmt "
			if (header != 0x20746d66) {
				return sl_false;
			}

			sl_uint32 subChunkSize = file.readUint32();
			sl_uint32 skipSize = subChunkSize - 16;
			if (subChunkSize != 16 && subChunkSize == 18 && chunkSize == 40) {
				return sl_false;
			}

			sl_uint16 flags = file.readUint16();
			sl_uint16 channels = file.readUint16();
			if (channels != 1 && channels != 2) {
				return sl_false;
			}
			sl_uint32 sampleRate = file.readUint32();
			if (sampleRate < 1) {
				return sl_false;
			}

			sl_uint32 byteRate = file.readUint32();
			sl_uint16 blockAlign = file.readUint16();
			sl_uint16 bitsPerSample = file.readUint16();
			if (bitsPerSample != 8 && bitsPerSample != 16) {
				return sl_false;
			}
			if (skipSize > 0) {
				SLIB_SCOPED_BUFFER(sl_uint8, 1024, buf, skipSize)
				if (buf) {
					file.read(buf, skipSize);
				}
			}

			header = file.readUint32();
			if (header != 0x61746164) {
				return sl_false;
			}

			sl_uint32 size = file.readUint32();
			
			out.format = (AudioFormat)SLIB_DEFINE_AUDIO_FORMAT(bitsPerSample == 8 ? AudioSampleType::Uint8 : AudioSampleType::Uint16, bitsPerSample, 1, 0);
			Memory mem = Memory::create(size);
			file.read(mem.getData(), size);
			out.data = mem.getData();
			out.count = size / (bitsPerSample / 8) / channels;
			out.ref = mem.ref;
			return sl_true;
		}

		return sl_false;
	}
}
