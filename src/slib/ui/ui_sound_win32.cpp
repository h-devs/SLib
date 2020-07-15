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

#include "slib/ui/sound.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/core/platform_windows.h"

namespace slib
{

	namespace priv
	{
		namespace ui_sound
		{

			HMODULE g_hModuleWinmm = NULL;

			typedef BOOL (WINAPI* TYPE_PlaySound)(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);
			TYPE_PlaySound g_funcPlaySound;

			void Prepare()
			{
				if (g_hModuleWinmm) {
					return;
				}
				g_hModuleWinmm = LoadLibraryW(L"winmm.dll");
				if (g_hModuleWinmm) {
					g_funcPlaySound = (TYPE_PlaySound)(GetProcAddress(g_hModuleWinmm, "PlaySoundW"));
				}
			}

		}
	}

	using namespace priv::ui_sound;

	void UISound::play(UISoundAlias sound)
	{
		Prepare();
		g_funcPlaySound((LPCWSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);
	}

	void UISound::stop()
	{
		g_funcPlaySound(NULL, NULL, 0);
	}

}

#endif