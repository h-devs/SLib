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

namespace slib
{

	sl_bool SAppDocument::_parseStringResources(const String& localNamespace, const Ref<XmlElement>& element, const Locale& localeDefault, const String16& source)
	{
		if (element.isNull()) {
			return sl_false;
		}

		String strLocale = element->getAttribute("locale");
		Locale locale = Locale::Unknown;
		if (strLocale.isNotEmpty()) {
			locale = Locale(strLocale);
			if (locale.isInvalid()) {
				logError(element, g_str_error_resource_string_locale_invalid, strLocale);
				return sl_false;
			}
			if (_isExcludedLocale(locale)) {
				return sl_true;
			}
		}
		if (locale == Locale::Unknown) {
			locale = localeDefault;
		}

		ListLocker< Ref<XmlElement> > children(element->getChildElements());
		for (sl_size i = 0; i < children.count; i++) {
			Ref<XmlElement>& child = children[i];
			if (child.isNotNull()) {
				String tagName = child->getName();
				sl_bool flagString = tagName == "string";
				if (flagString || tagName == "vstring") {
					if (!_parseStringResource(localNamespace, child, locale, !flagString, source)) {
						return sl_false;
					}
				} else {
					logError(child, g_str_error_invalid_tag, child->getName());
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_parseStringResource(const String& localNamespace, const Ref<XmlElement>& element, const Locale& localeDefault, sl_bool flagVariants, const String16& source)
	{
		if (element.isNull()) {
			return sl_false;
		}

		String strLocale = element->getAttribute("locale");
		Locale locale = Locale::Unknown;
		if (strLocale.isNotEmpty()) {
			locale = Locale(strLocale);
			if (locale.isInvalid()) {
				logError(element, g_str_error_resource_string_locale_invalid, strLocale);
				return sl_false;
			}
			if (_isExcludedLocale(locale)) {
				return sl_true;
			}
		}
		if (locale == Locale::Unknown) {
			locale = localeDefault;
		}

		String name = element->getAttribute("name");
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_string_name_is_empty);
			return sl_false;
		}
		if (!(SAppUtil::checkName(name.getData(), name.getLength()))) {
			logError(element, g_str_error_resource_string_name_invalid, name);
			return sl_false;
		}
		name = getNameInLocalNamespace(localNamespace, name);

		Ref<SAppStringResource> res = _registerOrGetStringResource(name, element);
		if (res.isNull()) {
			return sl_false;
		}

		if (flagVariants) {
			for (auto&& e : element->getChildElements()) {
				String vName = e->getName();
				if (vName == "default") {
					if (!(_registerStringResourceItem(*res, e, locale, source))) {
						return sl_false;
					}
				} else {
					if (!(SAppUtil::checkName(vName.getData(), vName.getLength()))) {
						logError(e, g_str_error_resource_string_name_invalid, vName);
						return sl_false;
					}
					MutexLocker lock(res->variants.getLocker());
					SAppStringResourceItem* item = res->variants.getItemPointer(vName);
					if (!item) {
						auto node = res->variants.add_NoLock(vName);
						if (!node) {
							logError(e, g_str_error_out_of_memory);
							return sl_false;
						}
						item = &(node->value);
					}
					if (!(_registerStringResourceItem(*item, e, locale, source))) {
						return sl_false;
					}
				}
			}
		} else {
			if (!(_registerStringResourceItem(*res, element, locale, source))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	Ref<SAppStringResource> SAppDocument::_registerOrGetStringResource(const String& name, const Ref<XmlElement>& element)
	{
		Ref<SAppStringResource> res = m_strings.getValue(name, Ref<SAppStringResource>::null());
		if (res.isNotNull()) {
			return res;
		}
		res = new SAppStringResource;
		if (res.isNotNull()) {
			res->name = name;
			if (m_strings.put(name, res)) {
				return res;
			}
		}
		logError(element, g_str_error_out_of_memory);
		return sl_null;
	}

	sl_bool SAppDocument::_registerStringResourceItem(SAppStringResourceItem& item, const Ref<XmlElement>& element, const Locale& locale, const String16& source)
	{
		String value;
		if (element->getChildElementCount() > 0 || element->getAttribute("raw").equals_IgnoreCase(StringView::literal("true"))) {
			value = String::create(source.substring(element->getStartContentPositionInSource(), element->getEndContentPositionInSource()));
		} else {
			value = element->getText();
		}
		sl_bool flagOverride = element->getAttribute("override").equals_IgnoreCase(StringView::literal("true"));
		if (locale == Locale::Unknown) {
			if (!flagOverride) {
				if (item.defaultValue.isNotNull()) {
					logError(element, g_str_error_resource_string_redefine_default);
					return sl_false;
				}
			}
			item.defaultValue = value;
		} else {
			if (!flagOverride) {
				if (item.values.find(locale)) {
					logError(element, g_str_error_resource_string_redefine_locale, locale.toString());
					return sl_false;
				}
			}
			if (!(item.values.put(locale, value))) {
				logError(element, g_str_error_out_of_memory);
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_generateStringsCpp(const String& targetPath)
	{
		log(g_str_log_generate_cpp_strings_begin);

		StringBuffer sbHeader, sbCpp, sbMap;

		sbHeader.add(String::format(
									"#pragma once%n%n"
									"#include <slib/core/resource.h>%n%n"
									"namespace %s%n"
									"{%n\tnamespace string%n\t{%n%n"
									, m_conf.generate_cpp_namespace));
		sbCpp.add(String::format(
								 "#include \"strings.h\"%n%n"
								 "namespace %s%n"
								 "{%n\tnamespace string%n\t{%n%n"
								 , m_conf.generate_cpp_namespace));

		if (m_conf.generate_cpp_string_map) {
			sbMap.add("\t\tSLIB_DEFINE_STRING_RESOURCE_MAP_BEGIN\r\n");
		}

		for (auto&& pair : m_strings) {
			if (m_conf.generate_cpp_string_filter_include.isNotEmpty()) {
				if (!(m_conf.generate_cpp_string_filter_include.contains_NoLock(pair.key))) {
					continue;
				}
			}
			if (m_conf.generate_cpp_string_filter_exclude .isNotEmpty()) {
				if (m_conf.generate_cpp_string_filter_exclude.contains_NoLock(pair.key)) {
					continue;
				}
			}
			if (pair.value.isNotNull()) {
				auto& res = *(pair.value);
				sbHeader.add(String::format("\t\tSLIB_DECLARE_STRING_RESOURCE(%s)%n", pair.key));
				if (m_conf.generate_cpp_string_map) {
					sbMap.add(String::format("\t\t\tSLIB_DEFINE_STRING_RESOURCE_MAP_ITEM(%s)%n", pair.key));
				}
				_generateStringsCpp_Item(sbCpp, pair.key, sl_null, res);
				for (auto&& var : res.variants) {
					sbHeader.add(String::format("\t\tSLIB_DECLARE_STRING_VARIANT(%s, %s)%n", pair.key, var.key));
					if (m_conf.generate_cpp_string_map) {
						sbMap.add(String::format("\t\t\tSLIB_DEFINE_STRING_VARIANT_MAP_ITEM(%s, %s)%n", pair.key, var.key));
					}
					_generateStringsCpp_Item(sbCpp, pair.key, var.key, var.value);
				}
			}
		}

		if (m_conf.generate_cpp_string_map) {
			sbMap.add("\t\tSLIB_DEFINE_STRING_RESOURCE_MAP_END\r\n");
			sbHeader.add("\r\n\t\tSLIB_DECLARE_STRING_RESOURCE_MAP\r\n\r\n\t}\r\n}\r\n");
			sbCpp.link(sbMap);
		} else {
			sbHeader.add("\r\n\r\n\t}\r\n}\r\n");
		}

		sbCpp.add("\r\n\t}\r\n}\r\n");

		String pathHeader = targetPath + "/strings.h";
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = targetPath + "/strings.cpp";
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				logError(g_str_error_file_write_failed, pathCpp);
				return sl_false;
			}
		}

		return sl_true;
	}

	void SAppDocument::_generateStringsCpp_Item(StringBuffer& sbCpp, const String& resourceName, const String& varName, const SAppStringResourceItem& item)
	{
		String defValue = item.defaultValue;
		if (item.values.isEmpty()) {
			if (varName.isNotNull()) {
				sbCpp.add(String::format("\t\tSLIB_DEFINE_STRING_VARIANT_SIMPLE(%s, %s, \"%s\")%n%n", resourceName, varName, Stringx::applyBackslashEscapes(defValue, sl_true, sl_false, sl_true)));
			} else {
				sbCpp.add(String::format("\t\tSLIB_DEFINE_STRING_RESOURCE_SIMPLE(%s, \"%s\")%n%n", resourceName, Stringx::applyBackslashEscapes(defValue, sl_true, sl_false, sl_true)));
			}
		} else {
			if (varName.isNotNull()) {
				if (defValue.isNotNull()) {
					sbCpp.add(String::format("\t\tSLIB_DEFINE_STRING_VARIANT_BEGIN(%s, %s, \"%s\")%n", resourceName, varName, Stringx::applyBackslashEscapes(defValue, sl_true, sl_false, sl_true)));
				} else {
					sbCpp.add(String::format("\t\tSLIB_DEFINE_STRING_VARIANT_BEGIN_NODEF(%s, %s)%n", resourceName, varName));
				}
			} else {
				sbCpp.add(String::format("\t\tSLIB_DEFINE_STRING_RESOURCE_BEGIN(%s, \"%s\")%n", resourceName, Stringx::applyBackslashEscapes(defValue, sl_true, sl_false, sl_true)));
			}
			{
				for (auto&& pairValues : item.values) {
					if (pairValues.key.getCountry() != Country::Unknown && pairValues.key.getScript() != LanguageScript::Unknown) {
						sbCpp.add(String::format("\t\t\tSLIB_DEFINE_STRING_RESOURCE_VALUE(%s, \"%s\")%n", pairValues.key.toString(), Stringx::applyBackslashEscapes(pairValues.value, sl_true, sl_false, sl_true)));
					}
				}
			}
			{
				for (auto&& pairValues : item.values) {
					if (pairValues.key.getCountry() != Country::Unknown && pairValues.key.getScript() == LanguageScript::Unknown) {
						sbCpp.add(String::format("\t\t\tSLIB_DEFINE_STRING_RESOURCE_VALUE(%s, \"%s\")%n", pairValues.key.toString(), Stringx::applyBackslashEscapes(pairValues.value, sl_true, sl_false, sl_true)));
					}
				}
			}
			{
				for (auto&& pairValues : item.values) {
					if (pairValues.key.getCountry() == Country::Unknown && pairValues.key.getScript() != LanguageScript::Unknown) {
						sbCpp.add(String::format("\t\t\tSLIB_DEFINE_STRING_RESOURCE_VALUE(%s, \"%s\")%n", pairValues.key.toString(), Stringx::applyBackslashEscapes(pairValues.value, sl_true, sl_false, sl_true)));
					}
				}
			}
			{
				for (auto&& pairValues : item.values) {
					if (pairValues.key.getCountry() == Country::Unknown && pairValues.key.getScript() == LanguageScript::Unknown) {
						sbCpp.add(String::format("\t\t\tSLIB_DEFINE_STRING_RESOURCE_VALUE(%s, \"%s\")%n", pairValues.key.toString(), Stringx::applyBackslashEscapes(pairValues.value, sl_true, sl_false, sl_true)));
					}
				}
			}
			if (varName.isNotNull()) {
				static sl_char8 strEnd[] = "\t\tSLIB_DEFINE_STRING_VARIANT_END\r\n\r\n";
				sbCpp.addStatic(strEnd, sizeof(strEnd) - 1);
			} else {
				static sl_char8 strEnd[] = "\t\tSLIB_DEFINE_STRING_RESOURCE_END\r\n\r\n";
				sbCpp.addStatic(strEnd, sizeof(strEnd) - 1);
			}
		}
	}

	sl_bool SAppDocument::_getStringAccessString(const String& localNamespace, const SAppStringValue& value, String& result)
	{
		if (!(value.flagDefined)) {
			result = "slib::String::null()";
			return sl_true;
		}
		if (value.flagReferResource) {
			String name;
			if (_checkStringResource(localNamespace, value, &name)) {
				result = String::format("string::%s::get()", name);
				return sl_true;
			} else {
				return sl_false;
			}
		} else {
			if (value.valueOrName.isNull()) {
				result = "slib::String::null()";
			} else {
				result = String::format("\"%s\"", Stringx::applyBackslashEscapes(value.valueOrName, sl_true, sl_false, sl_true));
			}
			return sl_true;
		}
	}

	sl_bool SAppDocument::_getStringDataAccessString(const String& localNamespace, const SAppStringValue& value, String& result)
	{
		if (value.flagFormattingDataValue) {
			result = String::format("slib::String::format(%s, data)", Stringx::applyBackslashEscapes(value.dataAccess));
		} else {
			String def;
			if (!(_getStringAccessString(localNamespace, value, def))) {
				return sl_false;
			}
			result = String::format("data%s.getString(%s)", value.dataAccess, def);
		}
		return sl_true;
	}

	sl_bool SAppDocument::_getStringValue(const String& localNamespace, const SAppStringValue& value, String& result)
	{
		Locale locale = getCurrentSimulatorLocale();
		if (!(value.flagDefined)) {
			result = String::null();
			return sl_true;
		}
		if (value.flagReferResource) {
			Ref<SAppStringResource> resource;
			SAppStringResourceItem item;
			if (_checkStringResource(localNamespace, value, sl_null, &resource, value.variant.isNotNull() ? &item : sl_null)) {
				if (value.variant.isNotNull()) {
					if (item.defaultValue.isNotNull()) {
						result = item.get(locale, resource->get(locale, item.defaultValue));
					} else {
						result = item.get(locale, resource->get(locale, resource->defaultValue));
					}
				} else {
					result = resource->get(locale, resource->defaultValue);
				}
				return sl_true;
			} else {
				return sl_false;
			}
		} else {
			result = value.valueOrName;
			return sl_true;
		}
	}

	sl_bool SAppDocument::_checkStringValue(const String& localNamespace, const SAppStringValue& value)
	{
		if (!(value.flagDefined)) {
			return sl_true;
		}
		if (value.flagReferResource) {
			return _checkStringResource(localNamespace, value);
		} else {
			return sl_true;
		}
	}

	sl_bool SAppDocument::_checkStringResource(const String& localNamespace, const SAppStringValue& value, String* outName, Ref<SAppStringResource>* outResource, SAppStringResourceItem* outItem)
	{
		if (value.variant.isNotNull()) {
			Ref<SAppStringResource> res;
			if (getItemFromMap(m_strings, localNamespace, value.valueOrName, outName, &res)) {
				if (res->variants.get(value.variant, outItem)) {
					if (outName) {
						*outName = String::concat(*outName, "::", value.variant);
					}
				}
				if (outResource) {
					*outResource = Move(res);
				}
				return sl_true;
			}
		} else {
			if (getItemFromMap(m_strings, localNamespace, value.valueOrName, outName, outResource)) {
				return sl_true;
			}
		}
		logError(value.referingElement, g_str_error_string_not_found, value.variant.isNotNull() ? String::concat(value.valueOrName, "/", value.variant) : value.valueOrName);
		return sl_false;
	}

}
