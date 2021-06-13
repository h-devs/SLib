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

#ifdef SLIB_PLATFORM_USE_JNI

#include "slib/core/java.h"
#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"

#ifdef SLIB_PLATFORM_IS_ANDROID
#include "slib/core/android/log.h"
#define LOG(...) android::Log("JNI", ##__VA_ARGS__)
#define LOG_ERROR(...) android::LogError("JNI", ##__VA_ARGS__)
#else
#include "slib/core/log.h"
#define LOG(...) Log("JNI", ##__VA_ARGS__)
#define LOG_ERROR(...) LogError("JNI", ##__VA_ARGS__)
#endif

#define JNIVERSION JNI_VERSION_1_4

namespace slib
{

	namespace priv
	{
		namespace java
		{

			JavaVM* g_jvmShared = sl_null;
			SLIB_THREAD JNIEnv* g_envCurrent = sl_null;
			SLIB_THREAD sl_bool g_flagAutoClearException = sl_true;
			SLIB_THREAD sl_bool g_flagAutoPrintException = sl_true;

			class SharedContext
			{
			public:
				CHashMap<String, JniClass> classes;
				
				CList<JClass*> singleton_classes;
				CList<JMethod*> singleton_methods;
				CList<JStaticMethod*> singleton_static_methods;
				CList<JField*> singleton_fields;
				CList<JStaticField*> singleton_static_fields;
				CList<JNativeMethod*> native_methods;
			};

			SLIB_SAFE_STATIC_GETTER(SharedContext, getSharedContext)

			static void ProcessException(JNIEnv* env)
			{
				if (g_flagAutoClearException) {
					if (env->ExceptionCheck()) {
						if (g_flagAutoPrintException) {
							env->ExceptionDescribe();
						}
						env->ExceptionClear();
					}
				}
			}

			static sl_bool CheckException(JNIEnv* env, sl_bool flagPermitException = sl_false)
			{
				if (env->ExceptionCheck()) {
					if (flagPermitException) {
						env->ExceptionClear();						
					} else {
						if (g_flagAutoClearException) {
							if (g_flagAutoPrintException) {
								env->ExceptionDescribe();
							}
							env->ExceptionClear();
						}
					}
					return sl_true;
				}
				return sl_false;
			}

			JClass::JClass(const char* name, sl_bool flagOptional) noexcept
			{
				this->name = name;
				this->flagOptional = flagOptional;
				SharedContext* shared = getSharedContext();
				if (shared) {
					shared->singleton_classes.add(this);
				}
			}

			jclass JClass::get() const noexcept
			{
				return cls.get();
			}

			JMethod::JMethod(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional) noexcept
			{
				this->gcls = gcls;
				this->name = name;
				this->sig = sig;
				this->cls = sl_null;
				this->id = sl_null;
				this->flagOptional = flagOptional;
				SharedContext* shared = getSharedContext();
				if (shared) {
					shared->singleton_methods.add(this);
				}
			}

			jobject JMethod::newObject(jobject _null, ...) noexcept
			{
				va_list args;
				va_start(args, _null);
				jobject ret = sl_null;
				if (cls && id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						ret = env->NewObjectV(cls, id, args);
						ProcessException(env);
					}
				}
				va_end(args);
				return ret;
			}

			void JMethod::call(jobject _this, ...) noexcept
			{
				va_list args;
				va_start(args, _this);
				if (cls && id && _this) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						env->CallVoidMethodV(_this, id, args);
						ProcessException(env);
					}
				}
				va_end(args);
			}

			String JMethod::callString(jobject _this, ...) noexcept
			{
				va_list args;
				va_start(args, _this);
				String ret;
				if (cls && id && _this) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str((jstring)(env->CallObjectMethodV(_this, id, args)));
						if (!(CheckException(env))) {
							if (str.isNotNull()) {
								ret = Jni::getString(str);
							}
						}
					}
				}
				va_end(args);
				return ret;
			}

			JStaticMethod::JStaticMethod(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional) noexcept
			{
				this->gcls = gcls;
				this->name = name;
				this->sig = sig;
				this->cls = sl_null;
				this->id = sl_null;
				this->flagOptional = flagOptional;
				SharedContext* shared = getSharedContext();
				if (shared) {
					shared->singleton_static_methods.add(this);
				}
			}

			void JStaticMethod::call(jobject _null, ...) noexcept
			{
				va_list args;
				va_start(args, _null);
				if (cls && id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						env->CallStaticVoidMethodV(cls, id, args);
						ProcessException(env);
					}
				}
				va_end(args);
			}

			String JStaticMethod::callString(jobject _null, ...) noexcept
			{
				va_list args;
				va_start(args, _null);
				String ret;
				if (cls && id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str((jstring)(env->CallStaticObjectMethodV(cls, id, args)));
						if (!(CheckException(env))) {
							if (str.isNotNull()) {
								ret = Jni::getString(str);
							}
						}
					}
				}
				va_end(args);
				return ret;
			}

#define DEFINE_JMETHOD_MEMBERS(TYPE, NAME) \
			TYPE JMethod::call##NAME(jobject _this, ...) noexcept \
			{ \
				va_list args; \
				va_start(args, _this); \
				TYPE ret = 0; \
				if (cls && id && _this) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						ret = env->Call##NAME##MethodV(_this, id, args); \
						ProcessException(env); \
					} \
				} \
				va_end(args); \
				return ret; \
			} \
			TYPE JStaticMethod::call##NAME(jobject _null, ...) noexcept \
			{ \
				va_list args; \
				va_start(args, _null); \
				TYPE ret = 0; \
				if (cls && id) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						ret = env->CallStatic##NAME##MethodV(cls, id, args); \
						ProcessException(env); \
					} \
				} \
				va_end(args); \
				return ret; \
			}

			DEFINE_JMETHOD_MEMBERS(jobject, Object)
			DEFINE_JMETHOD_MEMBERS(jboolean, Boolean)
			DEFINE_JMETHOD_MEMBERS(jbyte, Byte)
			DEFINE_JMETHOD_MEMBERS(jchar, Char)
			DEFINE_JMETHOD_MEMBERS(jshort, Short)
			DEFINE_JMETHOD_MEMBERS(jint, Int)
			DEFINE_JMETHOD_MEMBERS(jlong, Long)
			DEFINE_JMETHOD_MEMBERS(jfloat, Float)
			DEFINE_JMETHOD_MEMBERS(jdouble, Double)

			JField::JField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional) noexcept
			{
				this->gcls = gcls;
				this->name = name;
				this->sig = sig;
				this->cls = sl_null;
				this->id = sl_null;
				this->flagOptional = flagOptional;
				SharedContext* shared = getSharedContext();
				if (shared) {
					shared->singleton_fields.add(this);
				}
			}

			String JField::getString(jobject _this) noexcept
			{
				if (cls && id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str((jstring)(env->GetObjectField(_this, id)));
						if (!(CheckException(env))) {
							if (str.isNotNull()) {
								return Jni::getString(str);
							}
						}
					}
				}
				return sl_null;
			}

			void JField::setString(jobject _this, const StringParam& value) noexcept
			{
				if (cls && id && _this) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str(Jni::getJniString(value));
						env->SetObjectField(_this, id, str);
						ProcessException(env);
					}
				}
			}

			JStaticField::JStaticField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional) noexcept
			{
				this->gcls = gcls;
				this->name = name;
				this->sig = sig;
				this->cls = sl_null;
				this->id = sl_null;
				this->flagOptional = flagOptional;
				SharedContext* shared = getSharedContext();
				if (shared) {
					shared->singleton_static_fields.add(this);
				}
			}

			String JStaticField::getString() noexcept
			{
				if (cls && id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str((jstring)(env->GetStaticObjectField(cls, id)));
						if (!(CheckException(env))) {
							if (str.isNotNull()) {
								return Jni::getString(str);
							}
						}
					}
				}
				return sl_null;
			}

			void JStaticField::setString(const StringParam& value) noexcept
			{
				if (cls && id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str(Jni::getJniString(value));
						env->SetObjectField(cls, id, str);
						ProcessException(env);
					}
				}
			}

#define DEFINE_JFIELD_MEMBERS(TYPE, NAME) \
			TYPE JField::get##NAME(jobject _this) noexcept \
			{ \
				if (cls && id && _this) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						TYPE ret = env->Get##NAME##Field(_this, id); \
						if (!(CheckException(env))) { \
							return ret; \
						} \
					} \
				} \
				return 0; \
			} \
			void JField::set##NAME(jobject _this, TYPE value) noexcept \
			{ \
				if (cls && id && _this) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						env->Set##NAME##Field(_this, id, value); \
						ProcessException(env); \
					} \
				} \
			} \
			TYPE JStaticField::get##NAME() noexcept \
			{ \
				if (cls && id) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						TYPE ret = env->GetStatic##NAME##Field(cls, id); \
						if (!(CheckException(env))) { \
							return ret; \
						} \
					} \
				} \
				return 0; \
			} \
			void JStaticField::set##NAME(TYPE value) noexcept \
			{ \
				if (cls && id) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						env->SetStatic##NAME##Field(cls, id, value); \
						ProcessException(env); \
					} \
				} \
			}

			DEFINE_JFIELD_MEMBERS(jobject, Object)
			DEFINE_JFIELD_MEMBERS(jboolean, Boolean)
			DEFINE_JFIELD_MEMBERS(jbyte, Byte)
			DEFINE_JFIELD_MEMBERS(jchar, Char)
			DEFINE_JFIELD_MEMBERS(jshort, Short)
			DEFINE_JFIELD_MEMBERS(jint, Int)
			DEFINE_JFIELD_MEMBERS(jlong, Long)
			DEFINE_JFIELD_MEMBERS(jfloat, Float)
			DEFINE_JFIELD_MEMBERS(jdouble, Double)

#define DEFINE_JFIELD_TYPE_MEMBERS(TYPE, NAME, SIG) \
			J##NAME##Field::J##NAME##Field(JClass* gcls, const char* name, sl_bool flagOptional) noexcept: JField(gcls, name, SIG, flagOptional) {} \
			TYPE J##NAME##Field::get(jobject _this) noexcept \
			{ \
				return get##NAME(_this); \
			} \
			void J##NAME##Field::set(jobject _this, TYPE value) noexcept \
			{ \
				set##NAME(_this, value); \
			} \

			DEFINE_JFIELD_TYPE_MEMBERS(jboolean, Boolean, "Z")
			DEFINE_JFIELD_TYPE_MEMBERS(sl_int8, Byte, "B")
			DEFINE_JFIELD_TYPE_MEMBERS(sl_uint16, Char, "C")
			DEFINE_JFIELD_TYPE_MEMBERS(sl_int16, Short, "S")
			DEFINE_JFIELD_TYPE_MEMBERS(sl_int32, Int, "I")
			DEFINE_JFIELD_TYPE_MEMBERS(sl_int64, Long, "J")
			DEFINE_JFIELD_TYPE_MEMBERS(float, Float, "F")
			DEFINE_JFIELD_TYPE_MEMBERS(double, Double, "D")
			DEFINE_JFIELD_TYPE_MEMBERS(String, String, "Ljava/lang/String;")

			JObjectField::JObjectField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional) noexcept: JField(gcls, name, sig, flagOptional)
			{				
			}

			jobject JObjectField::get(jobject _this) noexcept
			{
				return getObject(_this);
			}

			void JObjectField::set(jobject _this, jobject value) noexcept
			{
				setObject(_this, value);
			}

#define DEFINE_JSTATICFIELD_TYPE_MEMBERS(TYPE, NAME, SIG) \
			JStatic##NAME##Field::JStatic##NAME##Field(JClass* gcls, const char* name, sl_bool flagOptional) noexcept: JStaticField(gcls, name, SIG, flagOptional) {} \
			TYPE JStatic##NAME##Field::get() noexcept \
			{ \
				return get##NAME(); \
			} \
			void JStatic##NAME##Field::set(TYPE value) noexcept \
			{ \
				set##NAME(value); \
			} \

			DEFINE_JSTATICFIELD_TYPE_MEMBERS(jboolean, Boolean, "Z")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int8, Byte, "B")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_uint16, Char, "C")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int16, Short, "S")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int32, Int, "I")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int64, Long, "J")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(float, Float, "F")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(double, Double, "D")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(String, String, "Ljava/lang/String;")

			JStaticObjectField::JStaticObjectField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional) noexcept: JStaticField(gcls, name, sig, flagOptional)
			{
			}

			jobject JStaticObjectField::get() noexcept
			{
				return getObject();
			}

			void JStaticObjectField::set(jobject value) noexcept
			{
				setObject(value);
			}

			JNativeMethod::JNativeMethod(JClass* gcls, const char* name, const char* sig, const void* fn) noexcept
			{
				this->gcls = gcls;
				this->name = name;
				this->sig = sig;
				this->fn = fn;
				SharedContext* shared = getSharedContext();
				if (shared) {
					shared->native_methods.add(this);
				}
			}

		}
	}

	using namespace priv::java;

//#define JNI_LOG_INIT_LOAD

	void Jni::initialize(JavaVM* jvm) noexcept
	{
		static sl_bool flagInit = sl_false;
		
		if (! flagInit) {
			
			flagInit = sl_true;
			Jni::setSharedJVM(jvm);

			SharedContext* shared = getSharedContext();
			if (!shared) {
				return;
			}

			// singleton classes
			{
				ListLocker< JClass* > list(shared->singleton_classes);
				for (sl_size i = 0; i < list.count; i++) {
					JClass* obj = list[i];
#if defined(JNI_LOG_INIT_LOAD)
					LOG("LOADING JAVA CLASS: %s", obj->name);
#endif
					obj->cls = Jni::getClass(obj->name, obj->flagOptional);
					if (obj->cls.isNull()) {
						if (!(obj->flagOptional)) {
							LOG_ERROR("LOADING JAVA CLASS FAILED: %s", obj->name);
						}
					}
				}
			}


			// singleton fields
			{
				ListLocker< JField* > list(shared->singleton_fields);
				for (sl_size i = 0; i < list.count; i++) {
					JField* obj = list[i];
					JniClass cls = obj->gcls->cls;
					if (cls.isNotNull()) {
#if defined(JNI_LOG_INIT_LOAD)
						LOG("LOADING JAVA FIELD: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
#endif
						obj->cls = cls;
						obj->id = cls.getFieldID(obj->name, obj->sig, obj->flagOptional);
						if (!(obj->id)) {
							if (!(obj->flagOptional)) {
								LOG_ERROR("LOADING JAVA FIELD FAILED: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
							}
						}
					}
				}
			}
			// singleton static fields
			{
				ListLocker< JStaticField* > list(shared->singleton_static_fields);
				for (sl_size i = 0; i < list.count; i++) {
					JStaticField* obj = list[i];
					JniClass cls = obj->gcls->cls;
					if (cls.isNotNull()) {
#if defined(JNI_LOG_INIT_LOAD)
						LOG("LOADING JAVA STATIC FIELD: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
#endif
						obj->cls = cls;
						obj->id = cls.getStaticFieldID(obj->name, obj->sig, obj->flagOptional);
						if (!(obj->id)) {
							if (!(obj->flagOptional)) {
								LOG_ERROR("LOADING JAVA STATIC FIELD FAILED: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
							}
						}
					}
				}
			}
			// singleton methods
			{
				ListLocker< JMethod* > list(shared->singleton_methods);
				for (sl_size i = 0; i < list.count; i++) {
					JMethod* obj = list[i];
					JniClass cls = obj->gcls->cls;
					if (cls.isNotNull()) {
#if defined(JNI_LOG_INIT_LOAD)
						LOG("LOADING JAVA METHOD: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
#endif
						obj->cls = cls;
						obj->id = cls.getMethodID(obj->name, obj->sig, obj->flagOptional);
						if (!(obj->id)) {
							if (!(obj->flagOptional)) {
								LOG_ERROR("LOADING JAVA METHOD FAILED: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
							}
						}
					}
				}
			}
			// singleton static methods
			{
				ListLocker< JStaticMethod* > list(shared->singleton_static_methods);
				for (sl_size i = 0; i < list.count; i++) {
					JStaticMethod* obj = list[i];
					JniClass cls = obj->gcls->cls;
					if (cls.isNotNull()) {
#if defined(JNI_LOG_INIT_LOAD)
						LOG("LOADING JAVA STATIC METHOD: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
#endif
						obj->cls = cls;
						obj->id = cls.getStaticMethodID(obj->name, obj->sig, obj->flagOptional);
						if (!(obj->id)) {
							if (!(obj->flagOptional)) {
								LOG_ERROR("LOADING JAVA STATIC METHOD FAILED: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
							}
						}
					}
				}
			}
			// native methods
			{
				ListLocker< JNativeMethod* > list(shared->native_methods);
				for (sl_size i = 0; i < list.count; i++) {
					JNativeMethod* obj = list[i];
					JniClass cls = obj->gcls->cls;
					if (cls.isNotNull()) {
#if defined(JNI_LOG_INIT_LOAD)
						LOG("REGISTERING JAVA NATIVE: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
#endif
						if (!(cls.registerNative(obj->name, obj->sig, obj->fn))) {
							LOG_ERROR("REGISTERING JAVA NATIVE FAILED: %s::%s (%s)", obj->gcls->name, obj->name, obj->sig);
						}
					}
				}
			}
		}
	}

	void Jni::setSharedJVM(JavaVM *jvm) noexcept
	{
		if (g_jvmShared != jvm) {
			g_jvmShared = jvm;
		}
	}

	JavaVM* Jni::getSharedJVM() noexcept
	{
		return g_jvmShared;
	}

	JNIEnv* Jni::getCurrent() noexcept
	{
		JNIEnv *env = g_envCurrent;
		if (!env) {
			env = Jni::attachThread();
		}
		return env;
	}

	void Jni::setCurrent(JNIEnv* jni) noexcept
	{
		g_envCurrent = jni;
	}

	JNIEnv* Jni::attachThread(JavaVM* jvm) noexcept
	{
		if (!jvm) {
			jvm = Jni::getSharedJVM();
		}
		JNIEnv* env = sl_null;
		if (jvm) {
			if (jvm->GetEnv((void**)&env, JNIVERSION) != JNI_OK) {
#if defined(SLIB_PLATFORM_IS_ANDROID)
				jint res = jvm->AttachCurrentThread(&env, sl_null);
#else
				jint res = jvm->AttachCurrentThread((void**)&env, sl_null);
#endif
				if ((res < 0) || !env) {
					LOG_ERROR("Failed to attach thread");
				}
				Jni::setCurrent(env);
			}
		}
		if (env) {
			Jni::setCurrent(env);
		}
		return env;
	}

	void Jni::detachThread(JavaVM* jvm) noexcept
	{
		if (! jvm) {
			jvm = Jni::getSharedJVM();
		}
		if (jvm) {
			jvm->DetachCurrentThread();
		}
	}

	JniClass Jni::findClass(const StringParam& _className, sl_bool flagOptional) noexcept
	{
		StringCstr className(_className);
		JNIEnv *env = getCurrent();
		if (env) {
			JniLocal<jclass> cls = env->FindClass(className.getData());
			if (!(CheckException(env, flagOptional))) {
				if (cls.isNotNull()) {
					return JniClass::from(cls);
				}
			}
		}
		return sl_null;
	}

	JniClass Jni::getClass(const String& className, sl_bool flagOptional) noexcept
	{
		SharedContext* shared = getSharedContext();
		if (!shared) {
			return sl_null;
		}
		JniClass ret;
		if (shared->classes.get(className, &ret)) {
			return ret;
		}
		ret = Jni::findClass(className, flagOptional);
		if (ret.isNotNull()) {
			Jni::registerClass(className, ret);
			return ret;
		}
		return sl_null;
	}

	void Jni::registerClass(const String& className, jclass cls) noexcept
	{
		SharedContext* shared = getSharedContext();
		if (shared) {
			shared->classes.put(className, cls);
		}
	}

	void Jni::unregisterClass(const String& className) noexcept
	{
		SharedContext* shared = getSharedContext();
		if (shared) {
			shared->classes.remove(className);
		}
	}

	sl_bool Jni::isSameObject(jobject ref1, jobject ref2) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			return env->IsSameObject(ref1, ref2) != 0;
		}
		return sl_false;
	}

	jobjectRefType Jni::getRefType(jobject obj) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			return env->GetObjectRefType(obj);
		}
		return JNIInvalidRefType;
	}

	sl_bool Jni::isInvalidRef(jobject obj) noexcept
	{
		return Jni::getRefType(obj) == JNIInvalidRefType;
	}

	sl_bool Jni::isLocalRef(jobject obj) noexcept
	{
		return Jni::getRefType(obj) == JNILocalRefType;
	}

	jobject Jni::newLocalRef(jobject obj) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			return env->NewLocalRef(obj);
		}
		return 0;
	}

	void Jni::deleteLocalRef(jobject obj) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jobjectRefType type = env->GetObjectRefType(obj);
			if (type == JNILocalRefType) {
				env->DeleteLocalRef(obj);
			}
		}
	}

	sl_bool Jni::isGlobalRef(jobject obj) noexcept
	{
		return Jni::getRefType(obj) == JNIGlobalRefType;
	}

	jobject Jni::newGlobalRef(jobject obj) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			return env->NewGlobalRef(obj);
		}
		return 0;
	}

	void Jni::deleteGlobalRef(jobject obj) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jobjectRefType type = env->GetObjectRefType(obj);
			if (type == JNIGlobalRefType) {
				env->DeleteGlobalRef(obj);
			}
		}
	}

	sl_bool Jni::isWeakRef(jobject obj) noexcept
	{
		return Jni::getRefType(obj) == JNIWeakGlobalRefType;
	}

	jobject Jni::newWeakRef(jobject obj) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			return env->NewWeakGlobalRef(obj);
		}
		return 0;
	}

	void Jni::deleteWeakRef(jobject obj) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jobjectRefType type = env->GetObjectRefType(obj);
			if (type == JNIWeakGlobalRefType) {
				env->DeleteWeakGlobalRef(obj);
			}
		}
	}

	jstring Jni::getJniString(const StringParam& _str) noexcept
	{
		if (_str.isNotNull()) {
			StringData16 str(_str);
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jstring ret = env->NewString((jchar*)(str.getData()), (jsize)(str.getLength()));
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	jstring Jni::getJniString(const sl_char16* str, sl_size length) noexcept
	{
		if (str) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jstring ret = env->NewString((jchar*)(str), (jsize)(length));
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	String Jni::getString(jstring str) noexcept
	{
		if (str) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				sl_uint32 len = (sl_uint32)(env->GetStringLength(str));
				const jchar* sz = env->GetStringChars(str, sl_null);
				if (sz) {
					String ret = String::create((const char16_t*)sz, len);
					env->ReleaseStringChars(str, sz);
					return ret;
				}
			}
		}
		return sl_null;
	}

	sl_uint32 Jni::getArrayLength(jarray array) noexcept
	{
		if (array) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				return env->GetArrayLength(array);
			}
		}
		return 0;
	}

	jobjectArray Jni::newObjectArray(jclass clsElement, sl_uint32 length) noexcept
	{
		if (clsElement) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jobjectArray ret = env->NewObjectArray(length, clsElement, sl_null);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	jobject Jni::getObjectArrayElement(jobjectArray array, sl_uint32 index) noexcept
	{
		if (array) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jobject ret = env->GetObjectArrayElement(array, index);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	void Jni::setObjectArrayElement(jobjectArray array, sl_uint32 index, jobject value) noexcept
	{
		if (array) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->SetObjectArrayElement(array, index, value);
				ProcessException(env);
			}
		}
	}

	jobjectArray Jni::newStringArray(sl_uint32 length) noexcept
	{
		SLIB_STATIC_STRING(cls, "java/lang/String");
		return Jni::newObjectArray(Jni::getClass(cls), length);
	}

	String Jni::getStringArrayElement(jobjectArray array, sl_uint32 index) noexcept
	{
		JniLocal<jstring> v((jstring)(Jni::getObjectArrayElement(array, index)));
		if (v.isNotNull()) {
			return Jni::getString(v);
		}
		return sl_null;
	}

	void Jni::setStringArrayElement(jobjectArray array, sl_uint32 index, const StringParam& value) noexcept
	{
		JniLocal<jstring> v(Jni::getJniString(value));
		Jni::setObjectArrayElement(array, index, v);
	}

#define DEFINE_JNI_ARRAY(TYPE, NAME) \
	TYPE##Array Jni::new##NAME##Array(sl_uint32 length) noexcept \
	{ \
		JNIEnv* env = Jni::getCurrent(); \
		if (env) { \
			TYPE##Array ret = env->New##NAME##Array(length); \
			if (!(CheckException(env))) { \
				return ret; \
			} \
		} \
		return sl_null; \
	} \
	TYPE* Jni::get##NAME##ArrayElements(TYPE##Array array, jboolean* isCopy) noexcept \
	{ \
		if (array) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				return env->Get##NAME##ArrayElements(array, isCopy); \
			} \
		} \
		return sl_null; \
	} \
	void Jni::release##NAME##ArrayElements(TYPE##Array array, TYPE* buf, jint mode) noexcept \
	{ \
		if (array && buf) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				env->Release##NAME##ArrayElements(array, buf, mode); \
			} \
		} \
	} \
	void Jni::get##NAME##ArrayRegion(TYPE##Array array, sl_uint32 index, sl_uint32 len, TYPE* buf) noexcept \
	{ \
		if (array && buf) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				env->Get##NAME##ArrayRegion(array, index, len, buf); \
			} \
		} \
	} \
	void Jni::set##NAME##ArrayRegion(TYPE##Array array, sl_uint32 index, sl_uint32 len, TYPE* buf) noexcept \
	{ \
		if (array && buf) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				env->Set##NAME##ArrayRegion(array, index, len, buf); \
			} \
		} \
	} \

	DEFINE_JNI_ARRAY(jboolean, Boolean)
	DEFINE_JNI_ARRAY(jbyte, Byte)
	DEFINE_JNI_ARRAY(jchar, Char)
	DEFINE_JNI_ARRAY(jshort, Short)
	DEFINE_JNI_ARRAY(jint, Int)
	DEFINE_JNI_ARRAY(jlong, Long)
	DEFINE_JNI_ARRAY(jfloat, Float)
	DEFINE_JNI_ARRAY(jdouble, Double)

	jobject Jni::newDirectByteBuffer(void* address, sl_size capacity) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env && address && capacity) {
			jobject ret = env->NewDirectByteBuffer(address, capacity);
			if (!(CheckException(env))) {
				return ret;
			}
		}
		return sl_null;
	}

	void* Jni::getDirectBufferAddress(jobject buf) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env && buf) {
			return env->GetDirectBufferAddress(buf);
		}
		return sl_null;
	}

	sl_size Jni::getDirectBufferCapacity(jobject buf) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env && buf) {
			return (sl_size)(env->GetDirectBufferCapacity(buf));
		}
		return 0;
	}

	sl_bool Jni::checkException() noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			return env->ExceptionCheck() != 0;
		}
		return sl_false;
	}

	void Jni::clearException() noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			env->ExceptionClear();
		}
	}

	void Jni::printException() noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			env->ExceptionDescribe();
		}
	}
	
	sl_bool Jni::checkExceptionAndClear() noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			if (env->ExceptionCheck()) {
				env->ExceptionClear();
				return sl_true;
			}
		}
		return sl_false;
	}
	
	sl_bool Jni::checkExceptionAndPrintClear() noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			if (env->ExceptionCheck()) {
				env->ExceptionDescribe();
				env->ExceptionClear();
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Jni::isAutoClearException() noexcept
	{
		return g_flagAutoClearException;
	}
	
	void Jni::setAutoClearException(sl_bool flag) noexcept
	{
		g_flagAutoClearException = flag;
	}
	
	sl_bool Jni::isAutoPrintException() noexcept
	{
		return g_flagAutoPrintException;
	}
	
	void Jni::setAutoPrintException(sl_bool flag) noexcept
	{
		g_flagAutoPrintException = flag;
	}
	

	SLIB_DEFINE_ROOT_OBJECT(CJniGlobalBase)

	JniClass::JniClass(jclass cls) noexcept: ref(CJniGlobal<jclass>::from(cls))
	{
	}

	JniClass& JniClass::operator=(jclass cls) noexcept
	{
		ref = CJniGlobal<jclass>::from(cls);
		return *this;
	}

	JniClass JniClass::from(jclass cls) noexcept
	{
		return cls;
	}

	JniClass JniClass::getClassOfObject(jobject obj) noexcept
	{
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jclass> cls = env->GetObjectClass(obj);
				return JniClass::from(cls);
			}
		}
		return sl_null;
	}

	jclass JniClass::get() const noexcept
	{
		CJniGlobal<jclass>* o = ref.get();
		if (ref.isNotNull()) {
			return o->object;
		}
		return sl_null;
	}

	JniClass::operator jclass() const noexcept
	{
		return get();
	}

	sl_bool JniClass::isInstanceOf(jobject obj) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			return env->IsInstanceOf(obj, get()) != 0;
		}
		return sl_false;
	}

	jmethodID JniClass::getMethodID(const char* name, const char* sig, sl_bool flagOptional) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jmethodID ret = env->GetMethodID(get(), name, sig);
			if (!(CheckException(env, flagOptional))) {
				return ret;
			}
		}
		return sl_null;
	}

	jmethodID JniClass::getStaticMethodID(const char* name, const char* sig, sl_bool flagOptional) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jmethodID ret = env->GetStaticMethodID(get(), name, sig);
			if (!(CheckException(env, flagOptional))) {
				return ret;
			}
		}
		return sl_null;
	}

	jfieldID JniClass::getFieldID(const char* name, const char* sig, sl_bool flagOptional) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jfieldID ret = env->GetFieldID(get(), name, sig);
			if (!(CheckException(env, flagOptional))) {
				return ret;
			}
		}
		return sl_null;
	}

	jfieldID JniClass::getStaticFieldID(const char* name, const char* sig, sl_bool flagOptional) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jfieldID ret = env->GetStaticFieldID(get(), name, sig);
			if (!(CheckException(env, flagOptional))) {
				return ret;
			}
		}
		return sl_null;
	}

#define DEFINE_JNI_CALL_METHOD(TYPE, NAME) \
	TYPE JniClass::call##NAME##Method(jmethodID method, jobject _this, ...) const noexcept \
	{ \
		va_list args; \
		va_start(args, _this); \
		TYPE ret = 0; \
		if (method && _this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				ret = env->Call##NAME##MethodV(_this, method, args); \
				ProcessException(env); \
			} \
		} \
		va_end(args); \
		return ret; \
	} \
	TYPE JniClass::call##NAME##Method(const char* name, const char* sig, jobject _this, ...) const noexcept \
	{ \
		va_list args; \
		va_start(args, _this); \
		TYPE ret = 0; \
		if (_this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				jmethodID method = env->GetMethodID(get(), name, sig); \
				if (!(CheckException(env))) { \
					if (method) { \
						ret = env->Call##NAME##MethodV(_this, method, args); \
						ProcessException(env); \
					} else { \
						LOG_ERROR("Failed to get method id: %s (%s) ", name, sig); \
					} \
				} \
			} \
		} \
		va_end(args); \
		return ret; \
	} \
	TYPE JniClass::callStatic##NAME##Method(jmethodID method, ...) const noexcept \
	{ \
		va_list args; \
		va_start(args, method); \
		TYPE ret = 0; \
		if (method) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				ret = env->CallStatic##NAME##MethodV(get(), method, args); \
				ProcessException(env); \
			} \
		} \
		va_end(args); \
		return ret; \
	} \
	TYPE JniClass::callStatic##NAME##Method(const char* name, const char* sig, ...) const noexcept \
	{ \
		va_list args; \
		va_start(args, sig); \
		TYPE ret = 0; \
		JNIEnv* env = Jni::getCurrent(); \
		if (env) { \
			jclass cls = get(); \
			jmethodID method = env->GetStaticMethodID(cls, name, sig); \
			if (!(CheckException(env))) { \
				if (method) { \
					ret = env->CallStatic##NAME##MethodV(cls, method, args); \
					ProcessException(env); \
				} else { \
					LOG_ERROR("Failed to get static method id: %s (%s)", name, sig); \
				} \
			} \
		} \
		va_end(args); \
		return ret; \
	}
	
	DEFINE_JNI_CALL_METHOD(jobject, Object)
	DEFINE_JNI_CALL_METHOD(jboolean, Boolean)
	DEFINE_JNI_CALL_METHOD(jbyte, Byte)
	DEFINE_JNI_CALL_METHOD(jchar, Char)
	DEFINE_JNI_CALL_METHOD(jshort, Short)
	DEFINE_JNI_CALL_METHOD(jint, Int)
	DEFINE_JNI_CALL_METHOD(jlong, Long)
	DEFINE_JNI_CALL_METHOD(jfloat, Float)
	DEFINE_JNI_CALL_METHOD(jdouble, Double)

	jobject JniClass::newObject(jmethodID method, ...) const noexcept
	{
		va_list args;
		va_start(args, method);
		jobject ret = sl_null;
		if (method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				ret = env->NewObjectV(get(), method, args);
				ProcessException(env);
			}
		}
		va_end(args);
		return ret;
	}

	jobject JniClass::newObject(const char* sig, ...) const noexcept
	{
		va_list args;
		va_start(args, sig);
		jobject ret = sl_null;
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jclass cls = get();
			jmethodID method = env->GetMethodID(cls, "<init>", sig);
			if (!CheckException(env)) {
				if (method) {
					ret = env->NewObjectV(get(), method, args);
					ProcessException(env);
				} else {
					LOG_ERROR("Failed to get constructor id: <init> (%s)", sig);
				}
			}
		}
		va_end(args);
		return ret;
	}

	jobject JniClass::newObject() const noexcept
	{
		return newObject("()V");
	}

	void JniClass::callVoidMethod(jmethodID method, jobject _this, ...) const noexcept
	{
		va_list args;
		va_start(args, _this);
		if (method && _this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->CallVoidMethodV(_this, method, args);
				ProcessException(env);
			}
		}
		va_end(args);
	}

	void JniClass::callVoidMethod(const char* name, const char* sig, jobject _this, ...) const noexcept
	{
		va_list args;
		va_start(args, _this);
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID method = env->GetMethodID(get(), name, sig);
				if (!(CheckException(env))) {
					if (method) {
						env->CallVoidMethodV(_this, method, args);
						ProcessException(env);
					} else {
						LOG_ERROR("Failed to get method id: %s (%s)", name, sig);
					}
				}
			}
		}
		va_end(args);
	}

	void JniClass::callStaticVoidMethod(jmethodID method, ...) const noexcept
	{
		va_list args;
		va_start(args, method);
		if (method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->CallStaticVoidMethodV(get(), method, args);
				ProcessException(env);
			}
		}
		va_end(args);
	}

	void JniClass::callStaticVoidMethod(const char* name, const char* sig, ...) const noexcept
	{
		va_list args;
		va_start(args, sig);
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jclass cls = get();
			jmethodID method = env->GetStaticMethodID(cls, name, sig);
			if (!(CheckException(env))) {
				if (method) {
					env->CallStaticVoidMethodV(cls, method, args);
					ProcessException(env);
				} else {
					LOG_ERROR("Failed to get static method id: %s (%s)", name, sig);
				}
			}
		}
		va_end(args);
	}

	String JniClass::callStringMethod(jmethodID method, jobject _this, ...) const noexcept
	{
		va_list args;
		va_start(args, _this);
		String ret;
		if (method && _this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jstring> str((jstring)(env->CallObjectMethodV(_this, method, args)));
				if (!(CheckException(env))) {
					if (str.isNotNull()) {
						ret = Jni::getString(str);
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	String JniClass::callStringMethod(const char* name, const char* sig, jobject _this, ...) const noexcept
	{
		va_list args;
		va_start(args, _this);
		String ret;
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID method = env->GetMethodID(get(), name, sig);
				if (!(CheckException(env))) {
					if (method) {
						JniLocal<jstring> str((jstring)(env->CallObjectMethodV(_this, method, args)));
						if (!(CheckException(env))) {
							if (str.isNotNull()) {
								ret = Jni::getString(str);
							}
						}
					} else {
						LOG_ERROR("Failed to get method id: %s (%s)", name, sig);
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	String JniClass::callStaticStringMethod(jmethodID method, ...) const noexcept
	{
		va_list args;
		va_start(args, method);
		String ret;
		if (method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jstring> str((jstring)(env->CallStaticObjectMethodV(get(), method, args)));
				if (!(CheckException(env))) {
					if (str.isNotNull()) {
						ret = Jni::getString(str);
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	String JniClass::callStaticStringMethod(const char* name, const char* sig, ...) const noexcept
	{
		va_list args;
		va_start(args, sig);
		String ret;
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jclass cls = get();
			jmethodID method = env->GetStaticMethodID(cls, name, sig);
			if (!(CheckException(env))) {
				if (method) {
					JniLocal<jstring> str((jstring)(env->CallStaticObjectMethodV(cls, method, args)));
					if (!(CheckException(env))) {
						if (str.isNotNull()) {
							ret = Jni::getString(str);
						}
					}
				} else {
					LOG_ERROR("Failed to get static method id: %s (%s)", name, sig);
				}
			}
		}
		va_end(args);
		return ret;
	}

#define DEFINE_JNI_FIELD(TYPE, NAME, SIG) \
	TYPE JniClass::get##NAME##Field(jfieldID field, jobject _this) const noexcept \
	{ \
		if (field && _this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				TYPE ret = env->Get##NAME##Field(_this, field); \
				if (!(CheckException(env))) { \
					return ret; \
				} \
			} \
		} \
		return 0; \
	} \
	TYPE JniClass::get##NAME##Field(const char* name, jobject _this) const noexcept \
	{ \
		if (_this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				jfieldID field = env->GetFieldID(get(), name, SIG); \
				if (!(CheckException(env))) { \
					if (field) { \
						TYPE ret = env->Get##NAME##Field(_this, field); \
						if (!(CheckException(env))) { \
							return ret; \
						} \
					} else { \
						LOG_ERROR("Failed to get field id: %s (%s)", name, SIG); \
					} \
				} \
			} \
		} \
		return 0; \
	} \
	TYPE JniClass::getStatic##NAME##Field(jfieldID field) const noexcept \
	{ \
		if (field) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				TYPE ret = env->GetStatic##NAME##Field(get(), field); \
				if (!(CheckException(env))) { \
					return ret; \
				} \
			} \
		} \
		return 0; \
	} \
	TYPE JniClass::getStatic##NAME##Field(const char* name) const noexcept \
	{ \
		JNIEnv* env = Jni::getCurrent(); \
		if (env) { \
			jclass cls = get(); \
			jfieldID field = env->GetStaticFieldID(cls, name, SIG); \
			if (!(CheckException(env))) { \
				if (field) { \
					TYPE ret = env->GetStatic##NAME##Field(cls, field); \
					if (!(CheckException(env))) { \
						return ret; \
					} \
				}  else { \
					LOG_ERROR("Failed to get static field id: %s (%s)", name, SIG); \
				} \
			} \
		} \
		return 0; \
	} \
	void JniClass::set##NAME##Field(jfieldID field, jobject _this, TYPE value) const noexcept \
	{ \
		if (field && _this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				env->Set##NAME##Field(_this, field, value); \
				ProcessException(env); \
			} \
		} \
	} \
	void JniClass::set##NAME##Field(const char* name, jobject _this, TYPE value) const noexcept \
	{ \
		if (_this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				jfieldID field = env->GetFieldID(get(), name, SIG); \
				if (!(CheckException(env))) { \
					if (field) { \
						env->Set##NAME##Field(_this, field, value); \
						ProcessException(env); \
					} else { \
						LOG_ERROR("Failed to get field id: %s (%s)", name, SIG); \
					} \
				} \
			} \
		} \
	} \
	void JniClass::setStatic##NAME##Field(jfieldID field, TYPE value) const noexcept \
	{ \
		if (field) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				env->SetStatic##NAME##Field(get(), field, value); \
				ProcessException(env); \
			} \
		} \
	} \
	void JniClass::setStatic##NAME##Field(const char* name, TYPE value) const noexcept \
	{ \
		JNIEnv* env = Jni::getCurrent(); \
		if (env) { \
			jclass cls = get(); \
			jfieldID field = env->GetStaticFieldID(cls, name, SIG); \
			if (!(CheckException(env))) { \
				if (field) { \
					env->SetStatic##NAME##Field(cls, field, value); \
					ProcessException(env); \
				} else { \
					LOG_ERROR("Failed to get static field id: %s (%s)", name, SIG); \
				} \
			} \
		} \
	}

	DEFINE_JNI_FIELD(jboolean, Boolean, "Z")
	DEFINE_JNI_FIELD(jbyte, Byte, "B")
	DEFINE_JNI_FIELD(jchar, Char, "C")
	DEFINE_JNI_FIELD(jshort, Short, "S")
	DEFINE_JNI_FIELD(jint, Int, "I")
	DEFINE_JNI_FIELD(jlong, Long, "J")
	DEFINE_JNI_FIELD(jfloat, Float, "F")
	DEFINE_JNI_FIELD(jdouble, Double, "D")

	jobject JniClass::getObjectField(jfieldID field, jobject _this) const noexcept
	{
		if (field && _this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jobject ret = env->GetObjectField(_this, field);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return 0;
	}

	jobject JniClass::getObjectField(const char* name, const char* sig, jobject _this) const noexcept
	{
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jfieldID field = env->GetFieldID(get(), name, sig);
				if (!(CheckException(env))) {
					if (field) {
						jobject ret = env->GetObjectField(_this, field);
						if (!(CheckException(env))) {
							return ret;
						}
					} else {
						LOG_ERROR("Failed to get field id: %s (%s)", name, sig);
					}
				}
			}
		}
		return 0;
	}

	jobject JniClass::getStaticObjectField(jfieldID field) const noexcept
	{
		if (field) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jobject ret = env->GetStaticObjectField(get(), field);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return 0;
	}

	jobject JniClass::getStaticObjectField(const char* name, const char* sig) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jclass cls = get();
			jfieldID field = env->GetStaticFieldID(cls, name, sig);
			if (!(CheckException(env))) {
				if (field) {
					jobject ret = env->GetStaticObjectField(cls, field);
					if (!(CheckException(env))) {
						return ret;
					}
				}  else {
					LOG_ERROR("Failed to get static field id: %s (%s)", name, sig);
				}
			}
		}
		return 0;
	}

	void JniClass::setObjectField(jfieldID field, jobject _this, jobject value) const noexcept
	{
		if (field && _this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->SetObjectField(_this, field, value);
				ProcessException(env);
			}
		}
	}

	void JniClass::setObjectField(const char* name, const char* sig, jobject _this, jobject value) const noexcept
	{
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jfieldID field = env->GetFieldID(get(), name, sig);
				if (!(CheckException(env))) {
					if (field) {
						env->SetObjectField(_this, field, value);
						ProcessException(env);
					} else {
						LOG_ERROR("Failed to get field id: %s (%s)", name, sig);
					}
				}
			}
		}
	}

	void JniClass::setStaticObjectField(jfieldID field, jobject value) const noexcept
	{
		if (field) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->SetStaticObjectField(get(), field, value);
				ProcessException(env);
			}
		}
	}

	void JniClass::setStaticObjectField(const char* name, const char* sig, jobject value) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			jclass cls = get();
			jfieldID field = env->GetStaticFieldID(cls, name, sig);
			if (!(CheckException(env))) {
				if (field) {
					env->SetStaticObjectField(cls, field, value);
					ProcessException(env);
				} else {
					LOG_ERROR("Failed to get static field id: %s (%s)", name, sig);
				}
			}
		}
	}

	String JniClass::getStringField(jfieldID field, jobject _this) const noexcept
	{
		JniLocal<jstring> str((jstring)(getObjectField(field, _this)));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	String JniClass::getStringField(const char* name, jobject _this) const noexcept
	{
		JniLocal<jstring> str((jstring)(getObjectField(name, "Ljava/lang/String;", _this)));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	String JniClass::getStaticStringField(jfieldID field) const noexcept
	{
		JniLocal<jstring> str((jstring)(getStaticObjectField(field)));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	String JniClass::getStaticStringField(const char* name) const noexcept
	{
		JniLocal<jstring> str((jstring)(getStaticObjectField(name, "Ljava/lang/String;")));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	void JniClass::setStringField(jfieldID field, jobject _this, const StringParam& value) const noexcept
	{
		JniLocal<jstring> str(Jni::getJniString(value));
		setObjectField(field, _this, str);
	}

	void JniClass::setStringField(const char* name, jobject _this, const StringParam& value) const noexcept
	{
		JniLocal<jstring> str(Jni::getJniString(value));
		setObjectField(name, "Ljava/lang/String;", _this, str);
	}

	void JniClass::setStaticStringField(jfieldID field, const StringParam& value) const noexcept
	{
		JniLocal<jstring> str(Jni::getJniString(value));
		setStaticObjectField(field, str);
	}

	void JniClass::setStaticStringField(const char* name, const StringParam& value) const noexcept
	{
		JniLocal<jstring> str(Jni::getJniString(value));
		setStaticObjectField(name, "Ljava/lang/String;", str);
	}

	sl_bool JniClass::registerNative(const char* name, const char* sig, const void* fn) const noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env) {
			JNINativeMethod method;
			method.name = (char*)name;
			method.signature = (char*)sig;
			method.fnPtr = (void*)fn;
			return !(env->RegisterNatives(get(), &method, 1));
		}
		return sl_false;
	}


	Atomic<JniClass>::Atomic(jclass cls) noexcept: ref(CJniGlobal<jclass>::from(cls))
	{
	}

	AtomicJniClass& Atomic<JniClass>::operator=(jclass cls) noexcept
	{
		ref = CJniGlobal<jclass>::from(cls);
		return *this;
	}


	JniPreserveExceptionScope::JniPreserveExceptionScope() noexcept
	{
		Jni::setAutoClearException(sl_false);
	}

	JniPreserveExceptionScope::~JniPreserveExceptionScope()
	{
		Jni::setAutoClearException(sl_true);
	}

}

#endif
