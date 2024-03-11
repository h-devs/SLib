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

	sl_bool SAppDocument::_registerRawResources(const String& fileDirPath)
	{
		log(g_str_log_open_raws_begin, fileDirPath);
		return _registerRawResources(sl_null, sl_null, fileDirPath);
	}

	sl_bool SAppDocument::_registerRawResources(SAppRawResource* parent, const String& resourcePath, const String& fileDirPath)
	{
		List<String> _list = File::getFiles(fileDirPath);
		_list.sort();
		ListElements<String> list(_list);
		for (sl_size i = 0; i < list.count; i++) {
			const String& fileName = list[i];
			if (fileName.isNotEmpty() && !(fileName.startsWith('.'))) {
				String name;
				String resourcePathChild;
				if (resourcePath.isNotEmpty()) {
					resourcePathChild = String::concat(resourcePath, "/", fileName);
				} else {
					resourcePathChild = fileName;
				}
				if (!(_registerRawResource(fileName, resourcePathChild, String::concat(fileDirPath, "/", fileName), name, parent))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_registerRawResource(const String& _resourceName, const String& resourcePath, const String& filePath, String& outName, SAppRawResource* parent)
	{
		String resourceName = Resources::makeResourceName(_resourceName);
		if (parent) {
			if (parent->sub.find(resourceName)) {
				logError(g_str_error_resource_raw_name_duplicated, resourceName, filePath);
				return sl_false;
			}
		} else {
			if (m_raws.find(resourceName)) {
				logError(g_str_error_resource_raw_name_duplicated, resourceName, filePath);
				return sl_false;
			}
		}
		Ref<SAppRawResource> res = new SAppRawResource;
		if (res.isNull()) {
			logError(g_str_error_out_of_memory);
			return sl_false;
		}
		res->name = resourceName;
		res->resourcePath = resourcePath;
		if (File::isDirectory(filePath)) {
			if (!(_registerRawResources(res.get(), resourcePath, filePath))) {
				return sl_false;
			}
		} else {
			res->filePath = filePath;
		}
		if (parent) {
			if (!(parent->sub.put(resourceName, res))) {
				logError(g_str_error_out_of_memory);
				return sl_false;
			}
		} else {
			if (!(m_raws.put(resourceName, res))) {
				logError(g_str_error_out_of_memory);
				return sl_false;
			}
		}
		outName = resourceName;
		return sl_true;
	}

	sl_bool SAppDocument::_generateRawCpp(const String& targetPath, const String& namespace1, const String& namespace2)
	{
		log(g_str_log_generate_cpp_raws_begin);

		if (!(File::isDirectory(targetPath + "/raw"))) {
			File::createDirectory(targetPath + "/raw");
			if (!(File::isDirectory(targetPath + "/raw"))) {
				log(g_str_error_directory_create_failed, targetPath + "/raw");
				return sl_false;
			}
		}

		StringBuffer sbHeader, sbCpp, sbMap;

		sl_uint32 tabCountStart = 0;
		if (namespace2.isNotEmpty()) {
			tabCountStart = 2;
			sbHeader.add(String::format(
										"#pragma once%n%n"
										"#include <slib/core/resource.h>%n%n"
										"namespace %s%n"
										"{%n\tnamespace %s%n\t{%n%n"
										, namespace1, namespace2));
			sbCpp.add(String::format(
									 "#include \"raws.h\"%n%n"
									 "namespace %s%n"
									 "{%n\tnamespace %s%n\t{%n%n"
									 , namespace1, namespace2));
		} else {
			tabCountStart = 1;
			sbHeader.add(String::format(
										"#pragma once%n%n"
										"#include <slib/core/resource.h>%n%n"
										"namespace %s%n"
										"{%n%n"
										, namespace1));
			sbCpp.add(String::format(
									 "#include \"raws.h\"%n%n"
									 "namespace %s%n"
									 "{%n%n"
									 , namespace1));
		}

		String tabs('\t', tabCountStart);
		sbMap.add(tabs);
		sbMap.add("SLIB_DEFINE_RAW_RESOURCE_MAP_BEGIN\r\n");

		StringBuffer sbData;

		for (auto&& pair : m_raws) {
			if (pair.value.isNotNull()) {
				Ref<SAppRawResource> res = pair.value;
				if (!(_generateRawCppItem(res.get(), targetPath, sl_null, sl_null, sbHeader, sbCpp, sbMap, sbData, tabCountStart, 0))) {
					return sl_false;
				}
			}
		}

		sbMap.add(tabs);
		sbMap.add("SLIB_DEFINE_RAW_RESOURCE_MAP_END\r\n");

		sbHeader.add("\r\n");
		sbHeader.add(tabs);

		sbCpp.add("\r\n");
		sbCpp.link(sbMap);

		sbCpp.add("\r\n");
		sbCpp.link(sbData);

		if (namespace2.isNotEmpty()) {
			sbHeader.add("SLIB_DECLARE_RAW_RESOURCE_MAP\r\n\r\n\t}\r\n}\r\n");
			sbCpp.add("\r\n\t}\r\n}\r\n");
		} else {
			sbHeader.add("SLIB_DECLARE_RAW_RESOURCE_MAP\r\n\r\n}\r\n");
			sbCpp.add("\r\n}\r\n");
		}

		String pathHeader = targetPath + "/raws.h";
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = targetPath + "/raws.cpp";
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				logError(g_str_error_file_write_failed, pathCpp);
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_generateRawCppItem(SAppRawResource* res,
		const String& targetPath, const String& _relativePath, const String& _namespacePath,
		StringBuffer& sbHeader, StringBuffer& sbCpp, StringBuffer& sbMap, StringBuffer& sbData,
		sl_uint32 tabCountStart, sl_uint32 tabCountRelative)
	{
		String relativePath;
		String namespacePath;
		if (_relativePath.isNotEmpty()) {
			relativePath = String::concat(_relativePath, "/", res->name);
			namespacePath = String::concat(_namespacePath, "::", res->name);
		} else {
			relativePath = res->name;
			namespacePath = res->name;
		}
		String tab('\t', tabCountStart + tabCountRelative);
		if (res->filePath.isNotNull()) {
			sbHeader.add(String::format("%sSLIB_DECLARE_RAW_RESOURCE(%s)%n", tab, res->name));
			sl_size size = (sl_size)(File::getSize(res->filePath));
			if (size > RAW_MAX_SIZE) {
				logError(g_str_error_resource_raw_size_big, res->filePath);
				return sl_false;
			}
			sbCpp.add(String::format("%sSLIB_DEFINE_RAW_RESOURCE(%s, %d)%n", tab, res->name, size));
			if (res->resourcePath.isNotEmpty()) {
				sbMap.add(String('\t', tabCountStart));
				sbMap.add(String::format("\tSLIB_DEFINE_RAW_RESOURCE_MAP_PATH(\"%s\", %s)%n", res->resourcePath, namespacePath));
			}
			sbData.add(String::format("%s#include \"raw/%s.inc\"%n", tab, relativePath));
			return _generateRawDataFile(String::format("%s/raw/%s.inc", targetPath, relativePath), res->filePath, res->name);
		} else {
			File::createDirectory(String::format("%s/raw/%s", targetPath, relativePath));
			String strNamespace = String::format("%snamespace %s {%n", tab, res->name);
			sbHeader.add(strNamespace);
			sbCpp.add(strNamespace);
			sbData.add(strNamespace);
			for (auto&& pair : res->sub) {
				if (pair.value.isNotNull()) {
					Ref<SAppRawResource> res = pair.value;
					if (!(_generateRawCppItem(res.get(), targetPath, relativePath, namespacePath, sbHeader, sbCpp, sbMap, sbData, tabCountStart, tabCountRelative + 1))) {
						return sl_false;
					}
				}
			}
			strNamespace = String::format("%s}%n", tab);
			sbHeader.add(strNamespace);
			sbCpp.add(strNamespace);
			sbData.add(strNamespace);
			return sl_true;
		}
	}

	sl_bool SAppDocument::_generateRawDataFile(const String& targetPath, const String& sourcePath, const String& resourceName)
	{
		if (!(File::exists(sourcePath))) {
			return sl_false;
		}
		File fileSrc = File::openForRead(sourcePath);
		if (fileSrc.isNone()) {
			return sl_false;
		}
		Time timeModified = fileSrc.getModifiedTime();
		String signature = String::format("// Source: %s Size: %d bytes, Modified Time: %04y-%02m-%02d %02H:%02M:%02S", File::getFileName(sourcePath), fileSrc.getSize(), timeModified);
		if (File::exists(targetPath)) {
			File fileDst = File::openForRead(targetPath);
			if (fileDst.isOpened()) {
				String line = fileDst.readLine();
				if (line == signature) {
					return sl_true;
				}
				fileDst.close();
				File::deleteFile(targetPath);
			}
		}
		Memory mem = fileSrc.readAllBytes();
		File fileDst = File::openForWrite(targetPath);
		if (fileDst.isOpened()) {
			fileDst.writeTextUTF8(signature);
			fileDst.writeTextUTF8(String::format("\r\nnamespace %s {%nconst sl_uint8 bytes[] = {%n", resourceName, mem.getData()));
			fileDst.writeTextUTF8(SAppUtil::generateBytesArrayDefinition(mem.getData(), mem.getSize(), 16, 0));
			static const sl_char8 strDataEnd[] = "};\r\n}\r\n";
			fileDst.write(strDataEnd, sizeof(strDataEnd) - 1);
			return sl_true;
		}
		logError(g_str_error_file_write_failed, targetPath);
		return sl_false;
	}

}
