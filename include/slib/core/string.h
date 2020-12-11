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

#ifndef CHECKHEADER_SLIB_CORE_STRING
#define CHECKHEADER_SLIB_CORE_STRING

#include "definition.h"

namespace slib
{

	class StringContainer;
	class StringContainer16;
	
	namespace priv
	{
		namespace string
		{
			struct ConstContainer
			{
				StringContainer* container;
				sl_int32 lock;
			};

			extern const ConstContainer g_null;
			extern const ConstContainer g_empty;
			
			struct ConstContainer16
			{
				StringContainer16* container;
				sl_int32 lock;
			};

			extern const ConstContainer16 g_null16;
			extern const ConstContainer16 g_empty16;

			extern const char* g_conv_radixPatternUpper;
			extern const char* g_conv_radixPatternLower;
			extern const sl_uint8* g_conv_radixInversePatternBig;
			extern const sl_uint8* g_conv_radixInversePatternSmall;

		}
	}

}

#include "string8.h"
#include "string16.h"
#include "string_common.h"
#include "string_view.h"
#include "string_param.h"
#include "string_op.h"

namespace slib
{

	PRIV_SLIB_DEFINE_STRING_CLASS_OP_TEMPLATE(sl_bool, equals, EqualsOperator)
	PRIV_SLIB_DEFINE_STRING_CLASS_OP_TEMPLATE(sl_compare_result, compare, CompareOperator)

	PRIV_SLIB_STRING_DEFINE_COMPARE_OPERATOR_TEMPLATE(== , s.equals(other), s.equals(other))
	PRIV_SLIB_STRING_DEFINE_COMPARE_OPERATOR_TEMPLATE(!= , !(s.equals(other)), !(s.equals(other)))
	PRIV_SLIB_STRING_DEFINE_COMPARE_OPERATOR_TEMPLATE(>= , s.compare(other) >= 0, s.compare(other) <= 0)
	PRIV_SLIB_STRING_DEFINE_COMPARE_OPERATOR_TEMPLATE(<= , s.compare(other) <= 0, s.compare(other) >= 0)
	PRIV_SLIB_STRING_DEFINE_COMPARE_OPERATOR_TEMPLATE(>, s.compare(other)>0, s.compare(other)<0)
	PRIV_SLIB_STRING_DEFINE_COMPARE_OPERATOR_TEMPLATE(<, s.compare(other)<0, s.compare(other)>0)

}

#endif
