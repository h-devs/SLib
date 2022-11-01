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

#include "util.h"

#include <slib/core.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	String pathRootDir;
	Println("Please input the path of the source directory:");
	while (1) {
		pathRootDir = Console::readLine().trim();
		if (pathRootDir.isNotEmpty()) {
			if (File::isDirectory(pathRootDir)) {
				break;
			} else {
				Println("[Error] The source path is not directory: %s", pathRootDir);
				return -1;
			}
		}
	}

	String copyrightNotice;
	Println("Please input the copyright notice ending a line \"end\":");
	while (1) {
		String line = Console::readLine();
		if (line.trim() == "end") {
			break;
		}
		copyrightNotice += line;
	}
	copyrightNotice = copyrightNotice.trim();

	Println("Started adding copyright notice");
	applyCopyrightNoticeToAllSourceFilesInPath(pathRootDir, copyrightNotice);
	Println("Finished adding copyright notice");

	return 0;
}
