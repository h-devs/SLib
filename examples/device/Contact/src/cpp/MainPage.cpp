#include "MainPage.h"

#include <slib/data/contact.h>

void MainPage::onOpen()
{
	UI::dispatchToUiThread([this]{
		auto readContacts = [this]() {
			StringBuffer sb;
			auto contacts = Device::getAllContacts();
			for (auto&& contact : contacts) {
				sb.add(contact.toJson().toJsonString());
				sb.add("\n");
			}
			lblReport->setText(sb.merge());
		};
#ifdef SLIB_PLATFORM_IS_ANDROID
		Application::grantPermissions(AppPermissions::ReadContacts, readContacts);
#else
		readContacts();
#endif
	}, 1000);
}
