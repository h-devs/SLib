/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_SYSTEM_SERVICE
#define CHECKHEADER_SLIB_SYSTEM_SERVICE

#include "definition.h"

#include "../core/app.h"

namespace slib
{

	class Event;

	class SLIB_EXPORT Service : public Application
	{
		SLIB_DECLARE_OBJECT

	public:
		Service();

		~Service();

	public:
		AppType getAppType() override;

	protected:
		virtual String getServiceId() = 0;

		virtual sl_bool onStartService() = 0;

		virtual void onStopService() = 0;

		sl_int32 doRun() override;

		sl_int32 onRunApp() override;

		String getApplicationId() override;

	public:
		sl_bool startService();

		sl_bool stopService();

		void statusService();

		sl_int32 runService();

	public:
		static Ref<Service> getApp();

		static void quitApp();

		void quit();

	private:
		sl_bool _tryPlatformService();

		void _runPlatformService();

	protected:
		sl_bool m_flagPlatformService;
		Ref<Event> m_eventQuit;
		sl_bool m_flagRequestQuit;

	};

}

#endif
