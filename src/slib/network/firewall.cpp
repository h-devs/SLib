/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/network/firewall.h"

#include "slib/core/system.h"
#include "slib/core/file.h"
#include "slib/core/process.h"
#include "slib/core/variant.h"

namespace slib
{

	void Firewall::allowApplication(const StringParam& path)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		String name = File::getFileName(path);
		if (Process::isCurrentProcessAdmin()) {
			String command = String::format("netsh advfirewall firewall add rule name=\"%s\" dir=in action=allow program=\"%s\"", name, path);
			System::execute(command);
		} else {
			Process::runAsAdmin(System::getSystemDirectory() + "\\netsh.exe", "advfirewall", "firewall", "add", "rule", String::format("name=\"%s\"", name), "dir=in", "action=allow", String::format("program=\"%s\"", path));
		}
#endif
	}

	void Firewall::disallowApplication(const StringParam& path)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		String name = File::getFileName(path);
		if (Process::isCurrentProcessAdmin()) {
			String command = String::format("netsh advfirewall firewall delete rule name=\"%s\" program=\"%s\"", name, path);
			System::execute(command);
		} else {
			Process::runAsAdmin(System::getSystemDirectory() + "\\netsh.exe", "advfirewall", "firewall", "delete", "rule", String::format("name=\"%s\"", name), String::format("program=\"%s\"", path));
		}
#endif
	}

}
