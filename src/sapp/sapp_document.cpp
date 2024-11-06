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

#include "slib/ui.h"
#include "slib/io/file.h"

#include "sapp_document.h"
#include "sapp_util.h"

#include "sapp_error.inc.h"

#define TAG "SApp"

namespace slib
{

#define RAW_MAX_SIZE 0x1000000 // 16MB

	SAppConfiguration::SAppConfiguration()
	{
		generate_cpp_string_map = sl_true;
		generate_cpp_color_map = sl_true;
		generate_cpp_drawable_map = sl_true;
		generate_cpp_raw_map = sl_true;
		simulator_locale = Locale::Unknown;
	}

	SAppConfiguration::SAppConfiguration(SAppConfiguration&& other) = default;

	SAppConfiguration::~SAppConfiguration()
	{
	}

	SAppConfiguration& SAppConfiguration::operator=(SAppConfiguration&& other) = default;

	SAppSimulateLayoutParam::SAppSimulateLayoutParam()
	{
		pageSize.x = 450;
		pageSize.y = 800;
	}


	SAppDocument::SAppDocument()
	{
		m_flagOpened = sl_false;
	}

	SAppDocument::~SAppDocument()
	{
	}

	sl_bool SAppDocument::open(const String& filePath)
	{
		ObjectLocker lock(this);

		SAppConfiguration conf;
		if (!_parseConfiguration(filePath, conf)) {
			return sl_false;
		}

		close();

		m_pathConf = filePath;

		m_conf = Move(conf);

		m_flagOpened = sl_true;

		return sl_true;
	}

	void SAppDocument::close()
	{
		ObjectLocker lock(this);
		if (m_flagOpened) {
			_freeResources();
			m_flagOpened = sl_false;
		}
	}

	sl_bool SAppDocument::isOpened()
	{
		return m_flagOpened;
	}

	void SAppDocument::clearAllResources()
	{
		m_drawables.removeAll();
		m_strings.removeAll();
		m_colors.removeAll();
		m_menus.removeAll();
		m_raws.removeAll();
		m_layouts.removeAll();
		m_layoutStyles.removeAll();
		m_layoutIncludes.removeAll();
	}

	sl_bool SAppDocument::openResources()
	{
		ObjectLocker lock(this);

		if (!m_flagOpened) {
			return sl_false;
		}

		_freeResources();

		if (!(_openResourcesExceptUi())) {
			return sl_false;
		}
		if (!(_openUiResources())) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool SAppDocument::openUiResource(const String& filePath)
	{
		ObjectLocker lock(this);

		if (!m_flagOpened) {
			return sl_false;
		}

		_freeResources();

		if (!(_openResourcesExceptUi())) {
			return sl_false;
		}
		if (!(_openUiResource(filePath))) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool SAppDocument::openRawResources(const String& path)
	{
		if (File::exists(path)) {
			if (_registerRawResources(path)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool SAppDocument::_openResourcesExceptUi()
	{
		HashSet<String> includedSet;
		return _openResourcesExceptUi(m_conf, includedSet);
	}

	sl_bool SAppDocument::_openResourcesExceptUi(const SAppModuleConfiguration& conf, HashSet<String>& includedSet)
	{
		if (includedSet.find_NoLock(conf.app_path)) {
			return sl_true;
		}
		includedSet.put_NoLock(conf.app_path);
		for (auto&& include : conf.includes) {
			if (!(_openResourcesExceptUi(include, includedSet))) {
				return sl_false;
			}
		}
		return _openResourcesExceptUi(conf.app_path);
	}

	sl_bool SAppDocument::_openResourcesExceptUi(const String& pathApp)
	{
		if (!(_openRawResources(pathApp))) {
			return sl_false;
		}
		if (!(_openImageResources(pathApp))) {
			return sl_false;
		}
		if (!(_openGlobalResources(pathApp, sl_null, sl_false))) {
			return sl_false;
		}
		if (!(_openGlobalResources(pathApp, "global", sl_true))) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool SAppDocument::_openImageResources(const String& pathApp)
	{
		List<String> fileList = File::getFiles(pathApp);
		fileList.sort();
		for (auto&& fileName : fileList) {
			if (fileName.isNotNull()) {
				if (fileName == "image") {
					if (!(_registerFileResources("image", pathApp + "/image", Locale::Unknown))) {
						return sl_false;
					}
				} else if (fileName.startsWith("image-")) {
					String strLocale = fileName.substring(6);
					Locale locale = Locale::Unknown;
					if (strLocale.isNotEmpty()) {
						locale = Locale(strLocale);
					}
					if (locale == Locale::Unknown || locale.isInvalid()) {
						logError(g_str_error_resource_drawable_locale_invalid, fileName);
						return sl_false;
					}
					if (!(_isExcludedLocale(locale))) {
						if (!(_registerFileResources(fileName, File::concatPath(pathApp, fileName), locale))) {
							return sl_false;
						}
					}
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_openRawResources(const String& pathApp)
	{
		String path = pathApp + "/raw";
		if (File::exists(path)) {
			if (!(_registerRawResources(path))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_openGlobalResources(const String& pathApp, const String& subdir, sl_bool flagLoadHierarchically)
	{
		String pathDir = File::concatPath(pathApp, subdir);
		List<String> fileNames = File::getFiles(pathDir);
		fileNames.sort_NoLock();
		{
			for (auto&& fileName : fileNames) {
				String path = File::concatPath(pathDir, fileName);
				if (File::exists(path)) {
					if (!File::isDirectory(path)) {
						if (File::getFileExtension(fileName) == "xml" && !(subdir.isEmpty() && fileName == "sapp.xml")) {
							if (!(_parseResourcesXml(sl_null, path))) {
								return sl_false;
							}
						}
					}
				}
			}
		}
		if (flagLoadHierarchically) {
			for (auto&& fileName : fileNames) {
				String path = File::concatPath(pathDir, fileName);
				if (File::exists(path)) {
					if (File::isDirectory(path)) {
						if (!(_openGlobalResources(pathApp, File::concatPath(subdir, fileName), sl_true))) {
							return sl_false;
						}
					}
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_openUiResources()
	{
		HashSet<String> includedSet;
		return _openUiResources(m_conf, includedSet);
	}

	sl_bool SAppDocument::_openUiResources(const SAppModuleConfiguration& conf, HashSet<String>& includedSet)
	{
		if (includedSet.find_NoLock(conf.app_path)) {
			return sl_true;
		}
		includedSet.put_NoLock(conf.app_path);
		for (auto&& include : m_conf.includes) {
			if (!(_openUiResources(include, includedSet))) {
				return sl_false;
			}
		}
		if (!(_openUiResources(File::concatPath(conf.app_path, "layout")))) {
			return sl_false;
		}
		if (!(_openUiResources(File::concatPath(conf.app_path, "ui")))) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool SAppDocument::_openUiResources(const String& pathLayouts)
	{
		for (auto&& fileName : File::getFiles(pathLayouts)) {
			String path = File::concatPath(pathLayouts, fileName);
			if (File::getFileExtension(fileName) == "xml" || File::getFileExtension(fileName) == "uiml") {
				if (!(_openUiResource(path))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_openUiResource(const String& path)
	{
		if (File::exists(path)) {
			if (!(File::isDirectory(path))) {
				String fileName = File::getFileName(path);
				String fileNamespace = File::getFileNameOnly(fileName);
				if (fileNamespace.isNotEmpty()) {
					if (SAppUtil::checkName(fileNamespace.getData())) {
						if (_parseResourcesXml(fileNamespace, path)) {
							return sl_true;
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool SAppDocument::_openUiResourceByName(const String& name)
	{
		sl_bool flagFound = sl_false;
		HashSet<String> includedSet;
		return _openUiResourceByName(name, flagFound, m_conf, includedSet);
	}

	sl_bool SAppDocument::_openUiResourceByName(const String& name, sl_bool& flagFound, const SAppModuleConfiguration& conf, HashSet<String>& includedSet)
	{
		if (includedSet.find_NoLock(conf.app_path)) {
			return sl_false;
		}
		includedSet.put_NoLock(conf.app_path);
		for (auto&& include : m_conf.includes) {
			if (_openUiResourceByName(name, flagFound, include, includedSet)) {
				return sl_true;
			}
			if (flagFound) {
				return sl_false;
			}
		}
		String path = File::concatPath(conf.app_path, "layout", name);
		if (File::isFile(path + ".xml")) {
			flagFound = sl_true;
			return _openUiResource(path + ".xml");
		} else if (File::isFile(path + ".uiml")) {
			flagFound = sl_true;
			return _openUiResource(path + ".uiml");
		} else {
			path = File::concatPath(conf.app_path, "ui", name);
			if (File::isFile(path + ".xml")) {
				flagFound = sl_true;
				return _openUiResource(path + ".xml");
			} else if (File::isFile(path + ".uiml")) {
				flagFound = sl_true;
				return _openUiResource(path + ".uiml");
			} else {
				return sl_false;
			}
		}
	}

	String SAppDocument::_resolvePath(const String& path, const String& currentFilePath)
	{
		String ret = Stringx::resolveEnvironmentVariables(path);
		if (ret.startsWith('.')) {
			ret = File::concatPath(File::getParentDirectoryPath(currentFilePath), path);
		}
		return ret;
	}

	sl_bool SAppDocument::_isExcludedLocale(const Locale& locale)
	{
		slib::Locale localeLang(locale.getLanguage());
		slib::Locale localeLangCountry(locale.getLanguage(), locale.getCountry());
		slib::Locale localeDetail(locale.getLanguage(), locale.getScript(), slib::Country::Unknown);
		ListLocker<Locale> excludes(m_conf.locale_excludes);
		for (sl_size i = 0; i < excludes.count; i++) {
			Locale& localeSource = excludes[i];
			if (locale == localeSource || localeLang == localeSource || localeLangCountry == localeSource || localeDetail == localeSource) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool SAppDocument::generateCpp()
	{
		ObjectLocker lock(this);

		if (!m_flagOpened) {
			return sl_false;
		}

		String path = m_conf.generate_cpp_target_path;
		if (path.isEmpty()) {
			logError(g_str_error_generate_cpp_target_path_is_empty);
			return sl_false;
		}
		if (!(File::isDirectory(path))) {
			File::createDirectories(path);
			if (!(File::isDirectory(path))) {
				logError(g_str_error_generate_cpp_target_path_invalid, path);
				return sl_false;
			}
		}

		if (m_conf.generate_cpp_namespace.isEmpty()) {
			logError(g_str_error_generate_cpp_namespace_is_empty);
			return sl_false;
		}
		if (!(SAppUtil::checkName(m_conf.generate_cpp_namespace.getData(), m_conf.generate_cpp_namespace.getLength()))) {
			logError(g_str_error_generate_cpp_namespace_invalid, m_conf.generate_cpp_namespace);
			return sl_false;
		}

		log(g_str_log_generate_cpp_begin, path);

		if (!_generateResourcesH(path)) {
			return sl_false;
		}
		if (!_generateStringsCpp(path)) {
			return sl_false;
		}
		if (!_generateRawCpp(path, m_conf.generate_cpp_namespace, "raw")) {
			return sl_false;
		}
		if (!_generateDrawablesCpp(path)) {
			return sl_false;
		}
		if (!_generateColorsCpp(path)) {
			return sl_false;
		}
		if (!_generateMenusCpp(path)) {
			return sl_false;
		}
		if (!_generateLayoutsCpp(path)) {
			return sl_false;
		}

		return sl_true;
	}

	sl_bool SAppDocument::generateCppForRawResources(const String& _namespace, const String& pathOut)
	{
		ObjectLocker lock(this);
		sl_reg index = _namespace.indexOf("::");
		if (index >= 0) {
			return _generateRawCpp(pathOut, _namespace.substring(0, index), _namespace.substring(index + 2));
		} else {
			return _generateRawCpp(pathOut, _namespace, sl_null);
		}
	}

	List< Ref<SAppLayoutResource> > SAppDocument::getLayouts()
	{
		ObjectLocker lock(this);

		if (!m_flagOpened) {
			return List< Ref<SAppLayoutResource> >::null();
		}

		return m_layouts.getAllValues();

	}

	sl_bool SAppDocument::simulateLayoutInWindow(const String& layoutName, SAppSimulateLayoutParam& param)
	{
		ObjectLocker lock(this);

		if (!m_flagOpened) {
			return sl_false;
		}

		Ref<SAppLayoutResource> layout = m_layouts.getValue(layoutName, Ref<SAppLayoutResource>::null());
		if (layout.isNotNull()) {
			if (_simulateLayoutInWindow(layout.get(), param)) {
				return sl_true;
			}
		}

		return sl_false;
	}

	Locale SAppDocument::getCurrentSimulatorLocale()
	{
		Locale locale = m_conf.simulator_locale;
		if (locale == Locale::Unknown) {
			locale = Locale::getCurrent();
		}
		return locale;
	}

	/***************************************************
	 					Utilities
	***************************************************/

	String SAppDocument::getShortcutKeyDefinitionString(const KeycodeAndModifiers& km, sl_bool flagMac)
	{
		if (km.getKeycode() == Keycode::Unknown) {
			return "slib::Keycode::Unknown";
		}
		String ret;
		if (km.isControlKey()) {
			ret = "slib::Modifiers::Control";
		}
		if (flagMac) {
			if (km.isOptionKey()) {
				if (ret.isNotEmpty()) {
					ret += " | ";
				}
				ret += "slib::Modifiers::Option";
			}
		} else {
			if (km.isAltKey()) {
				if (ret.isNotEmpty()) {
					ret += " | ";
				}
				ret += "slib::Modifiers::Alt";
			}
		}
		if (km.isShiftKey()) {
			if (ret.isNotEmpty()) {
				ret += " | ";
			}
			ret += "slib::Modifiers::Shift";
		}
		if (flagMac) {
			if (km.isCommandKey()) {
				if (ret.isNotEmpty()) {
					ret += " | ";
				}
				ret += "slib::Modifiers::Command";
			}
		} else {
			if (km.isWindowsKey()) {
				if (ret.isNotEmpty()) {
					ret += " | ";
				}
				ret += "slib::Modifiers::Windows";
			}
		}
		if (ret.isNotEmpty()) {
			ret += " | ";
		}
		ret += "slib::Keycode::";
		ret += UIEvent::getKeyName(km.getKeycode(), sl_false);
		return ret;
	}

	String SAppDocument::getGlobalName(const String& fileNamespace, const String& name)
	{
		if (fileNamespace.isNotEmpty()) {
			return fileNamespace + "_" + name;
		} else {
			return name;
		}
	}

	String SAppDocument::getGlobalStyleName(const String& fileNamespace, const String& theme, const String& name)
	{
		String s;
		if (theme.isNotEmpty()) {
			s = String::concat(theme, StringView::literal(":"), name);
		} else {
			s = name;
		}
		if (fileNamespace.isNotEmpty()) {
			return String::concat(fileNamespace, StringView::literal("/"), s);
		} else {
			return s;
		}
	}

	/***************************************************
						Log
	***************************************************/

	void SAppDocument::log(const StringView& text)
	{
		Log(TAG, text);
	}

	void SAppDocument::logError(const StringView& text)
	{
		LogError(TAG, text);
	}

	void SAppDocument::logErrorSource(const StringView& filePath, sl_size line, sl_size col, const StringView& text)
	{
		LogError(TAG, "%s(%d:%d)%n%s", filePath, line, col, text);
	}

	void SAppDocument::logError(const Ref<XmlElement>& element, const StringView& text)
	{
		if (element.isNotNull()) {
			LogError(TAG, "%s(%d:%d)%n%s", element->getSourceFilePath(), element->getLineNumberInSource(), element->getColumnNumberInSource(), text);
		} else {
			LogError(TAG, text);
		}
	}


	/***************************************************
					Resources Entry
	***************************************************/
	sl_bool SAppDocument::_parseModuleConfiguration(const String& filePath, SAppModuleConfiguration& conf, const Function<sl_bool(XmlElement* root)>& onAdditionalConfig)
	{
		if (!(File::exists(filePath))) {
			logError(g_str_error_file_not_found, filePath);
			return sl_false;
		}

		log(g_str_log_appconf_begin, filePath);

		Xml::ParseParam param;
		param.flagLogError = sl_false;
		param.setCreatingOnlyElementsAndTexts();
		Ref<XmlDocument> xml = Xml::parseTextFile(filePath, param);
		if (param.flagError) {
			logErrorSource(filePath, param.errorLine, param.errorColumn, param.errorMessage);
			return sl_false;
		}

		if (xml.isNull()) {
			return sl_false;
		}
		Ref<XmlElement> root = xml->getRoot();
		if (root.isNull()) {
			return sl_false;
		}
		if (root->getName() != "sapp") {
			logError(root, g_str_error_invalid_root_tag);
			return sl_false;
		}

		root = root->getFirstChildElement("configuration");
		if (root.isNull()) {
			logError(root, g_str_error_configuration_tag_not_found);
			return sl_false;
		}

		// app-path
		{
			Ref<XmlElement> el_app_path = root->getFirstChildElement("app-path");
			if (el_app_path.isNotNull()) {
				String strPath = _resolvePath(el_app_path->getText(), filePath);
				if (File::isDirectory(strPath)) {
					conf.app_path = strPath;
				} else {
					logError(el_app_path, String::format(g_str_error_directory_not_found, strPath));
					return sl_false;
				}
			} else {
				conf.app_path = File::getParentDirectoryPath(filePath);
			}
		}

		// include
		{
			ListElements< Ref<XmlElement> > el_includes(root->getChildElements("include"));
			for (sl_size i = 0; i < el_includes.count; i++) {
				Ref<XmlElement>& el_include = el_includes[i];
				String strPath = _resolvePath(el_include->getText(), filePath);
				if (File::isDirectory(strPath)) {
					String configPath = File::concatPath(strPath, "sapp.xml");
					SAppModuleConfiguration include;
					if (_parseModuleConfiguration(configPath, include, sl_null)) {
						if (!(conf.includes.add_NoLock(Move(include)))) {
							logError(el_include, g_str_error_out_of_memory);
							return sl_false;
						}
					} else {
						return sl_false;
					}
				} else {
					logError(el_include, String::format(g_str_error_directory_not_found, strPath));
					return sl_false;
				}
			}
		}
		if (onAdditionalConfig.isNotNull()) {
			if (!(onAdditionalConfig(root.get()))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_parseConfiguration(const String& filePath, SAppConfiguration& conf)
	{
		return _parseModuleConfiguration(filePath, conf, [this, filePath, &conf](XmlElement* root) {
			// generate-cpp
			Ref<XmlElement> el_generate_cpp = root->getFirstChildElement("generate-cpp");
			if (el_generate_cpp.isNotNull()) {
				Ref<XmlElement> el_target_path = el_generate_cpp->getFirstChildElement("target-path");
				if (el_target_path.isNotNull()) {
					String strPath = _resolvePath(el_target_path->getText(), filePath);
					if (strPath.isEmpty()) {
						logError(el_target_path, String::format(g_str_error_configuration_value_empty, "target-path"));
						return sl_false;
					}
					conf.generate_cpp_target_path = strPath;
				}
				Ref<XmlElement> el_namespace = el_generate_cpp->getFirstChildElement("namespace");
				if (el_namespace.isNotNull()) {
					conf.generate_cpp_namespace = el_namespace->getText();
					if (!(SAppUtil::checkName(conf.generate_cpp_namespace.getData(), conf.generate_cpp_namespace.getLength()))) {
						logError(el_namespace, String::format(g_str_error_configuration_value_invalid, "namespace", conf.generate_cpp_namespace));
						return sl_false;
					}
				}
				Ref<XmlElement> el_string = el_generate_cpp->getFirstChildElement("string");
				if (el_string.isNotNull()) {
					Ref<XmlElement> el_map = el_string->getFirstChildElement("map");
					if (el_map.isNotNull()) {
						String value = el_map->getText();
						if (value == "true") {
							conf.generate_cpp_string_map = sl_true;
						} else if (value == "false") {
							conf.generate_cpp_string_map = sl_false;
						} else {
							logError(el_map, String::format(g_str_error_configuration_value_invalid, "map", value));
							return sl_false;
						}
					}
					Ref<XmlElement> el_filter = el_string->getFirstChildElement("filter");
					if (el_filter.isNotNull()) {
						ListElements< Ref<XmlElement> > children(el_filter->getChildElements());
						for (sl_size i = 0; i < children.count; i++) {
							Ref<XmlElement>& child = children[i];
							String name = child->getName();
							if (name == "include") {
								conf.generate_cpp_string_filter_include.add_NoLock(child->getText());
							} else if (name == "exclude") {
								conf.generate_cpp_string_filter_exclude.add_NoLock(child->getText());
							}
						}
					}
				}
				Ref<XmlElement> el_color = el_generate_cpp->getFirstChildElement("color");
				if (el_color.isNotNull()) {
					Ref<XmlElement> el_map = el_color->getFirstChildElement("map");
					if (el_map.isNotNull()) {
						String value = el_map->getText();
						if (value == "true") {
							conf.generate_cpp_color_map = sl_true;
						} else if (value == "false") {
							conf.generate_cpp_color_map = sl_false;
						} else {
							logError(el_map, String::format(g_str_error_configuration_value_invalid, "map", value));
							return sl_false;
						}
					}
					Ref<XmlElement> el_filter = el_color->getFirstChildElement("filter");
					if (el_filter.isNotNull()) {
						ListElements< Ref<XmlElement> > children(el_filter->getChildElements());
						for (sl_size i = 0; i < children.count; i++) {
							Ref<XmlElement>& child = children[i];
							String name = child->getName();
							if (name == "include") {
								conf.generate_cpp_color_filter_include.add_NoLock(child->getText());
							} else if (name == "exclude") {
								conf.generate_cpp_color_filter_exclude.add_NoLock(child->getText());
							}
						}
					}
				}
				Ref<XmlElement> el_drawable = el_generate_cpp->getFirstChildElement("drawable");
				if (el_drawable.isNotNull()) {
					Ref<XmlElement> el_map = el_drawable->getFirstChildElement("map");
					if (el_map.isNotNull()) {
						String value = el_map->getText();
						if (value == "true") {
							conf.generate_cpp_drawable_map = sl_true;
						} else if (value == "false") {
							conf.generate_cpp_drawable_map = sl_false;
						} else {
							logError(el_map, String::format(g_str_error_configuration_value_invalid, "map", value));
							return sl_false;
						}
					}
					Ref<XmlElement> el_filter = el_drawable->getFirstChildElement("filter");
					if (el_filter.isNotNull()) {
						ListElements< Ref<XmlElement> > children(el_filter->getChildElements());
						for (sl_size i = 0; i < children.count; i++) {
							Ref<XmlElement>& child = children[i];
							String name = child->getName();
							if (name == "include") {
								conf.generate_cpp_drawable_filter_include.add_NoLock(child->getText());
							} else if (name == "exclude") {
								conf.generate_cpp_drawable_filter_exclude.add_NoLock(child->getText());
							}
						}
					}
				}
				Ref<XmlElement> el_raw = el_generate_cpp->getFirstChildElement("raw");
				if (el_raw.isNotNull()) {
					Ref<XmlElement> el_map = el_raw->getFirstChildElement("map");
					if (el_map.isNotNull()) {
						String value = el_map->getText();
						if (value == "true") {
							conf.generate_cpp_raw_map = sl_true;
						} else if (value == "false") {
							conf.generate_cpp_raw_map = sl_false;
						} else {
							logError(el_map, String::format(g_str_error_configuration_value_invalid, "map", value));
							return sl_false;
						}
					}
					Ref<XmlElement> el_filter = el_raw->getFirstChildElement("filter");
					if (el_filter.isNotNull()) {
						ListElements< Ref<XmlElement> > children(el_filter->getChildElements());
						for (sl_size i = 0; i < children.count; i++) {
							Ref<XmlElement>& child = children[i];
							String name = child->getName();
							if (name == "include") {
								conf.generate_cpp_raw_filter_include.add_NoLock(child->getText());
							} else if (name == "exclude") {
								conf.generate_cpp_raw_filter_exclude.add_NoLock(child->getText());
							}
						}
					}
					{
						ListElements< Ref<XmlElement> > children(el_raw->getChildElements("compress"));
						for (sl_size i = 0; i < children.count; i++) {
							Ref<XmlElement>& child = children[i];
							conf.generate_cpp_raw_compress.add_NoLock(child->getText());
						}
					}
				}
				Ref<XmlElement> el_layout = el_generate_cpp->getFirstChildElement("layout");
				if (el_layout.isNotNull()) {
					ListLocker< Ref<XmlElement> > children(el_layout->getChildElements());
					for (sl_size i = 0; i < children.count; i++) {
						Ref<XmlElement> child = children[i];
						if (child.isNotNull()) {
							if (child->getName() == "include-header") {
								String str = child->getText().trim();
								if (str.isNotEmpty()) {
									conf.generate_cpp_layout_include_headers.add(str);
								}
							} else if (child->getName() == "include-header-in-cpp") {
								String str = child->getText().trim();
								if (str.isNotEmpty()) {
									conf.generate_cpp_layout_include_headers_in_cpp.add(str);
								}
							}
						}
					}
				}
			}

			// simulator
			Ref<XmlElement> el_simulator = root->getFirstChildElement("simulator");
			if (el_simulator.isNotNull()) {
				Ref<XmlElement> el_locale = el_simulator->getFirstChildElement("locale");
				if (el_locale.isNotNull()) {
					String strLocale = el_locale->getText();
					if (strLocale.isNotEmpty()) {
						Locale locale;
						if (locale.parse(strLocale)) {
							conf.simulator_locale = locale;
						} else {
							logError(el_locale, String::format(g_str_error_configuration_value_invalid, "locale", strLocale));
							return sl_false;
						}
					}
				}
			}

			// locale
			{
				Ref<XmlElement> el_locale = root->getFirstChildElement("locale");
				if (el_locale.isNotNull()) {
					// exclude
					{
						ListElements< Ref<XmlElement> > el_excludes(el_locale->getChildElements("exclude"));
						for (sl_size i = 0; i < el_excludes.count; i++) {
							Ref<XmlElement>& el_exclude = el_excludes[i];
							String exclude = el_exclude->getText();
							Locale locale;
							if (locale.parse(exclude)) {
								if (!(conf.locale_excludes.add_NoLock(locale))) {
									logError(el_exclude, g_str_error_out_of_memory);
									return sl_false;
								}
							} else {
								logError(el_exclude, String::format(g_str_error_configuration_value_invalid, "exclude", exclude));
								return sl_false;
							}
						}
					}
				}
			}
			return sl_true;
		});
	}

	void SAppDocument::_freeResources()
	{
		clearAllResources();
		{
			ListLocker< Ref<SAppLayoutSimulationWindow> > windows(m_layoutSimulationWindows);
			for (sl_size i = 0; i < windows.count; i++) {
				Ref<SAppLayoutSimulationWindow> window = windows[i];
				if (window->isOpened()) {
					window->close();
				}
			}
		}
		m_layoutSimulationWindows.removeAll();
	}

	sl_bool SAppDocument::_parseResourcesXml(const String& fileNamespace, const String& filePath)
	{
		log(g_str_log_open_resource_begin, filePath);

		Xml::ParseParam param;
		param.flagLogError = sl_false;
		param.flagSupportCpp11String = sl_true;
		param.setCreatingOnlyElementsAndTexts();
		String16 textXML = File::readAllText(filePath).toString16();
		param.sourceFilePath = filePath;
		Ref<XmlDocument> xml = Xml::parse(textXML, param);
		if (param.flagError) {
			logErrorSource(filePath, param.errorLine, param.errorColumn, param.errorMessage);
			return sl_false;
		}
		if (xml.isNull()) {
			return sl_false;
		}
		Ref<XmlElement> root = xml->getRoot();
		if (root.isNull()) {
			return sl_false;
		}
		if (root->getName() != "sapp") {
			logError(root, g_str_error_invalid_root_tag);
			return sl_false;
		}

		String fileName = File::getFileNameOnly(filePath);
		Locale locale = Locale::Unknown;
		if (fileName.startsWith("strings_")) {
			locale = Locale(fileName.substring(8));
			if (locale.isInvalid()) {
				locale = Locale::Unknown;
			} else {
				if (_isExcludedLocale(locale)) {
					return sl_true;
				}
			}
		}

		ListLocker< Ref<XmlElement> > children(root->getChildElements());
		sl_size i;
		for (i = 0; i < children.count; i++) {
			Ref<XmlElement>& child = children[i];
			if (child.isNotNull()) {
				String type = child->getName().toLower();
				if (type == StringView::literal("layout")) {
					continue;
				}
				if (type == StringView::literal("strings") || type == StringView::literal("string") || type == StringView::literal("vstring")) {
					if (type == StringView::literal("strings")) {
						if (!_parseStringResources(fileNamespace, child, locale, textXML)) {
							return sl_false;
						}
					} else {
						if (!_parseStringResource(fileNamespace, child, locale, type == StringView::literal("vstring"), textXML)) {
							return sl_false;
						}
					}
				} else if (type == StringView::literal("color")) {
					if (!_parseColorResource(fileNamespace, child)) {
						return sl_false;
					}
				} else if (type == StringView::literal("nine-pieces")) {
					if (!_parseNinePiecesDrawableResource(fileNamespace, child)) {
						return sl_false;
					}
				} else if (type == StringView::literal("nine-patch")) {
					if (!_parseNinePatchDrawableResource(fileNamespace, child)) {
						return sl_false;
					}
				} else if (type == StringView::literal("menu") || type == StringView::literal("popup-menu")) {
					if (!_parseMenuResource(fileNamespace, child, type != "menu")) {
						return sl_false;
					}
				} else if (type == StringView::literal("include")) {
					if (!_parseLayoutInclude(fileNamespace, child)) {
						return sl_false;
					}
				} else if (type == StringView::literal("style")) {
					if (!_parseLayoutStyle(fileNamespace, sl_null, child)) {
						return sl_false;
					}
				} else if (type == StringView::literal("unit")) {
					if (!_parseLayoutUnit(fileNamespace, child)) {
						return sl_false;
					}
				} else {
					logError(child, String::format(g_str_error_invalid_tag, child->getName()));
					return sl_false;
				}
			}
		}
		for (i = 0; i < children.count; i++) {
			Ref<XmlElement>& child = children[i];
			if (child.isNotNull()) {
				if (child->getName() == StringView::literal("layout")) {
					Ref<SAppLayoutResource> layout = _parseLayoutResource(filePath, fileNamespace, child, textXML);
					if (layout.isNull()) {
						return sl_false;
					}
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_generateResourcesH(const String& targetPath)
	{
		log(g_str_log_generate_cpp_resources_h_begin);

		String content =
			"#pragma once\r\n\r\n"
			"#include \"raws.h\"\r\n"
			"#include \"strings.h\"\r\n"
			"#include \"colors.h\"\r\n"
			"#include \"drawables.h\"\r\n"
			"#include \"menus.h\"\r\n"
			"#include \"layouts.h\"\r\n";

		String pathHeader = targetPath + "/resources.h";
		if (File::readAllTextUTF8(pathHeader) != content) {
			if (!(File::writeAllTextUTF8(pathHeader, content))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}
		return sl_true;
	}

	template <class MAP, class ITEM>
	sl_bool SAppDocument::getItemFromMap(const MAP& map, const String& fileNamespace, const String& name, String* outName, ITEM* _out)
	{
		if (fileNamespace.isNotEmpty()) {
			String globalName = getGlobalName(fileNamespace, name);
			if (map.get(globalName, _out)) {
				if (outName) {
					*outName = globalName;
				}
				return sl_true;
			}
		}
		if (map.get(name, _out)) {
			if (outName) {
				*outName = name;
			}
			return sl_true;
		}
		return sl_false;
	}

}

#include "sapp_document_raw.cpp.h"
#include "sapp_document_string.cpp.h"
#include "sapp_document_color.cpp.h"
#include "sapp_document_drawable.cpp.h"
#include "sapp_document_menu.cpp.h"
#include "sapp_document_layout.cpp.h"

#include "sapp_resources.cpp.h"
#include "sapp_values.cpp.h"
#include "sapp_util.cpp.h"
