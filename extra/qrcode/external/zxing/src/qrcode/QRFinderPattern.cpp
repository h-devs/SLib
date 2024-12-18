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

#include "../qrcode/QRFinderPattern.h"

#include <cmath>

namespace ZXing {
namespace QRCode {

FinderPattern::FinderPattern(float posX, float posY, float estimatedModuleSize, int count) :
	ResultPoint(posX, posY),
	_estimatedModuleSize(estimatedModuleSize),
	_count(count)
{
}

/**
* <p>Determines if this finder pattern "about equals" a finder pattern at the stated
* position and size -- meaning, it is at nearly the same center with nearly the same size.</p>
*/
bool
FinderPattern::aboutEquals(float moduleSize, float i, float j) const
{
	if (std::abs(i - y()) <= moduleSize && std::abs(j - x()) <= moduleSize) {
		float moduleSizeDiff = std::abs(moduleSize - _estimatedModuleSize);
		return moduleSizeDiff <= 1.0f || moduleSizeDiff <= _estimatedModuleSize;
	}
	return false;
}

/**
* Combines this object's current estimate of a finder pattern position and module size
* with a new estimate. It returns a new {@code FinderPattern} containing a weighted average
* based on count.
*/
FinderPattern
FinderPattern::combineEstimate(float i, float j, float newModuleSize) const
{
	int combinedCount = _count + 1;
	float combinedX = (_count * x() + j) / combinedCount;
	float combinedY = (_count * y() + i) / combinedCount;
	float combinedModuleSize = (_count * _estimatedModuleSize + newModuleSize) / combinedCount;
	return {combinedX, combinedY, combinedModuleSize, combinedCount};
}

} // QRCode
} // ZXing
