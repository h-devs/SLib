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

#ifndef CHECKHEADER_SLIB_UI_PRIV_VIEW_STATE_MAP
#define CHECKHEADER_SLIB_UI_PRIV_VIEW_STATE_MAP

#include "../view_state_map.h"

#include "../../core/hash_map.h"

#define PRIV_SLIB_VIEW_STATE_MAP_VALUES *(reinterpret_cast<AtomicHashMap<ViewState, VALUE>*>(&values))

namespace slib
{

	template <class VALUE>
	sl_bool ViewStateMap<VALUE>::isNone()
	{
		return !((sl_bool)defaultValue) && values.isNull();
	}

	template <class VALUE>
	sl_bool ViewStateMap<VALUE>::isNotNone()
	{
		return (sl_bool)defaultValue || values.isNotNull();
	}

	template <class VALUE>
	sl_bool ViewStateMap<VALUE>::isDefinedDefault()
	{
		return (sl_bool)defaultValue;
	}

	template <class VALUE>
	sl_bool ViewStateMap<VALUE>::isDefinedStates()
	{
		return values.isNotNull();
	}

	template <class VALUE>
	VALUE ViewStateMap<VALUE>::get(ViewState state)
	{
		if (state == ViewState::Default || state == ViewState::All) {
			return defaultValue;
		} else {
			return HashMap<ViewState, VALUE>(PRIV_SLIB_VIEW_STATE_MAP_VALUES).getValue(state);
		}
	}

	template <class VALUE>
	VALUE ViewStateMap<VALUE>::getDefault()
	{
		return defaultValue;
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::set(ViewState state, const VALUE& value)
	{
		if (state == ViewState::Default) {
			defaultValue = value;
			return;
		}
		if (state == ViewState::All) {
			defaultValue = value;
			(PRIV_SLIB_VIEW_STATE_MAP_VALUES).setNull();
			return;
		}
		if (!((sl_bool)value)) {
			_remove(state);
		}
		HashMap<ViewState, VALUE> map(PRIV_SLIB_VIEW_STATE_MAP_VALUES);
		if (map.isNotNull()) {
			map.put(state, value);
		} else {
			map.put(state, value);
			PRIV_SLIB_VIEW_STATE_MAP_VALUES = Move(map);
		}
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::set(const VALUE& value)
	{
		defaultValue = value;
		(PRIV_SLIB_VIEW_STATE_MAP_VALUES).setNull();
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::setDefault(const VALUE& value)
	{
		defaultValue = value;
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::remove(ViewState state)
	{
		if (state == ViewState::Default) {
			defaultValue = VALUE();
			return;
		}
		if (state == ViewState::All) {
			defaultValue = VALUE();
			(PRIV_SLIB_VIEW_STATE_MAP_VALUES).setNull();
			return;
		}
		_remove(state);
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::remove()
	{
		defaultValue = VALUE();
		(PRIV_SLIB_VIEW_STATE_MAP_VALUES).setNull();
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::removeDefault()
	{
		defaultValue = VALUE();
	}

	template <class VALUE>
	VALUE ViewStateMap<VALUE>::evaluate(ViewState state, sl_bool* outFlagReturnDefault)
	{
		HashMap<ViewState, VALUE> map(PRIV_SLIB_VIEW_STATE_MAP_VALUES);
		if (map.isNotNull()) {
			VALUE ret;
			sl_bool flagReturnDefault = sl_false;
			if (_evaluate(ret, map, state, flagReturnDefault)) {
				if (outFlagReturnDefault) {
					*outFlagReturnDefault = flagReturnDefault;
				}
				return ret;
			}
		}
		if (outFlagReturnDefault) {
			*outFlagReturnDefault = sl_true;
		}
		return defaultValue;
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::copyFrom(const ViewStateMap& other)
	{
		defaultValue = other.defaultValue;
		HashMap<ViewState, VALUE> map(*(reinterpret_cast<const AtomicHashMap<ViewState, VALUE>*>(&(other.values))));
		if (map.isNotNull()) {
			map = map.duplicate();
			values = Move(map.ref);
		}
	}

	template <class VALUE>
	void ViewStateMap<VALUE>::_remove(ViewState state)
	{
		HashMap<ViewState, VALUE> map(PRIV_SLIB_VIEW_STATE_MAP_VALUES);
		map.remove(state);
		if (map.isEmpty()) {
			(PRIV_SLIB_VIEW_STATE_MAP_VALUES).setNull();
		}
	}

	template <class VALUE>
	template <class MAP>
	sl_bool ViewStateMap<VALUE>::_evaluate(VALUE& ret, const MAP& map, ViewState state, ViewState def, sl_bool& flagReturnDefault)
	{
		if (state == def) {
			return sl_false;
		}
		ViewState _state = (ViewState)((int)state - (int)def);
		if (_state == ViewState::Pressed) {
			if (map.get((ViewState)((int)def + (int)(ViewState::Hover)), &ret)) {
				return sl_true;
			}
		}
		if (map.get(def, &ret)) {
			flagReturnDefault = sl_true;
			return sl_true;
		}
		return _evaluate(ret, map, _state, flagReturnDefault);
	}

	template <class VALUE>
	template <class MAP>
	sl_bool ViewStateMap<VALUE>::_evaluate(VALUE& ret, const MAP& map, ViewState state, sl_bool& flagReturnDefault)
	{
		if (map.get(state, &ret)) {
			return sl_true;
		}
		if (state == ViewState::Pressed) {
			if (map.get(ViewState::Hover, &ret)) {
				return sl_true;
			}
		}
		if (state < ViewState::Focused) {
			return sl_false;
		}
		if (state < ViewState::Selected) {
			return _evaluate(ret, map, state, ViewState::Focused, flagReturnDefault);
		} else {
			return _evaluate(ret, map, state, ViewState::Selected, flagReturnDefault);
		}
	}

}

#endif