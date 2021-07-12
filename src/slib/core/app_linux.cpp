/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "slib/core/app.h"

#include "slib/core/system.h"
#include "slib/core/file.h"
#include "slib/core/string_buffer.h"

namespace slib
{

	void Application::registerRunAtStartup(const StringParam& _appName, const StringParam& _path)
	{
		String path = _path.toString();
		if (path.isEmpty()) {
			path = getApplicationPath();
		}
		String appName = _appName.toString();
		if (appName.isEmpty()) {
			appName = File::getFileNameOnly(path);
		}
		String appId;
		Ref<Application> app = getApp();
		if (app.isNotNull()) {
			appId = app->getApplicationId();
		}
		if (appId.isEmpty()) {
			appId = appName;
		}
		String pathAutoStart = System::getHomeDirectory() + "/.config/autostart/";
		String pathDesktopFile = String::join(pathAutoStart, appId, ".desktop");
		StringBuffer sb;
		sb.addStatic("[Desktop Entry]\nName=");
		sb.add(appName);
		sb.addStatic("\nExec=");
		sb.add(path);
		sb.addStatic("\nType=Application\nNoDisplay=false\nHidden=false\nX-GNOME-Autostart-enabled=true");
		if (!(File::exists(pathAutoStart))) {
			File::createDirectories(pathAutoStart);
		}
		File::writeAllTextUTF8(pathDesktopFile, sb.merge());
	}

	void Application::registerRunAtStartup(const StringParam& path)
	{
		registerRunAtStartup(sl_null, path);
	}

	void Application::registerRunAtStartup()
	{
		registerRunAtStartup(sl_null, sl_null);
	}

	void Application::unregisterRunAtStartup(const StringParam& path)
	{
	}

	void Application::unregisterRunAtStartup()
	{
	}

	void Application::registerAtStartMenu(const StartMenuParam& param)
	{
		StringParam appId = param.appId;
		if (appId.isEmpty()) {
			Ref<Application> app = getApp();
			if (app.isNotNull()) {
				appId = app->getApplicationId();
			}
			if (appId.isNull()) {
				return;
			}
		}
		String pathApps = System::getHomeDirectory() + "/.local/share/applications/";
		String pathDesktopFile = String::join(pathApps, appId, ".desktop");
		StringBuffer sb;
		sb.addStatic("[Desktop Entry]\nName=");
		sb.add(param.appName.toString());
		sb.addStatic("\nExec=");
		if (param.executablePath.isNotNull()) {
			sb.add(param.executablePath.toString());
		} else {
			sb.add(getApplicationPath());
		}
		sb.addStatic("\nIcon=");
		sb.add(param.iconPath.toString());
		sb.addStatic("\nType=Application\nCategories=");
		sb.add(param.categories.toString());
		if (!(File::exists(pathApps))) {
			File::createDirectories(pathApps);
		}
		File::writeAllTextUTF8(pathDesktopFile, sb.merge());
	}

}

#endif
