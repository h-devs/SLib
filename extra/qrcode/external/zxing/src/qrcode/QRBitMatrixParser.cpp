/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "../qrcode/QRBitMatrixParser.h"
#include "../qrcode/QRVersion.h"
#include "../qrcode/QRFormatInformation.h"
#include "../BitMatrix.h"
#include "../ByteArray.h"
#include "../DecodeStatus.h"

namespace ZXing {
namespace QRCode {

static inline int copyBit(const BitMatrix& bitMatrix, int i, int j, int versionBits, bool mirrored)
{
	bool bit = mirrored ? bitMatrix.get(j, i) : bitMatrix.get(i, j);
	return (versionBits << 1) | static_cast<int>(bit);
}

static inline bool hasValidDimension(const BitMatrix& bitMatrix)
{
	int dimension = bitMatrix.height();
	return !(dimension < 21 || (dimension & 0x03) != 1);
}

/**
* <p>Reads version information from one of its two locations within the QR Code.</p>
*
* @return {@link Version} encapsulating the QR Code's version
* @throws FormatException if both version information locations cannot be parsed as
* the valid encoding of version information
*/
const Version* BitMatrixParser::ReadVersion(const BitMatrix& bitMatrix, bool mirrored)
{
	if (!hasValidDimension(bitMatrix))
		return nullptr;

	int dimension = bitMatrix.height();

	int provisionalVersion = (dimension - 17) / 4;
	if (provisionalVersion <= 6) {
		return Version::VersionForNumber(provisionalVersion);
	}

	// Read top-right version info: 3 wide by 6 tall
	int versionBits = 0;
	int ijMin = dimension - 11;
	for (int j = 5; j >= 0; j--) {
		for (int i = dimension - 9; i >= ijMin; i--) {
			versionBits = copyBit(bitMatrix, i, j, versionBits, mirrored);
		}
	}

	auto theParsedVersion = Version::DecodeVersionInformation(versionBits);
	if (theParsedVersion != nullptr && theParsedVersion->dimensionForVersion() == dimension) {
		return theParsedVersion;
	}

	// Hmm, failed. Try bottom left: 6 wide by 3 tall
	versionBits = 0;
	for (int i = 5; i >= 0; i--) {
		for (int j = dimension - 9; j >= ijMin; j--) {
			versionBits = copyBit(bitMatrix, i, j, versionBits, mirrored);
		}
	}

	theParsedVersion = Version::DecodeVersionInformation(versionBits);
	if (theParsedVersion != nullptr && theParsedVersion->dimensionForVersion() == dimension) {
		return theParsedVersion;
	}
	return nullptr;
}

/**
* <p>Reads format information from one of its two locations within the QR Code.</p>
*
* @return {@link FormatInformation} encapsulating the QR Code's format info
* @throws FormatException if both format information locations cannot be parsed as
* the valid encoding of format information
*/
FormatInformation
BitMatrixParser::ReadFormatInformation(const BitMatrix& bitMatrix, bool mirrored)
{
	if (!hasValidDimension(bitMatrix))
		return {};

	// Read top-left format info bits
	int formatInfoBits1 = 0;
	for (int i = 0; i < 6; i++) {
		formatInfoBits1 = copyBit(bitMatrix, i, 8, formatInfoBits1, mirrored);
	}
	// .. and skip a bit in the timing pattern ...
	formatInfoBits1 = copyBit(bitMatrix, 7, 8, formatInfoBits1, mirrored);
	formatInfoBits1 = copyBit(bitMatrix, 8, 8, formatInfoBits1, mirrored);
	formatInfoBits1 = copyBit(bitMatrix, 8, 7, formatInfoBits1, mirrored);
	// .. and skip a bit in the timing pattern ...
	for (int j = 5; j >= 0; j--) {
		formatInfoBits1 = copyBit(bitMatrix, 8, j, formatInfoBits1, mirrored);
	}

	// Read the top-right/bottom-left pattern too
	int dimension = bitMatrix.height();
	int formatInfoBits2 = 0;
	int jMin = dimension - 7;
	for (int j = dimension - 1; j >= jMin; j--) {
		formatInfoBits2 = copyBit(bitMatrix, 8, j, formatInfoBits2, mirrored);
	}
	for (int i = dimension - 8; i < dimension; i++) {
		formatInfoBits2 = copyBit(bitMatrix, i, 8, formatInfoBits2, mirrored);
	}

	return FormatInformation::DecodeFormatInformation(formatInfoBits1, formatInfoBits2);
}

/**
* <p>Reads the bits in the {@link BitMatrix} representing the finder pattern in the
* correct order in order to reconstruct the codewords bytes contained within the
* QR Code.</p>
*
* @return bytes encoded within the QR Code
* @throws FormatException if the exact number of bytes expected is not read
*/
ByteArray
BitMatrixParser::ReadCodewords(const BitMatrix& bitMatrix, const Version& version)
{
	if (!hasValidDimension(bitMatrix))
		return {};

	BitMatrix functionPattern;
	version.buildFunctionPattern(functionPattern);

	bool readingUp = true;
	ByteArray result(version.totalCodewords());
	int resultOffset = 0;
	int currentByte = 0;
	int bitsRead = 0;
	int dimension = bitMatrix.height();
	// Read columns in pairs, from right to left
	for (int j = dimension - 1; j > 0; j -= 2) {
		if (j == 6) {
			// Skip whole column with vertical alignment pattern;
			// saves time and makes the other code proceed more cleanly
			j--;
		}
		// Read alternatingly from bottom to top then top to bottom
		for (int count = 0; count < dimension; count++) {
			int i = readingUp ? dimension - 1 - count : count;
			for (int col = 0; col < 2; col++) {
				// Ignore bits covered by the function pattern
				if (!functionPattern.get(j - col, i)) {
					// Read a bit
					bitsRead++;
					currentByte = (currentByte << 1) | static_cast<int>(bitMatrix.get(j - col, i));
					// If we've made a whole byte, save it off
					if (bitsRead == 8) {
						result[resultOffset++] = static_cast<uint8_t>(currentByte);
						bitsRead = 0;
						currentByte = 0;
					}
				}
			}
		}
		readingUp ^= true; // readingUp = !readingUp; // switch directions
	}
	if (resultOffset != version.totalCodewords())
		return {};

	return result;
}

} // QRCode
} // ZXing
