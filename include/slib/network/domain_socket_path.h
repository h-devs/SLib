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

#ifndef CHECKHEADER_SLIB_NETWORK_DOMAIN_SOCKET_PATH
#define CHECKHEADER_SLIB_NETWORK_DOMAIN_SOCKET_PATH

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT DomainSocketPath
	{
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DomainSocketPath)

	public:
		sl_char8 data[107];
		sl_uint32 length;
		sl_bool flagAbstract;

	public:
		DomainSocketPath() noexcept;

		DomainSocketPath(const StringParam& path, sl_bool flagAbstract = sl_false) noexcept;

	public:
		DomainSocketPath& operator=(const StringParam& path) noexcept;

		operator StringView() const noexcept;

	public:
		StringView get() const noexcept;

		void set(const StringParam& path) noexcept;

		sl_uint32 getSystemSocketAddress(void* addr) const noexcept;

		sl_bool setSystemSocketAddress(const void* addr, sl_uint32 size) noexcept;

	};

	class SLIB_EXPORT AbstractDomainSocketPath : public DomainSocketPath
	{
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AbstractDomainSocketPath)

	public:
		AbstractDomainSocketPath() noexcept;

		AbstractDomainSocketPath(const StringParam& path) noexcept;

	public:
		AbstractDomainSocketPath& operator=(const StringParam& path) noexcept;

		operator StringView() const noexcept;

	};

}

#endif
