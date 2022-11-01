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

#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{

	if (argc < 3) {
		Println("Usage: StaticWeb <PORT> <DOCUMENT_PATH> [d]");
		return 0;
	}

	// Set configuration
	HttpServerParam param;
	param.port = StringView(argv[1]).parseUint32();
	param.flagUseWebRoot = sl_true;
	param.webRootPath = argv[2];
	StringView flags(argc > 3 ? argv[3] : sl_null);
	param.flagLogDebug = flags.contains('d');

	Ref<HttpServer> server = HttpServer::create(param);
	if (server.isNull()) {
		return -1;
	}

	Console::println("Server is running on port: %d, Webroot: %s", param.port, param.webRootPath);

	for(;;) {
		Console::println("\nPress x to exit!!!");
		if (Console::readChar() == 'x') {
			break;
		}
	}
	return 0;
}
