/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

namespace slib
{

	sl_bool SAppDocument::_parseMenuResource(const String& localNamespace, const Ref<XmlElement>& element, sl_bool flagPopup)
	{
		if (element.isNull()) {
			return sl_false;
		}

		Ref<SAppMenuResource> menu = new SAppMenuResource;
		if (menu.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		menu->flagPopup = flagPopup;

		String name = element->getAttribute("name");
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_menu_name_is_empty);
			return sl_false;
		}
		if (!(SAppUtil::checkName(name.getData(), name.getLength()))) {
			logError(element, g_str_error_resource_menu_name_invalid, name);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_menus.find(name)) {
			logError(element, g_str_error_resource_menu_name_redefined, name);
			return sl_false;
		}

		menu->name = name;
		menu->localNamespace = localNamespace;

		ListLocker< Ref<XmlElement> > children(element->getChildElements());
		for (sl_size i = 0; i < children.count; i++) {
			Ref<XmlElement>& child = children[i];
			if (child.isNotNull()) {
				Ref<SAppMenuResourceItem> menuItem = _parseMenuResourceItem(localNamespace, child, menu.get(), SAppMenuResourceItem::all_platforms);
				if (menuItem.isNull()) {
					return sl_false;
				}
				if (!(menu->children.add(menuItem))) {
					logError(element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		}

		if (!(m_menus.put(name, menu))) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	Ref<SAppMenuResourceItem> SAppDocument::_parseMenuResourceItem(const String& localNamespace, const Ref<XmlElement>& element, SAppMenuResource* menu, int parentPlatforms)
	{
		if (element.isNull()) {
			return Ref<SAppMenuResourceItem>::null();
		}

		Ref<SAppMenuResourceItem> item = new SAppMenuResourceItem;
		if (item.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return Ref<SAppMenuResourceItem>::null();
		}

		if (element->getName() == "submenu") {
			item->type = SAppMenuResourceItem::typeSubmenu;
		} else if (element->getName() == "item") {
			item->type = SAppMenuResourceItem::typeItem;
		} else if (element->getName() == "separator") {
			item->type = SAppMenuResourceItem::typeSeparator;
		} else {
			logError(element, g_str_error_resource_menu_children_tag_invalid, element->getName());
			return Ref<SAppMenuResourceItem>::null();
		}

		String name = element->getAttribute("name");
		if (name.isEmpty()) {
			if (item->type != SAppMenuResourceItem::typeSeparator) {
				logError(element, g_str_error_resource_menu_name_is_empty);
				return Ref<SAppMenuResourceItem>::null();
			}
		} else {
			if (name == "root") {
				logError(element, g_str_error_resource_menu_name_is_root);
				return Ref<SAppMenuResourceItem>::null();
			}
			if (!(SAppUtil::checkName(name.getData(), name.getLength()))) {
				logError(element, g_str_error_resource_menu_name_invalid, name);
				return Ref<SAppMenuResourceItem>::null();
			}
		}
		item->name = name;

		String strPlatform = element->getAttribute("platform");
		if (strPlatform.isEmpty()) {
			item->platformFlags = parentPlatforms;
		} else {
			if (strPlatform == "no-mac") {
				item->platformFlags = SAppMenuResourceItem::no_mac;
			} else if (strPlatform == "no-windows") {
				item->platformFlags = SAppMenuResourceItem::no_windows;
			} else if (strPlatform == "no-linux") {
				item->platformFlags = SAppMenuResourceItem::no_linux;
			} else {
				item->platformFlags = 0;
				if (strPlatform.contains("mac")) {
					item->platformFlags |= SAppMenuResourceItem::mac;
				}
				if (strPlatform.contains("windows")) {
					item->platformFlags |= SAppMenuResourceItem::windows;
				}
				if (strPlatform.contains("linux")) {
					item->platformFlags |= SAppMenuResourceItem::linux;
				}
			}
			item->platformFlags &= parentPlatforms;
		}
		if (item->platformFlags == 0) {
			logError(element, g_str_error_resource_menu_platform_invalid, strPlatform);
			return Ref<SAppMenuResourceItem>::null();
		}

		if (name.isNotEmpty()) {
			if (item->platformFlags & SAppMenuResourceItem::mac) {
				if (menu->itemsMac.find(name)) {
					logError(element, g_str_error_resource_menu_item_name_redefined, name);
					return Ref<SAppMenuResourceItem>::null();
				}
				if (!(menu->itemsMac.put(name, item))) {
					logError(element, g_str_error_out_of_memory);
					return Ref<SAppMenuResourceItem>::null();
				}
			}
			if (item->platformFlags & SAppMenuResourceItem::windows) {
				if (menu->itemsWindows.find(name)) {
					logError(element, g_str_error_resource_menu_item_name_redefined, name);
					return Ref<SAppMenuResourceItem>::null();
				}
				if (!(menu->itemsWindows.put(name, item))) {
					logError(element, g_str_error_out_of_memory);
					return Ref<SAppMenuResourceItem>::null();
				}
			}
			if (item->platformFlags & SAppMenuResourceItem::linux) {
				if (menu->itemsLinux.find(name)) {
					logError(element, g_str_error_resource_menu_item_name_redefined, name);
					return Ref<SAppMenuResourceItem>::null();
				}
				if (!(menu->itemsLinux.put(name, item))) {
					logError(element, g_str_error_out_of_memory);
					return Ref<SAppMenuResourceItem>::null();
				}
			}
		}

		if (item->type != SAppMenuResourceItem::typeSeparator) {
			String title = element->getAttribute("title");
			if (title == "@") {
				item->title.flagDefined = sl_true;
				item->title.flagReferResource = sl_true;
				item->title.valueOrName = String::format("menu_%s_%s", menu->name, name);
			} else {
				if (!(item->title.parse(title, element))) {
					logError(element, g_str_error_resource_menu_title_refer_invalid, title);
					return Ref<SAppMenuResourceItem>::null();
				}
			}

			String strChecked;
			strChecked = element->getAttribute("checked");
			if (!(item->checked.parse(strChecked))) {
				logError(element, g_str_error_resource_menu_checked_invalid, strChecked);
				return Ref<SAppMenuResourceItem>::null();
			}

			String strIcon;
			strIcon = element->getAttribute("icon");
			if (!(item->icon.parseWhole(strIcon, element))) {
				logError(element, g_str_error_resource_menu_icon_invalid, strIcon);
				return Ref<SAppMenuResourceItem>::null();
			}
			strIcon = element->getAttribute("checkedIcon");
			if (!(item->checkedIcon.parseWhole(strIcon, element))) {
				logError(element, g_str_error_resource_menu_icon_invalid, strIcon);
				return Ref<SAppMenuResourceItem>::null();
			}
		}

		if (item->type == SAppMenuResourceItem::typeItem) {
			String strShortcutKey = element->getAttribute("shortcutKey");
			if (strShortcutKey.isEmpty()) {
				item->shortcutKey = 0;
			} else {
				if (!(item->shortcutKey.parse(strShortcutKey))) {
					logError(element, g_str_error_resource_menu_shortcutKey_invalid, strShortcutKey);
					return Ref<SAppMenuResourceItem>::null();
				}
			}
			strShortcutKey = element->getAttribute("macShortcutKey");
			if (strShortcutKey.isNull()) {
				item->macShortcutKey = item->shortcutKey;
			} else if (strShortcutKey.isEmpty()) {
				item->macShortcutKey = 0;
			} else {
				if (!(item->macShortcutKey.parse(strShortcutKey))) {
					logError(element, g_str_error_resource_menu_macShortcutKey_invalid, strShortcutKey);
					return Ref<SAppMenuResourceItem>::null();
				}
			}
		}

		if (item->type == SAppMenuResourceItem::typeSubmenu) {
			ListLocker< Ref<XmlElement> > children(element->getChildElements());
			for (sl_size i = 0; i < children.count; i++) {
				Ref<XmlElement>& child = children[i];
				if (child.isNotNull()) {
					Ref<SAppMenuResourceItem> menuItem = _parseMenuResourceItem(localNamespace, child, menu, item->platformFlags);
					if (menuItem.isNull()) {
						return Ref<SAppMenuResourceItem>::null();
					}
					if (!(item->children.add(menuItem))) {
						logError(element, g_str_error_out_of_memory);
						return Ref<SAppMenuResourceItem>::null();
					}
				}
			}
		}

		return item;
	}

	sl_bool SAppDocument::_generateMenusCpp(const String& targetPath)
	{
		log(g_str_log_generate_cpp_menus_begin);

		StringBuffer sbHeader, sbCpp;
		sbHeader.add(String::format(
									"#pragma once%n%n"
									"#include <slib/ui/resource.h>%n%n"
									"namespace %s%n"
									"{%n\tnamespace menu%n\t{%n%n"
									, m_conf.generate_cpp_namespace));

		sbCpp.add(String::format(
								 "#include \"menus.h\"%n%n"
								 "#include \"strings.h\"%n"
								 "#include \"drawables.h\"%n%n"
								 "namespace %s%n"
								 "{%n\tnamespace menu%n\t{%n%n"
								 , m_conf.generate_cpp_namespace));


		for (auto&& pair : m_menus) {
			if (pair.value.isNotNull()) {

				sbHeader.add(String::format("\t\tSLIB_DECLARE_MENU_BEGIN(%s)%n", pair.key));
				if (pair.value->flagPopup) {
					sbCpp.add(String::format("\t\tSLIB_DEFINE_MENU_BEGIN(%s, sl_true)%n", pair.key));
				} else {
					sbCpp.add(String::format("\t\tSLIB_DEFINE_MENU_BEGIN(%s)%n", pair.key));
				}

				ListLocker< Ref<SAppMenuResourceItem> > items(pair.value->children);
				for (sl_size i = 0; i < items.count; i++) {
					Ref<SAppMenuResourceItem>& item = items[i];
					if (item.isNotNull()) {
						if (!_generateMenusCpp_Item(pair.value.get(), "root", SAppMenuResourceItem::all_platforms, item.get(), sbHeader, sbCpp, 3)) {
							return sl_false;
						}
					}
				}

				static sl_char8 strEndHeader[] = "\t\tSLIB_DECLARE_MENU_END\r\n\r\n";
				sbHeader.addStatic(strEndHeader, sizeof(strEndHeader)-1);
				static sl_char8 strEndCpp[] = "\t\tSLIB_DEFINE_MENU_END\r\n\r\n";
				sbCpp.addStatic(strEndCpp, sizeof(strEndCpp)-1);
			}
		}


		sbHeader.add("\t}\r\n}\r\n");
		sbCpp.add("\t}\r\n}\r\n");

		String pathHeader = targetPath + "/menus.h";
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = targetPath + "/menus.cpp";
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				logError(g_str_error_file_write_failed, pathCpp);
				return sl_false;
			}
		}

		return sl_true;
	}

	sl_bool SAppDocument::_generateMenusCpp_Item(SAppMenuResource* resource, const String& parentName, int parentPlatforms, SAppMenuResourceItem* item, StringBuffer& sbHeader, StringBuffer& sbCpp, int tabLevel)
	{
		String header, footer;
		if (item->platformFlags != parentPlatforms) {
			if (item->platformFlags == SAppMenuResourceItem::no_mac) {
				header = "#if !defined(SLIB_PLATFORM_IS_MACOS)\r\n";
				footer = "#endif\r\n";
			} else if (item->platformFlags == SAppMenuResourceItem::no_windows) {
				header = "#if !defined(SLIB_PLATFORM_IS_WIN32)\r\n";
				footer = "#endif\r\n";
			} else if (item->platformFlags == SAppMenuResourceItem::no_linux) {
				header = "#if !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)\r\n";
				footer = "#endif\r\n";
			} else if (item->platformFlags != SAppMenuResourceItem::all_platforms) {
				String s;
				if (item->platformFlags & SAppMenuResourceItem::mac) {
					s = "defined(SLIB_PLATFORM_IS_MACOS)";
				}
				if (item->platformFlags & SAppMenuResourceItem::windows) {
					if (s.isNotEmpty()) {
						s += " || ";
					}
					s += "defined(SLIB_PLATFORM_IS_WIN32)";
				}
				if (item->platformFlags & SAppMenuResourceItem::linux) {
					if (s.isNotEmpty()) {
						s += " || ";
					}
					s += "defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)";
				}
				header = String::format("#if %s\r\n", s);
				footer = "#endif\r\n";
			}
		}

		if (item->name.isNotEmpty()) {
			sbHeader.add(header);
		}
		sbCpp.add(header);
		String tab('\t', tabLevel);

		if (item->type == SAppMenuResourceItem::typeSubmenu) {

			sbHeader.add(tab);
			sbHeader.add(String::format("SLIB_DECLARE_SUBMENU(%s)%n", item->name));

			sbCpp.add(tab);
			if (!(item->icon.flagDefined) && !(item->checkedIcon.flagDefined)) {
				String strTitle;
				if (_getStringAccessString(resource->localNamespace, item->title, strTitle)) {
					if (item->checked.flagDefined) {
						sbCpp.add(String::format("SLIB_DEFINE_SUBMENU(%s, %s, %s, %s)%n", parentName, item->name, strTitle, item->checked.getAccessString()));
					} else {
						sbCpp.add(String::format("SLIB_DEFINE_SUBMENU(%s, %s, %s)%n", parentName, item->name, strTitle));
					}
				} else {
					return sl_false;
				}
			} else {
				String strTitle, strIcon;
				if (_getStringAccessString(resource->localNamespace, item->title, strTitle) &&
					_getDrawableAccessString(resource->localNamespace, item->icon, strIcon)
				) {
					if (item->checkedIcon.flagDefined) {
						String strCheckedIcon;
						if (_getDrawableAccessString(resource->localNamespace, item->checkedIcon, strCheckedIcon)) {
							sbCpp.add(String::format("SLIB_DEFINE_SUBMENU(%s, %s, %s, %s, %s, %s)%n", parentName, item->name, strTitle, strIcon, strCheckedIcon, item->checked.getAccessString()));
						} else {
							return sl_false;
						}
					} else {
						sbCpp.add(String::format("SLIB_DEFINE_SUBMENU(%s, %s, %s, %s)%n", parentName, item->name, strTitle, strIcon));
					}
				} else {
					return sl_false;
				}
			}

			ListLocker< Ref<SAppMenuResourceItem> > items(item->children);
			for (sl_size i = 0; i < items.count; i++) {
				Ref<SAppMenuResourceItem>& childItem = items[i];
				if (childItem.isNotNull()) {
					_generateMenusCpp_Item(resource, item->name, item->platformFlags, childItem.get(), sbHeader, sbCpp, tabLevel + 1);
				}
			}

		} else if (item->type == SAppMenuResourceItem::typeSeparator) {

			if (item->name.isNotEmpty()) {
				sbHeader.add(tab);
				sbHeader.add(String::format("SLIB_DECLARE_MENU_SEPARATOR(%s)%n", item->name));
				sbCpp.add(tab);
				sbCpp.add(String::format("SLIB_DEFINE_MENU_SEPARATOR(%s, %s)%n", parentName, item->name));
			} else {
				sbCpp.add(tab);
				sbCpp.add(String::format("SLIB_DEFINE_MENU_SEPARATOR_NONAME(%s)%n", parentName));
			}

		} else {

			sbHeader.add(tab);
			sbHeader.add(String::format("SLIB_DECLARE_MENU_ITEM(%s)%n", item->name));

			sbCpp.add(tab);
			String strShortcutKey = getShortcutKeyDefinitionString(item->shortcutKey, sl_false);
			String strMacShortcutKey = getShortcutKeyDefinitionString(item->macShortcutKey, sl_true);
			if (strShortcutKey != strMacShortcutKey) {
				strShortcutKey = String::format("SLIB_IF_PLATFORM_IS_MACOS(%s, %s)", strMacShortcutKey, strShortcutKey);
			}
			if (!(item->icon.flagDefined) && !(item->checkedIcon.flagDefined)) {
				String strTitle;
				if (_getStringAccessString(resource->localNamespace, item->title, strTitle)) {
					if (item->checked.flagDefined) {
						sbCpp.add(String::format("SLIB_DEFINE_MENU_ITEM(%s, %s, %s, %s, %s)%n", parentName, item->name, strTitle, strShortcutKey, item->checked.getAccessString()));
					} else {
						sbCpp.add(String::format("SLIB_DEFINE_MENU_ITEM(%s, %s, %s, %s)%n", parentName, item->name, strTitle, strShortcutKey));
					}
				} else {
					return sl_false;
				}
			} else {
				String strTitle, strIcon, strCheckedIcon;
				if (_getStringAccessString(resource->localNamespace, item->title, strTitle) &&
					_getDrawableAccessString(resource->localNamespace, item->icon, strIcon)
				) {
					if (item->checkedIcon.flagDefined) {
						if (_getDrawableAccessString(resource->localNamespace, item->checkedIcon, strCheckedIcon)) {
							sbCpp.add(String::format("SLIB_DEFINE_MENU_ITEM(%s, %s, %s, %s, %s, %s, %s)%n", parentName, item->name, strTitle, strShortcutKey, strIcon, strCheckedIcon, item->checked.getAccessString()));
						} else {
							return sl_false;
						}
					} else {
						sbCpp.add(String::format("SLIB_DEFINE_MENU_ITEM(%s, %s, %s, %s, %s)%n", parentName, item->name, strTitle, strShortcutKey, strIcon));
					}
				} else {
					return sl_false;
				}
			}
		}
		if (item->name.isNotEmpty()) {
			sbHeader.add(footer);
		}
		sbCpp.add(footer);

		return sl_true;

	}

	sl_bool SAppDocument::_getMenuAccessString(const String& localNamespace, const SAppMenuValue& value, sl_bool flagForWindow, String& name, String& result)
	{
		if (!(value.flagDefined)) {
			result = "slib::Ref<slib::Menu>::null()";
			return sl_true;
		}
		if (value.flagNull) {
			result = "slib::Ref<slib::Menu>::null()";
			return sl_true;
		}
		if (_checkMenuName(localNamespace, value.resourceName, value.referingElement, &name)) {
			result = String::format(flagForWindow ? "menu::%s::create()" : "menu::%s::get()", name);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool SAppDocument::_getMenuValue(const String& localNamespace, const SAppMenuValue& value, Ref<Menu>& result)
	{
		if (!(value.flagDefined) || value.flagNull) {
			result = Ref<Menu>::null();
			return sl_true;
		}
		if (value.flagNull) {
			result = Ref<Menu>::null();
			return sl_true;
		}
		Ref<SAppMenuResource> res;
		if (_checkMenuName(localNamespace, value.resourceName, value.referingElement, sl_null, &res)) {
			Ref<Menu> menu = Menu::create(res->flagPopup);
			if (menu.isNull()) {
				logError(g_str_error_out_of_memory);
				return sl_false;
			}
			ListLocker< Ref<SAppMenuResourceItem> > items(res->children);
			for (sl_size i = 0; i < items.count; i++) {
				Ref<SAppMenuResourceItem>& item = items[i];
				if (item.isNotNull()) {
					if (!(_getMenuValue_Item(res.get(), menu, item.get()))) {
						logError(g_str_error_load_menu_failed, value.resourceName);
						return sl_false;
					}
				}
			}
			result = menu;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool SAppDocument::_checkMenuValue(const String& localNamespace, const SAppMenuValue& value)
	{
		if (!(value.flagDefined)) {
			return sl_true;
		}
		if (value.flagNull) {
			return sl_true;
		}
		return _checkMenuName(localNamespace, value.resourceName, value.referingElement);
	}

	sl_bool SAppDocument::_checkMenuName(const String& localNamespace, const String& name, const Ref<XmlElement>& element, String* outName, Ref<SAppMenuResource>* outResource)
	{
		if (getItemFromMap(m_menus, localNamespace, name, outName, outResource)) {
			return sl_true;
		} else {
			logError(element, g_str_error_menu_not_found, name);
			return sl_false;
		}
	}

	sl_bool SAppDocument::_getMenuValue_Item(SAppMenuResource* resource, const Ref<Menu>& parent, SAppMenuResourceItem* item)
	{
#if defined(SLIB_PLATFORM_IS_MACOS)
		if (!(item->platformFlags & SAppMenuResourceItem::mac)) {
			return sl_true;
		}
#elif defined(SLIB_PLATFORM_IS_WIN32)
		if (!(item->platformFlags & SAppMenuResourceItem::windows)) {
			return sl_true;
		}
#elif defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
		if (!(item->platformFlags & SAppMenuResourceItem::linux)) {
			return sl_true;
		}
#endif

		if (item->type == SAppMenuResourceItem::typeSubmenu) {

			Ref<Menu> submenu = Menu::create();
			if (submenu.isNull()) {
				return sl_false;
			}
			if (!(item->icon.flagDefined) && !(item->checkedIcon.flagDefined)) {
				String title;
				if (_getStringValue(resource->localNamespace, item->title, title)) {
					parent->addSubmenu(submenu, title);
				} else {
					return sl_false;
				}
			} else {
				String title;
				Ref<Drawable> icon, checkedIcon;
				if (_getStringValue(resource->localNamespace, item->title, title) &&
					_getDrawableValue(resource->localNamespace, item->icon, icon) &&
					_getDrawableValue(resource->localNamespace, item->checkedIcon, checkedIcon)
					) {
					parent->addSubmenu(submenu, title, icon, checkedIcon);
				} else {
					return sl_false;
				}
			}

			ListLocker< Ref<SAppMenuResourceItem> > items(item->children);
			for (sl_size i = 0; i < items.count; i++) {
				Ref<SAppMenuResourceItem>& childItem = items[i];
				if (childItem.isNotNull()) {
					if (!(_getMenuValue_Item(resource, submenu.get(), childItem.get()))) {
						return sl_false;
					}
				}
			}

		} else if (item->type == SAppMenuResourceItem::typeSeparator) {

			parent->addSeparator();

		} else {

			KeycodeAndModifiers km = SLIB_IF_PLATFORM_IS_MACOS(item->macShortcutKey, item->shortcutKey);

			if (!(item->icon.flagDefined) && !(item->checkedIcon.flagDefined)) {
				String title;
				if (_getStringValue(resource->localNamespace, item->title, title)) {
					parent->addMenuItem(title, km);
				} else {
					return sl_false;
				}
			} else {
				String title;
				Ref<Drawable> icon, checkedIcon;
				if (_getStringValue(resource->localNamespace, item->title, title) &&
					_getDrawableValue(resource->localNamespace, item->icon, icon) &&
					_getDrawableValue(resource->localNamespace, item->checkedIcon, checkedIcon)
					) {
					parent->addMenuItem(title, km, icon, checkedIcon);
				} else {
					return sl_false;
				}
			}

		}

		return sl_true;

	}

}
