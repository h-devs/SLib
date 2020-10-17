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

#include "slib/storage/dokany.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "dokany/dokan.h"

#include "slib/core/service_manager.h"

#include "dokany/dokany_core_files.h"

namespace slib
{

	namespace priv
	{
		namespace dokany
		{

			static String GetDriverFilePath()
			{

			}

		}
	}

	using namespace priv::dokany;
	
	sl_bool DokanyControl::install()
	{
		if (Dokany::getDriverState() != ServiceState::None) {
			return sl_true;
		}
		
		ServiceCreateParam param;
		return ServiceManager::createAndStart(param);
	}

	void DokanyControl::uninstall()
	{
	}

	void DokanyControl::unmount(const StringParam& mountPoint)
	{
	}

}

#endif