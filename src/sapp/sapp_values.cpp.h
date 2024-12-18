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

#include "sapp_values.h"

#include "sapp_document.h"
#include "sapp_util.h"

#include "slib/core/variant.h"
#include "slib/math/calculator.h"
#include "slib/ui/radio_button.h"

namespace slib
{

	namespace
	{
		template <class T>
		static sl_reg ParseFloat(T* _out, const sl_char8* str, sl_size start, sl_size end)
		{
			return Calculator::calculate(_out, sl_null, str, start, end);
		}

		template <class T>
		static sl_bool ParseFloat(T* _out, const String& str)
		{
			return Calculator::calculate(str, _out);
		}
	}

	namespace priv
	{
		sl_bool ParseDataAccess(const String& str, String& dataAccess)
		{
			if (str.isEmpty()) {
				return sl_true;
			}
			if (str == "%" || str == "%%") {
				dataAccess = String::getEmpty();
				return sl_true;
			}
			if (!(str.startsWith("%[") && str.endsWith("]%"))) {
				return sl_false;
			}
			sl_char8* data = str.getData();
			StringBuffer buf;
			sl_size start = 2;
			for (;;) {
				sl_reg index = str.indexOf("][", start);
				if (index < 0) {
					break;
				}
				buf.addStatic("[");
				buf.add(Stringx::applyBackslashEscapes(StringView(data + start, index - start)));
				buf.addStatic("]");
				start = index + 2;
			}
			buf.addStatic("[");
			buf.add(Stringx::applyBackslashEscapes(StringView(data + start, str.getLength() - 2 - start)));
			buf.addStatic("]");
			dataAccess = buf.merge();
			return sl_true;
		}
	}

	sl_bool SAppStringValue::parse(const String& _str, const Ref<XmlElement>& element)
	{
		String str = _str;
		if (str.isEmpty()) {
			return sl_true;
		}
		if (str.startsWith('@')) {
			str = str.substring(1);
			if (str.startsWith('@')) {
				flagReferResource = sl_false;
				valueOrName = str;
			} else {
				str = str.trim();
				if (str == "null") {
					flagReferResource = sl_false;
					valueOrName = String::null();
					flagDefined = sl_true;
					return sl_true;
				}
				if (str == "empty") {
					flagReferResource = sl_false;
					valueOrName = String::getEmpty();
					flagDefined = sl_true;
					return sl_true;
				}
				if (str.startsWith("string/")) {
					str = str.substring(7).trim();
				}
				String varName;
				sl_reg index = str.indexOf('/');
				if (index > 0) {
					varName = str.substring(index + 1);
					if (varName.isNotNull()) {
						if (!(SAppUtil::checkName(varName.getData(), varName.getLength()))) {
							return sl_false;
						}
					}
					str = str.substring(0, index);
				}
				if (!(SAppUtil::checkName(str.getData(), str.getLength()))) {
					return sl_false;
				}
				flagReferResource = sl_true;
				valueOrName = str;
				variant = varName;
				referingElement = element;
			}
		} else {
			flagReferResource = sl_false;
			valueOrName = str;
		}
		flagDefined = sl_true;
		return sl_true;
	}

	sl_bool SAppStringValue::parseDataAccess(const String& str)
	{
		if (priv::ParseDataAccess(str, dataAccess)) {
			return sl_true;
		}
		flagFormattingDataValue = sl_true;
		dataAccess = str;
		return sl_true;
	}


	String SAppDimensionValue::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		switch (unit) {
			case PX:
				return String::format("%d", (int)amount);
			case SW:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getScreenWidth()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getScreenWidth())", amount);
				}
			case SH:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getScreenHeight()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getScreenHeight())", amount);
				}
			case SMIN:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getScreenMinimum()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getScreenMinimum())", amount);
				}
			case SMAX:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getScreenMaximum()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getScreenMaximum())", amount);
				}
			case VW:
				if (Math::isAlmostZero(amount - 1)) {
					return "CONTENT_WIDTH";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*CONTENT_WIDTH)", amount);
				}
			case VH:
				if (Math::isAlmostZero(amount - 1)) {
					return "CONTENT_HEIGHT";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*CONTENT_HEIGHT)", amount);
				}
			case VMIN:
				if (Math::isAlmostZero(amount - 1)) {
					return "SLIB_MIN(CONTENT_WIDTH, CONTENT_HEIGHT)";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*SLIB_MIN(CONTENT_WIDTH, CONTENT_HEIGHT))", amount);
				}
			case VMAX:
				if (Math::isAlmostZero(amount - 1)) {
					return "SLIB_MAX(CONTENT_WIDTH, CONTENT_HEIGHT)";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*SLIB_MAX(CONTENT_WIDTH, CONTENT_HEIGHT))", amount);
				}
			case SP:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_ui_pos)(getScaledPixel())";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*getScaledPixel())", amount);
				}
			case DP:
				return String::format("slib::UIResource::toUiPos(slib::UIResource::dpToPixel(%ff))", amount);
			case PT:
				return String::format("slib::UIResource::toUiPos(slib::UIResource::pointToPixel(%ff))", amount);
			case M:
				return String::format("slib::UIResource::toUiPos(slib::UIResource::meterToPixel(%ff))", amount);
			case CM:
				return String::format("slib::UIResource::toUiPos(slib::UIResource::centimeterToPixel(%ff))", amount);
			case MM:
				return String::format("slib::UIResource::toUiPos(slib::UIResource::millimeterToPixel(%ff))", amount);
			case INCH:
				return String::format("slib::UIResource::toUiPos(slib::UIResource::inchToPixel(%ff))", amount);
			case SBAR:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getStatusBarHeight()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getStatusBarHeight())", amount);
				}
			case SAFE_L:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getSafeAreaInsetLeft()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getSafeAreaInsetLeft())", amount);
				}
			case SAFE_T:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getSafeAreaInsetTop()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getSafeAreaInsetTop())", amount);
				}
			case SAFE_R:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getSafeAreaInsetRight()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getSafeAreaInsetRight())", amount);
				}
			case SAFE_B:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getSafeAreaInsetBottom()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getSafeAreaInsetBottom())", amount);
				}
			case SAFE_W:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getSafeAreaWidth()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getSafeAreaWidth())", amount);
				}
			case SAFE_H:
				if (Math::isAlmostZero(amount - 1)) {
					return "slib::UIResource::getSafeAreaHeight()";
				} else {
					return String::format("slib::UIResource::toUiPos(%ff*slib::UIResource::getSafeAreaHeight())", amount);
				}
		}
		return "0";
	}

	String SAppDimensionFloatValue::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		switch (unit) {
			case PX:
				return String::format("%ff", amount);
			case SW:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getScreenWidth())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getScreenWidth())", amount);
				}
			case SH:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getScreenHeight())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getScreenHeight())", amount);
				}
			case SMIN:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getScreenMinimum())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getScreenMinimum())", amount);
				}
			case SMAX:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getScreenMaximum())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getScreenMaximum())", amount);
				}
			case VW:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(CONTENT_WIDTH)";
				} else {
					return String::format("%ff*(sl_real)(CONTENT_WIDTH)", amount);
				}
			case VH:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(CONTENT_HEIGHT)";
				} else {
					return String::format("%ff*(sl_real)(CONTENT_HEIGHT)", amount);
				}
			case VMIN:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(SLIB_MIN(CONTENT_WIDTH, CONTENT_HEIGHT))";
				} else {
					return String::format("%ff*(sl_real)(SLIB_MIN(CONTENT_WIDTH, CONTENT_HEIGHT))", amount);
				}
			case VMAX:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(SLIB_MAX(CONTENT_WIDTH, CONTENT_HEIGHT))";
				} else {
					return String::format("%ff*(sl_real)(SLIB_MAX(CONTENT_WIDTH, CONTENT_HEIGHT))", amount);
				}
			case SP:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(getScaledPixel())";
				} else {
					return String::format("%ff*getScaledPixel()", amount);
				}
			case DP:
				return String::format("slib::UIResource::dpToPixel(%ff)", amount);
			case PT:
				return String::format("slib::UIResource::pointToPixel(%ff)", amount);
			case M:
				return String::format("slib::UIResource::meterToPixel(%ff)", amount);
			case CM:
				return String::format("slib::UIResource::centimeterToPixel(%ff)", amount);
			case MM:
				return String::format("slib::UIResource::millimeterToPixel(%ff)", amount);
			case INCH:
				return String::format("slib::UIResource::inchToPixel(%ff)", amount);
			case SBAR:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getStatusBarHeight())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getStatusBarHeight())", amount);
				}
			case SAFE_L:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getSafeAreaInsetLeft())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getSafeAreaInsetLeft())", amount);
				}
			case SAFE_T:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getSafeAreaInsetTop())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getSafeAreaInsetTop())", amount);
				}
			case SAFE_R:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getSafeAreaInsetRight())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getSafeAreaInsetRight())", amount);
				}
			case SAFE_B:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getSafeAreaInsetBottom())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getSafeAreaInsetBottom())", amount);
				}
			case SAFE_W:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getSafeAreaWidth())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getSafeAreaWidth())", amount);
				}
			case SAFE_H:
				if (Math::isAlmostZero(amount - 1)) {
					return "(sl_real)(slib::UIResource::getSafeAreaHeight())";
				} else {
					return String::format("%ff*(sl_real)(slib::UIResource::getSafeAreaHeight())", amount);
				}
		}
		return "0";
	}

	sl_bool SAppDimensionBaseValue::parse(const String& _str, SAppDocument* doc)
	{
		do {
			String str = _str.trim();
			if (str.isEmpty()) {
				return sl_true;
			}
			str = str.toLower();
			if (str == "fill") {
				amount = 1;
				unit = FILL;
				break;
			}
			if (str == "*" || str == "match-parent") {
				amount = 1;
				unit = MATCH_PARENT;
				break;
			}
			if (str == "wrap") {
				amount = 1;
				unit = WRAP;
				break;
			}

			const sl_char8* data = str.getData();
			sl_size len = str.getLength();

			float f;
			sl_reg ret = ParseFloat(&f, data, 0, str.endsWith('*') ? len - 1 : len);
			if (ret == SLIB_PARSE_ERROR) {
				return sl_false;
			}
			sl_size pos = ret;
			while (pos < len) {
				sl_char8 c = data[pos];
				if (SLIB_CHAR_IS_SPACE_TAB(c)) {
					pos++;
				} else {
					break;
				}
			}
			if (pos >= len) {
				amount = f;
				unit = PX;
				break;
			}
			sl_bool flagPercent = sl_false;
			if (data[pos] == '%') {
				flagPercent = sl_true;
				f /= 100;
				pos++;
			}
			while (pos < len) {
				sl_char8 c = data[pos];
				if (SLIB_CHAR_IS_SPACE_TAB(c)) {
					pos++;
				} else {
					break;
				}
			}

			if (pos >= len) {
				amount = f;
				if (flagPercent) {
					unit = WEIGHT;
				} else {
					unit = PX;
				}
				break;
			}

			String strUnit(data + pos, len - pos);
			if (doc) {
				SAppDimensionValue refer;
				String strUnitLocal = SAppDocument::getGlobalName(doc->m_currentFileNamespace, strUnit);
				if (doc->m_layoutUnits.get(strUnitLocal, &refer)) {
					amount = refer.amount * f;
					unit = refer.unit;
					break;
				}
				if (doc->m_layoutUnits.get(strUnit, &refer)) {
					amount = refer.amount * f;
					unit = refer.unit;
					break;
				}
			}

			typedef HashMap<String, int> UnitMap;
			SLIB_SAFE_LOCAL_STATIC(UnitMap, units);
			if (units.isNull()) {
				units.put("fill", FILL);
				units.put("*", MATCH_PARENT);
				units.put("p", WEIGHT);
				units.put("px", PX);
				units.put("sw", SW);
				units.put("sh", SH);
				units.put("smin", SMIN);
				units.put("smax", SMAX);
				units.put("vw", VW);
				units.put("vh", VH);
				units.put("vmin", VMIN);
				units.put("vmax", VMAX);
				units.put("sp", SP);
				units.put("dp", DP);
				units.put("pt", PT);
				units.put("m", M);
				units.put("cm", CM);
				units.put("mm", MM);
				units.put("in", INCH);
				units.put("inch", INCH);
				units.put("sbar", SBAR);
				units.put("safel", SAFE_L);
				units.put("safet", SAFE_T);
				units.put("safer", SAFE_R);
				units.put("safeb", SAFE_B);
				units.put("safew", SAFE_W);
				units.put("safeh", SAFE_H);
			}

			if (units.get(strUnit, &unit)) {
				amount = f;
				break;
			}

			return sl_false;

		} while (0);

		if (Math::isAlmostZero(amount)) {
			amount = 0;
			unit = PX;
		}
		flagDefined = sl_true;
		return sl_true;
	}

	sl_bool SAppDimensionBaseValue::checkGlobal(sl_bool flagRoot)
	{
		if (!flagDefined) {
			return sl_true;
		}
		return isGlobalUnit(unit);
	}

	sl_bool SAppDimensionBaseValue::checkSP(sl_bool flagRoot)
	{
		if (!flagDefined) {
			return sl_true;
		}
		return amount > 0 && !(isSpecialUnit(unit));
	}

	sl_bool SAppDimensionBaseValue::checkPosition(sl_bool flagRoot)
	{
		if (flagRoot) {
			return checkGlobal();
		}
		if (!flagDefined) {
			return sl_true;
		}
		return !(isSpecialUnit(unit));
	}

	sl_bool SAppDimensionBaseValue::checkSize(sl_bool flagRoot)
	{
		if (!flagDefined) {
			return sl_true;
		}
		if (unit == WRAP) {
			return sl_true;
		}
		if (amount < 0) {
			return sl_false;
		}
		if (flagRoot) {
			return unit == WEIGHT || unit == FILL || unit == MATCH_PARENT || isGlobalUnit(unit);
		}
		return sl_true;
	}

	sl_bool SAppDimensionBaseValue::checkScalarSize(sl_bool flagRoot)
	{
		if (!flagDefined) {
			return sl_true;
		}
		if (amount < 0) {
			return sl_false;
		}
		if (flagRoot) {
			return isGlobalUnit(unit);
		} else {
			return !(isSpecialUnit(unit));
		}
	}

	sl_bool SAppDimensionBaseValue::checkScalarSizeOrWeight(sl_bool flagRoot)
	{
		if (!flagDefined) {
			return sl_true;
		}
		if (amount < 0) {
			return sl_false;
		}
		if (unit == WEIGHT) {
			return sl_true;
		}
		if (flagRoot) {
			return isGlobalUnit(unit);
		} else {
			return !(isSpecialUnit(unit));
		}
		return amount >= 0 && (unit == WEIGHT || !(isSpecialUnit(unit)));
	}

	sl_bool SAppDimensionBaseValue::checkMargin(sl_bool flagRoot)
	{
		if (!flagDefined) {
			return sl_true;
		}
		if (unit == WEIGHT) {
			return sl_true;
		}
		if (flagRoot) {
			return isGlobalUnit(unit);
		} else {
			return checkPosition();
		}
	}

	sl_bool SAppDimensionBaseValue::checkForWindow(sl_bool flagRoot)
	{
		return checkGlobal();
	}

	sl_bool SAppDimensionBaseValue::checkForWindowSize(sl_bool flagRoot)
	{
		if (!flagDefined) {
			return sl_true;
		}
		if (unit == WRAP || unit == FILL || unit == MATCH_PARENT) {
			return sl_true;
		}
		return amount >= 0 && isGlobalUnit(unit);
	}

	sl_bool SAppDimensionBaseValue::isNeededOnLayoutFunction()
	{
		if (!flagDefined) {
			return sl_false;
		}
		return isViewportUnit(unit);
	}

	sl_bool SAppDimensionBaseValue::isSpecialUnit(int unit)
	{
		return unit == FILL || unit == MATCH_PARENT || unit == WRAP || unit == WEIGHT;
	}

	sl_bool SAppDimensionBaseValue::isAbsoluteUnit(int unit)
	{
		return unit == PX || unit == INCH || unit == M || unit == CM || unit == MM || unit == PT || unit == DP;
	}

	sl_bool SAppDimensionBaseValue::isGlobalUnit(int unit)
	{
		return unit == PX || unit == SW || unit == SH || unit == SMIN || unit == SMAX || unit == INCH || unit == M || unit == CM || unit == MM || unit == PT || unit == DP || unit == SBAR || unit == SAFE_L || unit == SAFE_T || unit == SAFE_R || unit == SAFE_B || unit == SAFE_W || unit == SAFE_H;
	}

	sl_bool SAppDimensionBaseValue::isViewportUnit(int unit)
	{
		return unit == VW || unit == VH || unit == VMIN || unit == VMAX || unit == SP;
	}


	String SAppBooleanValue::getAccessString() const
	{
		if (!flagDefined) {
			return "sl_false";
		}
		if (value) {
			return "sl_true";
		} else {
			return "sl_false";
		}
	}

	sl_bool SAppBooleanValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "true" || str == "on") {
			value = sl_true;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "false" || str == "off") {
			value = sl_false;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppFloatValue::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		return String::format("%ff", value);
	}

	sl_bool SAppFloatValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		float f;
		if (ParseFloat(&f, str)) {
			value = f;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppDoubleValue::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		return String::format("%f", value);
	}

	sl_bool SAppDoubleValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		double f;
		if (ParseFloat(&f, str)) {
			value = f;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppInt32Value::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		return String::format("%d", value);
	}

	sl_bool SAppInt32Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		sl_int32 f;
		if (Calculator::calculate(str, &f)) {
			value = f;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	String SAppUint32Value::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		return String::format("%d", value);
	}

	sl_bool SAppUint32Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		sl_uint32 f;
		if (str.parseUint32(10, &f)) {
			value = f;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	String SAppInt64Value::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		return String::format("%d", value);
	}

	sl_bool SAppInt64Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		sl_int64 f;
		if (Calculator::calculate(str, &f)) {
			value = f;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	String SAppUint64Value::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		return String::format("%d", value);
	}

	sl_bool SAppUint64Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		sl_uint64 f;
		if (str.parseUint64(10, &f)) {
			value = f;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppChar8Value::getAccessString() const
	{
		if (!flagDefined) {
			return "0";
		}
		return String::format("'%c'", value);
	}

	sl_bool SAppChar8Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		if (str.getLength() == 1) {
			value = str.getAt(0);
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	namespace
	{
		template <class T>
		static sl_bool ParseFloatComponents(T* _out, sl_size nComponents, const String& str)
		{
			sl_size pos = 0;
			sl_size len = str.getLength();
			const sl_char8* data = str.getData();
			sl_size iComponent = 0;
			for (;;) {
				sl_reg iRet = ParseFloat(_out + iComponent, data, pos, len);
				if (iRet == SLIB_PARSE_ERROR) {
					return sl_false;
				}
				pos = iRet;
				for (; pos < len; pos++) {
					sl_char8 c = data[pos];
					if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
						break;
					}
				}
				iComponent++;
				if (iComponent >= nComponents) {
					break;
				}
				if (pos >= len) {
					return sl_false;
				}
				if (data[pos] != ',') {
					return sl_false;
				}
				pos++;
				for (; pos < len; pos++) {
					sl_char8 c = data[pos];
					if (!(SLIB_CHAR_IS_SPACE_TAB(c))) {
						break;
					}
				}
				if (pos >= len) {
					return sl_false;
				}
			}
			return pos == len;
		}
	}

	String SAppVector2Value::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::Vector2::zero()";
		}
		return String::format("slib::Vector2(%ff, %ff)", value.x, value.y);
	}

	sl_bool SAppVector2Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		float f[2];
		if (ParseFloatComponents(f, 2, str)) {
			value.x = f[0];
			value.y = f[1];
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	String SAppVector3Value::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::Vector3::zero()";
		}
		return String::format("slib::Vector3(%ff, %ff, %ff)", value.x, value.y, value.z);
	}

	sl_bool SAppVector3Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		float f[3];
		if (ParseFloatComponents(f, 3, str)) {
			value.x = f[0];
			value.y = f[1];
			value.z = f[2];
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	String SAppVector4Value::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::Vector4::zero()";
		}
		return String::format("slib::Vector4(%ff, %ff, %ff, %ff)", value.x, value.y, value.z, value.w);
	}

	sl_bool SAppVector4Value::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		float f[4];
		if (ParseFloatComponents(f, 3, str)) {
			value.x = f[0];
			value.y = f[1];
			value.z = f[2];
			value.w = f[3];
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppVisibilityValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::Visibility::Visible";
		}
		if (value == Visibility::Gone) {
			return "slib::Visibility::Gone";
		} else if (value == Visibility::Hidden) {
			return "slib::Visibility::Hidden";
		} else {
			return "slib::Visibility::Visible";
		}
	}

	sl_bool SAppVisibilityValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "visible") {
			value = Visibility::Visible;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "gone") {
			value = Visibility::Gone;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "hidden") {
			value = Visibility::Hidden;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppPenStyleValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::PenStyle::Solid";
		}
		if (value == PenStyle::Dot) {
			return "slib::PenStyle::Dot";
		} else if (value == PenStyle::Dash) {
			return "slib::PenStyle::Dash";
		} else if (value == PenStyle::DashDot) {
			return "slib::PenStyle::DashDot";
		} else if (value == PenStyle::DashDotDot) {
			return "slib::PenStyle::DashDotDot";
		} else {
			return "slib::PenStyle::Solid";
		}
	}

	sl_bool SAppPenStyleValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "solid") {
			value = PenStyle::Solid;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "dot") {
			value = PenStyle::Dot;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "dash") {
			value = PenStyle::Dash;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "dashdot") {
			value = PenStyle::DashDot;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "dashdotdot") {
			value = PenStyle::DashDotDot;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	sl_bool SAppScrollBarsValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "none") {
			flagDefined = sl_true;
			horizontalScrollBar = sl_false;
			verticalScrollBar = sl_false;
			return sl_true;
		} else if (str == "horizontal") {
			flagDefined = sl_true;
			horizontalScrollBar = sl_true;
			verticalScrollBar = sl_false;
			return sl_true;
		} else if (str == "vertical") {
			flagDefined = sl_true;
			horizontalScrollBar = sl_false;
			verticalScrollBar = sl_true;
			return sl_true;
		} else if (str == "both") {
			flagDefined = sl_true;
			horizontalScrollBar = sl_true;
			verticalScrollBar = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppNameValue::getAccessString() const
	{
		return value;
	}

	sl_bool SAppNameValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		if (!(SAppUtil::checkName(str.getData(), str.getLength()))) {
			return sl_false;
		}
		value = str;
		flagDefined = sl_true;
		return sl_true;
	}


	sl_bool SAppColorValue::parse(const String& _str, const Ref<XmlElement>& element)
	{
		String str = _str;
		if (str.isEmpty()) {
			return sl_true;
		}
		if (str.startsWith('@')) {
			str = str.substring(1).trim();
			if (str.startsWith("color/")) {
				str = str.substring(6).trim();
			}
			if (!(SAppUtil::checkName(str.getData(), str.getLength()))) {
				return sl_false;
			}
			resourceName = str;
			referingElement = element;
			flagDefined = sl_true;
			return sl_true;
		} else {
			str = str.trim();
			Color c;
			if (c.parse(str)) {
				color = c;
				flagDefined = sl_true;
				return sl_true;
			}
			return sl_false;
		}

	}


	String SAppTimeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::Time::zero()";
		}
		return String::format("slib::Time::fromInt(%s)", value.toInt());
	}

	sl_bool SAppTimeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		return value.parse(str);
	}


	sl_bool SAppDrawableValue::parse(const String& _str, SAppDocument* doc, const Ref<XmlElement>& element)
	{
		String str = _str;
		if (str.isEmpty()) {
			return sl_true;
		}

		if (str.startsWith('@')) {
			String res = str.substring(1).trim();
			if (res == "null") {
				flagDefined = sl_true;
				flagNull = sl_true;
				return sl_true;
			}
			if (res.startsWith("color/")) {
				if (SAppColorValue::parse(str, element)) {
					if (flagDefined) {
						flagNull = sl_false;
						flagColor = sl_true;
					}
					return sl_true;
				}
				return sl_false;
			}
			if (res.startsWith("drawable/")) {
				res = res.substring(9).trim();
			}
			str = Move(res);
		} else {
			if (SAppColorValue::parse(str, element)) {
				if (flagDefined) {
					flagNull = sl_false;
					flagColor = sl_true;
				}
				return sl_true;
			}
			return sl_false;
		}

		if (!doc) {
			if (!(SAppUtil::checkName(str.getData(), str.getLength()))) {
				return sl_false;
			}
			func = FUNC_NONE;
			flagWhole = sl_true;
			resourceName = str;
			referingElement = element;
			flagNull = sl_false;
			flagDefined = sl_true;
			return sl_true;
		}

		sl_char8* data = str.getData();
		sl_size len = str.getLength();
		sl_size pos = 0;

		while (pos < len) {
			sl_char8 c = data[pos];
			if (SLIB_CHAR_IS_C_NAME(c)) {
				pos++;
			} else {
				break;
			}
		}
		if (!(SAppUtil::checkName(data, pos))) {
			return sl_false;
		}

		resourceName = String(data, pos);
		referingElement = element;
		flagNull = sl_false;
		flagWhole = sl_true;
		func = FUNC_NONE;

		for (; pos < len; pos++) {
			sl_char8 c = data[pos];
			if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
				break;
			}
		}

		if (pos < len) {

			if (data[pos] == '[') {

				pos++;

				float f[4];
				for (sl_size i = 0; i < 4; i++) {
					for (; pos < len; pos++) {
						sl_char8 c = data[pos];
						if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
							break;
						}
					}
					if (pos >= len) {
						return sl_false;
					}
					sl_bool flagPlus = sl_false;
					if (data[pos] == '+') {
						if (i >= 2) {
							flagPlus = sl_true;
							pos++;
							for (; pos < len; pos++) {
								sl_char8 c = data[pos];
								if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
									break;
								}
							}
							if (pos >= len) {
								return sl_false;
							}
						} else {
							return sl_false;
						}
					}
					sl_reg iRet = ParseFloat(f + i, data, pos, len);
					if (iRet == SLIB_PARSE_ERROR) {
						return sl_false;
					}
					if (i >= 2) {
						if (!flagPlus) {
							f[i] -= f[i-2];
						}
						if (f[i] < 0) {
							return sl_false;
						}
					}
					pos = iRet;
					for (; pos < len; pos++) {
						sl_char8 c = data[pos];
						if (!(SLIB_CHAR_IS_SPACE_TAB(c))) {
							break;
						}
					}
					if (pos >= len) {
						return sl_false;
					}
					if (i == 3) {
						if (data[pos] != ']') {
							return sl_false;
						}
					} else {
						if (data[pos] != ',') {
							return sl_false;
						}
					}
					pos++;
				}

				flagWhole = sl_false;
				x = f[0];
				y = f[1];
				width = f[2];
				height = f[3];
			}

			for (; pos < len; pos++) {
				sl_char8 c = data[pos];
				if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
					break;
				}
			}

			if (data[pos] == ',') {
				pos++;
				for (; pos < len; pos++) {
					sl_char8 c = data[pos];
					if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
						break;
					}
				}
				if (pos >= len) {
					return sl_false;
				}

				sl_uint32 nFuncParams = 0;
				if (pos + 10 < len && Base::equalsMemory(data + pos, "nine-patch", 10)) {
					func = FUNC_NINEPATCH;
					nFuncParams = 8;
					pos += 10;
				} else if (pos + 22 < len && Base::equalsMemory(data + pos, "horizontal-three-patch", 22)) {
					func = FUNC_THREEPATCH_HORIZONTAL;
					nFuncParams = 4;
					pos += 22;
				} else if (pos + 20 < len && Base::equalsMemory(data + pos, "vertical-three-patch", 20)) {
					func = FUNC_THREEPATCH_VERTICAL;
					nFuncParams = 4;
					pos += 20;
				} else {
					return sl_false;
				}
				if (func == FUNC_NINEPATCH || func == FUNC_THREEPATCH_HORIZONTAL || func == FUNC_THREEPATCH_VERTICAL) {
					for (; pos < len; pos++) {
						sl_char8 c = data[pos];
						if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
							break;
						}
					}
					if (pos >= len) {
						return sl_false;
					}
					if (data[pos] != '(') {
						return sl_false;
					}
					pos++;

					SAppDimensionValue f[8];
					sl_size i = 0;
					for (; i < nFuncParams; i++) {
						for (; pos < len; pos++) {
							sl_char8 c = data[pos];
							if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
								break;
							}
						}
						if (pos >= len) {
							return sl_false;
						}
						sl_size posStart = pos;
						for (; pos < len; pos++) {
							sl_char8 c = data[pos];
							if (!(SLIB_CHAR_IS_ALNUM(c) || c == '.' || c == '%' || c == '\t' || c == ' ')) {
								break;
							}
						}
						String s = String(data + posStart, pos - posStart);
						if (s.isEmpty()) {
							return sl_false;
						}
						if (!(f[i].parse(s, doc))) {
							return sl_false;
						}
						if (!(f[i].flagDefined)) {
							return sl_false;
						}
						for (; pos < len; pos++) {
							sl_char8 c = data[pos];
							if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
								break;
							}
						}
						if (pos >= len) {
							return sl_false;
						}
						if (data[pos] != ',') {
							i++;
							break;
						}
						pos++;
					}
					if (pos >= len) {
						return sl_false;
					}
					if (data[pos] != ')') {
						return sl_false;
					}
					pos++;
					if (func == FUNC_NINEPATCH) {
						if (i != 4 && i != 8) {
							return sl_false;
						}
						if (i == 4) {
							f[4] = f[0];
							f[5] = f[1];
							f[6] = f[2];
							f[7] = f[3];
						}
						if (f[4].unit != SAppDimensionValue::PX || f[4].amount < 0) {
							return sl_false;
						}
						if (f[5].unit != SAppDimensionValue::PX || f[5].amount < 0) {
							return sl_false;
						}
						if (f[6].unit != SAppDimensionValue::PX || f[6].amount < 0) {
							return sl_false;
						}
						if (f[7].unit != SAppDimensionValue::PX || f[7].amount < 0) {
							return sl_false;
						}
						if (!(f[0].checkGlobal()) || f[0].amount < 0) {
							return sl_false;
						}
						if (!(f[1].checkGlobal()) || f[1].amount < 0) {
							return sl_false;
						}
						if (!(f[2].checkGlobal()) || f[2].amount < 0) {
							return sl_false;
						}
						if (!(f[3].checkGlobal()) || f[3].amount < 0) {
							return sl_false;
						}
						patchLeftWidthDst = f[0];
						patchRightWidthDst = f[1];
						patchTopHeightDst = f[2];
						patchBottomHeightDst = f[3];
						patchLeftWidth = f[4].amount;
						patchRightWidth = f[5].amount;
						patchTopHeight = f[6].amount;
						patchBottomHeight = f[7].amount;
					} else {
						if (i != 2 && i != 4) {
							return sl_false;
						}
						if (i == 2) {
							f[2] = f[0];
							f[3] = f[1];
						}
						if (f[2].unit != SAppDimensionValue::PX || f[2].amount < 0) {
							return sl_false;
						}
						if (f[3].unit != SAppDimensionValue::PX || f[3].amount < 0) {
							return sl_false;
						}
						if (!(f[0].checkGlobal()) || f[0].amount < 0) {
							return sl_false;
						}
						if (!(f[1].checkGlobal()) || f[1].amount < 0) {
							return sl_false;
						}
						if (func == FUNC_THREEPATCH_HORIZONTAL) {
							patchLeftWidthDst = f[0];
							patchRightWidthDst = f[1];
							patchLeftWidth = f[2].amount;
							patchRightWidth = f[3].amount;
						} else {
							patchTopHeightDst = f[0];
							patchBottomHeightDst = f[1];
							patchTopHeight = f[2].amount;
							patchBottomHeight = f[3].amount;
						}
					}
				}
			}

			for (; pos < len; pos++) {
				sl_char8 c = data[pos];
				if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
					return sl_false;
				}
			}

			if (pos < len) {
				return sl_false;
			}

		}

		flagDefined = sl_true;

		return sl_true;

	}

	sl_bool SAppDrawableValue::parseWhole(const String& str, const Ref<XmlElement>& element)
	{
		return parse(str, sl_null, element);
	}

	sl_bool SAppDrawableValue::isAbsoluteUnit()
	{
		if (func != FUNC_NONE) {
			if (patchLeftWidthDst.flagDefined && !(SAppDimensionValue::isAbsoluteUnit(patchLeftWidthDst.unit))) {
				return sl_false;
			}
			if (patchRightWidthDst.flagDefined && !(SAppDimensionValue::isAbsoluteUnit(patchRightWidthDst.unit))) {
				return sl_false;
			}
			if (patchTopHeightDst.flagDefined && !(SAppDimensionValue::isAbsoluteUnit(patchTopHeightDst.unit))) {
				return sl_false;
			}
			if (patchBottomHeightDst.flagDefined && !(SAppDimensionValue::isAbsoluteUnit(patchBottomHeightDst.unit))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool SAppDrawableValue::isGlobalUnit()
	{
		if (func != FUNC_NONE) {
			if (patchLeftWidthDst.flagDefined && !(SAppDimensionValue::isGlobalUnit(patchLeftWidthDst.unit))) {
				return sl_false;
			}
			if (patchRightWidthDst.flagDefined && !(SAppDimensionValue::isGlobalUnit(patchRightWidthDst.unit))) {
				return sl_false;
			}
			if (patchTopHeightDst.flagDefined && !(SAppDimensionValue::isGlobalUnit(patchTopHeightDst.unit))) {
				return sl_false;
			}
			if (patchBottomHeightDst.flagDefined && !(SAppDimensionValue::isGlobalUnit(patchBottomHeightDst.unit))) {
				return sl_false;
			}
		}
		return sl_true;
	}


	void SAppFontValue::inheritFrom(const SAppFontValue& parent)
	{
		if (!(family.flagDefined) && parent.family.flagDefined) {
			family = parent.family;
		}
		if (!(size.flagDefined) && parent.size.flagDefined) {
			size = parent.size;
		}
		if (!(bold.flagDefined) && parent.bold.flagDefined) {
			bold = parent.bold;
		}
		if (!(italic.flagDefined) && parent.italic.flagDefined) {
			italic = parent.italic;
		}
		if (!(underline.flagDefined) && parent.underline.flagDefined) {
			underline = parent.underline;
		}
	}

#define PRIV_DEFINE_PARSE_SUBITEM(SUBITEM, SUFFIX, ...) \
		String attr = String::concat(name, SUFFIX, suffix); \
		String str = item->getXmlAttribute(attr); \
		if (!(SUBITEM.parse(str, ##__VA_ARGS__))) { \
			doc->logError(xml, g_str_error_resource_layout_attribute_invalid, attr, str); \
			return sl_false; \
		} \
		if (SUBITEM.flagDefined) { \
			flagDefined = sl_true; \
		}

#define DEFINE_PARSE_SUBITEM(SUBITEM, SUFFIX, ...) \
	{ \
		PRIV_DEFINE_PARSE_SUBITEM(SUBITEM, SUFFIX, ##__VA_ARGS__) \
	}
#define DEFINE_PARSE_SUBITEM_DIMENSION(SUBITEM, SUFFIX, CHECK) \
	{ \
		PRIV_DEFINE_PARSE_SUBITEM(SUBITEM, SUFFIX, doc) \
		if (!(SUBITEM.CHECK(flagRoot))) { \
			doc->logError(xml, g_str_error_resource_layout_attribute_invalid, attr, str); \
			return sl_false; \
		} \
	}

	sl_bool SAppFontValue::parse(SAppLayoutXmlItem* item, const StringView& name, const StringView& suffix, SAppDocument* doc, sl_bool flagRoot)
	{
		const Ref<XmlElement>& xml = item->element;
		DEFINE_PARSE_SUBITEM(family, "Family", xml)
		DEFINE_PARSE_SUBITEM_DIMENSION(size, "Size", checkScalarSize)
		DEFINE_PARSE_SUBITEM(bold, "Bold")
		DEFINE_PARSE_SUBITEM(italic, "Italic")
		DEFINE_PARSE_SUBITEM(underline, "Underline")
		return sl_true;
	}


	void SAppBorderValue::inheritFrom(const SAppBorderValue& parent)
	{
		if (!(style.flagDefined) && parent.style.flagDefined) {
			style = parent.style;
		}
		if (!(width.flagDefined) && parent.width.flagDefined) {
			width = parent.width;
		}
		if (!(color.flagDefined) && parent.color.flagDefined) {
			color = parent.color;
		}
	}

	namespace
	{
		static void InheritBorderStateMap(SAppStateMap<SAppBorderValue>& map, const SAppBorderValue* base1, const SAppBorderValue* base2, ViewState stateDst)
		{
			SAppBorderValue* value = map.values.getItemPointer(stateDst);
			if (value) {
				if (base1) {
					value->inheritFrom(*base1);
				}
				if (base2) {
					value->inheritFrom(*base2);
				}
			} else {
				if (base1 && base2) {
					if ((!(base1->style.flagDefined) && base2->style.flagDefined) || (!(base1->width.flagDefined) && base2->width.flagDefined) || (!(base1->color.flagDefined) && base2->color.flagDefined)) {
						SAppBorderValue v = *base1;
						v.inheritFrom(*base2);
						map.values.put_NoLock(stateDst, v);
					}
				}
			}
		}
	}

	void SAppBorderValue::normalizeStateMap(SAppStateMap<SAppBorderValue>& map)
	{
		if (map.values.isNull()) {
			return;
		}
		SAppBorderValue* vNormal = map.values.getItemPointer(ViewState::Normal);
		SAppBorderValue* vHover = map.values.getItemPointer(ViewState::Hover);
		SAppBorderValue* vPressed = map.values.getItemPointer(ViewState::Pressed);
		if (vPressed && vHover) {
			vPressed->inheritFrom(*vHover);
		}

		SAppBorderValue* vSelectedHover = map.values.getItemPointer(ViewState::SelectedHover);
		if (vSelectedHover) {
			InheritBorderStateMap(map, vSelectedHover, vPressed, ViewState::SelectedPressed);
		}
		SAppBorderValue* vSelected = map.values.getItemPointer(ViewState::Selected);
		InheritBorderStateMap(map, vSelected, vNormal, ViewState::SelectedNormal);
		InheritBorderStateMap(map, vSelected, vHover, ViewState::SelectedHover);
		InheritBorderStateMap(map, vSelected, vPressed, ViewState::SelectedPressed);

		SAppBorderValue* vFocusedHover = map.values.getItemPointer(ViewState::FocusedHover);
		if (vFocusedHover) {
			InheritBorderStateMap(map, vFocusedHover, vPressed, ViewState::FocusedPressed);
		}
		SAppBorderValue* vFocused = map.values.getItemPointer(ViewState::Focused);
		InheritBorderStateMap(map, vFocused, vNormal, ViewState::FocusedNormal);
		InheritBorderStateMap(map, vFocused, vHover, ViewState::FocusedHover);
		InheritBorderStateMap(map, vFocused, vPressed, ViewState::FocusedPressed);

		{
			SAppBorderValue* v = map.values.getItemPointer(ViewState::All);
			if (v) {
				for (auto&& item : map.values) {
					if (item.key != ViewState::All) {
						item.value.inheritFrom(*v);
					}
				}
			}
		}
	}

	sl_bool SAppBorderValue::parse(SAppLayoutXmlItem* item, const StringView& name, const StringView& suffix, SAppDocument* doc, sl_bool flagRoot)
	{
		{
			String attr = String::concat(name, suffix);
			String str = item->getXmlAttribute(attr);
			if (str.isNotEmpty()) {
				do {
					if (str.startsWith('@')) {
						String v = str.substring(1).trim();
						if (v == "null") {
							flagDefined = sl_true;
							flagNull = sl_true;
							return sl_true;
						}
					} else {
						str = str.trim();
						if (str.equals_IgnoreCase(StringView::literal("false"))) {
							flagDefined = sl_true;
							flagNull = sl_true;
							return sl_true;
						} else if (str.equals_IgnoreCase(StringView::literal("true"))) {
							flagDefined = sl_true;
							break;
						}
					}
					doc->logError(item->element, g_str_error_resource_layout_attribute_invalid, attr, str);
					return sl_false;
				} while (0);
			}
		}
		const Ref<XmlElement>& xml = item->element;
		DEFINE_PARSE_SUBITEM(style, "Style")
		DEFINE_PARSE_SUBITEM_DIMENSION(width, "Width", checkScalarSize)
		DEFINE_PARSE_SUBITEM(color, "Color", xml)
		return sl_true;
	}


	sl_bool SAppMenuValue::parse(const String& _str, const Ref<XmlElement>& element)
	{
		String str = _str;
		if (str.isEmpty()) {
			return sl_true;
		}
		if (!(str.startsWith('@'))) {
			return sl_false;
		}
		str = str.substring(1).trim();
		if (str == "null") {
			flagDefined = sl_true;
			flagNull = sl_true;
			return sl_true;
		}
		if (str.startsWith("menu/")) {
			str = str.substring(5).trim();
		}

		if (!(SAppUtil::checkName(str.getData(), str.getLength()))) {
			return sl_false;
		}

		resourceName = str;
		referingElement = element;
		flagNull = sl_false;
		flagDefined = sl_true;
		return sl_true;
	}


	sl_bool SAppAlignLayoutValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		if (str == "false") {
			return sl_true;
		}
		if (str == "true") {
			flagAlignParent = sl_true;
			flagDefined = sl_true;
			return sl_true;
		}
		if (!(SAppUtil::checkName(str.getData(), str.getLength()))) {
			return sl_false;
		}
		referingView = str;
		flagAlignParent = sl_false;
		flagDefined = sl_true;
		return sl_true;
	}


	sl_bool SAppScrollingValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "horizontal") {
			flagDefined = sl_true;
			horizontal = sl_true;
			vertical = sl_false;
			return sl_true;
		} else if (str == "vertical") {
			flagDefined = sl_true;
			horizontal = sl_false;
			vertical = sl_true;
			return sl_true;
		} else if (str == "both") {
			flagDefined = sl_true;
			horizontal = sl_true;
			vertical = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppLayoutOrientationValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::LayoutOrientation::Vertical";
		}
		if (value == LayoutOrientation::Horizontal) {
			return "slib::LayoutOrientation::Horizontal";
		} else {
			return "slib::LayoutOrientation::Vertical";
		}
	}

	sl_bool SAppLayoutOrientationValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "horizontal") {
			value = LayoutOrientation::Horizontal;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "vertical") {
			value = LayoutOrientation::Vertical;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppAlignmentValue::getAccessString() const
	{
		if (flagDefined) {
			switch (value) {
				case Alignment::Left:
					return "slib::Alignment::Left";
				case Alignment::Center:
					return "slib::Alignment::Center";
				case Alignment::Right:
					return "slib::Alignment::Right";
				case Alignment::Top:
					return "slib::Alignment::Top";
				case Alignment::TopLeft:
					return "slib::Alignment::TopLeft";
				case Alignment::TopCenter:
					return "slib::Alignment::TopCenter";
				case Alignment::TopRight:
					return "slib::Alignment::TopRight";
				case Alignment::Middle:
					return "slib::Alignment::Middle";
				case Alignment::MiddleLeft:
					return "slib::Alignment::MiddleLeft";
				case Alignment::MiddleCenter:
					return "slib::Alignment::MiddleCenter";
				case Alignment::MiddleRight:
					return "slib::Alignment::MiddleRight";
				case Alignment::Bottom:
					return "slib::Alignment::Bottom";
				case Alignment::BottomLeft:
					return "slib::Alignment::BottomLeft";
				case Alignment::BottomCenter:
					return "slib::Alignment::BottomCenter";
				case Alignment::BottomRight:
					return "slib::Alignment::BottomRight";
			}
		}
		return "slib::Alignment::Default";
	}

	sl_bool SAppAlignmentValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		Alignment v = 0;
		ListLocker<String> items(str.split("|"));
		for (sl_size i = 0; i < items.count; i++) {
			String item = items[i].trim();
			if (item.isEmpty()) {
				return sl_false;
			}
			if (item == "top") {
				v |= Alignment::Top;
			} else if (item == "middle") {
				v |= Alignment::Middle;
			} else if (item == "bottom") {
				v |= Alignment::Bottom;
			} else if (item == "left") {
				v |= Alignment::Left;
			} else if (item == "center") {
				v |= Alignment::Center;
			} else if (item == "right") {
				v |= Alignment::Right;
			} else {
				return sl_false;
			}
		}
		flagDefined = sl_true;
		value = v;
		return sl_true;
	}


	String SAppScaleModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::ScaleMode::None";
		}
		switch (value) {
			case ScaleMode::Stretch:
				return "slib::ScaleMode::Stretch";
			case ScaleMode::Contain:
				return "slib::ScaleMode::Contain";
			case ScaleMode::Cover:
				return "slib::ScaleMode::Cover";
			default:
				break;
		}
		return "slib::ScaleMode::None";
	}

	sl_bool SAppScaleModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "stretch") {
			value = ScaleMode::Stretch;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "contain") {
			value = ScaleMode::Contain;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "cover") {
			value = ScaleMode::Cover;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "none") {
			value = ScaleMode::None;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppBoundShapeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::BoundShape::None";
		}
		switch (value) {
			case BoundShape::Rectangle:
				return "slib::BoundShape::Rectangle";
			case BoundShape::Ellipse:
				return "slib::BoundShape::Ellipse";
			case BoundShape::RoundRect:
				return "slib::BoundShape::RoundRect";
			default:
				break;
		}
		return "slib::BoundShape::Rectangle";
	}

	sl_bool SAppBoundShapeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "none") {
			value = BoundShape::None;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "rectangle") {
			value = BoundShape::Rectangle;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "ellipse") {
			value = BoundShape::Ellipse;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "round-rect") {
			value = BoundShape::RoundRect;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppRedrawModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::RedrawMode::Continuously";
		}
		switch (value) {
			case RedrawMode::WhenDirty:
				return "slib::RedrawMode::WhenDirty";
			default:
				break;
		}
		return "slib::RedrawMode::Continuously";
	}

	sl_bool SAppRedrawModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "continuously") {
			value = RedrawMode::Continuously;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "when-dirty") {
			value = RedrawMode::WhenDirty;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppMultiLineModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::MultiLineMode::Single";
		}
		switch (value) {
			case MultiLineMode::Multiple:
				return "slib::MultiLineMode::Multiple";
			case MultiLineMode::WordWrap:
				return "slib::MultiLineMode::WordWrap";
			case MultiLineMode::BreakWord:
				return "slib::MultiLineMode::BreakWord";
			case MultiLineMode::LatinWrap:
				return "slib::MultiLineMode::LatinWrap";
			default:
				break;
		}
		return "slib::MultiLineMode::Single";
	}

	sl_bool SAppMultiLineModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "single" || str == "false") {
			value = MultiLineMode::Single;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "multiple" || str == "true") {
			value = MultiLineMode::Multiple;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "word-wrap") {
			value = MultiLineMode::WordWrap;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "break-word") {
			value = MultiLineMode::BreakWord;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "latin-wrap") {
			value = MultiLineMode::LatinWrap;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppUIReturnKeyTypeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::UIReturnKeyType::Default";
		}
		switch (value) {
			case UIReturnKeyType::Default:
				return "slib::UIReturnKeyType::Default";
			case UIReturnKeyType::Return:
				return "slib::UIReturnKeyType::Return";
			case UIReturnKeyType::Done:
				return "slib::UIReturnKeyType::Done";
			case UIReturnKeyType::Search:
				return "slib::UIReturnKeyType::Search";
			case UIReturnKeyType::Next:
				return "slib::UIReturnKeyType::Next";
			case UIReturnKeyType::Continue:
				return "slib::UIReturnKeyType::Continue";
			case UIReturnKeyType::Go:
				return "slib::UIReturnKeyType::Go";
			case UIReturnKeyType::Send:
				return "slib::UIReturnKeyType::Send";
			case UIReturnKeyType::Join:
				return "slib::UIReturnKeyType::Join";
			case UIReturnKeyType::Route:
				return "slib::UIReturnKeyType::Route";
			case UIReturnKeyType::EmergencyCall:
				return "slib::UIReturnKeyType::EmergencyCall";
			case UIReturnKeyType::Google:
				return "slib::UIReturnKeyType::Google";
			case UIReturnKeyType::Yahoo:
				return "slib::UIReturnKeyType::Yahoo";
			default:
				break;
		}
		return "slib::UIReturnKeyType::Default";
	}

	sl_bool SAppUIReturnKeyTypeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "default") {
			value = UIReturnKeyType::Default;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "return") {
			value = UIReturnKeyType::Return;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "done") {
			value = UIReturnKeyType::Done;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "search") {
			value = UIReturnKeyType::Search;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "next") {
			value = UIReturnKeyType::Next;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "continue") {
			value = UIReturnKeyType::Continue;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "go") {
			value = UIReturnKeyType::Go;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "send") {
			value = UIReturnKeyType::Send;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "route") {
			value = UIReturnKeyType::Route;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "emergency") {
			value = UIReturnKeyType::EmergencyCall;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "google") {
			value = UIReturnKeyType::Google;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "yahoo") {
			value = UIReturnKeyType::Yahoo;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppUIKeyboardTypeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::UIKeyboardType::Default";
		}
		switch (value) {
			case UIKeyboardType::Default:
				return "slib::UIKeyboardType::Default";
			case UIKeyboardType::Numpad:
				return "slib::UIKeyboardType::Numpad";
			case UIKeyboardType::Phone:
				return "slib::UIKeyboardType::Phone";
			case UIKeyboardType::Email:
				return "slib::UIKeyboardType::Email";
			case UIKeyboardType::Decimal:
				return "slib::UIKeyboardType::Decimal";
			case UIKeyboardType::Alphabet:
				return "slib::UIKeyboardType::Alphabet";
			case UIKeyboardType::Url:
				return "slib::UIKeyboardType::Url";
			case UIKeyboardType::WebSearch:
				return "slib::UIKeyboardType::WebSearch";
			case UIKeyboardType::Twitter:
				return "slib::UIKeyboardType::Twitter";
			case UIKeyboardType::NumbersAndPunctuation:
				return "slib::UIKeyboardType::NumbersAndPunctuation";
			case UIKeyboardType::NamePhone:
				return "slib::UIKeyboardType::NamePhone";
			case UIKeyboardType::Ascii:
				return "slib::UIKeyboardType::Ascii";
			case UIKeyboardType::AsciiNumpad:
				return "slib::UIKeyboardType::AsciiNumpad";
			default:
				break;
		}
		return "slib::UIKeyboardType::Default";
	}

	sl_bool SAppUIKeyboardTypeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "default") {
			value = UIKeyboardType::Default;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "numpad") {
			value = UIKeyboardType::Numpad;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "phone") {
			value = UIKeyboardType::Phone;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "email") {
			value = UIKeyboardType::Email;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "decimal") {
			value = UIKeyboardType::Decimal;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "alphabet") {
			value = UIKeyboardType::Alphabet;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "url") {
			value = UIKeyboardType::Url;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "web-search") {
			value = UIKeyboardType::WebSearch;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "twitter") {
			value = UIKeyboardType::Twitter;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "numbers-and-punctuation") {
			value = UIKeyboardType::NumbersAndPunctuation;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "name-phone") {
			value = UIKeyboardType::NamePhone;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "ascii") {
			value = UIKeyboardType::Ascii;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "ascii-numpad") {
			value = UIKeyboardType::AsciiNumpad;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppUIAutoCapitalizationTypeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::UIAutoCapitalizationType::None";
		}
		switch (value) {
			case UIAutoCapitalizationType::None:
				return "slib::UIAutoCapitalizationType::None";
			case UIAutoCapitalizationType::Words:
				return "slib::UIAutoCapitalizationType::Words";
			case UIAutoCapitalizationType::Sentences:
				return "slib::UIAutoCapitalizationType::Sentences";
			case UIAutoCapitalizationType::AllCharacters:
				return "slib::UIAutoCapitalizationType::AllCharacters";
			default:
				break;
		}
		return "slib::UIAutoCapitalizationType::None";
	}

	sl_bool SAppUIAutoCapitalizationTypeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "none") {
			value = UIAutoCapitalizationType::None;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "words") {
			value = UIAutoCapitalizationType::Words;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "sentences") {
			value = UIAutoCapitalizationType::Sentences;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "all") {
			value = UIAutoCapitalizationType::AllCharacters;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppRotationModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::RotationMode::Rotate0";
		}
		switch (value) {
			case RotationMode::Rotate0:
				return "slib::RotationMode::Rotate0";
			case RotationMode::Rotate90:
				return "slib::RotationMode::Rotate90";
			case RotationMode::Rotate180:
				return "slib::RotationMode::Rotate180";
			case RotationMode::Rotate270:
				return "slib::RotationMode::Rotate270";
			default:
				break;
		}
		return "slib::RotationMode::Rotate0";
	}

	sl_bool SAppRotationModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "0") {
			value = RotationMode::Rotate0;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "90") {
			value = RotationMode::Rotate90;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "180") {
			value = RotationMode::Rotate180;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "270") {
			value = RotationMode::Rotate270;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppFlipModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::FlipMode::None";
		}
		switch (value) {
			case FlipMode::None:
				return "slib::FlipMode::None";
			case FlipMode::Horizontal:
				return "slib::FlipMode::Horizontal";
			case FlipMode::Vertical:
				return "slib::FlipMode::Vertical";
			case FlipMode::Both:
				return "slib::FlipMode::Both";
			default:
				break;
		}
		return "slib::FlipMode::None";
	}

	sl_bool SAppFlipModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "none") {
			value = FlipMode::None;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "horizontal") {
			value = FlipMode::Horizontal;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "vertical") {
			value = FlipMode::Vertical;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "both") {
			value = FlipMode::Both;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppEllipsizeModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::EllipsizeMode::None";
		}
		switch (value) {
			case EllipsizeMode::None:
				return "slib::EllipsizeMode::None";
			case EllipsizeMode::End:
				return "slib::EllipsizeMode::End";
			case EllipsizeMode::Start:
				return "slib::EllipsizeMode::Start";
			case EllipsizeMode::Middle:
				return "slib::EllipsizeMode::Middle";
			default:
				break;
		}
		return "slib::EllipsizeMode::None";
	}

	sl_bool SAppEllipsizeModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "none") {
			value = EllipsizeMode::None;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "end") {
			value = EllipsizeMode::End;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "start") {
			value = EllipsizeMode::Start;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "middle") {
			value = EllipsizeMode::Middle;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppCursorValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::Ref<slib::Cursor>::null()";
		}
		switch (type) {
			case ARROW:
				return "slib::Cursor::getArrow()";
			case IBEAM:
				return "slib::Cursor::getIBeam()";
			case CROSS:
				return "slib::Cursor::getCross()";
			case HAND:
				return "slib::Cursor::getHand()";
			case RESIZE_LEFT_RIGHT:
				return "slib::Cursor::getResizeLeftRight()";
			case RESIZE_UP_DOWN:
				return "slib::Cursor::getResizeUpDown()";
			default:
				break;
		}
		return "slib::Ref<slib::Cursor>::null()";
	}

	sl_bool SAppCursorValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "@null") {
			value.setNull();
			type = NONE;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "arrow") {
			value = Cursor::getArrow();
			type = ARROW;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "ibeam") {
			value = Cursor::getIBeam();
			type = IBEAM;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "cross") {
			value = Cursor::getCross();
			type = CROSS;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "hand" || str == "pointer") {
			value = Cursor::getHand();
			type = HAND;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "resize-x" || str == "resizex" || str == "resizeleftright") {
			value = Cursor::getResizeLeftRight();
			type = RESIZE_LEFT_RIGHT;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "resize-y" || str == "resizey" || str == "resizeupdown") {
			value = Cursor::getResizeUpDown();
			type = RESIZE_UP_DOWN;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppAntiAliasModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::AntiAliasMode::Inherit";
		}
		switch (value) {
			case AntiAliasMode::Inherit:
				return "slib::AntiAliasMode::Inherit";
			case AntiAliasMode::True:
				return "slib::AntiAliasMode::True";
			case AntiAliasMode::False:
				return "slib::AntiAliasMode::False";
			default:
				break;
		}
		return "slib::AntiAliasMode::Inherit";
	}

	sl_bool SAppAntiAliasModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "inherit") {
			value = AntiAliasMode::Inherit;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "true") {
			value = AntiAliasMode::True;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "false") {
			value = AntiAliasMode::False;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}


	String SAppGridSelectionModeValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::GridView::SelectionMode::Cell";
		}
		switch (value) {
			case GridView::SelectionMode::Row:
				return "slib::GridView::SelectionMode::Row";
			case GridView::SelectionMode::Column:
				return "slib::GridView::SelectionMode::Column";
			case GridView::SelectionMode::Record:
				return "slib::GridView::SelectionMode::Record";
			default:
				break;
		}
		return "slib::GridView::SelectionMode::Cell";
	}

	sl_bool SAppGridSelectionModeValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		str = str.toLower();
		if (str == "cell") {
			value = GridView::SelectionMode::Cell;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "row") {
			value = GridView::SelectionMode::Row;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "column") {
			value = GridView::SelectionMode::Column;
			flagDefined = sl_true;
			return sl_true;
		} else if (str == "record") {
			value = GridView::SelectionMode::Record;
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	String SAppLatLonValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::Latlon()";
		}
		return String::format("slib::LatLon(%f, %f)", value.latitude, value.longitude);
	}

	sl_bool SAppLatLonValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		double f[2];
		if (ParseFloatComponents(f, 2, str)) {
			value.latitude = f[0];
			value.longitude = f[1];
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

	String SAppGeoLocationValue::getAccessString() const
	{
		if (!flagDefined) {
			return "slib::GeoLocation()";
		}
		return String::format("slib::GeoLocation(%f, %f, %f)", value.latitude, value.longitude, value.altitude);
	}

	sl_bool SAppGeoLocationValue::parse(const String& _str)
	{
		String str = _str.trim();
		if (str.isEmpty()) {
			return sl_true;
		}
		double f[3];
		if (ParseFloatComponents(f, 3, str)) {
			value.latitude = f[0];
			value.longitude = f[1];
			value.altitude = f[2];
			flagDefined = sl_true;
			return sl_true;
		}
		return sl_false;
	}

}
