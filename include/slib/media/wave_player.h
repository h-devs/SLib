/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_MEDIA_WAVE_PLAYER
#define CHECKHEADER_SLIB_MEDIA_WAVE_PLAYER

#include "definition.h"

#include "../core/memory.h"
#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT WavePlayerParam
	{
	public:
		StringParam filePath;
		StringParam assetFileName;
		StringParam resourceName;
		Memory content;

		sl_bool flagSynchronous; // Default: false
		sl_bool flagLoop; // Default: false

	public:
		WavePlayerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(WavePlayerParam)

	};

	class SLIB_EXPORT WavePlayer
	{
	public:
		static sl_bool play(const WavePlayerParam& param);

		static sl_bool play(const Memory& wave);

		static sl_bool playSynchronous(const Memory& wave);

		static sl_bool playFile(const StringParam& path);

		static sl_bool playFileSynchronous(const StringParam& path);

		static sl_bool playAsset(const StringParam& fileName);

		static sl_bool playAssetSynchronous(const StringParam& fileName);

		static sl_bool playResource(const StringParam& resourceName);

		static sl_bool playResourceSynchronous(const StringParam& resourceName);

		static void stopAll();

	};

}

#endif
