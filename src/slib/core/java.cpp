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
#include "slib/core/function.h"
#include "slib/core/safe_static.h"

#include "slib/core/java/string.h"

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

#ifdef SLIB_DEBUG
//#define INIT_ON_LOAD
#endif

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
				CHashMap< String, JniGlobal<jclass> > classes;				
				CList< Function<void()> > callbacksInit;
			};

			SLIB_SAFE_STATIC_GETTER(SharedContext, getSharedContext)


			static void AddInitCallback(const Function<void()>& callback) noexcept
			{
				SharedContext* shared = getSharedContext();
				if (shared) {
					shared->callbacksInit.add(callback);
				}
			}

			static void ProcessException(JNIEnv* env) noexcept
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

			static sl_bool CheckException(JNIEnv* env) noexcept
			{
				if (env->ExceptionCheck()) {
					if (g_flagAutoClearException) {
						if (g_flagAutoPrintException) {
							env->ExceptionDescribe();
						}
						env->ExceptionClear();
					}
					return sl_true;
				}
				return sl_false;
			}


			JClass::JClass(const char* _name) noexcept: name(_name), m_flagLoaded(sl_false), m_cls(sl_null)
			{
#ifdef INIT_ON_LOAD
				AddInitCallback([this] {
					get();
				});
#endif
			}

			jclass JClass::get() noexcept
			{
				if (m_flagLoaded) {
					return m_cls;
				}
				SpinLocker locker(&m_lock);
				if (m_flagLoaded) {
					return m_cls;
				}
				if (!(Jni::getSharedJVM())) {
					return sl_null;
				}
#if defined(JNI_LOG_INIT_LOAD)
				LOG("LOADING JAVA CLASS: %s", name);
#endif
				jclass ret = Jni::getClass(name);
				if (ret) {
					m_cls = ret;
				} else {
					LOG_ERROR("LOADING JAVA CLASS FAILED: %s", name);
				}
				m_flagLoaded = sl_true;
				return ret;
			}


			JMethod::JMethod(JClass* _cls, const char* _name, const char* _sig) noexcept: cls(_cls), name(_name), sig(_sig), m_flagLoaded(sl_false), m_id(sl_null)
			{
#ifdef INIT_ON_LOAD
				AddInitCallback([this] {
					getId();
				});
#endif
			}

			jmethodID JMethod::getId() noexcept
			{
				if (m_flagLoaded) {
					return m_id;
				}
				SpinLocker locker(&m_lock);
				if (m_flagLoaded) {
					return m_id;
				}
				if (!(Jni::getSharedJVM())) {
					return sl_null;
				}
#if defined(JNI_LOG_INIT_LOAD)
				LOG("LOADING JAVA METHOD: %s::%s (%s)", cls->name, name, sig);
#endif
				jmethodID ret = Jni::getMethodID(cls->get(), name, sig);
				if (ret) {
					m_id = ret;
				} else {
					LOG_ERROR("LOADING JAVA METHOD FAILED: %s::%s (%s)", cls->name, name, sig);
				}
				m_flagLoaded = sl_true;
				return ret;
			}

			JniLocal<jobject> JMethod::newObject(jobject _null, ...) noexcept
			{				
				va_list args;
				va_start(args, _null);
				JniLocal<jobject> ret;
				jmethodID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						ret = env->NewObjectV(cls->get(), id, args);
						if (CheckException(env)) {
							ret.setNull();
						}
					}
				}
				va_end(args);
				return ret;
			}

			void JMethod::call(jobject _this, ...) noexcept
			{
				va_list args;
				va_start(args, _this);
				if (_this) {
					jmethodID id = getId();
					if (id) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							env->CallVoidMethodV(_this, id, args);
							ProcessException(env);
						}
					}
				}
				va_end(args);
			}

			JniLocal<jobject> JMethod::callObject(jobject _this, ...) noexcept
			{
				va_list args;
				va_start(args, _this);
				JniLocal<jobject> ret;
				if (_this) {
					jmethodID id = getId();
					if (id) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							ret = env->CallObjectMethodV(_this, id, args);
							if (CheckException(env)) {
								ret.setNull();
							}
						}
					}
				}
				va_end(args);
				return ret;
			}

			String JMethod::callString(jobject _this, ...) noexcept
			{
				va_list args;
				va_start(args, _this);
				String ret;
				if (_this) {
					jmethodID id = getId();
					if (id) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							JniLocal<jstring> str((jstring)(env->CallObjectMethodV(_this, id, args)));
							if (!(CheckException(env))) {
								ret = Jni::getString(str);
							}
						}
					}
				}
				va_end(args);
				return ret;
			}


			JStaticMethod::JStaticMethod(JClass* _cls, const char* _name, const char* _sig)  noexcept: cls(_cls), name(_name), sig(_sig), m_flagLoaded(sl_false), m_id(sl_null)
			{
#ifdef INIT_ON_LOAD
				AddInitCallback([this] {
					getId();
				});
#endif
			}

			jmethodID JStaticMethod::getId() noexcept
			{
				if (m_flagLoaded) {
					return m_id;
				}
				SpinLocker locker(&m_lock);
				if (m_flagLoaded) {
					return m_id;
				}
				if (!(Jni::getSharedJVM())) {
					return sl_null;
				}
#if defined(JNI_LOG_INIT_LOAD)
				LOG("LOADING JAVA STATIC METHOD: %s::%s (%s)", cls->name, name, sig);
#endif
				jmethodID ret = Jni::getStaticMethodID(cls->get(), name, sig);
				if (ret) {
					m_id = ret;
				} else {
					LOG_ERROR("LOADING JAVA STATIC METHOD FAILED: %s::%s (%s)", cls->name, name, sig);
				}
				m_flagLoaded = sl_true;
				return ret;
			}

			void JStaticMethod::call(jobject _null, ...) noexcept
			{
				va_list args;
				va_start(args, _null);
				jmethodID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						env->CallStaticVoidMethodV(cls->get(), id, args);
						ProcessException(env);
					}
				}
				va_end(args);
			}

			JniLocal<jobject> JStaticMethod::callObject(jobject _null, ...) noexcept
			{
				va_list args;
				va_start(args, _null);
				JniLocal<jobject> ret;
				jmethodID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						ret = env->CallStaticObjectMethodV(cls->get(), id, args);
						if (CheckException(env)) {
							ret.setNull();
						}
					}
				}
				va_end(args);
				return ret;
			}

			String JStaticMethod::callString(jobject _null, ...) noexcept
			{
				va_list args;
				va_start(args, _null);
				String ret;
				jmethodID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str((jstring)(env->CallStaticObjectMethodV(cls->get(), id, args)));
						if (!(CheckException(env))) {
							ret = Jni::getString(str);
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
				if (_this) { \
					jmethodID id = getId(); \
					if (id) { \
						JNIEnv* env = Jni::getCurrent(); \
						if (env) { \
							ret = env->Call##NAME##MethodV(_this, id, args); \
							if (CheckException(env)) { \
								ret = 0; \
							} \
						} \
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
				jmethodID id = getId(); \
				if (id) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						ret = env->CallStatic##NAME##MethodV(cls->get(), id, args); \
						if (CheckException(env)) { \
							ret = 0; \
						} \
					} \
				} \
				va_end(args); \
				return ret; \
			}

			DEFINE_JMETHOD_MEMBERS(jboolean, Boolean)
			DEFINE_JMETHOD_MEMBERS(jbyte, Byte)
			DEFINE_JMETHOD_MEMBERS(jchar, Char)
			DEFINE_JMETHOD_MEMBERS(jshort, Short)
			DEFINE_JMETHOD_MEMBERS(jint, Int)
			DEFINE_JMETHOD_MEMBERS(jlong, Long)
			DEFINE_JMETHOD_MEMBERS(jfloat, Float)
			DEFINE_JMETHOD_MEMBERS(jdouble, Double)


			JField::JField(JClass* _cls, const char* _name, const char* _sig) noexcept: cls(_cls), name(_name), sig(_sig), m_flagLoaded(sl_false), m_id(sl_null)
			{
#ifdef INIT_ON_LOAD
				AddInitCallback([this] {
					getId();
				});
#endif
			}

			jfieldID JField::getId() noexcept
			{
				if (m_flagLoaded) {
					return m_id;
				}
				SpinLocker locker(&m_lock);
				if (m_flagLoaded) {
					return m_id;
				}
				if (!(Jni::getSharedJVM())) {
					return sl_null;
				}
#if defined(JNI_LOG_INIT_LOAD)
				LOG("LOADING JAVA FIELD: %s::%s (%s)", cls->name, name, sig);
#endif
				jfieldID ret = Jni::getFieldID(cls->get(), name, sig);
				if (ret) {
					m_id = ret;
				} else {
					LOG_ERROR("LOADING JAVA FIELD FAILED: %s::%s (%s)", cls->name, name, sig);
				}
				m_flagLoaded = sl_true;
				return ret;
			}

			JniLocal<jobject> JField::getObject(jobject _this) noexcept
			{
				if (_this) {
					jfieldID id = getId();
					if (id) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							JniLocal<jobject> ret(env->GetObjectField(_this, id));
							if (!(CheckException(env))) {
								return ret;
							}
						}
					}
				}
				return sl_null;
			}

			void JField::setObject(jobject _this, jobject value) noexcept
			{
				if (_this) {
					jfieldID id = getId();
					if (id) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							env->SetObjectField(_this, id, value);
							ProcessException(env);
						}
					}
				}
			}

			String JField::getString(jobject _this) noexcept
			{
				if (_this) {
					jfieldID id = getId();
					if (id) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							JniLocal<jstring> str((jstring)(env->GetObjectField(_this, id)));
							if (!(CheckException(env))) {
								return Jni::getString(str);
							}
						}
					}
				}
				return sl_null;
			}

			void JField::setString(jobject _this, const StringParam& value) noexcept
			{
				if (_this) {
					jfieldID id = getId();
					if (id) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							JniLocal<jstring> str(Jni::getJniString(value));
							env->SetObjectField(_this, id, str.get());
							ProcessException(env);
						}
					}
				}
			}


			JStaticField::JStaticField(JClass* _cls, const char* _name, const char* _sig) noexcept: cls(_cls), name(_name), sig(_sig), m_flagLoaded(sl_false), m_id(sl_null)
			{
#ifdef INIT_ON_LOAD
				AddInitCallback([this] {
					getId();
				});
#endif
			}

			jfieldID JStaticField::getId() noexcept
			{
				if (m_flagLoaded) {
					return m_id;
				}
				SpinLocker locker(&m_lock);
				if (m_flagLoaded) {
					return m_id;
				}
				if (!(Jni::getSharedJVM())) {
					return sl_null;
				}
#if defined(JNI_LOG_INIT_LOAD)
				LOG("LOADING JAVA STATIC FIELD: %s::%s (%s)", cls->name, name, sig);
#endif
				jfieldID ret = Jni::getStaticFieldID(cls->get(), name, sig);
				if (ret) {
					m_id = ret;
				} else {
					LOG_ERROR("LOADING JAVA STATIC FIELD FAILED: %s::%s (%s)", cls->name, name, sig);
				}
				m_flagLoaded = sl_true;
				return ret;
			}

			JniLocal<jobject> JStaticField::getObject() noexcept
			{
				jfieldID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jobject> ret(env->GetStaticObjectField(cls->get(), id));
						if (!(CheckException(env))) {
							return ret;
						}
					}
				}
				return sl_null;
			}

			void JStaticField::setObject(jobject value) noexcept
			{
				jfieldID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						env->SetObjectField(cls->get(), id, value);
						ProcessException(env);
					}
				}
			}

			String JStaticField::getString() noexcept
			{
				jfieldID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str((jstring)(env->GetStaticObjectField(cls->get(), id)));
						if (!(CheckException(env))) {
							return Jni::getString(str);
						}
					}
				}
				return sl_null;
			}

			void JStaticField::setString(const StringParam& value) noexcept
			{
				jfieldID id = getId();
				if (id) {
					JNIEnv* env = Jni::getCurrent();
					if (env) {
						JniLocal<jstring> str(Jni::getJniString(value));
						env->SetObjectField(cls->get(), id, str.get());
						ProcessException(env);
					}
				}
			}

#define DEFINE_JFIELD_MEMBERS(TYPE, NAME) \
			TYPE JField::get##NAME(jobject _this) noexcept \
			{ \
				if (_this) { \
					jfieldID id = getId(); \
					if (id) { \
						JNIEnv* env = Jni::getCurrent(); \
						if (env) { \
							TYPE ret = env->Get##NAME##Field(_this, id); \
							if (!(CheckException(env))) { \
								return ret; \
							} \
						} \
					} \
				} \
				return 0; \
			} \
			void JField::set##NAME(jobject _this, TYPE value) noexcept \
			{ \
				if (_this) { \
					jfieldID id = getId(); \
					if (id) { \
						JNIEnv* env = Jni::getCurrent(); \
						if (env) { \
							env->Set##NAME##Field(_this, id, value); \
							ProcessException(env); \
						} \
					} \
				} \
			} \
			TYPE JStaticField::get##NAME() noexcept \
			{ \
				jfieldID id = getId(); \
				if (id) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						TYPE ret = env->GetStatic##NAME##Field(cls->get(), id); \
						if (!(CheckException(env))) { \
							return ret; \
						} \
					} \
				} \
				return 0; \
			} \
			void JStaticField::set##NAME(TYPE value) noexcept \
			{ \
				jfieldID id = getId(); \
				if (id) { \
					JNIEnv* env = Jni::getCurrent(); \
					if (env) { \
						env->SetStatic##NAME##Field(cls->get(), id, value); \
						ProcessException(env); \
					} \
				} \
			}

			DEFINE_JFIELD_MEMBERS(jboolean, Boolean)
			DEFINE_JFIELD_MEMBERS(jbyte, Byte)
			DEFINE_JFIELD_MEMBERS(jchar, Char)
			DEFINE_JFIELD_MEMBERS(jshort, Short)
			DEFINE_JFIELD_MEMBERS(jint, Int)
			DEFINE_JFIELD_MEMBERS(jlong, Long)
			DEFINE_JFIELD_MEMBERS(jfloat, Float)
			DEFINE_JFIELD_MEMBERS(jdouble, Double)

#define DEFINE_JFIELD_TYPE_MEMBERS(TYPE, NAME, SIG) \
			J##NAME##Field::J##NAME##Field(JClass* cls, const char* name) noexcept: JField(cls, name, SIG) {} \
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


			JObjectField::JObjectField(JClass* cls, const char* name, const char* sig) noexcept: JField(cls, name, sig)
			{				
			}

			JniLocal<jobject> JObjectField::get(jobject _this) noexcept
			{
				return getObject(_this);
			}

			void JObjectField::set(jobject _this, jobject value) noexcept
			{
				setObject(_this, value);
			}


			JStringField::JStringField(JClass* cls, const char* name) noexcept: JField(cls, name, "Ljava/lang/String;")
			{				
			}

			String JStringField::get(jobject _this) noexcept
			{
				return getString(_this);
			}

			void JStringField::set(jobject _this, const StringParam& value) noexcept
			{
				setString(_this, value);
			}

			JniLocal<jstring> JStringField::getObject(jobject _this) noexcept
			{
				return JField::getObject(_this);
			}

			void JStringField::setObject(jobject _this, jstring value) noexcept
			{
				JField::setObject(_this, (jobject)value);
			}


#define DEFINE_JSTATICFIELD_TYPE_MEMBERS(TYPE, NAME, SIG) \
			JStatic##NAME##Field::JStatic##NAME##Field(JClass* cls, const char* name) noexcept: JStaticField(cls, name, SIG) {} \
			TYPE JStatic##NAME##Field::get() noexcept \
			{ \
				return get##NAME(); \
			} \
			void JStatic##NAME##Field::set(TYPE value) noexcept \
			{ \
				set##NAME(value); \
			} \
			JFinal##NAME##Field::JFinal##NAME##Field(JClass* cls, const char* name) noexcept: JStatic##NAME##Field(cls, name), m_flagLoadedValue(sl_false) {} \
			TYPE JFinal##NAME##Field::get() noexcept \
			{ \
				if (m_flagLoadedValue) { \
					return m_value; \
				} \
				if (!(Jni::getSharedJVM())) { \
					return 0; \
				} \
				m_value = JStatic##NAME##Field::get(); \
				m_flagLoadedValue = sl_true; \
				return m_value; \
			}

			DEFINE_JSTATICFIELD_TYPE_MEMBERS(jboolean, Boolean, "Z")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int8, Byte, "B")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_uint16, Char, "C")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int16, Short, "S")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int32, Int, "I")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(sl_int64, Long, "J")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(float, Float, "F")
			DEFINE_JSTATICFIELD_TYPE_MEMBERS(double, Double, "D")


			JStaticObjectField::JStaticObjectField(JClass* cls, const char* name, const char* sig) noexcept: JStaticField(cls, name, sig)
			{
			}

			JniLocal<jobject> JStaticObjectField::get() noexcept
			{
				return getObject();
			}

			void JStaticObjectField::set(jobject value) noexcept
			{
				setObject(value);
			}


			JStaticStringField::JStaticStringField(JClass* cls, const char* name) noexcept: JStaticField(cls, name, "Ljava/lang/String;")
			{
			}

			String JStaticStringField::get() noexcept
			{
				return getString();
			}

			void JStaticStringField::set(const StringParam& value) noexcept
			{
				setString(value);
			}

			JniLocal<jstring> JStaticStringField::getObject() noexcept
			{
				return JStaticField::getObject();
			}

			void JStaticStringField::setObject(jstring value) noexcept
			{
				JStaticField::setObject((jobject)value);
			}


			JFinalObjectField::JFinalObjectField(JClass* cls, const char* name, const char* sig) noexcept: JStaticObjectField(cls, name, sig), m_flagLoadedValue(sl_false)
			{
			}

			jobject JFinalObjectField::get() noexcept
			{
				if (m_flagLoadedValue) {
					return m_value;
				}
				SpinLocker locker(&m_lockValue);
				if (m_flagLoadedValue) {
					return m_value;
				}
				if (!(Jni::getSharedJVM())) {
					return sl_null;
				}
				m_value = JStaticObjectField::get();
				m_flagLoadedValue = sl_true;
				return m_value;
			}


			JFinalStringObjectField::JFinalStringObjectField(JClass* cls, const char* name) noexcept: JFinalObjectField(cls, name, "Ljava/lang/String;")
			{
			}

			jstring JFinalStringObjectField::get() noexcept
			{
				return (jstring)(JFinalObjectField::get());
			}


			JFinalStringField::JFinalStringField(JClass* cls, const char* name) noexcept: JStaticStringField(cls, name), m_flagLoadedValue(sl_false)
			{
			}

			String JFinalStringField::get() noexcept
			{
				if (m_flagLoadedValue) {
					return m_value;
				}
				SpinLocker locker(&m_lockValue);
				if (m_flagLoadedValue) {
					return m_value;
				}
				if (!(Jni::getSharedJVM())) {
					return sl_null;
				}
				m_value = JStaticStringField::get();
				m_flagLoadedValue = sl_true;
				return m_value;
			}


			JNativeMethod::JNativeMethod(JClass* _cls, const char* _name, const char* _sig, const void* _fn) noexcept: cls(_cls), name(_name), sig(_sig), fn(_fn)
			{
				AddInitCallback([this]() {
					doRegister();
				});
			}

			void JNativeMethod::doRegister() noexcept
			{
#if defined(JNI_LOG_INIT_LOAD)
				LOG("REGISTERING JAVA NATIVE: %s::%s (%s)", cls->name, name, sig);
#endif
				if (!(Jni::registerNative(cls->get(), name, sig, fn))) {
					LOG_ERROR("REGISTERING JAVA NATIVE FAILED: %s::%s (%s)", cls->name, name, sig);
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

			// invoking initial callbacks
			{
				ListLocker< Function<void()> > list(shared->callbacksInit);
				for (sl_size i = 0; i < list.count; i++) {
					(list[i])();
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
		JNIEnv* env = g_envCurrent;
		if (!env) {
			env = Jni::attachThread();
		}
		return env;
	}

	void Jni::setCurrent(JNIEnv* env) noexcept
	{
		g_envCurrent = env;
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

	JniLocal<jclass> Jni::findClass(const StringParam& _className) noexcept
	{
		StringCstr className(_className);
		JNIEnv *env = getCurrent();
		if (env) {
			JniLocal<jclass> cls = env->FindClass(className.getData());
			if (!(CheckException(env))) {
				return cls;
			}
		}
		return sl_null;
	}

	jclass Jni::getClass(const StringParam& _className) noexcept
	{
		SharedContext* shared = getSharedContext();
		if (!shared) {
			return sl_null;
		}
		String className = _className.toString();
		ObjectLocker lock(&(shared->classes));
		JniGlobal<jclass>* pClass = shared->classes.getItemPointer(className);
		if (pClass) {
			return pClass->value;
		}
		JniGlobal<jclass> cls = Jni::findClass(className);
		if (cls.isNotNull()) {
			jclass ret = cls.get();
			shared->classes.put(className, Move(cls));
			return ret;
		}
		return sl_null;
	}

	jmethodID Jni::getMethodID(jclass cls, const char* name, const char* sig) noexcept
	{
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID ret = env->GetMethodID(cls, name, sig);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	jmethodID Jni::getStaticMethodID(jclass cls, const char* name, const char* sig) noexcept
	{
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID ret = env->GetStaticMethodID(cls, name, sig);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	jfieldID Jni::getFieldID(jclass cls, const char* name, const char* sig) noexcept
	{
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jfieldID ret = env->GetFieldID(cls, name, sig);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	jfieldID Jni::getStaticFieldID(jclass cls, const char* name, const char* sig) noexcept
	{
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jfieldID ret = env->GetStaticFieldID(cls, name, sig);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	JniLocal<jobject> Jni::newObject(jclass cls, jmethodID method, ...) noexcept
	{
		va_list args;
		va_start(args, method);
		JniLocal<jobject> ret = sl_null;
		if (cls && method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				ret = env->NewObjectV(cls, method, args);
				if (CheckException(env)) {
					ret = sl_null;
				}
			}
		}
		va_end(args);
		return ret;
	}

	JniLocal<jobject> Jni::newObject(jclass cls, const char* sig, ...) noexcept
	{
		va_list args;
		va_start(args, sig);
		JniLocal<jobject> ret = sl_null;
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID method = env->GetMethodID(cls, "<init>", sig);
				if (!CheckException(env)) {
					if (method) {
						ret = env->NewObjectV(cls, method, args);
						if (CheckException(env)) {
							ret = sl_null;
						}
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	JniLocal<jobject> Jni::newObject(jclass cls) noexcept
	{
		return newObject(cls, "()V");
	}

#define DEFINE_JNI_CALL_METHOD(TYPE, NAME) \
	TYPE Jni::call##NAME##Method(jobject _this, jmethodID method, ...) noexcept \
	{ \
		va_list args; \
		va_start(args, method); \
		TYPE ret = 0; \
		if (_this && method) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				ret = env->Call##NAME##MethodV(_this, method, args); \
				if (CheckException(env)) { \
					ret = 0; \
				} \
			} \
		} \
		va_end(args); \
		return ret; \
	} \
	TYPE Jni::call##NAME##Method(jobject _this, const char* name, const char* sig, ...) noexcept \
	{ \
		va_list args; \
		va_start(args, sig); \
		TYPE ret = 0; \
		if (_this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				JniLocal<jclass> cls = env->GetObjectClass(_this); \
				if (cls.isNotNull()) { \
					jmethodID method = env->GetMethodID(cls, name, sig); \
					if (!(CheckException(env))) { \
						if (method) { \
							ret = env->Call##NAME##MethodV(_this, method, args); \
							if (CheckException(env)) { \
								ret = 0; \
							} \
						} \
					} \
				} \
			} \
		} \
		va_end(args); \
		return ret; \
	} \
	TYPE Jni::callStatic##NAME##Method(jclass cls, jmethodID method, ...) noexcept \
	{ \
		va_list args; \
		va_start(args, method); \
		TYPE ret = 0; \
		if (cls && method) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				ret = env->CallStatic##NAME##MethodV(cls, method, args); \
				if (CheckException(env)) { \
					ret = 0; \
				} \
			} \
		} \
		va_end(args); \
		return ret; \
	} \
	TYPE Jni::callStatic##NAME##Method(jclass cls, const char* name, const char* sig, ...) noexcept \
	{ \
		va_list args; \
		va_start(args, sig); \
		TYPE ret = 0; \
		if (cls) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				jmethodID method = env->GetStaticMethodID(cls, name, sig); \
				if (!(CheckException(env))) { \
					if (method) { \
						ret = env->CallStatic##NAME##MethodV(cls, method, args); \
						if (CheckException(env)) { \
							ret = 0; \
						} \
					} \
				} \
			} \
		} \
		va_end(args); \
		return ret; \
	}
	
	DEFINE_JNI_CALL_METHOD(jboolean, Boolean)
	DEFINE_JNI_CALL_METHOD(jbyte, Byte)
	DEFINE_JNI_CALL_METHOD(jchar, Char)
	DEFINE_JNI_CALL_METHOD(jshort, Short)
	DEFINE_JNI_CALL_METHOD(jint, Int)
	DEFINE_JNI_CALL_METHOD(jlong, Long)
	DEFINE_JNI_CALL_METHOD(jfloat, Float)
	DEFINE_JNI_CALL_METHOD(jdouble, Double)

    JniLocal<jobject> Jni::callObjectMethod(jobject _this, jmethodID method, ...) noexcept
	{
		va_list args;
		va_start(args, method);
		JniLocal<jobject> ret;
		if (_this && method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				ret = env->CallObjectMethodV(_this, method, args);
				if (CheckException(env)) {
					ret.setNull();
				}
			}
		}
		va_end(args);
		return ret;
	}

	JniLocal<jobject> Jni::callObjectMethod(jobject _this, const char* name, const char* sig, ...) noexcept
	{
		va_list args;
		va_start(args, sig);
		JniLocal<jobject> ret;
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jclass> cls = env->GetObjectClass(_this);
				if (cls.isNotNull()) {
					jmethodID method = env->GetMethodID(cls, name, sig);
					if (!(CheckException(env))) {
						if (method) {
							ret = env->CallObjectMethodV(_this, method, args);
							if (CheckException(env)) {
								ret.setNull();
							}
						}
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	JniLocal<jobject> Jni::callStaticObjectMethod(jclass cls, jmethodID method, ...) noexcept
	{
		va_list args;
		va_start(args, method);
		JniLocal<jobject> ret;
		if (cls && method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				ret = env->CallStaticObjectMethodV(cls, method, args);
				if (CheckException(env)) {
					ret.setNull();
				}
			}
		}
		va_end(args);
		return ret;
	}

	JniLocal<jobject> Jni::callStaticObjectMethod(jclass cls, const char* name, const char* sig, ...) noexcept
	{
		va_list args;
		va_start(args, sig);
		JniLocal<jobject> ret;
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID method = env->GetStaticMethodID(cls, name, sig);
				if (!(CheckException(env))) {
					if (method) {
						ret = env->CallStaticObjectMethodV(cls, method, args);
						if (CheckException(env)) {
							ret.setNull();
						}
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	String Jni::callStringMethod(jobject _this, jmethodID method, ...) noexcept
	{
		va_list args;
		va_start(args, method);
		String ret;
		if (_this && method) {
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

	String Jni::callStringMethod(jobject _this, const char* name, const char* sig, ...) noexcept
	{
		va_list args;
		va_start(args, sig);
		String ret;
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jclass> cls = env->GetObjectClass(_this);
				if (cls.isNotNull()) {
					jmethodID method = env->GetMethodID(cls, name, sig);
					if (!(CheckException(env))) {
						if (method) {
							JniLocal<jstring> str((jstring)(env->CallObjectMethodV(_this, method, args)));
							if (!(CheckException(env))) {
								if (str.isNotNull()) {
									ret = Jni::getString(str);
								}
							}
						}
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	String Jni::callStaticStringMethod(jclass cls, jmethodID method, ...) noexcept
	{
		va_list args;
		va_start(args, method);
		String ret;
		if (cls && method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jstring> str((jstring)(env->CallStaticObjectMethodV(cls, method, args)));
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

	String Jni::callStaticStringMethod(jclass cls, const char* name, const char* sig, ...) noexcept
	{
		va_list args;
		va_start(args, sig);
		String ret;
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID method = env->GetStaticMethodID(cls, name, sig);
				if (!(CheckException(env))) {
					if (method) {
						JniLocal<jstring> str((jstring)(env->CallStaticObjectMethodV(cls, method, args)));
						if (!(CheckException(env))) {
							if (str.isNotNull()) {
								ret = Jni::getString(str);
							}
						}
					}
				}
			}
		}
		va_end(args);
		return ret;
	}

	void Jni::callVoidMethod(jobject _this, jmethodID method, ...) noexcept
	{
		va_list args;
		va_start(args, method);
		if (_this && method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->CallVoidMethodV(_this, method, args);
				ProcessException(env);
			}
		}
		va_end(args);
	}

	void Jni::callVoidMethod(jobject _this, const char* name, const char* sig, ...) noexcept
	{
		va_list args;
		va_start(args, sig);
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jclass> cls = env->GetObjectClass(_this);
				if (cls.isNotNull()) {
					jmethodID method = env->GetMethodID(cls, name, sig);
					if (!(CheckException(env))) {
						if (method) {
							env->CallVoidMethodV(_this, method, args);
							ProcessException(env);
						}
					}
				}
			}
		}
		va_end(args);
	}

	void Jni::callStaticVoidMethod(jclass cls, jmethodID method, ...) noexcept
	{
		va_list args;
		va_start(args, method);
		if (cls && method) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->CallStaticVoidMethodV(cls, method, args);
				ProcessException(env);
			}
		}
		va_end(args);
	}

	void Jni::callStaticVoidMethod(jclass cls, const char* name, const char* sig, ...) noexcept
	{
		va_list args;
		va_start(args, sig);
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jmethodID method = env->GetStaticMethodID(cls, name, sig);
				if (!(CheckException(env))) {
					if (method) {
						env->CallStaticVoidMethodV(cls, method, args);
						ProcessException(env);
					}
				}
			}
		}
		va_end(args);
	}

#define DEFINE_JNI_FIELD(TYPE, NAME, SIG) \
	TYPE Jni::get##NAME##Field(jobject _this, jfieldID field) noexcept \
	{ \
		if (_this && field) { \
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
	TYPE Jni::get##NAME##Field(jobject _this, const char* name) noexcept \
	{ \
		if (_this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				JniLocal<jclass> cls = env->GetObjectClass(_this); \
				if (cls.isNotNull()) { \
					jfieldID field = env->GetFieldID(cls, name, SIG); \
					if (!(CheckException(env))) { \
						if (field) { \
							TYPE ret = env->Get##NAME##Field(_this, field); \
							if (!(CheckException(env))) { \
								return ret; \
							} \
						} \
					} \
				} \
			} \
		} \
		return 0; \
	} \
	TYPE Jni::getStatic##NAME##Field(jclass cls, jfieldID field) noexcept \
	{ \
		if (cls && field) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				TYPE ret = env->GetStatic##NAME##Field(cls, field); \
				if (!(CheckException(env))) { \
					return ret; \
				} \
			} \
		} \
		return 0; \
	} \
	TYPE Jni::getStatic##NAME##Field(jclass cls, const char* name) noexcept \
	{ \
		if (cls) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				jfieldID field = env->GetStaticFieldID(cls, name, SIG); \
				if (!(CheckException(env))) { \
					if (field) { \
						TYPE ret = env->GetStatic##NAME##Field(cls, field); \
						if (!(CheckException(env))) { \
							return ret; \
						} \
					} \
				} \
			} \
		} \
		return 0; \
	} \
	void Jni::set##NAME##Field(jobject _this, jfieldID field, TYPE value) noexcept \
	{ \
		if (_this && field) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				env->Set##NAME##Field(_this, field, value); \
				ProcessException(env); \
			} \
		} \
	} \
	void Jni::set##NAME##Field(jobject _this, const char* name, TYPE value) noexcept \
	{ \
		if (_this) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				JniLocal<jclass> cls = env->GetObjectClass(_this); \
				if (cls.isNotNull()) { \
					jfieldID field = env->GetFieldID(cls, name, SIG); \
					if (!(CheckException(env))) { \
						if (field) { \
							env->Set##NAME##Field(_this, field, value); \
							ProcessException(env); \
						} \
					} \
				} \
			} \
		} \
	} \
	void Jni::setStatic##NAME##Field(jclass cls, jfieldID field, TYPE value) noexcept \
	{ \
		if (cls && field) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				env->SetStatic##NAME##Field(cls, field, value); \
				ProcessException(env); \
			} \
		} \
	} \
	void Jni::setStatic##NAME##Field(jclass cls, const char* name, TYPE value) noexcept \
	{ \
		if (cls) { \
			JNIEnv* env = Jni::getCurrent(); \
			if (env) { \
				jfieldID field = env->GetStaticFieldID(cls, name, SIG); \
				if (!(CheckException(env))) { \
					if (field) { \
						env->SetStatic##NAME##Field(cls, field, value); \
						ProcessException(env); \
					} \
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

	JniLocal<jobject> Jni::getObjectField(jobject _this, jfieldID field) noexcept
	{
		if (_this && field) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jobject> ret = env->GetObjectField(_this, field);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	JniLocal<jobject> Jni::getObjectField(jobject _this, const char* name, const char* sig) noexcept
	{
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jclass> cls = env->GetObjectClass(_this);
				if (cls.isNotNull()) {
					jfieldID field = env->GetFieldID(cls, name, sig);
					if (!(CheckException(env))) {
						if (field) {
							JniLocal<jobject> ret = env->GetObjectField(_this, field);
							if (!(CheckException(env))) {
								return ret;
							}
						}
					}
				}
			}
		}
		return sl_null;
	}

	JniLocal<jobject> Jni::getStaticObjectField(jclass cls, jfieldID field) noexcept
	{
		if (cls && field) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jobject> ret = env->GetStaticObjectField(cls, field);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	JniLocal<jobject> Jni::getStaticObjectField(jclass cls, const char* name, const char* sig) noexcept
	{
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jfieldID field = env->GetStaticFieldID(cls, name, sig);
				if (!(CheckException(env))) {
					if (field) {
						JniLocal<jobject> ret = env->GetStaticObjectField(cls, field);
						if (!(CheckException(env))) {
							return ret;
						}
					}
				}
			}
		}
		return sl_null;
	}

	void Jni::setObjectField(jobject _this, jfieldID field, jobject value) noexcept
	{
		if (_this && field) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->SetObjectField(_this, field, value);
				ProcessException(env);
			}
		}
	}

	void Jni::setObjectField(jobject _this, const char* name, const char* sig, jobject value) noexcept
	{
		if (_this) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jclass> cls = env->GetObjectClass(_this);
				if (cls.isNotNull()) {
					jfieldID field = env->GetFieldID(cls, name, sig);
					if (!(CheckException(env))) {
						if (field) {
							env->SetObjectField(_this, field, value);
							ProcessException(env);
						}
					}
				}
			}
		}
	}

	void Jni::setStaticObjectField(jclass cls, jfieldID field, jobject value) noexcept
	{
		if (cls && field) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->SetStaticObjectField(cls, field, value);
				ProcessException(env);
			}
		}
	}

	void Jni::setStaticObjectField(jclass cls, const char* name, const char* sig, jobject value) noexcept
	{
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				jfieldID field = env->GetStaticFieldID(cls, name, sig);
				if (!(CheckException(env))) {
					if (field) {
						env->SetStaticObjectField(cls, field, value);
						ProcessException(env);
					}
				}
			}
		}
	}

	String Jni::getStringField(jobject _this, jfieldID field) noexcept
	{
		JniLocal<jstring> str(getObjectField(_this, field));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	String Jni::getStringField(jobject _this, const char* name) noexcept
	{
		JniLocal<jstring> str(getObjectField(_this, name, "Ljava/lang/String;"));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	String Jni::getStaticStringField(jclass cls, jfieldID field) noexcept
	{
		JniLocal<jstring> str(getStaticObjectField(cls, field));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	String Jni::getStaticStringField(jclass cls, const char* name) noexcept
	{
		JniLocal<jstring> str(getStaticObjectField(cls, name, "Ljava/lang/String;"));
		if (str.isNotNull()) {
			return Jni::getString(str);
		}
		return sl_null;
	}

	void Jni::setStringField(jobject _this, jfieldID field, const StringParam& value) noexcept
	{
		if (_this && field) {
			JniLocal<jstring> str(Jni::getJniString(value));
			setObjectField(_this, field, str.get());
		}
	}

	void Jni::setStringField(jobject _this, const char* name, const StringParam& value) noexcept
	{
		if (_this) {
			JniLocal<jstring> str(Jni::getJniString(value));
			setObjectField(_this, name, "Ljava/lang/String;", str.get());
		}
	}

	void Jni::setStaticStringField(jclass cls, jfieldID field, const StringParam& value) noexcept
	{
		if (cls && field) {
			JniLocal<jstring> str(Jni::getJniString(value));
			setStaticObjectField(cls, field, str.get());
		}
	}

	void Jni::setStaticStringField(jclass cls, const char* name, const StringParam& value) noexcept
	{
		if (cls) {
			JniLocal<jstring> str(Jni::getJniString(value));
			setStaticObjectField(cls, name, "Ljava/lang/String;", str.get());
		}
	}

	sl_bool Jni::registerNative(jclass cls, const char* name, const char* sig, const void* fn) noexcept
	{
		if (cls) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JNINativeMethod method;
				method.name = (char*)name;
				method.signature = (char*)sig;
				method.fnPtr = (void*)fn;
				return !(env->RegisterNatives(cls, &method, 1));
			}
		}
		return sl_false;
	}

	JniLocal<jclass> Jni::getObjectClass(jobject obj) noexcept
	{
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				return env->GetObjectClass(obj);
			}
		}
		return sl_null;
	}

	sl_bool Jni::isInstanceOf(jobject obj, jclass cls) noexcept
	{
		if (cls && obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				return env->IsInstanceOf(obj, cls) != 0;
			}
		}
		return sl_false;
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
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				return env->GetObjectRefType(obj);
			}
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

	JniLocal<jobject> Jni::newLocalRef(jobject obj) noexcept
	{
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				return env->NewLocalRef(obj);
			}
		}
		return sl_null;
	}

	void Jni::deleteLocalRef(jobject obj) noexcept
	{
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
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
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				return env->NewGlobalRef(obj);
			}
		}
		return sl_null;
	}

	void Jni::deleteGlobalRef(jobject obj) noexcept
	{
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->DeleteGlobalRef(obj);
			}
		}
	}

	sl_bool Jni::isWeakRef(jobject obj) noexcept
	{
		return Jni::getRefType(obj) == JNIWeakGlobalRefType;
	}

	JniLocal<jobject> Jni::newWeakRef(jobject obj) noexcept
	{
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				return env->NewWeakGlobalRef(obj);
			}
		}
		return sl_null;
	}

	void Jni::deleteWeakRef(jobject obj) noexcept
	{
		if (obj) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				env->DeleteWeakGlobalRef(obj);
			}
		}
	}

	JniLocal<jstring> Jni::getJniString(const StringParam& _str) noexcept
	{
		if (_str.isNotNull()) {
			StringData16 str(_str);
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jstring> ret = env->NewString((jchar*)(str.getData()), (jsize)(str.getLength()));
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	JniLocal<jstring> Jni::getJniString(const sl_char16* str, sl_size length) noexcept
	{
		if (str) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jstring> ret = env->NewString((jchar*)(str), (jsize)(length));
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

	JniLocal<jobjectArray> Jni::newObjectArray(jclass clsElement, sl_uint32 length) noexcept
	{
		if (clsElement) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jobjectArray> ret = env->NewObjectArray(length, clsElement, sl_null);
				if (!(CheckException(env))) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	JniLocal<jobject> Jni::getObjectArrayElement(jobjectArray array, sl_uint32 index) noexcept
	{
		if (array) {
			JNIEnv* env = Jni::getCurrent();
			if (env) {
				JniLocal<jobject> ret = env->GetObjectArrayElement(array, index);
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

	JniLocal<jobjectArray> Jni::newStringArray(sl_uint32 length) noexcept
	{
		return Jni::newObjectArray(java::String::getClass(), length);
	}

	String Jni::getStringArrayElement(jobjectArray array, sl_uint32 index) noexcept
	{
		JniLocal<jstring> v(Jni::getObjectArrayElement(array, index));
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
	JniLocal<TYPE##Array> Jni::new##NAME##Array(sl_uint32 length) noexcept \
	{ \
		JNIEnv* env = Jni::getCurrent(); \
		if (env) { \
			JniLocal<TYPE##Array> ret = env->New##NAME##Array(length); \
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

	JniLocal<jobject> Jni::newDirectByteBuffer(void* address, sl_size capacity) noexcept
	{
		JNIEnv* env = Jni::getCurrent();
		if (env && address && capacity) {
			JniLocal<jobject> ret = env->NewDirectByteBuffer(address, capacity);
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


	JniStringConstant::JniStringConstant(const sl_char16* sz) noexcept: content(sz), m_flagLoaded(sl_false)
	{
	}

	JniStringConstant::~JniStringConstant()
	{
		m_object.setNull();
	}
	
	jstring JniStringConstant::get() noexcept
	{
		if (m_flagLoaded) {
			return m_object.get();
		}
		SpinLocker locker(&m_lock);
		if (m_flagLoaded) {
			return m_object.get();
		}
		if (!(Jni::getSharedJVM())) {
			return sl_null;
		}
		m_object = Jni::getJniString(content);
		m_flagLoaded = sl_true;
		return m_object.get();
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
