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

#include "slib/core/definition.h"

#if !defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/core/asset.h"

#include "slib/core/file.h"
#include "slib/core/system.h"
#include "slib/core/app.h"
#include "slib/core/platform.h"

namespace slib
{

	sl_bool Assets::isBasedOnFileSystem()
	{
		return sl_true;
	}

#if !defined(SLIB_PLATFORM_IS_APPLE) && !defined(SLIB_PLATFORM_IS_TIZEN)
	String Assets::getFilePath(const StringParam& path)
	{
		String name = File::makeSafeFilePath(path);
		if (name.isNotEmpty()) {
			return String::join(Application::getApplicationDirectory(), "/", name);
		}
		return sl_null;
	}
#endif

	Memory Assets::readAllBytes(const StringParam& _path)
	{
		String path = Assets::getFilePath(_path);
		if (path.isNotEmpty()) {
			return File::readAllBytes(path);
		}
		return sl_null;
	}

}

#endif
