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

	sl_bool WavFile::save(const StringParam& path, AudioData& data, sl_uint32 nSamplesPerSecond)
	{
		Writer<File> file = File::openForWrite(path);
		if (file.isOpened()) {
			// Header == "RIFF"
			if (!file.writeUint32(0x46464952)) {
				return sl_false;
			}
			// chunk size
			if (!file.writeUint32((sl_uint32)(data.getTotalSize()))) {
				return sl_false;
			}
			// Header == "WAVE"
			if (!file.writeUint32(0x45564157)) {
				return sl_false;
			}
			// Header == "fmt "
			if (!file.writeUint32(0x20746d66)) {
				return sl_false;
			}
			if (!file.writeUint32(16)) {
				return sl_false;
			}
			//flag
			if (!file.writeUint16(1)) {
				return sl_false;
			}
			//channel
			if (!file.writeUint16(AudioFormatHelper::getChannelCount(data.format))) {
				return sl_false;
			}

			if (!file.writeUint32(nSamplesPerSecond)) {
				return sl_false;
			}
			if (!file.writeUint32(nSamplesPerSecond * AudioFormatHelper::getBytesPerSample(data.format))) {
				return sl_false;
			}

			//block align
			if (!file.writeUint16(2)) {
				return sl_false;
			}

			if (!file.writeUint16(AudioFormatHelper::getBitsPerSample(data.format))) {
				return sl_false;
			}
			// data
			if (!file.writeUint32(0x61746164)) {
				return sl_false;
			}
			// size
			if (!file.writeUint32((sl_uint32)(data.getTotalSize()))) {
				return sl_false;
			}
			if (!file.writeFully(data.data, data.getTotalSize())) {
				return sl_false;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool WavFile::load(const StringParam& path, AudioData& out)
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
			SLIB_UNUSED(flags)
			sl_uint16 channels = file.readUint16();
			if (channels != 1 && channels != 2) {
				return sl_false;
			}
			sl_uint32 sampleRate = file.readUint32();
			if (sampleRate < 1) {
				return sl_false;
			}

			sl_uint32 byteRate = file.readUint32();
			SLIB_UNUSED(byteRate)
			sl_uint16 blockAlign = file.readUint16();
			SLIB_UNUSED(blockAlign)
			sl_uint16 bitsPerSample = file.readUint16();
			if (bitsPerSample != 8 && bitsPerSample != 16) {
				return sl_false;
			}
			if (skipSize > 0) {
				SLIB_SCOPED_BUFFER(sl_uint8, 1024, buf, skipSize)
				if (!buf) {
					return sl_false;
				}
				if (file.readFully(buf, skipSize) != skipSize) {
					return sl_false;
				}
			}

			header = file.readUint32();
			if (header != 0x61746164) {
				return sl_false;
			}

			sl_uint32 size = file.readUint32();
			
			out.format = (AudioFormat)SLIB_DEFINE_AUDIO_FORMAT(bitsPerSample == 8 ? AudioSampleType::Int8 : AudioSampleType::Int16, bitsPerSample, 1, 0);
			Memory mem = Memory::create(size);
			if (file.readFully(mem.getData(), size) != size) {
				return sl_false;
			}
			out.data = mem.getData();
			out.count = size / (bitsPerSample / 8) / channels;
			out.ref = mem.ref;
			return sl_true;
		}
		return sl_false;
	}

}
