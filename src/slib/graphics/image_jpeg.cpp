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

#include "slib/graphics/image.h"

#include "slib/io/file.h"
#include "slib/core/scoped_buffer.h"

#include <stdio.h>
#include <setjmp.h>
#if defined(SLIB_PLATFORM_IS_APPLE)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "libjpeg/jpeglib.h"

namespace slib
{

	namespace {

		struct JPEG_ERROR_MGR {
			jpeg_error_mgr pub;	/* "public" fields */
			jmp_buf setjmp_buffer;	/* for return to caller */
		};

		static void ExitError(j_common_ptr cinfo)
		{
			JPEG_ERROR_MGR* err = (JPEG_ERROR_MGR*)(cinfo->err);
			char jpegLastErrorMsg[JMSG_LENGTH_MAX];
			(*(cinfo->err->format_message)) (cinfo, jpegLastErrorMsg);
			longjmp(err->setjmp_buffer, 1);
		}

	}

	Ref<Image> Image::loadJpeg(const void* content, sl_size size)
	{
		if (!content || !size) {
			return sl_null;
		}
		jpeg_decompress_struct cinfo;
		JPEG_ERROR_MGR jerr;
		cinfo.err = jpeg_std_error(&(jerr.pub));
		jerr.pub.error_exit = ExitError;

		Ref<Image> ret;

		if (setjmp(jerr.setjmp_buffer)) {
			jpeg_destroy_decompress(&cinfo);
			return sl_null;
		}

		jpeg_create_decompress(&cinfo);

		jpeg_mem_src(&cinfo, (unsigned char*)content, (sl_uint32)size);

		jpeg_read_header(&cinfo, 1);

		cinfo.out_color_space = JCS_RGB;

		jpeg_start_decompress(&cinfo);

		sl_uint32 width = cinfo.output_width;
		sl_uint32 height = cinfo.output_height;
		ret = Image::create(width, height);
		if (ret.isNotNull()) {
			JSAMPROW row_pointer[1];
			SLIB_SCOPED_BUFFER(sl_uint8, 4096, row, width*3);
			if (row) {
				row_pointer[0] = (JSAMPROW)(row);
				Color* pixels = ret->getColors();
				sl_reg stride = ret->getStride();
				while (cinfo.output_scanline < height) {
					jpeg_read_scanlines(&cinfo, row_pointer, 1);
					sl_uint8* p = row;
					for (sl_uint32 i = 0; i < width; i++) {
						pixels[i].r = *(p++);
						pixels[i].g = *(p++);
						pixels[i].b = *(p++);
						pixels[i].a = 255;
					}
					pixels += stride;
				}
			}
		}

		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		return ret;
	}

	namespace {

		static Memory SaveJpeg(const Ref<Image>& image, float quality, sl_bool flagMonochrome)
		{
			if (image.isNull()) {
				return sl_null;
			}

			jpeg_compress_struct cinfo;
			JPEG_ERROR_MGR jerr;

			cinfo.err = jpeg_std_error(&(jerr.pub));
			jerr.pub.error_exit = ExitError;

			unsigned char* buf = sl_null;
			size_t size = 0;

			jpeg_create_compress(&cinfo);

			if (setjmp(jerr.setjmp_buffer)) {
				jpeg_destroy_compress(&cinfo);
				if (buf) {
					free(buf);
				}
				return sl_null;
			}

			jpeg_mem_dest(&cinfo, &buf, &size);

			sl_uint32 width = image->getWidth();
			sl_uint32 height = image->getHeight();
			sl_reg stride = image->getStride();
			Color* pixels = image->getColors();

			cinfo.image_width = (JDIMENSION)width;
			cinfo.image_height = (JDIMENSION)height;
			if (flagMonochrome) {
				cinfo.input_components = 1;
				cinfo.in_color_space = JCS_GRAYSCALE;
			} else {
				cinfo.input_components = 3;
				cinfo.in_color_space = JCS_RGB;
			}

			jpeg_set_defaults(&cinfo);

			sl_int32 q = (sl_int32)(quality * 100);
			if (q < 0) {
				q = 0;
			}
			if (q > 100) {
				q = 100;
			}
			jpeg_set_quality(&cinfo, (int)q, 1 /* limit to baseline-JPEG values */);

			jpeg_start_compress(&cinfo, 1);

			SLIB_SCOPED_BUFFER(sl_uint8, 4096, row, width * (flagMonochrome ? 1 : 3));
			if (row) {
				JSAMPROW row_pointer[1];
				row_pointer[0] = (JSAMPROW)(row);
				if (flagMonochrome) {
					while (cinfo.next_scanline < height) {
						sl_uint8* p = row;
						for (sl_uint32 i = 0; i < width; i++) {
							*(p++) = (sl_uint8)(((sl_uint32)(pixels[i].r) + (sl_uint32)(pixels[i].g) + (sl_uint32)(pixels[i].b)) / 3);
						}
						jpeg_write_scanlines(&cinfo, row_pointer, 1);
						pixels += stride;
					}
				} else {
					while (cinfo.next_scanline < height) {
						sl_uint8* p = row;
						for (sl_uint32 i = 0; i < width; i++) {
							*(p++) = pixels[i].r;
							*(p++) = pixels[i].g;
							*(p++) = pixels[i].b;
						}
						jpeg_write_scanlines(&cinfo, row_pointer, 1);
						pixels += stride;
					}
				}
			}

			jpeg_finish_compress(&cinfo);

			Memory ret;
			if (row && buf) {
				ret = Memory::create(buf, size);
			}

			jpeg_destroy_compress(&cinfo);

			if (buf) {
				free(buf);
			}

			return ret;
		}

	}

	Memory Image::saveJpeg(const Ref<Image>& image, float quality)
	{
		return SaveJpeg(image, quality, sl_false);
	}

	Memory Image::saveGrayJpeg(const Ref<Image>& image, float quality)
	{
		return SaveJpeg(image, quality, sl_true);
	}

	Memory Image::saveJpeg(float quality)
	{
		return saveJpeg(this, quality);
	}

	Memory Image::saveGrayJpeg(float quality)
	{
		return saveGrayJpeg(this, quality);
	}

	sl_bool Image::saveJpeg(const StringParam& filePath, const Ref<Image>& image, float quality)
	{
		if (image.isNull()) {
			return sl_false;
		}
		File file = File::openForWrite(filePath);
		if (file.isOpened()) {
			Memory mem = saveJpeg(image, quality);
			if (mem.isNotNull()) {
				sl_reg size = mem.getSize();
				if (file.writeFully(mem.getData(), size) == size) {
					return sl_true;
				}
			}
			file.close();
			File::deleteFile(filePath);
		}
		return sl_false;
	}

	sl_bool Image::saveJpeg(const StringParam& filePath, float quality)
	{
		return saveJpeg(filePath, this, quality);
	}

}
