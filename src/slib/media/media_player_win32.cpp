/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/media/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/media/media_player.h"
#include "slib/media/wave_player.h"

#include "slib/system/asset.h"
#include "slib/platform/win32/windows.h"

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace slib
{

	Ref<MediaPlayer> MediaPlayer::_createNative(const MediaPlayerParam& param)
	{
		return sl_null;
	}


	sl_bool WavePlayer::play(const WavePlayerParam& param)
	{
		DWORD flags = param.flagSynchronous ? SND_SYNC : SND_ASYNC;
		if (param.flagLoop) {
			flags |= SND_LOOP;
		}
		if (param.content.isNotNull()) {
			flags |= SND_MEMORY;
			return PlaySoundW((LPCWSTR)(param.content.getData()), NULL, flags) != FALSE;
		}
		if (param.filePath.isNotNull()) {
			flags |= SND_FILENAME;
			StringCstr16 filePath(param.filePath);
			return PlaySoundW((LPCWSTR)(filePath.getData()), NULL, flags) != FALSE;
		}
		if (param.resourceName.isNotNull()) {
			flags |= SND_RESOURCE;
			StringCstr16 resourceName(param.resourceName);
			return PlaySoundW((LPCWSTR)(resourceName.getData()), GetModuleHandleW(NULL), flags) != FALSE;
		}
		if (param.assetFileName.isNotNull()) {
			flags |= SND_FILENAME;
			String16 filePath = String16::from(Assets::getFilePath(param.assetFileName));
			return PlaySoundW((LPCWSTR)(filePath.getData()), NULL, flags) != FALSE;
		}
		return sl_false;
	}

	void WavePlayer::stopAll()
	{
		PlaySoundW(NULL, NULL, 0);
	}

}

#endif
