/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/device/printer.h"

#include "slib/platform/win32/wmi.h"

namespace slib
{

	List<PrinterInfo> Printer::getDevices()
	{
		List<PrinterInfo> ret;
		ListElements<VariantMap> items(win32::Wmi::getQueryResponseRecords(L"SELECT * FROM Win32_Printer", L"Name", L"Default", L"Network", L"PortName", L"PrintProcessor"));
		for (sl_size i = 0; i < items.count; i++) {
			PrinterInfo printer;
			VariantMap& item = items[i];
			printer.name = item.getValue("Name").getString();
			printer.flagDefault = item.getValue("Default").getBoolean();
			printer.flagNetwork = item.getValue("Network").getBoolean();
			printer.port = item.getValue("PortName").getString();
			printer.processor = item.getValue("PrintProcessor").getString();
			ret.add_NoLock(Move(printer));
		}
		return ret;
	}

}

#endif