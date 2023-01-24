#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#pragma warning(disable: 4101 4267 4244)
#endif

#include "src/BarcodeFormat.cpp"
#include "src/MultiFormatWriter.cpp"
#include "src/PerspectiveTransform.cpp"
#include "src/ReedSolomonDecoder.cpp"
#include "src/ReedSolomonEncoder.cpp"
#include "src/Result.cpp"
#include "src/ResultMetadata.cpp"
#include "src/ResultPoint.cpp"
#include "src/TextDecoder.cpp"
#define KRTextDecoder KRTextDecoder_
#include "src/TextEncoder.cpp"
#undef KRTextDecoder
#include "src/TextUtfEncoding.cpp"
#include "src/WhiteRectDetector.cpp"
#include "src/ZXBigInteger.cpp"
#include "src/BitArray.cpp"
#include "src/BitMatrix.cpp"
#include "src/BitSource.cpp"
#include "src/BitWrapperBinarizer.cpp"
#include "src/CharacterSetECI.cpp"
#include "src/DecodeHints.cpp"
#include "src/DecodeStatus.cpp"
#include "src/GenericGF.cpp"
#include "src/GenericGFPoly.cpp"
#include "src/GenericLuminanceSource.cpp"
#include "src/GlobalHistogramBinarizer.cpp"
#include "src/GridSampler.cpp"
#define InitBlackMatrix InitBlackMatrix_
#include "src/HybridBinarizer.cpp"
#undef InitBlackMatrix
#include "src/LuminanceSource.cpp"
#include "src/MultiFormatReader.cpp"
#include "src/aztec/AZDecoder.cpp"
#include "src/aztec/AZDetector.cpp"
#define TotalBitsInLayer TotalBitsInLayer_
#include "src/aztec/AZEncoder.cpp"
#undef TotalBitsInLayer
#include "src/aztec/AZHighLevelEncoder.cpp"
#include "src/aztec/AZReader.cpp"
#include "src/aztec/AZToken.cpp"
#include "src/aztec/AZWriter.cpp"
#include "src/datamatrix/DMWriter.cpp"
#include "src/datamatrix/DMBitMatrixParser.cpp"
#include "src/datamatrix/DMDataBlock.cpp"
#include "src/datamatrix/DMDecoder.cpp"
#include "src/datamatrix/DMDefaultPlacement.cpp"
#include "src/datamatrix/DMDetector.cpp"
#include "src/datamatrix/DMECEncoder.cpp"
#include "src/datamatrix/DMHighLevelEncoder.cpp"
#include "src/datamatrix/DMReader.cpp"
#include "src/datamatrix/DMSymbolInfo.cpp"
#include "src/datamatrix/DMVersion.cpp"
#include "src/maxicode/MCBitMatrixParser.cpp"
#include "src/maxicode/MCDecoder.cpp"
#include "src/maxicode/MCReader.cpp"
#include "src/oned/ODWriterHelper.cpp"
#include "src/oned/ODCodabarReader.cpp"
#define ALPHABET ALPHABET_
#define CHARACTER_ENCODINGS CHARACTER_ENCODINGS_
#include "src/oned/ODCodabarWriter.cpp"
#undef ALPHABET
#undef CHARACTER_ENCODINGS
#define CHARACTER_ENCODINGS CHARACTER_ENCODINGS_1
#include "src/oned/ODCode39Reader.cpp"
#undef CHARACTER_ENCODINGS
#define CHARACTER_ENCODINGS CHARACTER_ENCODINGS_2
#define ALPHABET_STRING ALPHABET_STRING_
#define ASTERISK_ENCODING ASTERISK_ENCODING_
#include "src/oned/ODCode39Writer.cpp"
#undef CHARACTER_ENCODINGS
#undef ALPHABET_STRING
#undef ASTERISK_ENCODING
#define ALPHABET_STRING ALPHABET_STRING_1
#define CHARACTER_ENCODINGS CHARACTER_ENCODINGS_3
#define ASTERISK_ENCODING ASTERISK_ENCODING_1
#define PERCENTAGE_MAPPING PERCENTAGE_MAPPING_
#define CounterContainer CounterContainer_
#define FindAsteriskPattern FindAsteriskPattern_
#define PatternToChar PatternToChar_
#include "src/oned/ODCode93Reader.cpp"
#undef ALPHABET_STRING
#undef CHARACTER_ENCODINGS
#undef ASTERISK_ENCODING
#undef PERCENTAGE_MAPPING
#undef CounterContainer
#undef FindAsteriskPattern
#undef PatternToChar
#define ALPHABET_STRING ALPHABET_STRING_2
#define CHARACTER_ENCODINGS CHARACTER_ENCODINGS_4
#define ASTERISK_ENCODING ASTERISK_ENCODING_2
#include "src/oned/ODCode93Writer.cpp"
#undef ALPHABET_STRING
#undef CHARACTER_ENCODINGS
#undef ASTERISK_ENCODING
#include "src/oned/ODCode128Patterns.cpp"
#include "src/oned/ODCode128Reader.cpp"
#define CODE_START_A CODE_START_A_
#define CODE_START_B CODE_START_B_
#define CODE_START_C CODE_START_C_
#define CODE_CODE_A CODE_CODE_A_
#define CODE_CODE_B CODE_CODE_B_
#define CODE_CODE_C CODE_CODE_C_
#define CODE_STOP CODE_STOP_
#define CODE_FNC_1 CODE_FNC_1_
#define CODE_FNC_2 CODE_FNC_2_
#define CODE_FNC_3 CODE_FNC_3_
#define CODE_FNC_4_A CODE_FNC_4_A_
#define CODE_FNC_4_B CODE_FNC_4_B_
#include "src/oned/ODCode128Writer.cpp"
#undef CODE_START_A
#undef CODE_START_B
#undef CODE_START_C
#undef CODE_CODE_A
#undef CODE_CODE_B
#undef CODE_CODE_C
#undef CODE_STOP
#undef CODE_FNC_1
#undef CODE_FNC_2
#undef CODE_FNC_3
#undef CODE_FNC_4_A
#undef CODE_FNC_4_B
#include "src/oned/ODEAN8Reader.cpp"
#define FIRST_DIGIT_ENCODINGS FIRST_DIGIT_ENCODINGS_
#define CODE_WIDTH CODE_WIDTH_
#include "src/oned/ODEAN8Writer.cpp"
#undef FIRST_DIGIT_ENCODINGS
#undef CODE_WIDTH
#include "src/oned/ODEAN13Reader.cpp"
#define FIRST_DIGIT_ENCODINGS FIRST_DIGIT_ENCODINGS_1
#include "src/oned/ODEAN13Writer.cpp"
#undef FIRST_DIGIT_ENCODINGS
#include "src/oned/ODEANManufacturerOrgSupport.cpp"
#define MAX_AVG_VARIANCE MAX_AVG_VARIANCE_
#define MAX_INDIVIDUAL_VARIANCE MAX_INDIVIDUAL_VARIANCE_
#include "src/oned/ODITFReader.cpp"
#undef MAX_AVG_VARIANCE
#undef MAX_INDIVIDUAL_VARIANCE
#define START_PATTERN START_PATTERN_
#define W W_
#define N N_
#define PATTERNS PATTERNS_
#include "src/oned/ODITFWriter.cpp"
#undef START_PATTERN
#undef W
#undef N
#undef PATTERNS
#include "src/oned/ODMultiUPCEANReader.cpp"
#include "src/oned/ODReader.cpp"
#include "src/oned/ODRowReader.cpp"
#include "src/oned/ODRSS14Reader.cpp"
#define FINDER_PATTERNS FINDER_PATTERNS_
#define ParseFoundFinderPattern ParseFoundFinderPattern_
#include "src/oned/ODRSSExpandedReader.cpp"
#undef FINDER_PATTERNS
#undef ParseFoundFinderPattern
#include "src/oned/ODUPCAReader.cpp"
#include "src/oned/ODUPCAWriter.cpp"
#include "src/oned/ODUPCEANCommon.cpp"
#include "src/oned/ODUPCEANExtensionSupport.cpp"
#include "src/oned/ODUPCEANReader.cpp"
#include "src/oned/ODUPCEReader.cpp"
#define NUMSYS_AND_CHECK_DIGIT_PATTERNS NUMSYS_AND_CHECK_DIGIT_PATTERNS_
#define CODE_WIDTH CODE_WIDTH_1
#include "src/oned/ODUPCEWriter.cpp"
#undef NUMSYS_AND_CHECK_DIGIT_PATTERNS
#undef CODE_WIDTH
#include "src/oned/rss/ODRSSExpandedBinaryDecoder.cpp"
#include "src/oned/rss/ODRSSFieldParser.cpp"
#include "src/oned/rss/ODRSSGenericAppIdDecoder.cpp"
#include "src/oned/rss/ODRSSReaderHelper.cpp"
#include "src/pdf417/PDFWriter.cpp"
#include "src/pdf417/PDFBarcodeValue.cpp"
#include "src/pdf417/PDFBoundingBox.cpp"
#include "src/pdf417/PDFCodewordDecoder.cpp"
#include "src/pdf417/PDFDecodedBitStreamParser.cpp"
#include "src/pdf417/PDFDetectionResult.cpp"
#include "src/pdf417/PDFDetectionResultColumn.cpp"
#include "src/pdf417/PDFDetector.cpp"
#define START_PATTERN START_PATTERN_1
#define STOP_PATTERN STOP_PATTERN_
#define CODEWORD_TABLE CODEWORD_TABLE_
#include "src/pdf417/PDFEncoder.cpp"
#undef START_PATTERN
#undef STOP_PATTERN
#undef CODEWORD_TABLE
#define ECI_USER_DEFINED ECI_USER_DEFINED_
#define ECI_GENERAL_PURPOSE ECI_GENERAL_PURPOSE_
#define ECI_CHARSET ECI_CHARSET_
#include "src/pdf417/PDFHighLevelEncoder.cpp"
#undef ECI_USER_DEFINED
#undef ECI_GENERAL_PURPOSE
#undef ECI_CHARSET
#include "src/pdf417/PDFModulusGF.cpp"
#include "src/pdf417/PDFModulusPoly.cpp"
#include "src/pdf417/PDFReader.cpp"
#include "src/pdf417/PDFScanningDecoder.cpp"
#include "src/qrcode/QRWriter.cpp"
#include "src/qrcode/QRAlignmentPattern.cpp"
#define StateCount StateCount_
#include "src/qrcode/QRAlignmentPatternFinder.cpp"
#undef StateCount
#include "src/qrcode/QRBitMatrixParser.cpp"
#include "src/qrcode/QRCodecMode.cpp"
#include "src/qrcode/QRDataBlock.cpp"
#include "src/qrcode/QRDataMask.cpp"
#include "src/qrcode/QRDecoder.cpp"
#include "src/qrcode/QRDetector.cpp"
#include "src/qrcode/QREncoder.cpp"
#include "src/qrcode/QRErrorCorrectionLevel.cpp"
#include "src/qrcode/QRFinderPattern.cpp"
#define CenterFromEnd CenterFromEnd_
#include "src/qrcode/QRFinderPatternFinder.cpp"
#undef CenterFromEnd
#include "src/qrcode/QRFormatInformation.cpp"
#include "src/qrcode/QRMaskUtil.cpp"
#include "src/qrcode/QRMatrixUtil.cpp"
#include "src/qrcode/QRReader.cpp"
#include "src/qrcode/QRVersion.cpp"
#include "src/textcodec/Big5MapTable.cpp"
#include "src/textcodec/Big5TextDecoder.cpp"
#include "src/textcodec/Big5TextEncoder.cpp"
#define REPLACEMENT REPLACEMENT_
#define ValidChar ValidChar_
#define indexTbl_t indexTbl_t_
#include "src/textcodec/GBTextDecoder.cpp"
#undef REPLACEMENT
#undef ValidChar
#undef indexTbl_t
#include "src/textcodec/GBTextEncoder.cpp"
#define REPLACEMENT REPLACEMENT_1
#define ValidChar ValidChar_1
#include "src/textcodec/JPTextDecoder.cpp"
#undef REPLACEMENT
#undef ValidChar
#define IsKana IsKana_
#define sjisToJisx0208 sjisToJisx0208_
#include "src/textcodec/JPTextEncoder.cpp"
#undef IsKana
#undef sjisToJisx0208
#include "src/textcodec/KRHangulMapping.cpp"
#define REPLACEMENT REPLACEMENT_2
#define IsEucChar IsEucChar_
#define ValidChar ValidChar_2
#include "src/textcodec/KRTextDecoder.cpp"
#undef REPLACEMENT
#undef IsEucChar
#undef ValidChar
#define KRTextDecoder KRTextDecoder_
#include "src/textcodec/KRTextEncoder.cpp"
#undef KRTextDecoder
