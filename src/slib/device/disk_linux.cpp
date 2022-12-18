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
#include <string.h>

using namespace std;

namespace slib
{

#define UDEVADM "udevadm info --query=all --name=/dev/sd%c | grep -E 'ID_BUS|ID_SERIAL_SHORT' | awk '{print $2}'"

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		String ret;
		FILE *cmd;

		char cmd_line[100] = { 0 };
		sprintf(cmd_line, UDEVADM, 0x61 + diskNo);
		if (cmd = popen(cmd_line, "r"))
		{
			char out_line[300] = { 0 };
			int nRead = 0;
			if (nRead = fread(out_line, 1, sizeof(out_line), cmd))
			{
				out_line[nRead] = 0;

				char* pszFind = NULL;
				if (pszFind = strstr(out_line, "ID_BUS"))
				{
					if (strncasecmp(pszFind, "ID_BUS=USB", 10))
					{
						if (pszFind = strstr(out_line, "ID_SERIAL_SHORT"))
						{
							ret = String::from(pszFind + 16);
						}
					}
				}
			}
			pclose(cmd);
		}

		return ret;
	}

}

#endif
