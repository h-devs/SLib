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

#ifndef CHECKHEADER_SLIB_CORE_FUNCTION
#define CHECKHEADER_SLIB_CORE_FUNCTION

#include "list.h"
#include "ptr.h"
#include "tuple.h"

namespace slib
{

	template <class T>
	class Callable;

	template <class T>
	class Function;

	template <class T>
	using AtomicFunction = Atomic< Function<T> >;

	template <class T>
	class FunctionList;

	namespace priv
	{

		template <class RET_TYPE, class INVOKE_TYPE>
		class FunctionInvoker
		{
		public:
			template <class FUNC, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, ARGS&&... args)
			{
				return func(Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, ARGS&&... args)
			{
				return (obj->*func)(Forward<ARGS>(args)...);
			}

		};

		template <class RET_TYPE>
		class FunctionInvoker<RET_TYPE, void>
		{
		public:
			template <class FUNC, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, ARGS&&... args)
			{
				func(Forward<ARGS>(args)...);
				return RET_TYPE();
			}

			template <class CLASS, class FUNC, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, ARGS&&... args)
			{
				(obj->*func)(Forward<ARGS>(args)...);
				return RET_TYPE();
			}

		};

		template <class INVOKE_TYPE>
		class FunctionInvoker<void, INVOKE_TYPE>
		{
		public:
			template <class FUNC, class... ARGS>
			static void invoke(FUNC&& func, ARGS&&... args)
			{
				func(Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, ARGS&&... args)
			{
				(obj->*func)(Forward<ARGS>(args)...);
			}

		};

		template <>
		class FunctionInvoker<void, void>
		{
		public:
			template <class FUNC, class... ARGS>
			static void invoke(FUNC&& func, ARGS&&... args)
			{
				func(Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, ARGS&&... args)
			{
				(obj->*func)(Forward<ARGS>(args)...);
			}

		};

		template <class RET_TYPE>
		class BindInvoker
		{
		public:
			template <class FUNC, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<>& t, ARGS&&... args)
			{
				return func(Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<>& t, ARGS&&... args)
			{
				return (obj->*func)(Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1>& t, ARGS&&... args)
			{
				return func(t.m1, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3, T4>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, t.m4, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, t.m4, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, t.m4, t.m5, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class... ARGS>
			static RET_TYPE invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& t, ARGS&&... args)
			{
				return func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, t.m10, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class... ARGS>
			static RET_TYPE invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& t, ARGS&&... args)
			{
				return (obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, t.m10, Forward<ARGS>(args)...);
			}

		};

		template<>
		class BindInvoker<void>
		{
		public:
			template <class FUNC, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<>& t, ARGS&&... args)
			{
				func(Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<>& t, ARGS&&... args)
			{
				(obj->*func)(Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1>& t, ARGS&&... args)
			{
				func(t.m1, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3, T4>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, t.m4, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, t.m4, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, t.m4, t.m5, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, Forward<ARGS>(args)...);
			}

			template <class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class... ARGS>
			static void invoke(FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& t, ARGS&&... args)
			{
				func(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, t.m10, Forward<ARGS>(args)...);
			}

			template <class CLASS, class FUNC, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class... ARGS>
			static void invokeMember(CLASS* obj, FUNC&& func, const Tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& t, ARGS&&... args)
			{
				(obj->*func)(t.m1, t.m2, t.m3, t.m4, t.m5, t.m6, t.m7, t.m8, t.m9, t.m10, Forward<ARGS>(args)...);
			}

		};

		template <class FUNC, class RET_TYPE, class... ARGS>
		class CallableFromFunction : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			FUNC func;

		public:
			template <class OTHER_FUNC>
			CallableFromFunction(OTHER_FUNC&& _func) noexcept: func(Forward<OTHER_FUNC>(_func)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				return FunctionInvoker<RET_TYPE, decltype(func(Forward<ARGS>(args)...))>::invoke(func, Forward<ARGS>(args)...);
			}
		};

		// prevent infinite encapsulation
		template <class RET_TYPE, class... ARGS>
		class CallableFromFunction<Function<RET_TYPE(ARGS...)>, RET_TYPE, ARGS...> : public Callable<RET_TYPE(ARGS...)> {};
		template <class RET_TYPE, class... ARGS>
		class CallableFromFunction<Atomic< Function<RET_TYPE(ARGS...)> >, RET_TYPE, ARGS...> : public Callable<RET_TYPE(ARGS...)> {};

		template <class BIND_TUPLE, class FUNC, class RET_TYPE, class... ARGS>
		class BindFromFunction : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			FUNC func;
			BIND_TUPLE binds;

		public:
			template <class OTHER_FUNC, class OTHER_BIND_TUPLE>
			BindFromFunction(OTHER_FUNC&& _func, OTHER_BIND_TUPLE&& _binds) noexcept: func(Forward<OTHER_FUNC>(_func)), binds(Forward<OTHER_BIND_TUPLE>(_binds)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				return BindInvoker<RET_TYPE>::invoke(func, binds, Forward<ARGS>(args)...);
			}
		};

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableFromMember : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			CLASS* object;
			FUNC func;

		public:
			CallableFromMember(CLASS* _object, const FUNC& _func) noexcept: object(_object), func(_func) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				return FunctionInvoker<RET_TYPE, decltype((object->*func)(Forward<ARGS>(args)...))>::invokeMember(object, func, Forward<ARGS>(args)...);
			}
		};

		template <class BIND_TUPLE, class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class BindFromMember : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			CLASS* object;
			FUNC func;
			BIND_TUPLE binds;

		public:
			template <class OTHER_BIND_TUPLE>
			BindFromMember(CLASS* _object, const FUNC& _func, OTHER_BIND_TUPLE&& _binds) noexcept: object(_object), func(_func), binds(Forward<OTHER_BIND_TUPLE>(_binds)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				return BindInvoker<RET_TYPE>::invokeMember(object, func, binds, Forward<ARGS>(args)...);
			}
		};

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableFromRef : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			Ref<CLASS> object;
			FUNC func;

		public:
			template <class T>
			CallableFromRef(T&& _object, const FUNC& _func) noexcept: object(Forward<T>(_object)), func(_func) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				return FunctionInvoker<RET_TYPE, decltype((object.ptr->*func)(Forward<ARGS>(args)...))>::invokeMember(object.ptr, func, Forward<ARGS>(args)...);
			}
		};

		template <class BIND_TUPLE, class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class BindFromRef : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			Ref<CLASS> object;
			FUNC func;
			BIND_TUPLE binds;

		public:
			template <class T, class OTHER_BIND_TUPLE>
			BindFromRef(T&& _object, const FUNC& _func, OTHER_BIND_TUPLE&& _binds) noexcept: object(Forward<T>(_object)), func(_func), binds(Forward<OTHER_BIND_TUPLE>(_binds)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				return BindInvoker<RET_TYPE>::invokeMember(object.ptr, func, binds, Forward<ARGS>(args)...);
			}
		};

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableFromWeakRef : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			WeakRef<CLASS> object;
			FUNC func;

		public:
			template <class T>
			CallableFromWeakRef(T&& _object, const FUNC& _func) noexcept: object(Forward<T>(_object)), func(_func) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				Ref<CLASS> o(object);
				if (o.isNotNull()) {
					return FunctionInvoker<RET_TYPE, decltype((o.ptr->*func)(Forward<ARGS>(args)...))>::invokeMember(o.ptr, func, Forward<ARGS>(args)...);
				} else {
					return RET_TYPE();
				}
			}
		};

		template <class BIND_TUPLE, class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class BindFromWeakRef : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			WeakRef<CLASS> object;
			FUNC func;
			BIND_TUPLE binds;

		public:
			template <class T, class OTHER_BIND_TUPLE>
			BindFromWeakRef(T&& _object, const FUNC& _func, OTHER_BIND_TUPLE&& _binds) noexcept: object(Forward<T>(_object)), func(_func), binds(Forward<OTHER_BIND_TUPLE>(_binds)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				Ref<CLASS> o(object);
				if (o.isNotNull()) {
					return BindInvoker<RET_TYPE>::invokeMember(o.ptr, func, binds, Forward<ARGS>(args)...);
				} else {
					return RET_TYPE();
				}
			}
		};

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableFromPtr : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			Ptr<CLASS> object;
			FUNC func;

		public:
			template <class T>
			CallableFromPtr(T&& _object, const FUNC& _func) noexcept: object(Forward<T>(_object)), func(_func) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				Ptr<CLASS> o(object.lock());
				if (o.isNotNull()) {
					return FunctionInvoker<RET_TYPE, decltype((o.ptr->*func)(Forward<ARGS>(args)...))>::invokeMember(o.ptr, func, Forward<ARGS>(args)...);
				} else {
					return RET_TYPE();
				}
			}
		};

		template <class BIND_TUPLE, class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class BindFromPtr : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			Ptr<CLASS> object;
			FUNC func;
			BIND_TUPLE binds;

		public:
			template <class T, class OTHER_BIND_TUPLE>
			BindFromPtr(T&& _object, const FUNC& _func, OTHER_BIND_TUPLE&& _binds) noexcept: object(Forward<T>(_object)), func(_func), binds(Forward<OTHER_BIND_TUPLE>(_binds)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				Ptr<CLASS> o(object.lock());
				if (o.isNotNull()) {
					return BindInvoker<RET_TYPE>::invokeMember(o.ptr, func, binds, Forward<ARGS>(args)...);
				} else {
					return RET_TYPE();
				}
			}
		};

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableWithRef : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			Ref<CLASS> object;
			FUNC func;

		public:
			template <class OTHER_CLASS, class OTHER_FUNC>
			CallableWithRef(OTHER_CLASS&& _object, OTHER_FUNC&& _func) noexcept: object(Forward<OTHER_CLASS>(_object)), func(Forward<OTHER_FUNC>(_func)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				return FunctionInvoker<RET_TYPE, decltype(func(Forward<ARGS>(args)...))>::invoke(func, Forward<ARGS>(args)...);
			}
		};

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableWithWeakRef : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			WeakRef<CLASS> object;
			FUNC func;

		public:
			template <class OTHER_CLASS, class OTHER_FUNC>
			CallableWithWeakRef(OTHER_CLASS&& _object, OTHER_FUNC&& _func) noexcept: object(Forward<OTHER_CLASS>(_object)), func(Forward<OTHER_FUNC>(_func)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				Ref<CLASS> o(object);
				if (o.isNotNull()) {
					return FunctionInvoker<RET_TYPE, decltype(func(Forward<ARGS>(args)...))>::invoke(func, Forward<ARGS>(args)...);
				} else {
					return RET_TYPE();
				}
			}
		};

		template <class CLASS, class FUNC, class RET_TYPE, class... ARGS>
		class CallableWithPtr : public Callable<RET_TYPE(ARGS...)>
		{
		protected:
			Ptr<CLASS> object;
			FUNC func;

		public:
			template <class OTHER_CLASS, class OTHER_FUNC>
			CallableWithPtr(OTHER_CLASS&& _object, OTHER_FUNC&& _func) noexcept: object(Forward<OTHER_CLASS>(_object)), func(Forward<OTHER_FUNC>(_func)) {}

		public:
			RET_TYPE invoke(ARGS... args) override
			{
				Ptr<CLASS> o(object.lock());
				if (o.isNotNull()) {
					return FunctionInvoker<RET_TYPE, decltype(func(Forward<ARGS>(args)...))>::invoke(func, Forward<ARGS>(args)...);
				} else {
					return RET_TYPE();
				}
			}
		};

		template <class T>
		struct FunctionTypeHelper;

		template <class T>
		struct FunctionTypeHelper<T*>
		{
			typedef typename RemoveConst<T>::Type Type;
		};

		template <class T>
		struct FunctionTypeHelper< Ref<T> >
		{
			typedef T Type;
		};

		template <class T>
		struct FunctionTypeHelper< Atomic< Ref<T> > >
		{
			typedef T Type;
		};

		template <class T>
		struct FunctionTypeHelper< WeakRef<T> >
		{
			typedef T Type;
		};

		template <class T>
		struct FunctionTypeHelper< Atomic< WeakRef<T> > >
		{
			typedef T Type;
		};

		template <class T>
		struct FunctionTypeHelper< Ptr<T> >
		{
			typedef T Type;
		};

		template <class T>
		struct FunctionTypeHelper< Atomic< Ptr<T> > >
		{
			typedef T Type;
		};

#define PRIV_SLIB_FUNCTION_GET_CLASS(OBJECT) slib::priv::FunctionTypeHelper<typename slib::RemoveConstReference<decltype(OBJECT)>::Type>::Type

	}


	class CallableBase : public CRef
	{
	public:
		SLIB_DECLARE_OBJECT

	public:
		CallableBase();

		~CallableBase();

	};

	template <class RET_TYPE, class... ARGS>
	class SLIB_EXPORT Callable<RET_TYPE(ARGS...)> : public CallableBase
	{
	public:
		virtual RET_TYPE invoke(ARGS... params) = 0;

	};

	template <class RET_TYPE, class... ARGS>
	class SLIB_EXPORT Function<RET_TYPE(ARGS...)>
	{
	public:
		typedef RET_TYPE ReturnType;
		Ref< Callable<RET_TYPE(ARGS...)> > ref;
		SLIB_REF_WRAPPER(Function, Callable<RET_TYPE(ARGS...)>)
		SLIB_REF_WRAPPER_ALL_LRVALUES(Function, Callable<RET_TYPE(ARGS...)>)

	public:
		template <class FUNC>
		Function(FUNC&& func) noexcept: ref(new priv::CallableFromFunction<typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(Forward<FUNC>(func))) {}

	public:
		template <class FUNC>
		Function& operator=(FUNC&& func) noexcept
		{
			ref = new priv::CallableFromFunction<typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(Forward<FUNC>(func));
			return *this;
		}

		RET_TYPE operator()(ARGS... args) const
		{
			Callable<RET_TYPE(ARGS...)>* object = ref.ptr;
			if (object) {
				return object->invoke(args...);
			} else {
				return RET_TYPE();
			}
		}

		Function& operator+=(const Function& function) noexcept
		{
			if (function.isNotNull()) {
				*this = *this + function;
			}
			return *this;
		}

		Function operator+(const Function& function) noexcept
		{
			if (function.isNull()) {
				return *this;
			}
			Callable<RET_TYPE(ARGS...)>* object = ref.ptr;
			if (object) {
				if (object->isInstanceOf(FunctionList<RET_TYPE(ARGS...)>::ObjectType())) {
					List<Function> list(((FunctionList<RET_TYPE(ARGS...)>*)object)->list.duplicate_NoLock());
					list.add_NoLock(function);
					return (Callable<RET_TYPE(ARGS...)>*)(new FunctionList<RET_TYPE(ARGS...)>(list));
				} else {
					return (Callable<RET_TYPE(ARGS...)>*)(new FunctionList<RET_TYPE(ARGS...)>(List<Function>::createFromElements(*this, function)));
				}
			} else {
				return function;
			}
		}

		Function& operator-=(const Function& function) noexcept
		{
			if (function.isNotNull()) {
				*this = *this - function;
			}
			return *this;
		}

		Function operator-(const Function& function) noexcept
		{
			if (function.isNull()) {
				return *this;
			}
			if (ref == function.ref) {
				return sl_null;
			}
			Callable<RET_TYPE(ARGS...)>* object = ref.ptr;
			if (object) {
				if (object->isInstanceOf(FunctionList<RET_TYPE(ARGS...)>::ObjectType())) {
					List<Function>& list = ((FunctionList<RET_TYPE(ARGS...)>*)object)->list;
					sl_size count = list.getCount();
					if (count > 0) {
						Function* data = list.getData();
						if (count == 1) {
							if (*data == function) {
								return sl_null;
							} else {
								return *data;
							}
						} else if (count == 2) {
							if (*data == function) {
								return data[1];
							} else if (data[1] == function) {
								return *data;
							}
						} else {
							sl_reg index = list.indexOf_NoLock(function);
							if (index >= 0) {
								List<Function> newList(data, index);
								if ((sl_size)index + 1 < count) {
									newList.addElements_NoLock(data + index + 1, count - index - 1);
								}
								return (Callable<RET_TYPE(ARGS...)>*)(new FunctionList<RET_TYPE(ARGS...)>(newList));
							}
						}
					} else {
						return sl_null;
					}
				}
			}
			return *this;
		}

	public:
		template <class FUNC>
		static Function create(FUNC&& func) noexcept
		{
			return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromFunction<typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(Forward<FUNC>(func)));
		}

		template <class CLASS, class FUNC>
		static Function fromMember(CLASS* object, const FUNC& func) noexcept
		{
			if (object) {
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromMember<CLASS, FUNC, RET_TYPE, ARGS...>(object, func));
			}
			return sl_null;
		}

		template <class OBJECT, class FUNC>
		static Function fromRef(OBJECT&& _object, const FUNC& func) noexcept
		{
			typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) CLASS;
			Ref<CLASS> object(Forward<OBJECT>(_object));
			if (object.isNotNull()) {
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromRef<CLASS, FUNC, RET_TYPE, ARGS...>(Move(object), func));
			}
			return sl_null;
		}

		template <class OBJECT, class FUNC>
		static Function fromWeakRef(OBJECT&& _object, const FUNC& func) noexcept
		{
			typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) CLASS;
			WeakRef<CLASS> object(Forward<OBJECT>(_object));
			if (object.isNotNull()) {
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromWeakRef<CLASS, FUNC, RET_TYPE, ARGS...>(Move(object), func));
			}
			return sl_null;
		}

		template <class OBJECT, class FUNC>
		static Function fromPtr(OBJECT&& _object, const FUNC& func) noexcept
		{
			typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) CLASS;
			Ptr<CLASS> object(Forward<OBJECT>(_object));
			if (object.isNotNull()) {
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromPtr<CLASS, FUNC, RET_TYPE, ARGS...>(Move(object), func));
			}
			return sl_null;
		}

		template <class FUNC>
		static Function bind(FUNC&& func) noexcept
		{
			return create(Forward<FUNC>(func));
		}

		template <class FUNC, class... BINDS>
		static Function bind(FUNC&& func, BINDS&&... binds) noexcept
		{
			typedef Tuple<typename RemoveConstReference<BINDS>::Type...> TUPLE;
			return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::BindFromFunction<TUPLE, typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(Forward<FUNC>(func), TUPLE(Forward<BINDS>(binds)...)));
		}

		template <class CLASS, class FUNC>
		static Function bindMember(CLASS* object, const FUNC& func) noexcept
		{
			return fromMember(object, func);
		}

		template <class CLASS, class FUNC, class... BINDS>
		static Function bindMember(CLASS* object, const FUNC& func, BINDS&&... binds) noexcept
		{
			if (object) {
				typedef Tuple<typename RemoveConstReference<BINDS>::Type...> TUPLE;
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::BindFromMember<TUPLE, CLASS, FUNC, RET_TYPE, ARGS...>(object, func, TUPLE(Forward<BINDS>(binds)...)));
			}
			return sl_null;
		}

		template <class OBJECT, class FUNC>
		static Function bindRef(OBJECT&& object, const FUNC& func) noexcept
		{
			return fromRef(Forward<OBJECT>(object), func);
		}

		template <class OBJECT, class FUNC, class... BINDS>
		static Function bindRef(OBJECT&& _object, const FUNC& func, BINDS&&... binds) noexcept
		{
			typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) CLASS;
			Ref<CLASS> object(Forward<OBJECT>(_object));
			if (object.isNotNull()) {
				typedef Tuple<typename RemoveConstReference<BINDS>::Type...> TUPLE;
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::BindFromRef<TUPLE, CLASS, FUNC, RET_TYPE, ARGS...>(Move(object), func, TUPLE(Forward<BINDS>(binds)...)));
			}
			return sl_null;
		}

		template <class OBJECT, class FUNC>
		static Function bindWeakRef(OBJECT&& object, const FUNC& func) noexcept
		{
			return fromWeakRef(Forward<OBJECT>(object), func);
		}

		template <class OBJECT, class FUNC, class... BINDS>
		static Function bindWeakRef(OBJECT&& _object, const FUNC& func, BINDS&&... binds) noexcept
		{
			typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) CLASS;
			WeakRef<CLASS> object(Forward<OBJECT>(_object));
			if (object.isNotNull()) {
				typedef Tuple<typename RemoveConstReference<BINDS>::Type...> TUPLE;
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::BindFromWeakRef<TUPLE, CLASS, FUNC, RET_TYPE, ARGS...>(Move(object), func, TUPLE(Forward<BINDS>(binds)...)));
			}
			return sl_null;
		}

		template <class OBJECT, class FUNC>
		static Function bindPtr(OBJECT&& object, const FUNC& func) noexcept
		{
			return fromPtr(Forward<OBJECT>(object), func);
		}

		template <class OBJECT, class FUNC, class... BINDS>
		static Function bindPtr(OBJECT&& _object, const FUNC& func, BINDS&&... binds) noexcept
		{
			typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) CLASS;
			Ptr<CLASS> object(Forward<OBJECT>(_object));
			if (object.isNotNull()) {
				typedef Tuple<typename RemoveConstReference<BINDS>::Type...> TUPLE;
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::BindFromPtr<TUPLE, CLASS, FUNC, RET_TYPE, ARGS...>(Move(object), func, TUPLE(Forward<BINDS>(binds)...)));
			}
			return sl_null;
		}

		template <class CLASS, class FUNC>
		static Function with(const Ref<CLASS>& object, FUNC&& func) noexcept
		{
			if (object.isNotNull()) {
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableWithRef<CLASS, typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(object, Forward<FUNC>(func)));
			}
			return sl_null;
		}

		template <class CLASS, class FUNC>
		static Function with(const WeakRef<CLASS>& object, FUNC&& func) noexcept
		{
			if (object.isNotNull()) {
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableWithWeakRef<CLASS, typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(object, Forward<FUNC>(func)));
			}
			return sl_null;
		}

		template <class CLASS, class FUNC>
		static Function with(const Ptr<CLASS>& object, FUNC&& func) noexcept
		{
			if (object.isNotNull()) {
				return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableWithPtr<CLASS, typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(object, Forward<FUNC>(func)));
			}
			return sl_null;
		}

		static Function fromList(const List< Function<RET_TYPE(ARGS...)> >& list) noexcept
		{
			return static_cast<Callable<RET_TYPE(ARGS...)>*>(new FunctionList<RET_TYPE(ARGS...)>(list));
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class TYPE, Function, Function<TYPE>)

	public:
		sl_bool isList() const noexcept
		{
			return IsInstanceOf< FunctionList<RET_TYPE(ARGS...)> >(ref.get());
		}

		const List<Function>& getList() const noexcept
		{
			FunctionList<RET_TYPE(ARGS...)>* obj = CastInstance< FunctionList<RET_TYPE(ARGS...)> >(ref.get());
			if (obj) {
				return obj->list;
			} else {
				return List<Function>::null();
			}
		}

		// return `function`
		Function add(const Function& function) noexcept
		{
			if (function.isNull()) {
				return function;
			}
			Callable<RET_TYPE(ARGS...)>* object = ref.ptr;
			if (object) {
				if (object->isInstanceOf(FunctionList<RET_TYPE(ARGS...)>::ObjectType())) {
					((FunctionList<RET_TYPE(ARGS...)>*)object)->list.add_NoLock(function);
				} else {
					ref = new FunctionList<RET_TYPE(ARGS...)>(List<Function>::createFromElements(*this, function));
				}
			} else {
				ref = function.ref;
			}
			return function;
		}

		// return `function`
		Function addIfNotExist(const Function& function) noexcept
		{
			if (function.isNull()) {
				return function;
			}
			Callable<RET_TYPE(ARGS...)>* object = ref.ptr;
			if (object) {
				if (object->isInstanceOf(FunctionList<RET_TYPE(ARGS...)>::ObjectType())) {
					((FunctionList<RET_TYPE(ARGS...)>*)object)->list.addIfNotExist_NoLock(function);
				} else {
					if (ref != function.ref) {
						ref = new FunctionList<RET_TYPE(ARGS...)>(List<Function>::createFromElements(*this, function));
					}
				}
			} else {
				ref = function.ref;
			}
			return function;
		}

		void remove(const Function& function, sl_bool flagRemoveAllMatches = sl_false) noexcept
		{
			if (function.isNull()) {
				return;
			}
			if (ref == function.ref) {
				ref.setNull();
				return;
			}
			Callable<RET_TYPE(ARGS...)>* object = ref.ptr;
			if (object) {
				if (object->isInstanceOf(FunctionList<RET_TYPE(ARGS...)>::ObjectType())) {
					List<Function>& list = ((FunctionList<RET_TYPE(ARGS...)>*)object)->list;
					sl_size count = list.getCount();
					Function* data = list.getData();
					if (count == 0) {
						ref.setNull();
					} else if (count == 1) {
						if (*data == function) {
							ref.setNull();
						}
					} else if (count == 2) {
						if (*data == function) {
							if (flagRemoveAllMatches) {
								if (data[1] == function) {
									ref.setNull();
								} else {
									ref = data[1].ref;
								}
							} else {
								ref = data[1].ref;
							}
						} else {
							if (data[1] == function) {
								ref = data->ref;
							}
						}
					} else {
						if (flagRemoveAllMatches) {
							list.removeValues_NoLock(function);
						} else {
							list.remove_NoLock(function);
						}
					}
				}
			}
		}

		sl_bool contains(const Function& function) const noexcept
		{
			if (function.isNull()) {
				return sl_false;
			}
			Callable<RET_TYPE(ARGS...)>* object = ref.ptr;
			if (object) {
				if (object->isInstanceOf(FunctionList<RET_TYPE(ARGS...)>::ObjectType())) {
					return ((FunctionList<RET_TYPE(ARGS...)>*)object)->list.contains_NoLock(function);
				}
			}
			return sl_false;
		}

	};

	template <class RET_TYPE, class... ARGS>
	class SLIB_EXPORT Atomic< Function<RET_TYPE(ARGS...)> >
	{
	public:
		AtomicRef< Callable<RET_TYPE(ARGS...)> > ref;
		SLIB_ATOMIC_REF_WRAPPER(Callable<RET_TYPE(ARGS...)>)
		SLIB_ATOMIC_REF_WRAPPER_ALL_LRVALUES(Callable<RET_TYPE(ARGS...)>)

	public:
		template <class FUNC>
		Atomic(FUNC&& func) noexcept: ref(new priv::CallableFromFunction<typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(Forward<FUNC>(func))) {}

	public:
		template <class FUNC>
		Atomic& operator=(FUNC&& func) noexcept
		{
			ref = new priv::CallableFromFunction<typename RemoveConstReference<FUNC>::Type, RET_TYPE, ARGS...>(Forward<FUNC>(func));
			return *this;
		}

		RET_TYPE operator()(ARGS... args) const
		{
			Ref< Callable<RET_TYPE(ARGS...)> > object(ref);
			if (object.isNotNull()) {
				return object->invoke(args...);
			} else {
				return RET_TYPE();
			}
		}

	public:
		SLIB_DEFINE_CAST_REF_FUNCTIONS(class TYPE, Atomic, AtomicFunction<TYPE>)

	public:
		// return `function`
		Function<RET_TYPE(ARGS...)> add(const Function<RET_TYPE(ARGS...)>& function) noexcept
		{
			SpinLocker lock(SpinLockPoolForFunction::get(this));
			Function<RET_TYPE(ARGS...)> _this(*this);
			*this = _this + function;
			return function;
		}

		// return `function`
		Function<RET_TYPE(ARGS...)> addIfNotExist(const Function<RET_TYPE(ARGS...)>& function) noexcept
		{
			SpinLocker lock(SpinLockPoolForFunction::get(this));
			Function<RET_TYPE(ARGS...)> _this(*this);
			if (!(_this.contains(function))) {
				*this = _this + function;
			}
			return function;
		}

		void remove(const Function<RET_TYPE(ARGS...)>& function, sl_bool flagRemoveAllMatch = sl_false) noexcept
		{
			SpinLocker lock(SpinLockPoolForFunction::get(this));
			Function<RET_TYPE(ARGS...)> _this(*this);
			if (flagRemoveAllMatch) {
				while (_this.contains(function)) {
					_this -= function;
				}
				*this = _this;
			} else {
				*this = _this - function;
			}
		}

	};


	namespace priv
	{
		namespace function_list
		{
			sl_object_type GetObjectType();
		}
	}

	template <class RET_TYPE, class... ARGS>
	class SLIB_EXPORT FunctionList<RET_TYPE(ARGS...)> : public Callable<RET_TYPE(ARGS...)>
	{
	public:
		List< Function<RET_TYPE(ARGS...)> > list;

	public:
		static sl_object_type ObjectType() noexcept
		{
			return priv::function_list::GetObjectType();
		}

		static sl_bool isDerivedFrom(sl_object_type type) noexcept
		{
			if (type == priv::function_list::GetObjectType()) {
				return sl_true;
			}
			return CallableBase::isDerivedFrom(type);
		}

		sl_object_type getObjectType() const noexcept override
		{
			return priv::function_list::GetObjectType();
		}

		sl_bool isInstanceOf(sl_object_type type) const noexcept override
		{
			if (type == priv::function_list::GetObjectType()) {
				return sl_true;
			}
			return CallableBase::isDerivedFrom(type);
		}

	public:
		FunctionList() noexcept {}

		FunctionList(const List< Function<RET_TYPE(ARGS...)> >& _list) noexcept: list(_list) {}

	public:
		RET_TYPE invoke(ARGS... args) override
		{
			ListElements< Function<RET_TYPE(ARGS...)> > functions(list);
			for (sl_size i = 0; i < functions.count; i++) {
				if (i + 1 == functions.count) {
					return functions[i](args...);
				} else {
					functions[i](args...);
				}
			}
			return RET_TYPE();
		}

	};


	template <class OBJECT, class CLASS, class RET_TYPE, class... ARGS>
	SLIB_INLINE Function<RET_TYPE(ARGS...)> CreateMemberFunction(OBJECT* object, RET_TYPE (CLASS::*func)(ARGS...)) noexcept
	{
		if (object) {
			return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromMember<CLASS, RET_TYPE (CLASS::*)(ARGS...), RET_TYPE, ARGS...>(object, func));
		}
		return sl_null;
	}

	template <class OBJECT, class CLASS, class RET_TYPE, class... ARGS>
	SLIB_INLINE Function<RET_TYPE(ARGS...)> CreateRefFunction(OBJECT&& _object, RET_TYPE (CLASS::*func)(ARGS...)) noexcept
	{
		typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) REF_CLASS;
		Ref<REF_CLASS> object(Forward<OBJECT>(_object));
		if (object.isNotNull()) {
			return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromRef<REF_CLASS, RET_TYPE(CLASS::*)(ARGS...), RET_TYPE, ARGS...>(Move(object), func));
		}
		return sl_null;
	}

	template <class OBJECT, class CLASS, class RET_TYPE, class... ARGS>
	SLIB_INLINE Function<RET_TYPE(ARGS...)> CreateWeakRefFunction(OBJECT&& _object, RET_TYPE (CLASS::*func)(ARGS...)) noexcept
	{
		typedef typename PRIV_SLIB_FUNCTION_GET_CLASS(_object) REF_CLASS;
		WeakRef<REF_CLASS> object(Forward<OBJECT>(_object));
		if (object.isNotNull()) {
			return static_cast<Callable<RET_TYPE(ARGS...)>*>(new priv::CallableFromWeakRef<REF_CLASS, RET_TYPE (CLASS::*)(ARGS...), RET_TYPE, ARGS...>(Move(object), func));
		}
		return sl_null;
	}

}

#define SLIB_BIND_MEMBER(TYPE, OBJECT, CALLBACK, ...) slib::Function<TYPE>::bindMember(OBJECT, &PRIV_SLIB_FUNCTION_GET_CLASS(OBJECT)::CALLBACK, ##__VA_ARGS__)
#define SLIB_BIND_REF(TYPE, OBJECT, CALLBACK, ...) slib::Function<TYPE>::bindRef(OBJECT, &PRIV_SLIB_FUNCTION_GET_CLASS(OBJECT)::CALLBACK, ##__VA_ARGS__)
#define SLIB_BIND_WEAKREF(TYPE, OBJECT, CALLBACK, ...) slib::Function<TYPE>::bindWeakRef(OBJECT, &PRIV_SLIB_FUNCTION_GET_CLASS(OBJECT)::CALLBACK, ##__VA_ARGS__)

#define SLIB_FUNCTION_MEMBER(OBJECT, CALLBACK) slib::CreateMemberFunction(OBJECT, &PRIV_SLIB_FUNCTION_GET_CLASS(OBJECT)::CALLBACK)
#define SLIB_FUNCTION_REF(OBJECT, CALLBACK) slib::CreateRefFunction(OBJECT, &PRIV_SLIB_FUNCTION_GET_CLASS(OBJECT)::CALLBACK)
#define SLIB_FUNCTION_WEAKREF(OBJECT, CALLBACK) slib::CreateWeakRefFunction(OBJECT, &PRIV_SLIB_FUNCTION_GET_CLASS(OBJECT)::CALLBACK)

#define SLIB_PROPERTY_FUNCTION(TYPE, NAME) \
	protected: \
		slib::AtomicFunction<TYPE> m_function_##NAME; \
	public: \
		slib::Function<TYPE> get##NAME() const { return m_function_##NAME; } \
 		slib::Function<TYPE> set##NAME(const slib::Function<TYPE>& value) { m_function_##NAME = value; return value; } \
		slib::Function<TYPE> add##NAME(const slib::Function<TYPE>& value) { return m_function_##NAME.add(value); } \
		void remove##NAME(const slib::Function<TYPE>& value) { m_function_##NAME.remove(value); }

#define SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON2(CLASS, NAME, RET, ...) \
	public: \
		typedef slib::Function<RET(CLASS* sender, ##__VA_ARGS__)> On##NAME; \
		slib::Function<RET(CLASS* sender, ##__VA_ARGS__)> getOn##NAME() const; \
		On##NAME setOn##NAME(const slib::Function<RET(CLASS* sender, ##__VA_ARGS__)>& handler); \
		On##NAME addOn##NAME(const slib::Function<RET(CLASS* sender, ##__VA_ARGS__)>& handler); \
		void removeOn##NAME(const slib::Function<RET(CLASS* sender, ##__VA_ARGS__)>& handler); 

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
	CLASS::On##NAME CLASS::getOn##NAME() const { return m_eventHandler_on##NAME; } \
	CLASS::On##NAME CLASS::setOn##NAME(const On##NAME& handler) { m_eventHandler_on##NAME = handler; return handler; } \
	CLASS::On##NAME CLASS::addOn##NAME(const On##NAME& handler) { return m_eventHandler_on##NAME.add(handler); } \
	void CLASS::removeOn##NAME(const On##NAME& handler) { m_eventHandler_on##NAME.remove(handler); }

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
