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

#if defined(SLIB_PLATFORM_IS_APPLE)

#include "slib/core/asset.h"

#include "slib/core/file.h"
#include "slib/core/apple/platform.h"

namespace slib
{

	String Assets::getAssetFilePath(const StringParam& _path)
	{
		StringData path(_path);
		String fileExt = File::getFileExtension(path);
		String fileName = File::getFileNameOnly(path);
		String dirPath = File::getParentDirectoryPath(path);
		
		NSString* strFileName = Apple::getNSStringFromString(fileName);
		NSString* strFolderPath = Apple::getNSStringFromString(dirPath);
		NSString* strFileExtension = Apple::getNSStringFromString(fileExt);
		
		NSString *filePath = [[NSBundle mainBundle] pathForResource:strFileName ofType:strFileExtension inDirectory:strFolderPath];
		return Apple::getStringFromNSString(filePath);
	}
	
}

#endif
