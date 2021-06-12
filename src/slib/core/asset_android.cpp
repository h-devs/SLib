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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/core/asset.h"
#include "slib/core/android/asset.h"

#include "slib/core/java.h"
#include "slib/core/android/platform.h"
#include "slib/core/java/input_stream.h"

namespace slib
{

	namespace priv
	{
		namespace android_asset
		{
			
			SLIB_JNI_BEGIN_CLASS(JActivity, "android/app/Activity")
				SLIB_JNI_METHOD(getAssets, "getAssets", "()Landroid/content/res/AssetManager;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JAssetManager, "android/content/res/AssetManager")
				SLIB_JNI_METHOD(open, "open", "(Ljava/lang/String;)Ljava/io/InputStream;")
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::android_asset;

	namespace android
	{

		jobject Assets::open(const StringParam& _path) noexcept
		{
			jobject activity = Android::getCurrentActivity();
			if (activity) {
				JniLocal<jobject> assets = JActivity::getAssets.callObject(activity);
				if (assets.isNotNull()) {
					JniLocal<jstring> path = Jni::getJniString(_path);
					if (path.isNotNull()) {
						return JAssetManager::open.callObject(assets.get(), path.get());
					}
				}
			}
			return sl_null;
		}

		Memory Assets::readAllBytes(const StringParam& path) noexcept
		{
			JniLocal<jobject> stream = open(path);
			if (stream.isNotNull()) {
				return java::InputStream::readAllBytes(stream.get());
			}
			return sl_null;
		}

	}

	sl_bool Assets::isBasedOnFileSystem()
	{
		return sl_false;
	}

	String Assets::getFilePath(const StringParam& path)
	{
		return sl_null;
	}

	Memory Assets::readAllBytes(const StringParam& path)
	{
		return android::Assets::readAllBytes(path);
	}

}

#endif
