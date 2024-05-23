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

#include "slib/ui/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "slib/ui/notification.h"

#include "slib/ui/platform.h"

#include "../resources.h"

namespace slib
{

	namespace {

		static String GetIdentifier(const UserNotificationMessage& message)
		{
			if (message.identifier.isNotNull()) {
				return message.identifier;
			}
			return String::fromUint32(message.id);
		}

		class UserNotificationImpl : public UserNotification
		{
		public:
			GNotification* m_notification;
			String m_id;

		public:
			UserNotificationImpl()
			{
				m_notification = sl_null;
			}

			~UserNotificationImpl()
			{
				if (m_notification) {
					g_object_unref(m_notification);
				}
			}

		public:
			static Ref<UserNotificationImpl> create(const UserNotificationMessage& message)
			{
				auto funcCreate = gio::getApi_g_notification_new();
				if (!funcCreate) {
					return sl_null;
				}
				GtkApplication* app = UIPlatform::getApp();
				if (!app) {
					return sl_null;
				}
				StringCstr title(message.title);
				GNotification* notification = funcCreate(title.getData());
				if (notification) {
					Ref<UserNotificationImpl> ret = new UserNotificationImpl;
					if (ret.isNotNull()) {
						StringCstr body(message.content);
						gio::getApi_g_notification_set_body()(notification, body.getData());
						ret->m_notification = notification;
						ret->m_id = GetIdentifier(message);
						StringCstr id(ret->m_id);
						gio::getApi_g_application_send_notification()((GApplication*)app, id.getData(), notification);
						return ret;
					}
					g_object_unref(notification);
				}
				return sl_null;
			}

		public:
			void cancelPending() override
			{
			}

			void removeFromDeliveredList() override
			{
				removeDeliveredNotification(m_id);
			}

		};

	}

	void UserNotification::startInternal()
	{
		UIPlatform::getApp();
	}

	Ref<UserNotification> UserNotification::add(const UserNotificationMessage& message)
	{
		return Ref<UserNotification>::cast(UserNotificationImpl::create(message));
	}

	void UserNotification::removePendingNotification(const String& identifier)
	{
	}

	void UserNotification::removePendingNotification(sl_uint32 _id)
	{
	}

	void UserNotification::removeAllPendingNotifications()
	{
	}

	void UserNotification::removeDeliveredNotification(const String& identifier)
	{
		GtkApplication* app = UIPlatform::getApp();
		if (app) {
			StringCstr id(identifier);
			gio::getApi_g_application_withdraw_notification()((GApplication*)app, id.getData());
		}
	}

	void UserNotification::removeDeliveredNotification(sl_uint32 _id)
	{
		removeDeliveredNotification(String::fromUint32(_id));
	}

	void UserNotification::removeAllDeliveredNotifications()
	{
		// Not Supported
	}

}

#endif
