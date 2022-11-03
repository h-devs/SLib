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

#ifndef CHECKHEADER_SLIB_CRYPTO_JSON_ECC
#define CHECKHEADER_SLIB_CRYPTO_JSON_ECC

#include "../ecc.h"

#include "../../core/json/core.h"

namespace slib
{

	SLIB_INLINE Json ECPoint::toJson() const noexcept
	{
		Json ret;
		ret.putItem("x", x.toJson());
		ret.putItem("y", y.toJson());
		return ret;
	}

	SLIB_INLINE sl_bool ECPoint::setJson(const Json& json) noexcept
	{
		if (json.isUndefined()) {
			return sl_false;
		}
		x.setJson(json.getItem("x"));
		y.setJson(json.getItem("y"));
		return sl_true;
	}


	SLIB_INLINE Json ECPublicKey::toJson() const noexcept
	{
		return Q.toJson();
	}

	SLIB_INLINE sl_bool ECPublicKey::setJson(const Json& json) noexcept
	{
		return Q.setJson(json);
	}


	SLIB_INLINE Json ECPrivateKey::toJson() const noexcept
	{
		Json ret;
		ret.putItem("Q", Q.toJson());
		ret.putItem("d", d.toJson());
		return ret;
	}

	SLIB_INLINE sl_bool ECPrivateKey::setJson(const Json& json) noexcept
	{
		if (json.isUndefined()) {
			return sl_false;
		}
		Q.setJson(json.getItem("Q"));
		d.setJson(json.getItem("d"));
		return sl_true;
	}


	SLIB_INLINE Json ECDSA_Signature::toJson() const noexcept
	{
		Json ret;
		ret.putItem("r", r.toJson());
		ret.putItem("s", s.toJson());
		return ret;
	}

	SLIB_INLINE sl_bool ECDSA_Signature::setJson(const Json& json) noexcept
	{
		if (json.isUndefined()) {
			return sl_false;
		}
		r.setJson(json.getItem("r"));
		s.setJson(json.getItem("s"));
		return sl_true;
	}

}

#endif
