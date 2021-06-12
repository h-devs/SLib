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

#ifndef CHECKHEADER_SLIB_CORE_JAVA
#define CHECKHEADER_SLIB_CORE_JAVA

/**********************************************************

 This JNI library is implemented for shared JVM (one JVM)

**********************************************************/

#include "definition.h"

#if defined(SLIB_PLATFORM_USE_JNI)

#include "object.h"
#include "string.h"

#include <jni.h>

namespace slib
{

	class JniClass;

	class SLIB_EXPORT Jni
	{
	public:
		static void initialize(JavaVM* jvm) noexcept;

		static void setSharedJVM(JavaVM* jvm) noexcept;
		static JavaVM* getSharedJVM() noexcept;

		// thread-storage
		static JNIEnv* getCurrent() noexcept;
		static void setCurrent(JNIEnv* jni) noexcept;

		// attach and set current
		static JNIEnv* attachThread(JavaVM* jvm = sl_null) noexcept;
		static void detachThread(JavaVM* jvm = sl_null) noexcept;

		// class
		static JniClass findClass(const StringParam& className, sl_bool flagOptional = sl_false) noexcept;
		static JniClass getClass(const String& className, sl_bool flagOptional = sl_false) noexcept;
		static void registerClass(const String& className, jclass cls) noexcept;
		static void unregisterClass(const String& className) noexcept;

		// object
		static sl_bool isSameObject(jobject ref1, jobject ref2) noexcept;

		static jobjectRefType getRefType(jobject obj) noexcept;
		static sl_bool isInvalidRef(jobject obj) noexcept;

		static sl_bool isLocalRef(jobject obj) noexcept;
		static jobject newLocalRef(jobject obj) noexcept;
		static void deleteLocalRef(jobject obj) noexcept;

		static sl_bool isGlobalRef(jobject obj) noexcept;
		static jobject newGlobalRef(jobject obj) noexcept;
		static void deleteGlobalRef(jobject obj) noexcept;

		static sl_bool isWeakRef(jobject obj) noexcept;
		static jobject newWeakRef(jobject obj) noexcept;
		static void deleteWeakRef(jobject obj) noexcept;

		// string
		static jstring getJniString(const StringParam& str) noexcept;
		static jstring getJniString(const sl_char16* str, const sl_size length) noexcept;
		static String getString(jstring str) noexcept;

		/*
		 * Array release<TYPE>ArrayElements Mode
		 * 0 - commit and free
		 * JNI_COMMIT - commit only
		 * JNI_ABORT - free only
		 */
		static sl_uint32 getArrayLength(jarray array) noexcept;
		static jobjectArray newObjectArray(jclass clsElement, sl_uint32 length) noexcept;
		static jobject getObjectArrayElement(jobjectArray array, sl_uint32 index) noexcept;
		static void setObjectArrayElement(jobjectArray array, sl_uint32 index, jobject value) noexcept;
		static jobjectArray newStringArray(sl_uint32 length) noexcept;
		static String getStringArrayElement(jobjectArray array, sl_uint32 index) noexcept;
		static void setStringArrayElement(jobjectArray array, sl_uint32 index, const StringParam& value) noexcept;
		static jbooleanArray newBooleanArray(sl_uint32 length) noexcept;
		static jboolean* getBooleanArrayElements(jbooleanArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseBooleanArrayElements(jbooleanArray array, jboolean* buf, jint mode = 0) noexcept;
		static void getBooleanArrayRegion(jbooleanArray array, sl_uint32 index, sl_uint32 len, jboolean* buf) noexcept;
		static void setBooleanArrayRegion(jbooleanArray array, sl_uint32 index, sl_uint32 len, jboolean* buf) noexcept;
		static jbyteArray newByteArray(sl_uint32 length) noexcept;
		static jbyte* getByteArrayElements(jbyteArray array, jboolean* isCopy) noexcept;
		static void releaseByteArrayElements(jbyteArray array, jbyte* buf, jint mode = 0) noexcept;
		static void getByteArrayRegion(jbyteArray array, sl_uint32 index, sl_uint32 len, jbyte* buf) noexcept;
		static void setByteArrayRegion(jbyteArray array, sl_uint32 index, sl_uint32 len, jbyte* buf) noexcept;
		static jcharArray newCharArray(sl_uint32 length) noexcept;
		static jchar* getCharArrayElements(jcharArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseCharArrayElements(jcharArray array, jchar* buf, jint mode = 0) noexcept;
		static void getCharArrayRegion(jcharArray array, sl_uint32 index, sl_uint32 len, jchar* buf) noexcept;
		static void setCharArrayRegion(jcharArray array, sl_uint32 index, sl_uint32 len, jchar* buf) noexcept;
		static jshortArray newShortArray(sl_uint32 length) noexcept;
		static jshort* getShortArrayElements(jshortArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseShortArrayElements(jshortArray array, jshort* buf, jint mode = 0) noexcept;
		static void getShortArrayRegion(jshortArray array, sl_uint32 index, sl_uint32 len, jshort* buf) noexcept;
		static void setShortArrayRegion(jshortArray array, sl_uint32 index, sl_uint32 len, jshort* buf) noexcept;
		static jintArray newIntArray(sl_uint32 length) noexcept;
		static jint* getIntArrayElements(jintArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseIntArrayElements(jintArray array, jint* buf, jint mode = 0) noexcept;
		static void getIntArrayRegion(jintArray array, sl_uint32 index, sl_uint32 len, jint* buf) noexcept;
		static void setIntArrayRegion(jintArray array, sl_uint32 index, sl_uint32 len, jint* buf) noexcept;
		static jlongArray newLongArray(sl_uint32 length) noexcept;
		static jlong* getLongArrayElements(jlongArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseLongArrayElements(jlongArray array, jlong* buf, jint mode = 0) noexcept;
		static void getLongArrayRegion(jlongArray array, sl_uint32 index, sl_uint32 len, jlong* buf) noexcept;
		static void setLongArrayRegion(jlongArray array, sl_uint32 index, sl_uint32 len, jlong* buf) noexcept;
		static jfloatArray newFloatArray(sl_uint32 length) noexcept;
		static jfloat* getFloatArrayElements(jfloatArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseFloatArrayElements(jfloatArray array, jfloat* buf, jint mode = 0) noexcept;
		static void getFloatArrayRegion(jfloatArray array, sl_uint32 index, sl_uint32 len, jfloat* buf) noexcept;
		static void setFloatArrayRegion(jfloatArray array, sl_uint32 index, sl_uint32 len, jfloat* buf) noexcept;
		static jdoubleArray newDoubleArray(sl_uint32 length) noexcept;
		static jdouble* getDoubleArrayElements(jdoubleArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseDoubleArrayElements(jdoubleArray array, jdouble* buf, jint mode = 0) noexcept;
		static void getDoubleArrayRegion(jdoubleArray array, sl_uint32 index, sl_uint32 len, jdouble* buf) noexcept;
		static void setDoubleArrayRegion(jdoubleArray array, sl_uint32 index, sl_uint32 len, jdouble* buf) noexcept;

		// direct buffer
		static jobject newDirectByteBuffer(void* address, sl_size capacity) noexcept;
		static void* getDirectBufferAddress(jobject buf) noexcept;
		static sl_size getDirectBufferCapacity(jobject buf) noexcept;

		// exception
		static sl_bool checkException() noexcept;
		static void clearException() noexcept;
		static void printException() noexcept;
		static sl_bool checkExceptionAndClear() noexcept;
		static sl_bool checkExceptionAndPrintClear() noexcept;
		static sl_bool isAutoClearException() noexcept;
		static void setAutoClearException(sl_bool flag) noexcept;
		static sl_bool isAutoPrintException() noexcept;
		static void setAutoPrintException(sl_bool flag) noexcept;

	};


	template <class T>
	class SLIB_EXPORT JniLocal
	{
	public:
		T value;

	public:
		constexpr JniLocal(): value(sl_null) {}

		constexpr JniLocal(T _value): value(_value) {}

		~JniLocal()
		{
			free();
		}

	public:
		operator T&() noexcept
		{
			return value;
		}

		constexpr operator T() const
		{
			return value;
		}

		T operator=(T value) noexcept
		{
			this->value = value;
			return value;
		}

		constexpr T get() const
		{
			return value;
		}

		constexpr sl_bool isNotNull() const
		{
			return value != sl_null;
		}

		constexpr sl_bool isNull() const
		{
			return !value;
		}

		void setNull() noexcept
		{
			this->value = sl_null;
		}

		void free() noexcept
		{
			if (value) {
				Jni::deleteLocalRef(value);
				value = sl_null;
			}
		}

	};

	class SLIB_EXPORT CJniGlobalBase : public Referable
	{
		SLIB_DECLARE_OBJECT
	};

	template <class T>
	class SLIB_EXPORT CJniGlobal : public CJniGlobalBase
	{
	protected:
		CJniGlobal() = default;

		~CJniGlobal()
		{
			Jni::deleteGlobalRef(object);
		}

	public:
		static Ref< CJniGlobal<T> > from(T obj) noexcept
		{
			Ref< CJniGlobal<T> > ret;
			if (obj) {
				jobject jglobal = Jni::newGlobalRef(obj);
				if (jglobal) {
					ret = new CJniGlobal<T>();
					if (ret.isNotNull()) {
						ret->object = (T)jglobal;
						return ret;
					}
					Jni::deleteGlobalRef(jglobal);
				}
			}
			return sl_null;
		}


	public:
		T object;

	};

	template <class T>
	class SLIB_EXPORT JniGlobal
	{
	public:
		Ref< CJniGlobal<T> > ref;
		SLIB_REF_WRAPPER(JniGlobal, CJniGlobal<T>)

	public:
		JniGlobal(T obj) noexcept: ref(CJniGlobal<T>::from(obj)) {}

		JniGlobal(const JniLocal<T>& obj) noexcept: ref(CJniGlobal<T>::from(obj.value)) {}

	public:
		static JniGlobal<T> from(T obj) noexcept
		{
			return JniGlobal<T>(obj);
		}

	public:
		JniGlobal<T>& operator=(T obj) noexcept
		{
			ref = CJniGlobal<T>::from(obj);
			return *this;
		}

		JniGlobal<T>& operator=(const JniLocal<T>& obj) noexcept
		{
			ref = CJniGlobal<T>::from(obj.value);
			return *this;
		}


	public:
		T get() const noexcept
		{
			CJniGlobal<T>* o = ref.get();
			if (o) {
				return o->object;
			} else {
				return 0;
			}
		}

		operator T() const noexcept
		{
			CJniGlobal<T>* o = ref.get();
			if (o) {
				return o->object;
			} else {
				return 0;
			}
		}

	};


	template <class T>
	class SLIB_EXPORT Atomic< JniGlobal<T> >
	{
	public:
		AtomicRef< CJniGlobal<T> > ref;
		SLIB_ATOMIC_REF_WRAPPER(CJniGlobal<T>)

	public:
		Atomic(T obj) noexcept: ref(CJniGlobal<T>::from(obj)) {}

		Atomic(JniLocal<T>& obj) noexcept: ref(CJniGlobal<T>::from(obj.value)) {}

	public:
		Atomic& operator=(T obj) noexcept
		{
			ref = CJniGlobal<T>::from(obj);
			return *this;
		}

		Atomic& operator=(JniLocal<T>& obj) noexcept
		{
			ref = CJniGlobal<T>::from(obj.value);
			return *this;
		}

	public:
		T get() const noexcept
		{
			Ref< CJniGlobal<T> > o(ref);
			if (o.isNotNull()) {
				return o->object;
			} else {
				return 0;
			}
		}

	};

	template <class T>
	using AtomicJniGlobal = Atomic< JniGlobal<T> >;


	template <>
	class SLIB_EXPORT Atomic<JniClass>
	{
	public:
		AtomicRef< CJniGlobal<jclass> > ref;
		SLIB_ATOMIC_REF_WRAPPER(CJniGlobal<jclass>)

	public:
		Atomic(jclass cls) noexcept;
	
	public:
		Atomic& operator=(jclass cls) noexcept;
	
	};
	
	typedef Atomic<JniClass> AtomicJniClass;
	
	class SLIB_EXPORT JniClass
	{
	public:
		Ref< CJniGlobal<jclass> > ref;
		SLIB_REF_WRAPPER(JniClass, CJniGlobal<jclass>)
		
	public:
		JniClass(jclass cls) noexcept;

	public:
		JniClass& operator=(jclass cls) noexcept;

	public:
		static JniClass from(jclass cls) noexcept;

		static JniClass getClassOfObject(jobject object) noexcept;

	public:
		jclass get() const noexcept;

		operator jclass() const noexcept;

	public:
		sl_bool isInstanceOf(jobject obj) const noexcept;

		/*
		 * Signature
		 * Z - boolean
		 * B - byte
		 * C - char
		 * S - short
		 * I - int
		 * J - long long
		 * F - float
		 * D - double
		 * V - void
		 * L<class-name>; - object
		 * [<type> - type[]
		 * (arg-types)ret-type : method type
		 */
		jmethodID getMethodID(const char* name, const char* sig, sl_bool flagOptional = sl_false) const noexcept;
		jmethodID getStaticMethodID(const char* name, const char* sig, sl_bool flagOptional = sl_false) const noexcept;
		jfieldID getFieldID(const char* name, const char* sig, sl_bool flagOptional = sl_false) const noexcept;
		jfieldID getStaticFieldID(const char* name, const char* sig, sl_bool flagOptional = sl_false) const noexcept;

		jobject newObject(jmethodID method, ...) const noexcept;
		jobject newObject(const char* sigConstructor, ...) const noexcept;
		jobject newObject() const noexcept;

		jobject callObjectMethod(jmethodID method, jobject _this, ...) const noexcept;
		jobject callObjectMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jobject callStaticObjectMethod(jmethodID method, ...) const noexcept;
		jobject callStaticObjectMethod(const char* name, const char* sig, ...) const noexcept;
		jboolean callBooleanMethod(jmethodID method, jobject _this, ...) const noexcept;
		jboolean callBooleanMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jboolean callStaticBooleanMethod(jmethodID method, ...) const noexcept;
		jboolean callStaticBooleanMethod(const char* name, const char* sig, ...) const noexcept;
		jbyte callByteMethod(jmethodID method, jobject _this, ...) const noexcept;
		jbyte callByteMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jbyte callStaticByteMethod(jmethodID method, ...) const noexcept;
		jbyte callStaticByteMethod(const char* name, const char* sig, ...) const noexcept;
		jchar callCharMethod(jmethodID method, jobject _this, ...) const noexcept;
		jchar callCharMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jchar callStaticCharMethod(jmethodID method, ...) const noexcept;
		jchar callStaticCharMethod(const char* name, const char* sig, ...) const noexcept;
		jshort callShortMethod(jmethodID method, jobject _this, ...) const noexcept;
		jshort callShortMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jshort callStaticShortMethod(jmethodID method, ...) const noexcept;
		jshort callStaticShortMethod(const char* name, const char* sig, ...) const noexcept;
		jint callIntMethod(jmethodID method, jobject _this, ...) const noexcept;
		jint callIntMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jint callStaticIntMethod(jmethodID method, ...) const noexcept;
		jint callStaticIntMethod(const char* name, const char* sig, ...) const noexcept;
		jlong callLongMethod(jmethodID method, jobject _this, ...) const noexcept;
		jlong callLongMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jlong callStaticLongMethod(jmethodID method, ...) const noexcept;
		jlong callStaticLongMethod(const char* name, const char* sig, ...) const noexcept;
		jfloat callFloatMethod(jmethodID method, jobject _this, ...) const noexcept;
		jfloat callFloatMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jfloat callStaticFloatMethod(jmethodID method, ...) const noexcept;
		jfloat callStaticFloatMethod(const char* name, const char* sig, ...) const noexcept;
		jdouble callDoubleMethod(jmethodID method, jobject _this, ...) const noexcept;
		jdouble callDoubleMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		jdouble callStaticDoubleMethod(jmethodID method, ...) const noexcept;
		jdouble callStaticDoubleMethod(const char* name, const char* sig, ...) const noexcept;
		void callVoidMethod(jmethodID method, jobject _this, ...) const noexcept;
		void callVoidMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		void callStaticVoidMethod(jmethodID method, ...) const noexcept;
		void callStaticVoidMethod(const char* name, const char* sig, ...) const noexcept;
		String callStringMethod(jmethodID method, jobject _this, ...) const noexcept;
		String callStringMethod(const char* name, const char* sig, jobject _this, ...) const noexcept;
		String callStaticStringMethod(jmethodID method, ...) const noexcept;
		String callStaticStringMethod(const char* name, const char* sig, ...) const noexcept;

		jobject getObjectField(jfieldID field, jobject _this) const noexcept;
		jobject getObjectField(const char* name, const char* sig, jobject _this) const noexcept;
		void setObjectField(jfieldID field, jobject _this, jobject value) const noexcept;
		void setObjectField(const char* name, const char* sig, jobject _this, jobject value) const noexcept;
		jobject getStaticObjectField(jfieldID field) const noexcept;
		jobject getStaticObjectField(const char* name, const char* sig) const noexcept;
		void setStaticObjectField(jfieldID field, jobject value) const noexcept;
		void setStaticObjectField(const char* name, const char* sig, jobject value) const noexcept;
		jboolean getBooleanField(jfieldID field, jobject _this) const noexcept;
		jboolean getBooleanField(const char* name, jobject _this) const noexcept;
		void setBooleanField(jfieldID field, jobject _this, jboolean value) const noexcept;
		void setBooleanField(const char* name, jobject _this, jboolean value) const noexcept;
		jboolean getStaticBooleanField(jfieldID field) const noexcept;
		jboolean getStaticBooleanField(const char* name) const noexcept;
		void setStaticBooleanField(jfieldID field, jboolean value) const noexcept;
		void setStaticBooleanField(const char* name, jboolean value) const noexcept;
		jbyte getByteField(jfieldID field, jobject _this) const noexcept;
		jbyte getByteField(const char* name, jobject _this) const noexcept;
		void setByteField(jfieldID field, jobject _this, jbyte value) const noexcept;
		void setByteField(const char* name, jobject _this, jbyte value) const noexcept;
		jbyte getStaticByteField(jfieldID field) const noexcept;
		jbyte getStaticByteField(const char* name) const noexcept;
		void setStaticByteField(jfieldID field, jbyte value) const noexcept;
		void setStaticByteField(const char* name, jbyte value) const noexcept;
		jchar getCharField(jfieldID field, jobject _this) const noexcept;
		jchar getCharField(const char* name, jobject _this) const noexcept;
		void setCharField(jfieldID field, jobject _this, jchar value) const noexcept;
		void setCharField(const char* name, jobject _this, jchar value) const noexcept;
		jchar getStaticCharField(jfieldID field) const noexcept;
		jchar getStaticCharField(const char* name) const noexcept;
		void setStaticCharField(jfieldID field, jchar value) const noexcept;
		void setStaticCharField(const char* name, jchar value) const noexcept;
		jshort getShortField(jfieldID field, jobject _this) const noexcept;
		jshort getShortField(const char* name, jobject _this) const noexcept;
		void setShortField(jfieldID field, jobject _this, jshort value) const noexcept;
		void setShortField(const char* name, jobject _this, jshort value) const noexcept;
		jshort getStaticShortField(jfieldID field) const noexcept;
		jshort getStaticShortField(const char* name) const noexcept;
		void setStaticShortField(jfieldID field, jshort value) const noexcept;
		void setStaticShortField(const char* name, jshort value) const noexcept;
		jint getIntField(jfieldID field, jobject _this) const noexcept;
		jint getIntField(const char* name, jobject _this) const noexcept;
		void setIntField(jfieldID field, jobject _this, jint value) const noexcept;
		void setIntField(const char* name, jobject _this, jint value) const noexcept;
		jint getStaticIntField(jfieldID field) const noexcept;
		jint getStaticIntField(const char* name) const noexcept;
		void setStaticIntField(jfieldID field, jint value) const noexcept;
		void setStaticIntField(const char* name, jint value) const noexcept;
		jlong getLongField(jfieldID field, jobject _this) const noexcept;
		jlong getLongField(const char* name, jobject _this) const noexcept;
		void setLongField(jfieldID field, jobject _this, jlong value) const noexcept;
		void setLongField(const char* name, jobject _this, jlong value) const noexcept;
		jlong getStaticLongField(jfieldID field) const noexcept;
		jlong getStaticLongField(const char* name) const noexcept;
		void setStaticLongField(jfieldID field, jlong value) const noexcept;
		void setStaticLongField(const char* name, jlong value) const noexcept;
		jfloat getFloatField(jfieldID field, jobject _this) const noexcept;
		jfloat getFloatField(const char* name, jobject _this) const noexcept;
		void setFloatField(jfieldID field, jobject _this, jfloat value) const noexcept;
		void setFloatField(const char* name, jobject _this, jfloat value) const noexcept;
		jfloat getStaticFloatField(jfieldID field) const noexcept;
		jfloat getStaticFloatField(const char* name) const noexcept;
		void setStaticFloatField(jfieldID field, jfloat value) const noexcept;
		void setStaticFloatField(const char* name, jfloat value) const noexcept;
		jdouble getDoubleField(jfieldID field, jobject _this) const noexcept;
		jdouble getDoubleField(const char* name, jobject _this) const noexcept;
		void setDoubleField(jfieldID field, jobject _this, jdouble value) const noexcept;
		void setDoubleField(const char* name, jobject _this, jdouble value) const noexcept;
		jdouble getStaticDoubleField(jfieldID field) const noexcept;
		jdouble getStaticDoubleField(const char* name) const noexcept;
		void setStaticDoubleField(jfieldID field, jdouble value) const noexcept;
		void setStaticDoubleField(const char* name, jdouble value) const noexcept;

		String getStringField(jfieldID field, jobject _this) const noexcept;
		String getStringField(const char* name, jobject _this) const noexcept;
		String getStaticStringField(jfieldID field) const noexcept;
		String getStaticStringField(const char* name) const noexcept;
		void setStringField(jfieldID field, jobject _this, const StringParam& value) const noexcept;
		void setStringField(const char* name, jobject _this, const StringParam& value) const noexcept;
		void setStaticStringField(jfieldID field, const StringParam& value) const noexcept;
		void setStaticStringField(const char* name, const StringParam& value) const noexcept;

		sl_bool registerNative(const char* name, const char* sig, const void* fn) const noexcept;

	};

	class SLIB_EXPORT JniPreserveExceptionScope
	{
	public:
		JniPreserveExceptionScope() noexcept;

		~JniPreserveExceptionScope();
	};


	namespace priv
	{
		namespace java
		{

			class SLIB_EXPORT JClass
			{
			public:
				JClass(const char* name, sl_bool flagOptional = sl_false) noexcept;

			public:
				jclass get() const noexcept;

			public:
				const char* name;
				JniClass cls;
				sl_bool flagOptional;
			};

			class SLIB_EXPORT JNativeMethod
			{
			public:
				JNativeMethod(priv::java::JClass* gcls, const char* name, const char* sig, const void* fn) noexcept;

			public:
				priv::java::JClass*gcls;
				const char* name;
				const char* sig;
				const void* fn;
			};

			class SLIB_EXPORT JMethod
			{
			public:
				JMethod(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional = sl_false) noexcept;

			public:
				jobject callObject(jobject _this, ...) noexcept;
				jboolean callBoolean(jobject _this, ...) noexcept;
				jbyte callByte(jobject _this, ...) noexcept;
				jchar callChar(jobject _this, ...) noexcept;
				jshort callShort(jobject _this, ...) noexcept;
				jint callInt(jobject _this, ...) noexcept;
				jlong callLong(jobject _this, ...) noexcept;
				jfloat callFloat(jobject _this, ...) noexcept;
				jdouble callDouble(jobject _this, ...) noexcept;
				void call(jobject _this, ...) noexcept;
				String callString(jobject _this, ...) noexcept;
				jobject newObject(jobject _null, ...) noexcept;

			public:
				JClass* gcls;
				const char* name;
				const char* sig;
				jclass cls;
				jmethodID id;
				sl_bool flagOptional;
			};

			class SLIB_EXPORT JStaticMethod
			{
			public:
				JStaticMethod(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional = sl_false) noexcept;

			public:
				jobject callObject(jobject _null, ...) noexcept;
				jboolean callBoolean(jobject _null, ...) noexcept;
				jbyte callByte(jobject _null, ...) noexcept;
				jchar callChar(jobject _null, ...) noexcept;
				jshort callShort(jobject _null, ...) noexcept;
				jint callInt(jobject _null, ...) noexcept;
				jlong callLong(jobject _null, ...) noexcept;
				jfloat callFloat(jobject _null, ...) noexcept;
				jdouble callDouble(jobject _null, ...) noexcept;
				void call(jobject _null, ...) noexcept;
				String callString(jobject _null, ...) noexcept;

			public:
				JClass* gcls;
				const char* name;
				const char* sig;
				jclass cls;
				jmethodID id;
				sl_bool flagOptional;
			};

			class SLIB_EXPORT JField
			{
			public:
				JField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional = sl_false) noexcept;

			public:
				jobject getObject(jobject _this) noexcept;
				void setObject(jobject _this, jobject value) noexcept;
				jboolean getBoolean(jobject _this) noexcept;
				void setBoolean(jobject _this, jboolean value) noexcept;
				jbyte getByte(jobject _this) noexcept;
				void setByte(jobject _this, jbyte value) noexcept;
				jchar getChar(jobject _this) noexcept;
				void setChar(jobject _this, jchar value) noexcept;
				jshort getShort(jobject _this) noexcept;
				void setShort(jobject _this, jshort value) noexcept;
				jint getInt(jobject _this) noexcept;
				void setInt(jobject _this, jint value) noexcept;
				jlong getLong(jobject _this) noexcept;
				void setLong(jobject _this, jlong value) noexcept;
				jfloat getFloat(jobject _this) noexcept;
				void setFloat(jobject _this, jfloat value) noexcept;
				jdouble getDouble(jobject _this) noexcept;
				void setDouble(jobject _this, jdouble value) noexcept;
				String getString(jobject _this) noexcept;
				void setString(jobject _this, const StringParam& value) noexcept;

			public:
				JClass* gcls;
				const char* name;
				const char* sig;
				jclass cls;
				jfieldID id;
				sl_bool flagOptional;
			};


			class SLIB_EXPORT JObjectField : protected JField
			{
			public:
				JObjectField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional = sl_false) noexcept;
			public:
				jobject get(jobject _this) noexcept;
				void set(jobject _this, jobject value) noexcept;
			};

			class SLIB_EXPORT JStaticField
			{
			public:
				JStaticField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional = sl_false) noexcept;

			public:
				jobject getObject() noexcept;
				void setObject(jobject value) noexcept;
				jboolean getBoolean() noexcept;
				void setBoolean(jboolean value) noexcept;
				jbyte getByte() noexcept;
				void setByte(jbyte value) noexcept;
				jchar getChar() noexcept;
				void setChar(jchar value) noexcept;
				jshort getShort() noexcept;
				void setShort(jshort value) noexcept;
				jint getInt() noexcept;
				void setInt(jint value) noexcept;
				jlong getLong() noexcept;
				void setLong(jlong value) noexcept;
				jfloat getFloat() noexcept;
				void setFloat(jfloat value) noexcept;
				jdouble getDouble() noexcept;
				void setDouble(jdouble value) noexcept;
				String getString() noexcept;
				void setString(const StringParam& value) noexcept;

			public:
				JClass* gcls;
				const char* name;
				const char* sig;
				jclass cls;
				jfieldID id;
				sl_bool flagOptional;
			};

			class SLIB_EXPORT JStaticObjectField : protected JStaticField
			{
			public:
				JStaticObjectField(JClass* gcls, const char* name, const char* sig, sl_bool flagOptional = sl_false) noexcept;
			public:
				jobject get() noexcept;
				void set(jobject value) noexcept;
			};

		}
	}

	#define SLIB_JNI_BEGIN_CLASS(CLASS, NAME) \
		namespace CLASS \
		{ \
			static slib::priv::java::JClass _gcls(NAME); \
			SLIB_INLINE slib::JniClass get() noexcept { \
				return _gcls.cls; \
			} \
			SLIB_INLINE jclass getClass() noexcept { \
				return _gcls.get(); \
			}

	#define SLIB_JNI_BEGIN_CLASS_OPTIONAL(CLASS, NAME) \
		namespace CLASS \
		{ \
			static slib::priv::java::JClass _gcls(NAME, sl_true); \
			SLIB_INLINE slib::JniClass get() noexcept { \
				return _gcls.cls; \
			} \
			SLIB_INLINE jclass getClass() noexcept { \
				return _gcls.get(); \
			}

	#define SLIB_JNI_END_CLASS \
		}

	#define SLIB_JNI_BEGIN_CLASS_SECTION(CLASS) \
		namespace CLASS \
		{ \

	#define SLIB_JNI_END_CLASS_SECTION \
		}

	#define SLIB_JNI_NEW(VAR, SIG) static slib::priv::java::JMethod VAR(&_gcls, "<init>", SIG);
	#define SLIB_JNI_NEW_OPTIONAL(VAR, SIG) static slib::priv::java::JMethod VAR(&_gcls, "<init>", SIG, sl_true);

	#define SLIB_JNI_METHOD(VAR, NAME, SIG) static slib::priv::java::JMethod VAR(&_gcls, NAME, SIG);
	#define SLIB_JNI_METHOD_OPTIONAL(VAR, NAME, SIG) static slib::priv::java::JMethod VAR(&_gcls, NAME, SIG, sl_true);
	#define SLIB_JNI_STATIC_METHOD(VAR, NAME, SIG) static slib::priv::java::JStaticMethod VAR(&_gcls, NAME, SIG);
	#define SLIB_JNI_STATIC_METHOD_OPTIONAL(VAR, NAME, SIG) static slib::priv::java::JStaticMethod VAR(&_gcls, NAME, SIG, sl_true);

	#define SLIB_JNI_FIELD(VAR, NAME, SIG) static slib::priv::java::JField VAR(&_gcls, NAME, SIG);
	#define SLIB_JNI_OBJECT_FIELD(VAR, SIG) static slib::priv::java::JObjectField VAR(&_gcls, (#VAR), SIG);
	#define SLIB_JNI_BOOLEAN_FIELD(VAR) static slib::priv::java::JBooleanField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_BYTE_FIELD(VAR) static slib::priv::java::JByteField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_CHAR_FIELD(VAR) static slib::priv::java::JCharField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_SHORT_FIELD(VAR) static slib::priv::java::JShortField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_INT_FIELD(VAR) static slib::priv::java::JIntField VAR(&_gcls, (#VAR));;
	#define SLIB_JNI_LONG_FIELD(VAR) static slib::priv::java::JLongField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FLOAT_FIELD(VAR) static slib::priv::java::JFloatField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_DOUBLE_FIELD(VAR) static slib::priv::java::JDoubleField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STRING_FIELD(VAR) static slib::priv::java::JStringField VAR(&_gcls, (#VAR));

	#define SLIB_JNI_FIELD_OPTIONAL(VAR, NAME, SIG) static slib::priv::java::JField VAR(&_gcls, NAME, SIG, sl_true);
	#define SLIB_JNI_OBJECT_FIELD_OPTIONAL(VAR, SIG) static slib::priv::java::JObjectField VAR(&_gcls, (#VAR), SIG, sl_true);
	#define SLIB_JNI_BOOLEAN_FIELD_OPTIONAL(VAR) static slib::priv::java::JBooleanField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_BYTE_FIELD_OPTIONAL(VAR) static slib::priv::java::JByteField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_CHAR_FIELD_OPTIONAL(VAR) static slib::priv::java::JCharField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_SHORT_FIELD_OPTIONAL(VAR) static slib::priv::java::JShortField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_INT_FIELD_OPTIONAL(VAR) static slib::priv::java::JIntField VAR(&_gcls, (#VAR), sl_true);;
	#define SLIB_JNI_LONG_FIELD_OPTIONAL(VAR) static slib::priv::java::JLongField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_FLOAT_FIELD_OPTIONAL(VAR) static slib::priv::java::JFloatField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_DOUBLE_FIELD_OPTIONAL(VAR) static slib::priv::java::JDoubleField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STRING_FIELD_OPTIONAL(VAR) static slib::priv::java::JStringField VAR(&_gcls, (#VAR), sl_true);

	#define SLIB_JNI_STATIC_FIELD(VAR, NAME, SIG) static slib::priv::java::JStaticField VAR(&_gcls, NAME, SIG);
	#define SLIB_JNI_STATIC_OBJECT_FIELD(VAR, SIG) static slib::priv::java::JStaticObjectField VAR(&_gcls, (#VAR), SIG);
	#define SLIB_JNI_STATIC_BOOLEAN_FIELD(VAR) static slib::priv::java::JStaticBooleanField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_BYTE_FIELD(VAR) static slib::priv::java::JStaticByteField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_CHAR_FIELD(VAR) static slib::priv::java::JStaticCharField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_SHORT_FIELD(VAR) static slib::priv::java::JStaticShortField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_INT_FIELD(VAR) static slib::priv::java::JStaticIntField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_LONG_FIELD(VAR) static slib::priv::java::JStaticLongField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_FLOAT_FIELD(VAR) static slib::priv::java::JStaticFloatField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_DOUBLE_FIELD(VAR) static slib::priv::java::JStaticDoubleField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_STATIC_STRING_FIELD(VAR) static slib::priv::java::JStaticStringField VAR(&_gcls, (#VAR));

	#define SLIB_JNI_STATIC_FIELD_OPTIONAL(VAR, NAME, SIG) static slib::priv::java::JStaticField VAR(&_gcls, NAME, SIG, sl_true);
	#define SLIB_JNI_STATIC_OBJECT_FIELD_OPTIONAL(VAR, SIG) static slib::priv::java::JStaticObjectField VAR(&_gcls, (#VAR), SIG, sl_true);
	#define SLIB_JNI_STATIC_BOOLEAN_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticBooleanField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_BYTE_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticByteField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_CHAR_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticCharField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_SHORT_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticShortField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_INT_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticIntField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_LONG_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticLongField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_FLOAT_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticFloatField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_DOUBLE_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticDoubleField VAR(&_gcls, (#VAR), sl_true);
	#define SLIB_JNI_STATIC_STRING_FIELD_OPTIONAL(VAR) static slib::priv::java::JStaticStringField VAR(&_gcls, (#VAR), sl_true);

	#define SLIB_JNI_NATIVE(VAR, NAME, SIG, fn) static slib::priv::java::JNativeMethod native_##VAR(&_gcls, NAME, SIG, (const void*)(fn));
	#define SLIB_JNI_NATIVE_IMPL(VAR, NAME, SIG, RET, ...) \
		static RET JNICALL JNativeMethodImpl_##VAR(JNIEnv* env, jobject _this, ##__VA_ARGS__) noexcept; \
		static slib::priv::java::JNativeMethod native_##VAR(&_gcls, NAME, SIG, (const void*)(JNativeMethodImpl_##VAR)); \
		RET JNICALL JNativeMethodImpl_##VAR(JNIEnv* env, jobject _this, ##__VA_ARGS__) noexcept


	#define PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(TYPE, NAME) \
		namespace priv { \
			namespace java { \
				class J##NAME##Field : protected JField \
				{ \
				public: \
					J##NAME##Field(JClass* gcls, const char* name, sl_bool flagOptional = sl_false) noexcept; \
					TYPE get(jobject _this) noexcept; \
					void set(jobject _this, TYPE value) noexcept; \
				}; \
			} \
		}

	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(jboolean, Boolean)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(sl_int8, Byte)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(sl_uint16, Char)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(sl_int16, Short)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(sl_int32, Int)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(sl_int64, Long)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(float, Float)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(double, Double)
	PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(String, String)

	#define PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(TYPE, NAME) \
		namespace priv { \
			namespace java { \
				class JStatic##NAME##Field : protected JStaticField \
				{ \
				public: \
					JStatic##NAME##Field(JClass* gcls, const char* name, sl_bool flagOptional = sl_false) noexcept; \
					TYPE get() noexcept; \
					void set(TYPE value) noexcept; \
				}; \
			} \
		}

	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(jboolean, Boolean)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(sl_int8, Byte)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(sl_uint16, Char)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(sl_int16, Short)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(sl_int32, Int)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(sl_int64, Long)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(float, Float)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(double, Double)
	PRIV_SLIB_JNI_DECLARE_STATIC_FIELD_TYPE(String, String)

}

#endif

#endif
