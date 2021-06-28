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

#include "slib/core/regex.h"

#include "slib/core/base.h"
#include "slib/core/safe_static.h"

#include <regex>
#include <bitset>

namespace slib
{

	namespace priv
	{
		namespace regex
		{
			
			static constexpr int ToInt(int n)
			{
				return n;
			}

			template<std::size_t N>
			static int ToInt(const std::bitset<N>& n) noexcept
			{
				return (int)(n.to_ulong());
			}

			static std::regex* Create(const StringParam& _pattern, int _flags) noexcept
			{
				StringData pattern(_pattern);
				std::regex* handle = (std::regex*)(Base::createMemory(sizeof(std::regex)));
				if (handle) {
					int flags = 0;
					if (_flags & RegExFlags::Icase) {
						flags |= std::regex_constants::icase;
					}
					if (_flags & RegExFlags::Nosubs) {
						flags |= std::regex_constants::nosubs;
					}
					if (_flags & RegExFlags::Optimize) {
						flags |= std::regex_constants::optimize;
					}
					if (_flags & RegExFlags::Collate) {
						flags |= std::regex_constants::collate;
					}
					if (_flags & RegExFlags::ECMAScript) {
						flags |= std::regex_constants::ECMAScript;
					}
					if (_flags & RegExFlags::Basic) {
						flags |= std::regex_constants::basic;
					}
					if (_flags & RegExFlags::Extended) {
						flags |= std::regex_constants::extended;
					}
					if (_flags & RegExFlags::Awk) {
						flags |= std::regex_constants::awk;
					}
					if (_flags & RegExFlags::Grep) {
						flags |= std::regex_constants::grep;
					}
					if (_flags & RegExFlags::Egrep) {
						flags |= std::regex_constants::egrep;
					}
					try {
						if (flags) {
							new (handle) std::regex((char*)(pattern.getData()), (std::size_t)(pattern.getLength()), (std::regex_constants::syntax_option_type)flags);
						} else {
							new (handle) std::regex((char*)(pattern.getData()), (std::size_t)(pattern.getLength()));
						}
					} catch (std::regex_error&) {
						Base::freeMemory(handle);
						return sl_null;
					}
					return handle;
				}
				return sl_null;
			}

			static void DeleteRegExHandle(void* _handle) noexcept
			{
				std::regex* handle = reinterpret_cast<std::regex*>(_handle);
				if (handle) {
					handle->~basic_regex();
					Base::freeMemory(handle);
				}
			}

		}
	}

	using namespace priv::regex;

	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(RegEx, HRegEx, m_handle, sl_null, DeleteRegExHandle)

	RegEx::RegEx(const StringParam& pattern) noexcept
	{
		m_handle = reinterpret_cast<HRegEx>(Create(pattern, 0));
	}
	
	RegEx::RegEx(const StringParam& pattern, const RegExFlags& flags) noexcept
	{
		m_handle = reinterpret_cast<HRegEx>(Create(pattern, flags));
	}
	
	sl_bool RegEx::match(const StringParam& _str, const RegExMatchFlags& _flags) noexcept
	{
		std::regex* handle = reinterpret_cast<std::regex*>(m_handle);
		if (handle) {
			StringData str(_str);
			int flags = 0;
			int v = _flags.value;
			if (v) {
				if (v & RegExMatchFlags::NotBol) {
					flags |= ToInt(std::regex_constants::match_not_bol);
				}
				if (v & RegExMatchFlags::NotEol) {
					flags |= ToInt(std::regex_constants::match_not_eol);
				}
				if (v & RegExMatchFlags::NotBow) {
					flags |= ToInt(std::regex_constants::match_not_bow);
				}
				if (v & RegExMatchFlags::NotEow) {
					flags |= ToInt(std::regex_constants::match_not_eow);
				}
				if (v & RegExMatchFlags::Any) {
					flags |= ToInt(std::regex_constants::match_any);
				}
				if (v & RegExMatchFlags::NotNull) {
					flags |= ToInt(std::regex_constants::match_not_null);
				}
				if (v & RegExMatchFlags::Continuous) {
					flags |= ToInt(std::regex_constants::match_continuous);
				}
				if (v & RegExMatchFlags::PrevAvail) {
					flags |= ToInt(std::regex_constants::match_prev_avail);
				}
				if (v & RegExMatchFlags::FormatSed) {
					flags |= ToInt(std::regex_constants::format_sed);
				}
				if (v & RegExMatchFlags::FormatNoCopy) {
					flags |= ToInt(std::regex_constants::format_no_copy);
				}
				if (v & RegExMatchFlags::FormatFirstOnly) {
					flags |= ToInt(std::regex_constants::format_first_only);
				}
			}
			const char* start = (char*)(str.getData());
			const char* end = start + str.getLength();
			return std::regex_match(start, end, *handle, (std::regex_constants::match_flag_type)flags);
		}
		return sl_false;
	}
	
	sl_bool RegEx::matchEmail(const StringParam& str) noexcept
	{
		SLIB_SAFE_LOCAL_STATIC(RegEx, regex, "^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9-]+(?:\\.[a-zA-Z0-9-]+)*$");
		if (SLIB_SAFE_STATIC_CHECK_FREED(regex)) {
			return sl_false;
		}
		return regex.match(str);
	}
	
}
