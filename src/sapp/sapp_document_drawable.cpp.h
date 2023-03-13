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

	sl_bool SAppDocument::_generateDrawablesCpp(const String& targetPath)
	{
		log(g_str_log_generate_cpp_drawables_begin);

		StringBuffer sbHeader, sbCpp, sbMap;

		sbHeader.add(String::format(
									"#pragma once%n%n"
									"#include <slib/graphics/resource.h>%n%n"
									"namespace %s%n"
									"{%n\tnamespace drawable%n\t{%n%n"
									, m_conf.generate_cpp_namespace));
		sbCpp.add(String::format(
								 "#include \"drawables.h\"%n%n"
								 "#include \"raws.h\"%n%n"
								 "namespace %s%n"
								 "{%n\tnamespace drawable%n\t{%n%n"
								 , m_conf.generate_cpp_namespace));

		sbMap.add("\t\tSLIB_DEFINE_DRAWABLE_RESOURCE_MAP_BEGIN\r\n");

		// iterate file resources
		{
			for (auto& pair : m_drawables) {
				if (pair.value.isNotNull()) {
					Ref<SAppDrawableResource>& res = pair.value;
					if (res->type == SAppDrawableResource::typeFile) {
						_generateDrawablesCpp_File(res.get(), sbHeader, sbCpp, sbMap);
					}
				}
			}
		}

		// iterate other resources
		{
			for (auto& pair : m_drawables) {
				if (pair.value.isNotNull()) {
					Ref<SAppDrawableResource>& res = pair.value;
					if (res->type == SAppDrawableResource::typeNinePieces) {
						_generateDrawablesCpp_NinePieces(res.get(), sbHeader, sbCpp, sbMap);
					} else if (res->type == SAppDrawableResource::typeNinePatch) {
						_generateDrawablesCpp_NinePatch(res.get(), sbHeader, sbCpp, sbMap);
					}
				}
			}
		}

		sbMap.add("\t\tSLIB_DEFINE_DRAWABLE_RESOURCE_MAP_END\r\n");

		sbHeader.add("\r\n\t\tSLIB_DECLARE_DRAWABLE_RESOURCE_MAP\r\n\r\n\t}\r\n}\r\n");

		sbCpp.link(sbMap);
		sbCpp.add("\r\n\t}\r\n}\r\n");


		String pathHeader = targetPath + "/drawables.h";
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = targetPath + "/drawables.cpp";
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				logError(g_str_error_file_write_failed, pathCpp);
				return sl_false;
			}
		}

		return sl_true;
	}

	sl_bool SAppDocument::_getDrawableAccessString(const String& localNamespace, const SAppDrawableValue& value, String& result)
	{
		if (!(value.flagDefined)) {
			result = "slib::Ref<slib::Drawable>::null()";
			return sl_true;
		}
		if (value.flagNull) {
			result = "slib::Ref<slib::Drawable>::null()";
			return sl_true;
		}
		if (value.flagColor) {
			if (value.resourceName.isNotNull()) {
				String name;
				if (_checkColorName(localNamespace, value.resourceName, value.referingElement, &name)) {
					result = String::format("slib::Drawable::createColorDrawable(color::%s::get())", name);
					return sl_true;
				} else {
					return sl_false;
				}
			} else {
				Color color = value.color;
				result = String::format("slib::Drawable::createColorDrawable(slib::Color(%d, %d, %d, %d))", color.r, color.g, color.b, color.a);
				return sl_true;
			}
		}

		String name;
		if (!(_checkDrawableName(localNamespace, value.resourceName, value.referingElement, &name))) {
			return sl_false;
		}

		String str;
		if (value.flagWhole) {
			str = String::format("drawable::%s::get()", name);
		} else {
			str = String::format("slib::Drawable::createSubDrawable(drawable::%s::get(), %ff, %ff, %ff, %ff)", name, value.x, value.y, value.width, value.height);
		}
		if (value.func == SAppDrawableValue::FUNC_NINEPATCH) {
			str = String::format("slib::NinePatchDrawable::create(%s, %s, %s, %s, %s, %ff, %ff, %ff, %ff)", value.patchLeftWidthDst.getAccessString(), value.patchRightWidthDst.getAccessString(), value.patchTopHeightDst.getAccessString(), value.patchBottomHeightDst.getAccessString(), str, value.patchLeftWidth, value.patchRightWidth, value.patchTopHeight, value.patchBottomHeight);
		} else if (value.func == SAppDrawableValue::FUNC_THREEPATCH_HORIZONTAL) {
			str = String::format("slib::HorizontalThreePatchDrawable::create(%s, %s, %s, %ff, %ff)", value.patchLeftWidthDst.getAccessString(), value.patchRightWidthDst.getAccessString(), str, value.patchLeftWidth, value.patchRightWidth);
		} else if (value.func == SAppDrawableValue::FUNC_THREEPATCH_VERTICAL) {
			str = String::format("slib::VerticalThreePatchDrawable::create(%s, %s, %s,%ff, %ff)", value.patchTopHeightDst.getAccessString(), value.patchBottomHeightDst.getAccessString(), str, value.patchTopHeight, value.patchBottomHeight);
		}

		result = str;

		return sl_true;

	}

	sl_bool SAppDocument::_getDrawableValue(const String& localNamespace, const SAppDrawableValue& value, Ref<Drawable>& result)
	{
		if (!(value.flagDefined)) {
			result.setNull();
			return sl_true;
		}
		if (value.flagNull) {
			result.setNull();
			return sl_true;
		}
		if (value.flagColor) {
			if (value.resourceName.isNotNull()) {
				Ref<SAppColorResource> res;
				if (_checkColorName(localNamespace, value.resourceName, value.referingElement, sl_null, &res)) {
					result = Drawable::createColorDrawable(res->value);
					return sl_true;
				} else {
					return sl_false;
				}
			} else {
				result = Drawable::createColorDrawable(value.color);
				return sl_true;
			}
		}

		Ref<SAppDrawableResource> res;
		if (!(_checkDrawableName(localNamespace, value.resourceName, value.referingElement, sl_null, &res))) {
			return sl_false;
		}

		Ref<Drawable> drawable;
		if (res->type == SAppDrawableResource::typeFile) {
			drawable = _getDrawableValue_File(res.get());
		} else if (res->type == SAppDrawableResource::typeNinePieces) {
			drawable = _getDrawableValue_NinePieces(res.get());
		} else if (res->type == SAppDrawableResource::typeNinePatch) {
			drawable = _getDrawableValue_NinePatch(res.get());
		}
		if (drawable.isNotNull()) {
			if (!(value.flagWhole)) {
				drawable = Drawable::createSubDrawable(drawable, value.x, value.y, value.width, value.height);
			}
			if (value.func == SAppDrawableValue::FUNC_NINEPATCH) {
				drawable = NinePatchDrawable::create(_getDimensionValue(value.patchLeftWidthDst), _getDimensionValue(value.patchRightWidthDst), _getDimensionValue(value.patchTopHeightDst), _getDimensionValue(value.patchBottomHeightDst), drawable, value.patchLeftWidth, value.patchRightWidth, value.patchTopHeight, value.patchBottomHeight);
			} else if (value.func == SAppDrawableValue::FUNC_THREEPATCH_HORIZONTAL) {
				drawable = HorizontalThreePatchDrawable::create(_getDimensionValue(value.patchLeftWidthDst), _getDimensionValue(value.patchRightWidthDst), drawable, value.patchLeftWidth, value.patchRightWidth);
			} else if (value.func == SAppDrawableValue::FUNC_THREEPATCH_VERTICAL) {
				drawable = VerticalThreePatchDrawable::create(_getDimensionValue(value.patchTopHeightDst), _getDimensionValue(value.patchBottomHeightDst), drawable, value.patchTopHeight, value.patchBottomHeight);
			}
		}
		if (drawable.isNotNull()) {
			result = drawable;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_bool SAppDocument::_checkDrawableValue(const String& localNamespace, const SAppDrawableValue& value)
	{
		if (!(value.flagDefined)) {
			return sl_true;
		}
		if (value.flagNull) {
			return sl_true;
		}
		if (value.flagColor) {
			if (value.resourceName.isNotNull()) {
				return _checkColorName(localNamespace, value.resourceName, value.referingElement);
			} else {
				return sl_true;
			}
		}
		return _checkDrawableName(localNamespace, value.resourceName, value.referingElement);
	}

	sl_bool SAppDocument::_checkDrawableName(const String& localNamespace, const String& name, const Ref<XmlElement>& element, String* outName, Ref<SAppDrawableResource>* outResource)
	{
		if (getItemFromMap(m_drawables, localNamespace, name, outName, outResource)) {
			return sl_true;
		} else {
			logError(element, String::format(g_str_error_drawable_not_found, name));
			return sl_false;
		}
	}

	sl_bool SAppDocument::_registerFileResources(const String& resourcePath, const String& fileDirPath, const Locale& locale)
	{
		log(g_str_log_open_drawables_begin, fileDirPath);
		List<String> _list = File::getFiles(fileDirPath);
		_list.sort();
		ListElements<String> list(_list);
		for (sl_size i = 0; i < list.count; i++) {
			const String& fileName = list[i];
			if (!(fileName.startsWith('.'))) {
				String ext = File::getFileExtension(fileName);
				if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" || ext == "svg") {
					String name = File::getFileNameOnly(fileName);
					sl_reg indexSharp = name.indexOf('#');
					sl_bool flagMain = sl_true;
					if (indexSharp >= 0) {
						flagMain = sl_false;
						name = name.substring(0, indexSharp);
					}
					if (name.isEmpty()) {
						logError(g_str_error_resource_drawable_filename_invalid, File::concatPath(resourcePath, fileName));
						return sl_false;
					}
					name = Resources::makeResourceName(name);
					Ref<SAppDrawableResource> res = m_drawables.getValue(name, Ref<SAppDrawableResource>::null());
					if (res.isNull()) {
						if (locale != Locale::Unknown) {
							logError(g_str_error_resource_drawable_not_defined_default, name);
							return sl_false;
						}
						res = new SAppDrawableResource;
						if (res.isNull()) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
						res->name = name;
						res->type = SAppDrawableResource::typeFile;
						res->fileAttrs = new SAppDrawableResourceFileAttributes;
						if (res->fileAttrs.isNull()) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
						if (!(m_drawables.put(name, res))) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					}
					if (res->type != SAppDrawableResource::typeFile) {
						logError(g_str_error_resource_drawable_type_duplicated, File::concatPath(resourcePath, fileName));
						return sl_false;
					}

					SAppDrawableResourceFileAttributes* fileAttr = res->fileAttrs.get();

					List< Ref<SAppDrawableResourceFileItem> > list;
					if (locale == Locale::Unknown) {
						list = fileAttr->defaultFiles;
						if (list.isNull()) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					} else {
						fileAttr->files.get(locale, &list);
						if (list.isNull()) {
							list = List< Ref<SAppDrawableResourceFileItem> >::create();
							if (list.isNull()) {
								logError(g_str_error_out_of_memory);
								return sl_false;
							}
							if (!(fileAttr->files.put(locale, list))) {
								logError(g_str_error_out_of_memory);
								return sl_false;
							}
						}
					}

					Ref<SAppDrawableResourceFileItem> item = new SAppDrawableResourceFileItem;
					if (item.isNull()) {
						logError(g_str_error_out_of_memory);
						return sl_false;
					}
					item->fileName = fileName;
					item->filePath = File::concatPath(fileDirPath, fileName);
					if (!(_registerRawResource(File::concatPath(resourcePath, fileName), sl_null, item->filePath, item->rawName))) {
						return sl_false;
					}
					if (flagMain) {
						if (!(list.insert(0, item))) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					} else {
						if (!(list.add(item))) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					}
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_generateDrawablesCpp_File(SAppDrawableResource* res, StringBuffer& sbHeader, StringBuffer& sbCpp, StringBuffer& sbMap)
	{
		SAppDrawableResourceFileAttributes* fileAttr = res->fileAttrs.get();

		Ref<SAppDrawableResourceFileItem> item;

		sl_bool flagSuccess = sl_false;

		sl_uint32 defaultWidth = 0;
		sl_uint32 defaultHeight = 0;

		if (fileAttr->defaultFiles.getAt(0, &item)) {

			Ref<Drawable> source = item->load();

			if (source.isNotNull()) {
				defaultWidth = (sl_uint32)(source->getDrawableWidth());
				defaultHeight = (sl_uint32)(source->getDrawableHeight());
				flagSuccess = sl_true;
			}

		}

		if (!flagSuccess) {
			logError(g_str_error_load_image_failed, res->name);
			return sl_false;
		}


		sbHeader.add(String::format("\t\tSLIB_DECLARE_IMAGE_RESOURCE(%s)%n", res->name));

		if (fileAttr->files.isEmpty() && fileAttr->defaultFiles.getCount() == 1 && item.isNotNull()) {

			sbCpp.add(String::format("\t\tSLIB_DEFINE_IMAGE_RESOURCE_SIMPLE(%s, %d, %d, raw::%s::size, raw::%s::bytes)%n%n", res->name, defaultWidth, defaultHeight, item->rawName));

		} else {

			sbCpp.add(String::format("\t\tSLIB_DEFINE_IMAGE_RESOURCE_BEGIN(%s, %d, %d)%n", res->name, defaultWidth, defaultHeight));

			CList< Pair<Locale, List< Ref<SAppDrawableResourceFileItem> > > > listPairs;
			{
				for (auto& pairItems : fileAttr->files) {
					if (pairItems.key.getCountry() != Country::Unknown && pairItems.key.getScript() != LanguageScript::Unknown) {
						if (!(listPairs.add_NoLock(pairItems.key, pairItems.value))) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					}
				}
			}
			{
				for (auto& pairItems : fileAttr->files) {
					if (pairItems.key.getCountry() != Country::Unknown && pairItems.key.getScript() == LanguageScript::Unknown) {
						if (!(listPairs.add_NoLock(pairItems.key, pairItems.value))) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					}
				}
			}
			{
				for (auto& pairItems : fileAttr->files) {
					if (pairItems.key.getCountry() == Country::Unknown && pairItems.key.getScript() != LanguageScript::Unknown) {
						if (!(listPairs.add_NoLock(pairItems.key, pairItems.value))) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					}
				}
			}
			{
				for (auto& pairItems : fileAttr->files) {
					if (pairItems.key.getCountry() == Country::Unknown && pairItems.key.getScript() == LanguageScript::Unknown) {
						if (!(listPairs.add_NoLock(pairItems.key, pairItems.value))) {
							logError(g_str_error_out_of_memory);
							return sl_false;
						}
					}
				}
			}
			// default
			{
				Pair<Locale, List< Ref<SAppDrawableResourceFileItem> > > pairItems;
				pairItems.first = Locale::Unknown;
				pairItems.second = fileAttr->defaultFiles;
				if (!(listPairs.add_NoLock(pairItems))) {
					logError(g_str_error_out_of_memory);
					return sl_false;
				}
			}

			ListElements< Pair<Locale, List< Ref<SAppDrawableResourceFileItem> > > > pairs(listPairs);
			for (sl_size iPair = 0; iPair < pairs.count; iPair++) {

				String strLocale = pairs[iPair].first.toString();

				ListLocker< Ref<SAppDrawableResourceFileItem> > items(pairs[iPair].second);

				if (pairs[iPair].first == Locale::Unknown) {
					static sl_char8 str[] = "\t\t\tSLIB_DEFINE_IMAGE_RESOURCE_DEFAULT_LIST_BEGIN\r\n";
					sbCpp.addStatic(str, sizeof(str)-1);
				} else {
					sbCpp.add(String::format("\t\t\tSLIB_DEFINE_IMAGE_RESOURCE_LIST_BEGIN(%s)%n", strLocale));
				}

				for (sl_size i = 0; i < items.count; i++) {

					item = items[i];

					Ref<Drawable> source = item->load();
					if (source.isNull()) {
						logError(g_str_error_resource_drawable_load_image_failed, item->filePath);
						return sl_false;
					}
					sbCpp.add(String::format("\t\t\t\tSLIB_DEFINE_IMAGE_RESOURCE_ITEM(%d, %d, raw::%s::size, raw::%s::bytes)%n", source->getDrawableWidth(), source->getDrawableHeight(), item->rawName));
				}

				static sl_char8 strEnd[] = "\t\t\tSLIB_DEFINE_IMAGE_RESOURCE_LIST_END\r\n";
				sbCpp.addStatic(strEnd, sizeof(strEnd)-1);


			}

			static sl_char8 strEnd[] = "\t\tSLIB_DEFINE_IMAGE_RESOURCE_END\r\n\r\n";
			sbCpp.addStatic(strEnd, sizeof(strEnd)-1);

		}

		sbMap.add(String::format("\t\t\tSLIB_DEFINE_DRAWABLE_RESOURCE_MAP_ITEM(%s)%n", res->name));

		return sl_true;
	}

	Ref<Drawable> SAppDocument::_getDrawableValue_File(SAppDrawableResource* res)
	{
		SAppDrawableResourceFileAttributes* fileAttr = res->fileAttrs.get();


		List< Ref<SAppDrawableResourceFileItem> > listItems;
		do {
			Locale locale = getCurrentSimulatorLocale();
			{
				for (auto& item : fileAttr->files) {
					if (item.key == locale) {
						listItems = item.value;
						break;
					}
				}
				if (listItems.isNotNull()) {
					break;
				}
			}
			{
				for (auto& item : fileAttr->files) {
					if (item.key == Locale(locale.getLanguage(), locale.getCountry())) {
						listItems = item.value;
						break;
					}
				}
				if (listItems.isNotNull()) {
					break;
				}
			}
			{
				for (auto& item : fileAttr->files) {
					if (item.key == Locale(locale.getLanguage(), locale.getScript(), Country::Unknown)) {
						listItems = item.value;
						break;
					}
				}
				if (listItems.isNotNull()) {
					break;
				}
			}
			{
				for (auto& item : fileAttr->files) {
					if (item.key == Locale(locale.getLanguage())) {
						listItems = item.value;
						break;
					}
				}
				if (listItems.isNotNull()) {
					break;
				}
			}
		} while (0);

		if (listItems.isNull()) {
			listItems = fileAttr->defaultFiles;
		}

		Ref<SAppDrawableResourceFileItem> item;
		sl_size n = listItems.getCount();
		if (n == 1) {
			if (listItems.getAt(0, &item)) {
				if (item.isNotNull()) {
					Ref<Drawable> source = item->load();
					if (source.isNotNull()) {
						return source;
					} else {
						logError(g_str_error_load_image_failed, res->name);
						return Ref<Drawable>::null();
					}
				}
			}
		} else {
			Ref<MipmapDrawable> mipmap = new MipmapDrawable;
			if (mipmap.isNotNull()) {
				sl_real defaultWidth = 1;
				sl_real defaultHeight = 1;
				ListLocker< Ref<SAppDrawableResourceFileItem> > items(listItems);
				for (sl_size i = 0; i < items.count; i++) {
					item = items[i];
					if (item.isNotNull()) {
						Ref<Drawable> source = item->load();
						if (source.isNotNull()) {
							sl_real width = source->getDrawableWidth();
							sl_real height = source->getDrawableHeight();
							mipmap->addSource(source, width, height);
							if (width > SLIB_EPSILON && height > SLIB_EPSILON && i == 0) {
								defaultWidth = width;
								defaultHeight = height;
							}
						} else {
							logError(g_str_error_resource_drawable_load_image_failed, item->filePath);
							return Ref<Drawable>::null();
						}
					}
				}
				mipmap->setDrawableWidth(defaultWidth);
				mipmap->setDrawableHeight(defaultHeight);
				return mipmap;
			}
		}
		return Ref<Drawable>::null();
	}

#define LOG_ERROR_NINEPIECES_ATTR(NAME) \
	logError(element, g_str_error_resource_ninepieces_attribute_invalid, #NAME, str_##NAME);

#define PARSE_AND_CHECK_NINEPIECES_DIMENSION_ATTR(ATTR, NAME) \
	String str_##NAME = element->getAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME, this))) { \
		LOG_ERROR_NINEPIECES_ATTR(NAME) \
		return sl_false; \
	}
	
#define PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(ATTR, NAME) \
	String str_##NAME = element->getAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME, this, element))) { \
		LOG_ERROR_NINEPIECES_ATTR(NAME) \
		return sl_false; \
	}

	sl_bool SAppDocument::_parseNinePiecesDrawableResource(const String& localNamespace, const Ref<XmlElement>& element)
	{
		if (element.isNull()) {
			return sl_false;
		}

		String name = element->getAttribute("name");
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_ninepieces_name_is_empty);
			return sl_false;
		}
		if (!(SAppUtil::checkName(name.getData(), name.getLength()))) {
			logError(element, g_str_error_resource_ninepieces_name_invalid, name);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_drawables.find(name)) {
			logError(element, g_str_error_resource_ninepieces_name_redefined, name);
			return sl_false;
		}

		Ref<SAppDrawableResource> res = new SAppDrawableResource;
		if (res.isNull()) {
			logError(g_str_error_out_of_memory);
			return sl_false;
		}
		res->name = name;
		res->type = SAppDrawableResource::typeNinePieces;
		res->ninePiecesAttrs = new SAppDrawableResourceNinePiecesAttributes;
		if (res->ninePiecesAttrs.isNull()) {
			logError(g_str_error_out_of_memory);
			return sl_false;
		}

		SAppDrawableResourceNinePiecesAttributes* attr = res->ninePiecesAttrs.get();
		attr->localNamespace = localNamespace;

		PARSE_AND_CHECK_NINEPIECES_DIMENSION_ATTR(attr->, leftWidth)
		if (!(attr->leftWidth.checkGlobal())) {
			LOG_ERROR_NINEPIECES_ATTR(leftWidth)
			return sl_false;
		}
		PARSE_AND_CHECK_NINEPIECES_DIMENSION_ATTR(attr->, rightWidth)
		if (!(attr->rightWidth.checkGlobal())) {
			LOG_ERROR_NINEPIECES_ATTR(rightWidth)
			return sl_false;
		}
		PARSE_AND_CHECK_NINEPIECES_DIMENSION_ATTR(attr->, topHeight)
		if (!(attr->topHeight.checkGlobal())) {
			LOG_ERROR_NINEPIECES_ATTR(topHeight)
			return sl_false;
		}
		PARSE_AND_CHECK_NINEPIECES_DIMENSION_ATTR(attr->, bottomHeight)
		if (!(attr->bottomHeight.checkGlobal())) {
			LOG_ERROR_NINEPIECES_ATTR(bottomHeight)
			return sl_false;
		}

		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, topLeft)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, top)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, topRight)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, left)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, center)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, right)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, bottomLeft)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, bottom)
		PARSE_AND_CHECK_NINEPIECES_DRAWABLE_ATTR(attr->, bottomRight)

		if (!(m_drawables.put(name, res))) {
			logError(g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	sl_bool SAppDocument::_generateDrawablesCpp_NinePieces(SAppDrawableResource* res, StringBuffer& sbHeader, StringBuffer& sbCpp, StringBuffer& sbMap)
	{
		SAppDrawableResourceNinePiecesAttributes* attr = res->ninePiecesAttrs.get();

		sbHeader.add(String::format("\t\tSLIB_DECLARE_NINEPIECES_RESOURCE(%s)%n", res->name));

		String strTopLeft;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->topLeft, strTopLeft))) {
			return sl_false;
		}
		String strTop;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->top, strTop))) {
			return sl_false;
		}
		String strTopRight;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->topRight, strTopRight))) {
			return sl_false;
		}
		String strLeft;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->left, strLeft))) {
			return sl_false;
		}
		String strCenter;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->center, strCenter))) {
			return sl_false;
		}
		String strRight;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->right, strRight))) {
			return sl_false;
		}
		String strBottomLeft;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->bottomLeft, strBottomLeft))) {
			return sl_false;
		}
		String strBottom;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->bottom, strBottom))) {
			return sl_false;
		}
		String strBottomRight;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->bottomRight, strBottomRight))) {
			return sl_false;
		}

		sbCpp.add(String::format("\t\tSLIB_DEFINE_NINEPIECES_RESOURCE(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)%n%n", res->name, attr->leftWidth.getAccessString(), attr->rightWidth.getAccessString(), attr->topHeight.getAccessString(), attr->bottomHeight.getAccessString(), strTopLeft, strTop, strTopRight, strLeft, strCenter, strRight, strBottomLeft, strBottom, strBottomRight));

		sbMap.add(String::format("\t\t\tSLIB_DEFINE_DRAWABLE_RESOURCE_MAP_ITEM(%s)%n", res->name));

		return sl_true;
	}

	Ref<Drawable> SAppDocument::_getDrawableValue_NinePieces(SAppDrawableResource* res)
	{
		SAppDrawableResourceNinePiecesAttributes* attr = res->ninePiecesAttrs.get();

		do {
			Ref<Drawable> topLeft;
			if (!(_getDrawableValue(attr->localNamespace, attr->topLeft, topLeft))) {
				break;
			}
			Ref<Drawable> top;
			if (!(_getDrawableValue(attr->localNamespace, attr->top, top))) {
				break;
			}
			Ref<Drawable> topRight;
			if (!(_getDrawableValue(attr->localNamespace, attr->topRight, topRight))) {
				break;
			}
			Ref<Drawable> left;
			if (!(_getDrawableValue(attr->localNamespace, attr->left, left))) {
				break;
			}
			Ref<Drawable> center;
			if (!(_getDrawableValue(attr->localNamespace, attr->center, center))) {
				break;
			}
			Ref<Drawable> right;
			if (!(_getDrawableValue(attr->localNamespace, attr->right, right))) {
				break;
			}
			Ref<Drawable> bottomLeft;
			if (!(_getDrawableValue(attr->localNamespace, attr->bottomLeft, bottomLeft))) {
				break;
			}
			Ref<Drawable> bottom;
			if (!(_getDrawableValue(attr->localNamespace, attr->bottom, bottom))) {
				break;
			}
			Ref<Drawable> bottomRight;
			if (!(_getDrawableValue(attr->localNamespace, attr->bottomRight, bottomRight))) {
				break;
			}

			return NinePiecesDrawable::create(_getDimensionValue(attr->leftWidth), _getDimensionValue(attr->rightWidth), _getDimensionValue(attr->topHeight), _getDimensionValue(attr->bottomHeight), topLeft, top, topRight, left, center, right, bottomLeft, bottom, bottomRight);

		} while (0);

		return Ref<Drawable>::null();
	}

#define LOG_ERROR_NINEPATCH_ATTR(NAME) \
	logError(element, g_str_error_resource_ninepatch_attribute_invalid, #NAME, str_##NAME);

#define PARSE_AND_CHECK_NINEPATCH_DIMENSION_ATTR(ATTR, NAME) \
	String str_##NAME = element->getAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME, this))) { \
		LOG_ERROR_NINEPATCH_ATTR(NAME) \
		return sl_false; \
	}
#define PARSE_AND_CHECK_NINEPATCH_DRAWABLE_ATTR(ATTR, NAME) \
	String str_##NAME = element->getAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME, this, element))) { \
		LOG_ERROR_NINEPATCH_ATTR(NAME) \
		return sl_false; \
	}

#define PARSE_AND_CHECK_NINEPATCH_ATTR(ATTR, NAME) \
	String str_##NAME = element->getAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME))) { \
		LOG_ERROR_NINEPATCH_ATTR(NAME) \
		return sl_false; \
	}

	sl_bool SAppDocument::_parseNinePatchDrawableResource(const String& localNamespace, const Ref<XmlElement>& element)
	{
		if (element.isNull()) {
			return sl_false;
		}

		String name = element->getAttribute("name");
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_ninepatch_name_is_empty);
			return sl_false;
		}
		if (!(SAppUtil::checkName(name.getData(), name.getLength()))) {
			logError(element, g_str_error_resource_ninepatch_name_invalid, name);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_drawables.find(name)) {
			logError(element, g_str_error_resource_ninepatch_name_redefined, name);
			return sl_false;
		}

		Ref<SAppDrawableResource> res = new SAppDrawableResource;
		if (res.isNull()) {
			logError(g_str_error_out_of_memory);
			return sl_false;
		}
		res->name = name;
		res->type = SAppDrawableResource::typeNinePatch;
		res->ninePatchAttrs = new SAppDrawableResourceNinePatchAttributes;
		if (res->ninePatchAttrs.isNull()) {
			logError(g_str_error_out_of_memory);
			return sl_false;
		}

		SAppDrawableResourceNinePatchAttributes* attr = res->ninePatchAttrs.get();
		attr->localNamespace = localNamespace;

		PARSE_AND_CHECK_NINEPATCH_DIMENSION_ATTR(attr->, dstLeftWidth)
		if (!(attr->dstLeftWidth.checkGlobal())) {
			LOG_ERROR_NINEPATCH_ATTR(dstLeftWidth)
			return sl_false;
		}
		PARSE_AND_CHECK_NINEPATCH_DIMENSION_ATTR(attr->, dstRightWidth)
		if (!(attr->dstRightWidth.checkGlobal())) {
			LOG_ERROR_NINEPATCH_ATTR(dstRightWidth)
			return sl_false;
		}
		PARSE_AND_CHECK_NINEPATCH_DIMENSION_ATTR(attr->, dstTopHeight)
		if (!(attr->dstTopHeight.checkGlobal())) {
			LOG_ERROR_NINEPATCH_ATTR(dstTopHeight)
			return sl_false;
		}
		PARSE_AND_CHECK_NINEPATCH_DIMENSION_ATTR(attr->, dstBottomHeight)
		if (!(attr->dstBottomHeight.checkGlobal())) {
			LOG_ERROR_NINEPATCH_ATTR(dstBottomHeight)
			return sl_false;
		}

		PARSE_AND_CHECK_NINEPATCH_DRAWABLE_ATTR(attr->, src)
		if (!(attr->src.flagDefined)) {
			LOG_ERROR_NINEPATCH_ATTR(src)
			return sl_false;
		}

		SAppFloatValue leftWidth;
		PARSE_AND_CHECK_NINEPATCH_ATTR(, leftWidth)
		attr->leftWidth = leftWidth.value;
		if (!(attr->dstLeftWidth.flagDefined)) {
			attr->dstLeftWidth.amount = leftWidth.value;
			attr->dstLeftWidth.unit = SAppDimensionValue::PX;
			attr->dstLeftWidth.flagDefined = sl_true;
		}

		SAppFloatValue rightWidth;
		PARSE_AND_CHECK_NINEPATCH_ATTR(, rightWidth)
		attr->rightWidth = rightWidth.value;
		if (!(attr->dstRightWidth.flagDefined)) {
			attr->dstRightWidth.amount = rightWidth.value;
			attr->dstRightWidth.unit = SAppDimensionValue::PX;
			attr->dstRightWidth.flagDefined = sl_true;
		}

		SAppFloatValue topHeight;
		PARSE_AND_CHECK_NINEPATCH_ATTR(, topHeight)
		attr->topHeight = topHeight.value;
		if (!(attr->dstTopHeight.flagDefined)) {
			attr->dstTopHeight.amount = topHeight.value;
			attr->dstTopHeight.unit = SAppDimensionValue::PX;
			attr->dstTopHeight.flagDefined = sl_true;
		}

		SAppFloatValue bottomHeight;
		PARSE_AND_CHECK_NINEPATCH_ATTR(, bottomHeight)
		attr->bottomHeight = bottomHeight.value;
		if (!(attr->dstBottomHeight.flagDefined)) {
			attr->dstBottomHeight.amount = bottomHeight.value;
			attr->dstBottomHeight.unit = SAppDimensionValue::PX;
			attr->dstBottomHeight.flagDefined = sl_true;
		}

		if (!(m_drawables.put(name, res))) {
			logError(g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	sl_bool SAppDocument::_generateDrawablesCpp_NinePatch(SAppDrawableResource* res, StringBuffer& sbHeader, StringBuffer& sbCpp, StringBuffer& sbMap)
	{
		SAppDrawableResourceNinePatchAttributes* attr = res->ninePatchAttrs.get();

		sbHeader.add(String::format("\t\tSLIB_DECLARE_NINEPATCH_RESOURCE(%s)%n", res->name));

		String strSrc;
		if (!(_getDrawableAccessString(attr->localNamespace, attr->src, strSrc))) {
			return sl_false;
		}

		sbCpp.add(String::format("\t\tSLIB_DEFINE_NINEPATCH_RESOURCE(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)%n%n", res->name, attr->dstLeftWidth.getAccessString(), attr->dstRightWidth.getAccessString(), attr->dstTopHeight.getAccessString(), attr->dstBottomHeight.getAccessString(), strSrc, attr->leftWidth, attr->rightWidth, attr->topHeight, attr->bottomHeight));

		sbMap.add(String::format("\t\t\tSLIB_DEFINE_DRAWABLE_RESOURCE_MAP_ITEM(%s)%n", res->name));

		return sl_true;
	}

	Ref<Drawable> SAppDocument::_getDrawableValue_NinePatch(SAppDrawableResource* res)
	{
		SAppDrawableResourceNinePatchAttributes* attr = res->ninePatchAttrs.get();

		Ref<Drawable> src;
		if (!(_getDrawableValue(attr->localNamespace, attr->src, src))) {
			return Ref<Drawable>::null();
		}
		return NinePatchDrawable::create(_getDimensionValue(attr->dstLeftWidth), _getDimensionValue(attr->dstRightWidth), _getDimensionValue(attr->dstTopHeight), _getDimensionValue(attr->dstBottomHeight), src, attr->leftWidth, attr->rightWidth, attr->topHeight, attr->bottomHeight);
	}

}
