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

#include "slib/device/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX)

#include "slib/device/device.h"

#include <stdio.h>

namespace slib
{

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		sl_char8 chDrive = (sl_char8)('a' + diskNo);
		String cmd = String::concat(StringView::literal("udevadm info --query=all --name=/dev/sd"), StringView(&chDrive, 1), StringView::literal(" | grep -E 'ID_BUS|ID_SERIAL_SHORT' | awk '{print $2}'"));
		FILE* fp = popen(cmd.getData(), "r");
		if (fp) {
			char buf[1024];
			size_t n = fread(buf, 1, sizeof(buf), fp);
			pclose(fp);
			if (n) {
				StringView output(buf, (sl_size)n);
				sl_reg index = output.indexOf(StringView::literal("ID_BUS="));
				if (index >= 0) {
					index += 7;
					if (!(output.substring(index, index + 3).equalsIgnoreCase(StringView::literal("USB")))) {
						index = output.indexOf(StringView::literal("ID_SERIAL_SHORT="), index);
						if (index >= 0) {
							return output.substring(index + 16);
						}
					}
				}
			}
		}
		return sl_null;
	}

}

#endif
