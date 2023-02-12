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

#include "zxing.h"

#include "external/zxing/src/MultiFormatWriter.h"
#include "external/zxing/src/MultiFormatReader.h"
#include "external/zxing/src/Result.h"
#include "external/zxing/src/BitMatrix.h"
#include "external/zxing/src/DecodeHints.h"
#include "external/zxing/src/HybridBinarizer.h"
#include "external/zxing/src/GenericLuminanceSource.h"

using namespace ZXing;

namespace slib
{

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(ZXing, GenerateParam)

	ZXing::GenerateParam::GenerateParam()
	{
		format = Format::QR_CODE;
		margin = 0;
		width = 512;
		height = 512;
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(ZXing, ScanParam)

	ZXing::ScanParam::ScanParam()
	{
		format = Format::QR_CODE;
		flagTryHarder = sl_true;
		flagTryRotate = sl_false;
		flagSubRegion = sl_false;
	}

	namespace {
		static BarcodeFormat GetBarcodeFormat(ZXing::Format format)
		{
			switch (format) {
				case ZXing::Format::AZTEC:
					return BarcodeFormat::AZTEC;
				case ZXing::Format::CODABAR:
					return BarcodeFormat::CODABAR;
				case ZXing::Format::CODE_39:
					return BarcodeFormat::CODE_39;
				case ZXing::Format::CODE_93:
					return BarcodeFormat::CODE_93;
				case ZXing::Format::CODE_128:
					return BarcodeFormat::CODE_128;
				case ZXing::Format::DATA_MATRIX:
					return BarcodeFormat::DATA_MATRIX;
				case ZXing::Format::EAN_8:
					return BarcodeFormat::EAN_8;
				case ZXing::Format::EAN_13:
					return BarcodeFormat::EAN_13;
				case ZXing::Format::ITF:
					return BarcodeFormat::ITF;
				case ZXing::Format::MAXICODE:
					return BarcodeFormat::MAXICODE;
				case ZXing::Format::PDF_417:
					return BarcodeFormat::PDF_417;
				case ZXing::Format::QR_CODE:
					return BarcodeFormat::QR_CODE;
				case ZXing::Format::RSS_14:
					return BarcodeFormat::RSS_14;
				case ZXing::Format::RSS_EXPANDED:
					return BarcodeFormat::RSS_EXPANDED;
				case ZXing::Format::UPC_A:
					return BarcodeFormat::UPC_A;
				case ZXing::Format::UPC_E:
					return BarcodeFormat::UPC_E;
				case ZXing::Format::UPC_EAN_EXTENSION:
					return BarcodeFormat::UPC_EAN_EXTENSION;
			}
			return BarcodeFormat::QR_CODE;
		}
	}

	Ref<Image> ZXing::generate(const ZXing::GenerateParam& param)
	{
		MultiFormatWriter writer(GetBarcodeFormat(param.format));
		writer.setEncoding(CharacterSet::UTF8);
		writer.setMargin((int)(param.margin));
		Memory data;
		if (sizeof(wchar_t) == 2) {
			data = param.text.toUtf16();
		} else if (sizeof(wchar_t) == 4) {
			data = param.text.toUtf32();
		} else {
			return sl_null;
		}
		try {
			BitMatrix matrix(writer.encode((wchar_t*)(data.getData()), (int)(param.width), (int)(param.height)));
			sl_int32 width = (sl_int32)(matrix.width());
			sl_int32 height = (sl_int32)(matrix.height());
			if (width > 0 && height > 0) {
				Ref<Image> ret = Image::create(width, height);
				if (ret.isNotNull()) {
					for (sl_int32 y = 0; y < height; y++) {
						Color* color = ret->getColorsAt(0, y);
						for (sl_int32 x = 0; x < width; x++) {
							*color = matrix.get(x, y) ? Color::Black : Color::White;
							color++;
						}
					}
					return ret;
				}
			}
		} catch (std::exception&) {
		}
		return sl_null;
	}

	String ZXing::scan(const ZXing::ScanParam& param)
	{
		Ref<Image> image = param.image;
		if (image.isNull()) {
			return sl_null;
		}
		DecodeHints hints;
		hints.setCharacterSet("UTF-8");
		hints.setShouldTryHarder(param.flagTryHarder);
		hints.setShouldTryRotate(sl_false);
		hints.setPossibleFormats({GetBarcodeFormat(param.format)});
		MultiFormatReader reader(hints);
		try {
			std::shared_ptr<GenericLuminanceSource> src;
			if (param.flagSubRegion) {
				Rectanglei region = param.subRegion;
				int width = region.getWidth();
				if (region.left + width > (int)(image->getWidth())) {
					return sl_null;
				}
				int height = region.getHeight();
				if (region.top + height > (int)(image->getHeight())) {
					return sl_null;
				}
				int stride = image->getWidth();
				src = std::make_shared<GenericLuminanceSource>(width, height, image->getColors() + (stride * region.top + region.left), stride * 4, 4, 0, 1, 2);
			} else {
				src = std::make_shared<GenericLuminanceSource>(image->getWidth(), image->getHeight(), image->getColors(), (int)(image->getStride() * 4), 4, 0, 1, 2);
			}
			HybridBinarizer binary(src);
			Result result = reader.read(binary);
			if (param.flagTryRotate) {
				if (!(result.isValid())) {
					std::shared_ptr<BinaryBitmap> r = binary.rotated(180);
					result = reader.read(*r);
				}
				if (!(result.isValid())) {
					std::shared_ptr<BinaryBitmap> r = binary.rotated(90);
					result = reader.read(*r);
				}
				if (!(result.isValid())) {
					std::shared_ptr<BinaryBitmap> r = binary.rotated(270);
					result = reader.read(*r);
				}
			}
			if (result.isValid()) {
				std::wstring wstr(result.text());
				return String::create(wstr.c_str(), wstr.length());
			}
		} catch (std::exception&) {
		}
		return sl_null;
	}

}
