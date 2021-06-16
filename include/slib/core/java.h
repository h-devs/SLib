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

#include "string.h"
#include "unique_ptr.h"
#include "shared_ptr.h"
#include "primitive_wrapper.h"

#include <jni.h>

/*
	JNI Signature

		Z -> boolean
		B -> byte
		C -> char
		S -> short
		I -> int
		J -> long long
		F -> float
		D -> double
		V -> void
		L<class-name>; -> object
		[<type> -> type[]
		(<arg-types>)<ret-type> -> method
		<Package>/.../<Name> -> class
		<Package>/.../<ParentName>$<Name> -> inner class
*/

namespace slib
{

	template <class T>
	class JniLocal;

	class SLIB_EXPORT Jni
	{
	public:
		static void initialize(JavaVM* jvm) noexcept;

		static void setSharedJVM(JavaVM* jvm) noexcept;
		static JavaVM* getSharedJVM() noexcept;

		// thread-storage
		static JNIEnv* getCurrent() noexcept;
		static void setCurrent(JNIEnv* env) noexcept;

		// attach and set current
		static JNIEnv* attachThread(JavaVM* jvm = sl_null) noexcept;
		static void detachThread(JavaVM* jvm = sl_null) noexcept;

		// class
		static JniLocal<jclass> findClass(const StringParam& className) noexcept;
		static jclass getClass(const StringParam& className) noexcept;

		// method & field
		static jmethodID getMethodID(jclass cls, const char* name, const char* sig) noexcept;
		static jmethodID getStaticMethodID(jclass cls, const char* name, const char* sig) noexcept;
		static jfieldID getFieldID(jclass cls, const char* name, const char* sig) noexcept;
		static jfieldID getStaticFieldID(jclass cls, const char* name, const char* sig) noexcept;

		static JniLocal<jobject> newObject(jclass cls, jmethodID method, ...) noexcept;
		static JniLocal<jobject> newObject(jclass cls, const char* sigConstructor, ...) noexcept;
		static JniLocal<jobject> newObject(jclass cls) noexcept;

		static JniLocal<jobject> callObjectMethod(jobject _this, jmethodID method, ...) noexcept;
		static JniLocal<jobject> callObjectMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static JniLocal<jobject> callStaticObjectMethod(jclass cls, jmethodID method, ...) noexcept;
		static JniLocal<jobject> callStaticObjectMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jboolean callBooleanMethod(jobject _this, jmethodID method, ...) noexcept;
		static jboolean callBooleanMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jboolean callStaticBooleanMethod(jclass cls, jmethodID method, ...) noexcept;
		static jboolean callStaticBooleanMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jbyte callByteMethod(jobject _this, jmethodID method, ...) noexcept;
		static jbyte callByteMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jbyte callStaticByteMethod(jclass cls, jmethodID method, ...) noexcept;
		static jbyte callStaticByteMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jchar callCharMethod(jobject _this, jmethodID method, ...) noexcept;
		static jchar callCharMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jchar callStaticCharMethod(jclass cls, jmethodID method, ...) noexcept;
		static jchar callStaticCharMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jshort callShortMethod(jobject _this, jmethodID method, ...) noexcept;
		static jshort callShortMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jshort callStaticShortMethod(jclass cls, jmethodID method, ...) noexcept;
		static jshort callStaticShortMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jint callIntMethod(jobject _this, jmethodID method, ...) noexcept;
		static jint callIntMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jint callStaticIntMethod(jclass cls, jmethodID method, ...) noexcept;
		static jint callStaticIntMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jlong callLongMethod(jobject _this, jmethodID method, ...) noexcept;
		static jlong callLongMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jlong callStaticLongMethod(jclass cls, jmethodID method, ...) noexcept;
		static jlong callStaticLongMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jfloat callFloatMethod(jobject _this, jmethodID method, ...) noexcept;
		static jfloat callFloatMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jfloat callStaticFloatMethod(jclass cls, jmethodID method, ...) noexcept;
		static jfloat callStaticFloatMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static jdouble callDoubleMethod(jobject _this, jmethodID method, ...) noexcept;
		static jdouble callDoubleMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static jdouble callStaticDoubleMethod(jclass cls, jmethodID method, ...) noexcept;
		static jdouble callStaticDoubleMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static void callVoidMethod(jobject _this, jmethodID method, ...) noexcept;
		static void callVoidMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static void callStaticVoidMethod(jclass cls, jmethodID method, ...) noexcept;
		static void callStaticVoidMethod(jclass cls, const char* name, const char* sig, ...) noexcept;
		static String callStringMethod(jobject _this, jmethodID method, ...) noexcept;
		static String callStringMethod(jobject _this, const char* name, const char* sig, ...) noexcept;
		static String callStaticStringMethod(jclass cls, jmethodID method, ...) noexcept;
		static String callStaticStringMethod(jclass cls, const char* name, const char* sig, ...) noexcept;

		static JniLocal<jobject> getObjectField(jobject _this, jfieldID field) noexcept;
		static JniLocal<jobject> getObjectField(jobject _this, const char* name, const char* sig) noexcept;
		static void setObjectField(jobject _this, jfieldID field, jobject value) noexcept;
		static void setObjectField(jobject _this, const char* name, const char* sig, jobject value) noexcept;
		static JniLocal<jobject> getStaticObjectField(jclass cls, jfieldID field) noexcept;
		static JniLocal<jobject> getStaticObjectField(jclass cls, const char* name, const char* sig) noexcept;
		static void setStaticObjectField(jclass cls, jfieldID field, jobject value) noexcept;
		static void setStaticObjectField(jclass cls, const char* name, const char* sig, jobject value) noexcept;
		static jboolean getBooleanField(jobject _this, jfieldID field) noexcept;
		static jboolean getBooleanField(jobject _this, const char* name) noexcept;
		static void setBooleanField(jobject _this, jfieldID field, jboolean value) noexcept;
		static void setBooleanField(jobject _this, const char* name, jboolean value) noexcept;
		static jboolean getStaticBooleanField(jclass cls, jfieldID field) noexcept;
		static jboolean getStaticBooleanField(jclass cls, const char* name) noexcept;
		static void setStaticBooleanField(jclass cls, jfieldID field, jboolean value) noexcept;
		static void setStaticBooleanField(jclass cls, const char* name, jboolean value) noexcept;
		static jbyte getByteField(jobject _this, jfieldID field) noexcept;
		static jbyte getByteField(jobject _this, const char* name) noexcept;
		static void setByteField(jobject _this, jfieldID field, jbyte value) noexcept;
		static void setByteField(jobject _this, const char* name, jbyte value) noexcept;
		static jbyte getStaticByteField(jclass cls, jfieldID field) noexcept;
		static jbyte getStaticByteField(jclass cls, const char* name) noexcept;
		static void setStaticByteField(jclass cls, jfieldID field, jbyte value) noexcept;
		static void setStaticByteField(jclass cls, const char* name, jbyte value) noexcept;
		static jchar getCharField(jobject _this, jfieldID field) noexcept;
		static jchar getCharField(jobject _this, const char* name) noexcept;
		static void setCharField(jobject _this, jfieldID field, jchar value) noexcept;
		static void setCharField(jobject _this, const char* name, jchar value) noexcept;
		static jchar getStaticCharField(jclass cls, jfieldID field) noexcept;
		static jchar getStaticCharField(jclass cls, const char* name) noexcept;
		static void setStaticCharField(jclass cls, jfieldID field, jchar value) noexcept;
		static void setStaticCharField(jclass cls, const char* name, jchar value) noexcept;
		static jshort getShortField(jobject _this, jfieldID field) noexcept;
		static jshort getShortField(jobject _this, const char* name) noexcept;
		static void setShortField(jobject _this, jfieldID field, jshort value) noexcept;
		static void setShortField(jobject _this, const char* name, jshort value) noexcept;
		static jshort getStaticShortField(jclass cls, jfieldID field) noexcept;
		static jshort getStaticShortField(jclass cls, const char* name) noexcept;
		static void setStaticShortField(jclass cls, jfieldID field, jshort value) noexcept;
		static void setStaticShortField(jclass cls, const char* name, jshort value) noexcept;
		static jint getIntField(jobject _this, jfieldID field) noexcept;
		static jint getIntField(jobject _this, const char* name) noexcept;
		static void setIntField(jobject _this, jfieldID field, jint value) noexcept;
		static void setIntField(jobject _this, const char* name, jint value) noexcept;
		static jint getStaticIntField(jclass cls, jfieldID field) noexcept;
		static jint getStaticIntField(jclass cls, const char* name) noexcept;
		static void setStaticIntField(jclass cls, jfieldID field, jint value) noexcept;
		static void setStaticIntField(jclass cls, const char* name, jint value) noexcept;
		static jlong getLongField(jobject _this, jfieldID field) noexcept;
		static jlong getLongField(jobject _this, const char* name) noexcept;
		static void setLongField(jobject _this, jfieldID field, jlong value) noexcept;
		static void setLongField(jobject _this, const char* name, jlong value) noexcept;
		static jlong getStaticLongField(jclass cls, jfieldID field) noexcept;
		static jlong getStaticLongField(jclass cls, const char* name) noexcept;
		static void setStaticLongField(jclass cls, jfieldID field, jlong value) noexcept;
		static void setStaticLongField(jclass cls, const char* name, jlong value) noexcept;
		static jfloat getFloatField(jobject _this, jfieldID field) noexcept;
		static jfloat getFloatField(jobject _this, const char* name) noexcept;
		static void setFloatField(jobject _this, jfieldID field, jfloat value) noexcept;
		static void setFloatField(jobject _this, const char* name, jfloat value) noexcept;
		static jfloat getStaticFloatField(jclass cls, jfieldID field) noexcept;
		static jfloat getStaticFloatField(jclass cls, const char* name) noexcept;
		static void setStaticFloatField(jclass cls, jfieldID field, jfloat value) noexcept;
		static void setStaticFloatField(jclass cls, const char* name, jfloat value) noexcept;
		static jdouble getDoubleField(jobject _this, jfieldID field) noexcept;
		static jdouble getDoubleField(jobject _this, const char* name) noexcept;
		static void setDoubleField(jobject _this, jfieldID field, jdouble value) noexcept;
		static void setDoubleField(jobject _this, const char* name, jdouble value) noexcept;
		static jdouble getStaticDoubleField(jclass cls, jfieldID field) noexcept;
		static jdouble getStaticDoubleField(jclass cls, const char* name) noexcept;
		static void setStaticDoubleField(jclass cls, jfieldID field, jdouble value) noexcept;
		static void setStaticDoubleField(jclass cls, const char* name, jdouble value) noexcept;

		static String getStringField(jobject _this, jfieldID field) noexcept;
		static String getStringField(jobject _this, const char* name) noexcept;
		static void setStringField(jobject _this, jfieldID field, const StringParam& value) noexcept;
		static void setStringField(jobject _this, const char* name, const StringParam& value) noexcept;
		static String getStaticStringField(jclass cls, jfieldID field) noexcept;
		static String getStaticStringField(jclass cls, const char* name) noexcept;
		static void setStaticStringField(jclass cls, jfieldID field, const StringParam& value) noexcept;
		static void setStaticStringField(jclass cls, const char* name, const StringParam& value) noexcept;

		static sl_bool registerNative(jclass cls, const char* name, const char* sig, const void* fn) noexcept;

		// object
		static JniLocal<jclass> getObjectClass(jobject obj) noexcept;
		static sl_bool isInstanceOf(jobject obj, jclass cls) noexcept;
		static sl_bool isSameObject(jobject ref1, jobject ref2) noexcept;

		static jobjectRefType getRefType(jobject obj) noexcept;
		static sl_bool isInvalidRef(jobject obj) noexcept;

		static sl_bool isLocalRef(jobject obj) noexcept;
		static JniLocal<jobject> newLocalRef(jobject obj) noexcept;
		static void deleteLocalRef(jobject obj) noexcept;

		static sl_bool isGlobalRef(jobject obj) noexcept;
		static JniLocal<jobject> newGlobalRef(jobject obj) noexcept;
		static void deleteGlobalRef(jobject obj) noexcept;

		static sl_bool isWeakRef(jobject obj) noexcept;
		static JniLocal<jobject> newWeakRef(jobject obj) noexcept;
		static void deleteWeakRef(jobject obj) noexcept;

		// string
		static JniLocal<jstring> getJniString(const StringParam& str) noexcept;
		static JniLocal<jstring> getJniString(const sl_char16* str, const sl_size length) noexcept;
		static String getString(jstring str) noexcept;

		/*
		 * Array release<TYPE>ArrayElements Mode
		 * 0 - commit and free
		 * JNI_COMMIT - commit only
		 * JNI_ABORT - free only
		 */
		static sl_uint32 getArrayLength(jarray array) noexcept;
		static JniLocal<jobjectArray> newObjectArray(jclass clsElement, sl_uint32 length) noexcept;
		static JniLocal<jobject> getObjectArrayElement(jobjectArray array, sl_uint32 index) noexcept;
		static void setObjectArrayElement(jobjectArray array, sl_uint32 index, jobject value) noexcept;
		static JniLocal<jobjectArray> newStringArray(sl_uint32 length) noexcept;
		static String getStringArrayElement(jobjectArray array, sl_uint32 index) noexcept;
		static void setStringArrayElement(jobjectArray array, sl_uint32 index, const StringParam& value) noexcept;
		static JniLocal<jbooleanArray> newBooleanArray(sl_uint32 length) noexcept;
		static jboolean* getBooleanArrayElements(jbooleanArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseBooleanArrayElements(jbooleanArray array, jboolean* buf, jint mode = 0) noexcept;
		static void getBooleanArrayRegion(jbooleanArray array, sl_uint32 index, sl_uint32 len, jboolean* buf) noexcept;
		static void setBooleanArrayRegion(jbooleanArray array, sl_uint32 index, sl_uint32 len, jboolean* buf) noexcept;
		static JniLocal<jbyteArray> newByteArray(sl_uint32 length) noexcept;
		static jbyte* getByteArrayElements(jbyteArray array, jboolean* isCopy) noexcept;
		static void releaseByteArrayElements(jbyteArray array, jbyte* buf, jint mode = 0) noexcept;
		static void getByteArrayRegion(jbyteArray array, sl_uint32 index, sl_uint32 len, jbyte* buf) noexcept;
		static void setByteArrayRegion(jbyteArray array, sl_uint32 index, sl_uint32 len, jbyte* buf) noexcept;
		static JniLocal<jcharArray> newCharArray(sl_uint32 length) noexcept;
		static jchar* getCharArrayElements(jcharArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseCharArrayElements(jcharArray array, jchar* buf, jint mode = 0) noexcept;
		static void getCharArrayRegion(jcharArray array, sl_uint32 index, sl_uint32 len, jchar* buf) noexcept;
		static void setCharArrayRegion(jcharArray array, sl_uint32 index, sl_uint32 len, jchar* buf) noexcept;
		static JniLocal<jshortArray> newShortArray(sl_uint32 length) noexcept;
		static jshort* getShortArrayElements(jshortArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseShortArrayElements(jshortArray array, jshort* buf, jint mode = 0) noexcept;
		static void getShortArrayRegion(jshortArray array, sl_uint32 index, sl_uint32 len, jshort* buf) noexcept;
		static void setShortArrayRegion(jshortArray array, sl_uint32 index, sl_uint32 len, jshort* buf) noexcept;
		static JniLocal<jintArray> newIntArray(sl_uint32 length) noexcept;
		static jint* getIntArrayElements(jintArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseIntArrayElements(jintArray array, jint* buf, jint mode = 0) noexcept;
		static void getIntArrayRegion(jintArray array, sl_uint32 index, sl_uint32 len, jint* buf) noexcept;
		static void setIntArrayRegion(jintArray array, sl_uint32 index, sl_uint32 len, jint* buf) noexcept;
		static JniLocal<jlongArray> newLongArray(sl_uint32 length) noexcept;
		static jlong* getLongArrayElements(jlongArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseLongArrayElements(jlongArray array, jlong* buf, jint mode = 0) noexcept;
		static void getLongArrayRegion(jlongArray array, sl_uint32 index, sl_uint32 len, jlong* buf) noexcept;
		static void setLongArrayRegion(jlongArray array, sl_uint32 index, sl_uint32 len, jlong* buf) noexcept;
		static JniLocal<jfloatArray> newFloatArray(sl_uint32 length) noexcept;
		static jfloat* getFloatArrayElements(jfloatArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseFloatArrayElements(jfloatArray array, jfloat* buf, jint mode = 0) noexcept;
		static void getFloatArrayRegion(jfloatArray array, sl_uint32 index, sl_uint32 len, jfloat* buf) noexcept;
		static void setFloatArrayRegion(jfloatArray array, sl_uint32 index, sl_uint32 len, jfloat* buf) noexcept;
		static JniLocal<jdoubleArray> newDoubleArray(sl_uint32 length) noexcept;
		static jdouble* getDoubleArrayElements(jdoubleArray array, jboolean* isCopy = sl_null) noexcept;
		static void releaseDoubleArrayElements(jdoubleArray array, jdouble* buf, jint mode = 0) noexcept;
		static void getDoubleArrayRegion(jdoubleArray array, sl_uint32 index, sl_uint32 len, jdouble* buf) noexcept;
		static void setDoubleArrayRegion(jdoubleArray array, sl_uint32 index, sl_uint32 len, jdouble* buf) noexcept;

		// direct buffer
		static JniLocal<jobject> newDirectByteBuffer(void* address, sl_size capacity) noexcept;
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
		SLIB_DEFINE_UNIQUE_PTR_MEMBERS(JniLocal, T, value, sl_null, Jni::deleteLocalRef)

	public:
		template <class OTHER>
		JniLocal(JniLocal<OTHER>&& other) noexcept
		{
			value = (T)(other.value);
			other.value = sl_null;
		}

		template <class OTHER>
		JniLocal& operator=(JniLocal<OTHER>&& other) noexcept
		{
			Jni::deleteLocalRef(value);
			value = (T)(other.value);
			other.value = sl_null;
			return *this;
		}

	};

	template <class T>
	class SLIB_EXPORT JniGlobal
	{
		SLIB_DEFINE_UNIQUE_PTR_MEMBERS_NO_ASSIGN(JniGlobal, T, value, sl_null, Jni::deleteGlobalRef)
		SLIB_DEFINE_UNIQUE_PTR_ATOMIC_MEMBERS(JniGlobal, T, value, sl_null, Jni::deleteGlobalRef)

	public:		
		JniGlobal(T _value) noexcept
		{
			value = (T)(Jni::newGlobalRef(_value));
		}

		JniGlobal& operator=(T _value) noexcept
		{
			Jni::deleteGlobalRef(value);
			value = (T)(Jni::newGlobalRef(_value));
			return *this;
		}

		template <class OTHER>
		JniGlobal(JniGlobal<OTHER>&& other) noexcept
		{
			value = (T)(other.value);
			other.value = sl_null;
		}

		template <class OTHER>
		JniGlobal& operator=(JniGlobal<OTHER>&& other) noexcept
		{
			Jni::deleteLocalRef(value);
			value = (T)(other.value);
			other.value = sl_null;
			return *this;
		}

		template <class OTHER>
		JniGlobal(const JniLocal<OTHER>& other) noexcept
		{
			value = (T)(Jni::newGlobalRef(other.value));
		}

		template <class OTHER>
		JniGlobal& operator=(const JniLocal<OTHER>& other) noexcept
		{
			Jni::deleteGlobalRef(value);
			value = (T)(Jni::newGlobalRef(other.value));
			return *this;
		}

	};

	template <class T>
	class SLIB_EXPORT Atomic< JniGlobal<T> >
	{
		SLIB_DEFINE_ATOMIC_UNIQUE_PTR_MEMBERS(JniGlobal<T>, T, value, sl_null, Jni::deleteGlobalRef)		
	};

	template <class T>
	using AtomicJniGlobal = Atomic< JniGlobal<T> >;

	class SLIB_EXPORT JniStringConstant
	{
	public:
		JniStringConstant(const sl_char16* sz) noexcept;

		~JniStringConstant();
	
	public:
		jstring get() noexcept;

	public:
		const sl_char16* content;

	private:
		sl_bool m_flagLoaded;
		SpinLock m_lock;
		JniGlobal<jstring> m_object;
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
				JClass(const char* name) noexcept;

			public:
				jclass get() noexcept;

			public:
				const char* name;

			private:
				sl_bool m_flagLoaded;
				SpinLock m_lock;
				jclass m_cls;
			};

			class SLIB_EXPORT JMethod
			{
			public:
				JMethod(JClass* cls, const char* name, const char* sig) noexcept;

			public:
				jmethodID getId() noexcept;
				JniLocal<jobject> callObject(jobject _this, ...) noexcept;
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
				JniLocal<jobject> newObject(jobject _null, ...) noexcept;

			public:
				JClass* cls;
				const char* name;
				const char* sig;

			private:
				sl_bool m_flagLoaded;
				SpinLock m_lock;
				jmethodID m_id;
			};

			class SLIB_EXPORT JStaticMethod
			{
			public:
				JStaticMethod(JClass* cls, const char* name, const char* sig) noexcept;

			public:
				jmethodID getId() noexcept;
				JniLocal<jobject> callObject(jobject _null, ...) noexcept;
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
				JClass* cls;
				const char* name;
				const char* sig;

			private:
				sl_bool m_flagLoaded;
				SpinLock m_lock;
				jmethodID m_id;
			};

			class SLIB_EXPORT JField
			{
			public:
				JField(JClass* cls, const char* name, const char* sig) noexcept;

			public:
				jfieldID getId() noexcept;
				JniLocal<jobject> getObject(jobject _this) noexcept;
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
				JClass* cls;
				const char* name;
				const char* sig;

			private:
				sl_bool m_flagLoaded;
				SpinLock m_lock;
				jfieldID m_id;
			};

			class SLIB_EXPORT JObjectField : protected JField
			{
			public:
				JObjectField(JClass* cls, const char* name, const char* sig) noexcept;

			public:
				JniLocal<jobject> get(jobject _this) noexcept;
				void set(jobject _this, jobject value) noexcept;
			};

			class SLIB_EXPORT JStringField : protected JField
			{
			public:
				JStringField(JClass* cls, const char* name) noexcept;

			public:
				String get(jobject _this) noexcept;
				void set(jobject _this, const StringParam& value) noexcept;
				JniLocal<jstring> getObject(jobject _this) noexcept;
				void setObject(jobject _this, jstring value) noexcept;
			};

			class SLIB_EXPORT JStaticField
			{
			public:
				JStaticField(JClass* cls, const char* name, const char* sig) noexcept;

			public:
				jfieldID getId() noexcept;
				JniLocal<jobject> getObject() noexcept;
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
				JClass* cls;
				const char* name;
				const char* sig;

			private:
				sl_bool m_flagLoaded;
				SpinLock m_lock;
				jfieldID m_id;
			};

			class SLIB_EXPORT JStaticObjectField : protected JStaticField
			{
			public:
				JStaticObjectField(JClass* cls, const char* name, const char* sig) noexcept;

			public:
				JniLocal<jobject> get() noexcept;
				void set(jobject value) noexcept;
			};
			
			class JStaticStringField : protected JStaticField
			{
			public:
				JStaticStringField(JClass* cls, const char* name) noexcept;

			public:
				String get() noexcept;
				void set(const StringParam& value) noexcept;
				JniLocal<jstring> getObject() noexcept;
				void setObject(jstring value) noexcept;
			};

			class SLIB_EXPORT JFinalObjectField : protected JStaticObjectField
			{
			public:
				JFinalObjectField(JClass* cls, const char* name, const char* sig) noexcept;

			public:
				jobject get() noexcept;

			private:
				sl_bool m_flagLoadedValue;
				SpinLock m_lockValue;
				JniGlobal<jobject> m_value;
			};

			class SLIB_EXPORT JFinalStringObjectField : protected JFinalObjectField
			{
			public:
				JFinalStringObjectField(JClass* cls, const char* name) noexcept;

			public:
				jstring get() noexcept;

			};

			class SLIB_EXPORT JFinalStringField : protected JStaticStringField
			{
			public:
				JFinalStringField(JClass* cls, const char* name) noexcept;

			public:
				String get() noexcept;

			private:
				sl_bool m_flagLoadedValue;
				SpinLock m_lockValue;
				String m_value;
			};

			class SLIB_EXPORT JNativeMethod
			{
			public:
				JNativeMethod(priv::java::JClass* cls, const char* name, const char* sig, const void* fn) noexcept;

			public:
				void doRegister() noexcept;

			public:
				priv::java::JClass* cls;
				const char* name;
				const char* sig;
				const void* fn;
			};

			struct StringConstantContainer
			{
				const sl_char16* content;
				sl_bool flagLoaded;
				sl_int32 lock;
				jstring object;
			};

		}
	}

	#define SLIB_JNI_BEGIN_CLASS(CLASS, NAME) \
		namespace CLASS \
		{ \
			static slib::priv::java::JClass _gcls(NAME); \
			SLIB_INLINE jclass get() noexcept { \
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

	#define SLIB_JNI_METHOD(VAR, NAME, SIG) static slib::priv::java::JMethod VAR(&_gcls, NAME, SIG);
	#define SLIB_JNI_STATIC_METHOD(VAR, NAME, SIG) static slib::priv::java::JStaticMethod VAR(&_gcls, NAME, SIG);

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

	#define SLIB_JNI_FINAL_OBJECT_FIELD(VAR, SIG) static slib::priv::java::JFinalObjectField VAR(&_gcls, (#VAR), SIG);
	#define SLIB_JNI_FINAL_BOOLEAN_FIELD(VAR) static slib::priv::java::JFinalBooleanField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_BYTE_FIELD(VAR) static slib::priv::java::JFinalByteField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_CHAR_FIELD(VAR) static slib::priv::java::JFinalCharField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_SHORT_FIELD(VAR) static slib::priv::java::JFinalShortField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_INT_FIELD(VAR) static slib::priv::java::JFinalIntField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_LONG_FIELD(VAR) static slib::priv::java::JFinalLongField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_FLOAT_FIELD(VAR) static slib::priv::java::JFinalFloatField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_DOUBLE_FIELD(VAR) static slib::priv::java::JFinalDoubleField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_STRING_FIELD(VAR) static slib::priv::java::JFinalStringField VAR(&_gcls, (#VAR));
	#define SLIB_JNI_FINAL_STRING_OBJECT_FIELD(VAR) static slib::priv::java::JFinalStringObjectField VAR(&_gcls, (#VAR));

	#define SLIB_JNI_NATIVE(VAR, NAME, SIG, fn) static slib::priv::java::JNativeMethod native_##VAR(&_gcls, NAME, SIG, (const void*)(fn));
	#define SLIB_JNI_NATIVE_IMPL(VAR, NAME, SIG, RET, ...) \
		static RET JNICALL JNativeMethodImpl_##VAR(JNIEnv* env, jobject _this, ##__VA_ARGS__) noexcept; \
		static slib::priv::java::JNativeMethod native_##VAR(&_gcls, NAME, SIG, (const void*)(JNativeMethodImpl_##VAR)); \
		RET JNICALL JNativeMethodImpl_##VAR(JNIEnv* env, jobject _this, ##__VA_ARGS__) noexcept

	#define SLIB_JNI_STRING(NAME, VALUE) \
		static slib::priv::java::StringConstantContainer _static_jni_string_constant_##NAME = { SLIB_UNICODE(VALUE), sl_false, 0, 0 };
		static const slib::JniStringConstant& NAME = *(reinterpret_cast<slib::JniStringConstant*>(&_static_jni_string_constant_##name));

	#define PRIV_SLIB_JNI_DECLARE_FIELD_TYPE(TYPE, NAME) \
		namespace priv { \
			namespace java { \
				class J##NAME##Field : protected JField \
				{ \
				public: \
					J##NAME##Field(JClass* cls, const char* name) noexcept; \
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
					JStatic##NAME##Field(JClass* cls, const char* name) noexcept; \
					TYPE get() noexcept; \
					void set(TYPE value) noexcept; \
				}; \
				class JFinal##NAME##Field : protected JStatic##NAME##Field \
				{ \
				public: \
					JFinal##NAME##Field(JClass* cls, const char* name) noexcept; \
					TYPE get() noexcept; \
				public: \
					sl_bool m_flagLoadedValue; \
					TYPE m_value; \
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

}

#endif

#endif
