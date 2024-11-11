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

#ifndef CHECKHEADER_SLIB_CORE_EVENT_HANDLER
#define CHECKHEADER_SLIB_CORE_EVENT_HANDLER

#include "function.h"

namespace slib
{
	namespace priv
	{

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableFromOnEventMember : public Callable<RET_TYPE(CLASS*, ARGS...)>
		{
		protected:
			FUNC func;

		public:
			CallableFromOnEventMember(const FUNC& _func) noexcept: func(_func) {}

		public:
			RET_TYPE invoke(CLASS* object, ARGS... args) override
			{
				return FunctionInvoker<RET_TYPE, decltype((object->*func)(Forward<ARGS>(args)...))>::invokeMember(object, func, Forward<ARGS>(args)...);
			}
		};

		template <class CLASS, class RET_TYPE, class... ARGS>
		static Function<RET_TYPE(CLASS*, ARGS...)> CreateMemberEventHandler(RET_TYPE (CLASS::*func)(ARGS...))
		{
			return static_cast<Callable<RET_TYPE(CLASS*, ARGS...)>*>(new CallableFromOnEventMember<CLASS, RET_TYPE (CLASS::*)(ARGS...), RET_TYPE, ARGS...>(func));
		}
	}
}

#define SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON2(CLASS, NAME, RET, ...) \
	public: \
		typedef slib::Function<RET(CLASS* sender, ##__VA_ARGS__)> On##NAME; \
		slib::Function<RET(CLASS* sender, ##__VA_ARGS__)> getOn##NAME() const; \
		slib::Function<RET(CLASS* sender, ##__VA_ARGS__)> getOn##NAME(sl_bool flagDefaultHandler) const; \
		void setOn##NAME(const slib::Function<RET(CLASS* sender, ##__VA_ARGS__)>& handler);

#define SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON(CLASS, NAME, ...) SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON2(CLASS, NAME, void, ##__VA_ARGS__)

#define SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS2(CLASS, NAME, RET, ...) \
	SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON2(CLASS, NAME, RET, ##__VA_ARGS__); \
	public: \
		virtual RET on##NAME(__VA_ARGS__); \
		RET invoke##NAME(__VA_ARGS__);

#define SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(CLASS, NAME, ...) SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS2(CLASS, NAME, void, ##__VA_ARGS__)

#define SLIB_DECLARE_EVENT_HANDLER2(CLASS, NAME, RET, ...) \
	protected: \
		slib::AtomicFunction<RET(CLASS* sender, ##__VA_ARGS__)> m_eventHandler_on##NAME; \
	SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS2(CLASS, NAME, RET, ##__VA_ARGS__)

#define SLIB_DECLARE_EVENT_HANDLER(CLASS, NAME, ...) SLIB_DECLARE_EVENT_HANDLER2(CLASS, NAME, void, ##__VA_ARGS__)

#define SLIB_DEFINE_EVENT_HANDLER_WITHOUT_INVOKE(CLASS, NAME, DEFINE_ARGS, ...) \
	CLASS::On##NAME CLASS::getOn##NAME() const \
	{ \
		return m_eventHandler_on##NAME; \
	} \
	CLASS::On##NAME CLASS::getOn##NAME(sl_bool flagDefaultHandler) const \
	{ \
		if (m_eventHandler_on##NAME.isNotNull()) { \
			return m_eventHandler_on##NAME; \
		} else { \
			if (flagDefaultHandler) { \
				return priv::CreateMemberEventHandler(&CLASS::on##NAME); \
			} else { \
				return sl_null; \
			} \
		} \
	} \
	void CLASS::setOn##NAME(const On##NAME& handler) \
	{ \
		m_eventHandler_on##NAME = handler; \
	}

#define SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(CLASS, NAME, DEFINE_ARGS, ...) \
	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_INVOKE(CLASS, NAME, DEFINE_ARGS, ##__VA_ARGS__) \
	CLASS::On##NAME::ReturnType CLASS::invoke##NAME DEFINE_ARGS \
	{ \
		if (m_eventHandler_on##NAME.isNotNull()) { \
			m_eventHandler_on##NAME(this, ##__VA_ARGS__); \
		} else { \
			on##NAME(__VA_ARGS__); \
		} \
	}

#define SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON2(CLASS, NAME, DEFINE_ARGS, ...) \
	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_INVOKE(CLASS, NAME, DEFINE_ARGS, ##__VA_ARGS__) \
	CLASS::On##NAME::ReturnType CLASS::invoke##NAME DEFINE_ARGS \
	{ \
		if (m_eventHandler_on##NAME.isNotNull()) { \
			return m_eventHandler_on##NAME(this, ##__VA_ARGS__); \
		} else { \
			return on##NAME(__VA_ARGS__); \
		} \
	}

#define SLIB_DEFINE_EVENT_HANDLER(CLASS, NAME, DEFINE_ARGS, ...) \
	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(CLASS, NAME, DEFINE_ARGS, ##__VA_ARGS__) \
	void CLASS::on##NAME DEFINE_ARGS {}

#define SLIB_DEFINE_EVENT_HANDLER2(CLASS, NAME, DEFAULT, DEFINE_ARGS, ...) \
	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON2(CLASS, NAME, DEFINE_ARGS, ##__VA_ARGS__) \
	CLASS::On##NAME::ReturnType CLASS::on##NAME DEFINE_ARGS { return DEFAULT; }

#endif
