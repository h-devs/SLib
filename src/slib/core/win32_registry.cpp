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

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/core/win32/registry.h"

#include "slib/core/variant.h"
#include "slib/core/string_buffer.h"
#include "slib/core/endian.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{

	namespace priv
	{
		namespace registry
		{

			static String ParseRegistryPath(const StringParam& _path, HKEY* hRootKeyOut)
			{
				HKEY tmp;
				if (!hRootKeyOut) {
					hRootKeyOut = &tmp;
				}
				String path = _path.toString();
				if (path.startsWith("HKLM\\")) {
					*hRootKeyOut = HKEY_LOCAL_MACHINE;
					path = path.substring(5);
				} else if (path.startsWith("HKCU\\")) {
					*hRootKeyOut = HKEY_CURRENT_USER;
					path = path.substring(5);
				} else if (path.startsWith("HKCR\\")) {
					*hRootKeyOut = HKEY_CLASSES_ROOT;
					path = path.substring(5);
				} else if (path.startsWith("HKCC\\")) {
					*hRootKeyOut = HKEY_CURRENT_CONFIG;
					path = path.substring(5);
				} else if (path.startsWith("HKU\\")) {
					*hRootKeyOut = HKEY_USERS;
					path = path.substring(4);
				} else {
					*hRootKeyOut = HKEY_CURRENT_USER;
				}
				return path;
			}

			static sl_bool GetRegistryValue(HKEY hKey, LPCWSTR name, Variant* out)
			{
				DWORD type = 0;
				DWORD size = 0;
				sl_bool flagSuccess = sl_false;
				if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, &type, NULL, &size)) {
					if (out) {
						if (size > 0) {
							switch (type) {
								case REG_BINARY:
									{
										SLIB_SCOPED_BUFFER(BYTE, 512, buf, size);
										if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, &type, buf, &size)) {
											Memory mem = Memory::create(buf, size);
											if (mem.isNotNull()) {
												out->setMemory(mem);
												flagSuccess = sl_true;
											}
										}
									}
									break;
								case REG_MULTI_SZ:
									{
										SLIB_SCOPED_BUFFER(BYTE, 512, buf, size);
										if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, &type, buf, &size)) {
											if (size >= 2) {
												sl_size len = size >> 1;
												WCHAR* str = (WCHAR*)buf;
												VariantList ret = VariantList::create();
												sl_size start = 0;
												for (sl_size i = 0; i < len; i++) {
													WCHAR ch = str[i];
													if (!ch) {
														if (i < len - 1 || i > start) {
															ret.add_NoLock(String16::from(str + start, i - start));
														}
														start = i + 1;
													}
												}
												if (len > start) {
													ret.add_NoLock(String16::from(str + start, len - start));
												}
												out->setVariantList(ret);
												flagSuccess = sl_true;
											} else {
												out->setNull();
												flagSuccess = sl_true;
											}
										}
									}
									break;
								case REG_EXPAND_SZ:
								case REG_SZ:
									{
										SLIB_SCOPED_BUFFER(BYTE, 512, buf, size);
										if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, &type, buf, &size)) {
											if (size >= 2) {
												String16 s(reinterpret_cast<sl_char16*>(buf), size / 2 - 1);
												out->setString(s);
												flagSuccess = sl_true;
											} else {
												out->setNull();
												flagSuccess = sl_true;
											}
										}
									}
									break;
								case REG_DWORD:
								case REG_DWORD_BIG_ENDIAN:
									if (size == 4) {
										sl_uint32 n;
										if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, &type, reinterpret_cast<BYTE*>(&n), &size)) {
											if (size == 4) {
												if (type == REG_DWORD) {
													out->setUint32(n);
												} else {
													out->setUint32(Endian::swap32(n));
												}
												flagSuccess = sl_true;
											}
										}
									}
									break;
								case REG_QWORD:
									if (size == 8) {
										sl_uint64 n;
										if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, &type, reinterpret_cast<BYTE*>(&n), &size)) {
											if (size == 8) {
												out->setUint64(n);
												flagSuccess = sl_true;
											}
										}
									}
									break;
								default: // REG_NONE
									out->setNull();
									flagSuccess = sl_true;
									break;
							}
						} else {
							out->setNull();
							flagSuccess = sl_true;
						}
					} else {
						flagSuccess = sl_true;
					}
				}
				return flagSuccess;
			}

		}
	}

	using namespace priv::registry;

	namespace win32
	{

		Registry Registry::open(HKEY hKeyParent, const StringParam& _path, REGSAM sam, sl_bool flagCreate)
		{
			if (!hKeyParent) {
				return NULL;
			}
			StringCstr16 path(_path);
			HKEY hKey;
			if (path.isEmpty()) {
				return NULL;
			}
			hKey = NULL;
			RegOpenKeyExW(hKeyParent, (LPCWSTR)(path.getData()), 0, sam, &hKey);
			if (!hKey) {
				if (flagCreate) {
					RegCreateKeyExW(hKeyParent, (LPCWSTR)(path.getData()), NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
				}
			}
			return hKey;
		}

		Registry Registry::create(HKEY hKeyParent, const StringParam& path, REGSAM sam)
		{
			return open(hKeyParent, path, sam, sl_true);
		}

		VariantMap Registry::getValues()
		{
			HKEY hKey = get();
			if (!hKey) {
				return sl_null;
			}

			DWORD retCode, cValues;
			retCode = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL);
			if (retCode != ERROR_SUCCESS || !(cValues)) {
				return sl_null;
			}

			WCHAR achValue[16383];
			DWORD cchValue;
			VariantMap ret;
			for (DWORD i = 0; i < cValues; i++)
			{
				cchValue = sizeof(achValue);
				achValue[0] = '\0';
				if (ERROR_SUCCESS == RegEnumValueW(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL)) {
					Variant out;
					if (GetRegistryValue(hKey, achValue, &out)) {
						ret.add(String::from(achValue, cchValue), out);
					}
				}
			}

			return ret;
		}

		VariantMap Registry::getValues(HKEY hKeyParent, const StringParam& subPath)
		{
			Registry key(open(hKeyParent, subPath, KEY_QUERY_VALUE));
			return key.getValues();
		}

		VariantMap Registry::getValues(const StringParam& path)
		{
			HKEY hRootKey;
			String subPath = ParseRegistryPath(path, &hRootKey);
			Registry key(open(hRootKey, subPath, KEY_QUERY_VALUE));
			return key.getValues();
		}
		
		sl_bool Registry::getValue(const StringParam& _name, Variant* out)
		{
			HKEY hKey = get();
			if (!hKey) {
				return sl_false;
			}

			StringCstr16 name(_name);
			return GetRegistryValue(hKey, (LPCWSTR)(name.getData()), out);
		}

		sl_bool Registry::getValue(HKEY hKeyParent, const StringParam& subPath, const StringParam& name, Variant* out)
		{
			Registry key(open(hKeyParent, subPath, KEY_QUERY_VALUE));
			return key.getValue(name, out);
		}

		sl_bool Registry::getValue(const StringParam& path, const StringParam& name, Variant* out)
		{
			HKEY hRootKey;
			String subPath = ParseRegistryPath(path, &hRootKey);
			Registry key(open(hRootKey, subPath, KEY_QUERY_VALUE));
			return key.getValue(name, out);
		}

		sl_size Registry::setValues(const VariantMap& values)
		{
			HKEY hKey = get();
			if (!hKey) {
				return sl_false;
			}

			sl_size ret = 0;
			for (auto& item : values) {
				if (setValue(item.key, item.value)) {
					ret++;
				}
			}
			return ret;
		}

		sl_size Registry::setValues(HKEY hKeyParent, const StringParam& subPath, const VariantMap& values)
		{
			Registry key(create(hKeyParent, subPath, KEY_SET_VALUE));
			return key.setValues(values);
		}

		sl_size Registry::setValues(const StringParam& path, const VariantMap& values)
		{
			HKEY hRootKey;
			String subPath = ParseRegistryPath(path, &hRootKey);
			Registry key(create(hRootKey, subPath, KEY_SET_VALUE));
			return key.setValues(values);
		}

		sl_bool Registry::setValue(const StringParam& _name, const Variant& value)
		{
			HKEY hKey = get();
			if (!hKey) {
				return sl_false;
			}

			StringCstr16 name(_name);
			sl_bool flagSuccess = sl_false;
			if (value.isNull()) {
				if (ERROR_SUCCESS == RegDeleteValueW(hKey, (LPCWSTR)(name.getData()))) {
					flagSuccess = sl_true;
				}
			} else if (value.isInt64() || value.isUint64()) {
				sl_uint64 n = value.getUint64();
				if (ERROR_SUCCESS == RegSetValueExW(hKey, (LPCWSTR)(name.getData()), NULL, REG_QWORD, reinterpret_cast<BYTE*>(&n), 8)) {
					flagSuccess = sl_true;
				}
			} else if (value.isInteger()) {
				sl_uint32 n = value.getUint32();
				if (ERROR_SUCCESS == RegSetValueExW(hKey, (LPCWSTR)(name.getData()), NULL, REG_DWORD, reinterpret_cast<BYTE*>(&n), 4)) {
					flagSuccess = sl_true;
				}
			} else if (value.isMemory()) {
				Memory mem = value.getMemory();
				if (mem.isNotNull()) {
					if (ERROR_SUCCESS == RegSetValueExW(hKey, (LPCWSTR)(name.getData()), NULL, REG_BINARY, reinterpret_cast<BYTE*>(mem.getData()), (DWORD)(mem.getSize()))) {
						flagSuccess = sl_true;
					}
				}
			} else if (value.isVariantList()) {
				StringBuffer16 sb;
				sl_char16 zero = 0;
				ListLocker<Variant> list(value.getVariantList());
				for (sl_size i = 0; i < list.count; i++) {
					sb.add(list[i].getString16());
					sb.addStatic(&zero, 1);
				}
				sb.addStatic(&zero, 1);
				String16 str = sb.merge();
				if (ERROR_SUCCESS == RegSetValueExW(hKey, (LPCWSTR)(name.getData()), NULL, REG_MULTI_SZ, reinterpret_cast<BYTE*>(str.getData()), (DWORD)(str.getLength()) * 2)) {
					flagSuccess = sl_true;
				}
			} else if (value.isString()) {
				String16 str = value.getString16().toNullTerminated();
				if (ERROR_SUCCESS == RegSetValueExW(hKey, (LPCWSTR)(name.getData()), NULL, REG_SZ, reinterpret_cast<BYTE*>(str.getData()), (DWORD)(str.getLength() + 1) * 2)) {
					flagSuccess = sl_true;
				}
			}

			return flagSuccess;
		}

		sl_bool Registry::setValue(HKEY hKeyParent, const StringParam& subPath, const StringParam& name, const Variant& value)
		{
			Registry key(create(hKeyParent, subPath, KEY_SET_VALUE));
			return key.setValue(name, value);
		}

		sl_bool Registry::setValue(const StringParam& path, const StringParam& name, const Variant& value)
		{
			HKEY hRootKey;
			String subPath = ParseRegistryPath(path, &hRootKey);
			Registry key(create(hRootKey, subPath, KEY_SET_VALUE));
			return key.setValue(name, value);
		}

	}

}

#endif
