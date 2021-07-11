/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_SYSTEM_TRAY_ICON
#define CHECKHEADER_SLIB_UI_SYSTEM_TRAY_ICON

#include "menu.h"
#include "event.h"
#include "common_dialogs.h"

namespace slib
{

	class SystemTrayIcon;

	class SLIB_EXPORT SystemTrayIconParam
	{
	public:
		String identifier; // [Linux] id of tray icon
		String iconName; // [Win32] Resource name, [macOS] Image name ([NSImage imageNamed:]), [Linux] Gnome standard icon name or image file path (https://developer.gnome.org/icon-naming-spec/)
		Ref<Drawable> icon; // [Win32, macOS] Supported, [Linux] Not supported on modern desktops
		String toolTip;
		sl_bool flagHighlight;
		Ref<Menu> menu;
		
		Function<void(SystemTrayIcon*, UIEvent*)> onClick;
		Function<void(SystemTrayIcon*, UIEvent*)> onRightClick;
		Function<void(SystemTrayIcon*, UIEvent*)> onKeySelect;
		Function<void(SystemTrayIcon*, UIEvent*)> onEvent;
		
		Function<void(SystemTrayIcon*)> onShowBalloon;
		Function<void(SystemTrayIcon*)> onHideBalloon;
		Function<void(SystemTrayIcon*)> onClickBalloon;
		Function<void(SystemTrayIcon*)> onBalloonTimeout;

	public:
		SystemTrayIconParam();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SystemTrayIconParam)
		
	};

	class SystemTrayIconNotifyParam;

	class SLIB_EXPORT SystemTrayIcon : public Object
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		SystemTrayIcon();
		
		~SystemTrayIcon();
		
	public:
		static Ref<SystemTrayIcon> create(const SystemTrayIconParam& param);
		
	public:
		Ref<Drawable> getIcon();
		
		void setIcon(const Ref<Drawable>& icon);
		
		String getIconName();
		
		void setIconName(const String& name);
		
		String getToolTip();
		
		void setToolTip(const String& toolTip);
		
		Ref<Menu> getMenu();
		
		void setMenu(const Ref<Menu>& menu);
		
		enum class NotifyIcon
		{
			None = 0,
			Information = 1,
			Warning = 2,
			Error = 3
		};

		void notify(const SystemTrayIconNotifyParam& param);
		
		void notify(const String& title, const String& message);
		
		void notify(const String& title, const String& message, const Ref<Drawable>& icon);
		
		void notify(const String& title, const String& message, const String& iconName);
		
		void notify(const String& title, const String& message, NotifyIcon icon);

	public:
		void dispatchClick(UIEvent* ev);
		
		void dispatchRightClick(UIEvent* ev);
		
		void dispatchKeySelect(UIEvent* ev);
		
		void dispatchEvent(UIEvent* ev);

		void dispatchShowBalloon();

		void dispatchHideBalloon();

		void dispatchClickBalloon();

		void dispatchBalloonTimeout();

	protected:
		String m_identifier;
		AtomicRef<Drawable> m_icon;
		AtomicString m_iconName;
		AtomicString m_toolTip;
		sl_bool m_flagHighlight;
		AtomicRef<Menu> m_menu;
		
		Function<void(SystemTrayIcon*, UIEvent*)> m_onClick;
		Function<void(SystemTrayIcon*, UIEvent*)> m_onRightClick;
		Function<void(SystemTrayIcon*, UIEvent*)> m_onKeySelect;
		Function<void(SystemTrayIcon*, UIEvent*)> m_onEvent;
		
		Function<void(SystemTrayIcon*)> m_onShowBalloon;
		Function<void(SystemTrayIcon*)> m_onHideBalloon;
		Function<void(SystemTrayIcon*)> m_onClickBalloon;
		Function<void(SystemTrayIcon*)> m_onBalloonTimeout;

	protected:
		void _init(const SystemTrayIconParam& param);
		
	protected:
		virtual void setIcon_NI(const Ref<Drawable>& icon, const String& name) = 0;
		
		virtual void setToolTip_NI(const String& toolTip) = 0;
				
		virtual void setMenu_NI(const Ref<Menu>& menu) = 0;
		
		virtual void notify_NI(const SystemTrayIconNotifyParam& param);
		
	};

	class SLIB_EXPORT SystemTrayIconNotifyParam
	{
	public:
		String title;
		String message;

		SystemTrayIcon::NotifyIcon iconType;
		String iconName;
		Ref<Drawable> icon;

		sl_uint32 timeout; // In milliseconds (not used on Windows Vista and later)
		sl_bool flagSound;
		sl_bool flagLargeIcon;
		
	public:
		SystemTrayIconNotifyParam();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SystemTrayIconNotifyParam)
		
	};

}

#endif
