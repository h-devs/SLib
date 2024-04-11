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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_ANDROID)

#include "slib/ui/photo.h"

#include "slib/io/file.h"
#include "slib/core/app.h"
#include "slib/core/safe_static.h"
#include "slib/ui/core.h"
#include "slib/ui/platform.h"
#include "slib/platform/android/context.h"
#include "slib/platform/java/file.h"

namespace slib
{

	namespace {

		static void JNICALL OnCompleteTakePhoto(JNIEnv* env, jobject _this, jstring filePath, jint fd, jint rotation, jboolean flipHorz, jboolean flipVert, jboolean flagCancel);

		SLIB_JNI_BEGIN_CLASS(JTakePhoto, "slib/android/camera/TakePhoto")
			SLIB_JNI_STATIC_METHOD(open, "open", "(Landroid/app/Activity;ZLjava/lang/String;)V");
			SLIB_JNI_NATIVE(onComplete, "navtiveOnComplete", "(Ljava/lang/String;IIZZZ)V", OnCompleteTakePhoto);
		SLIB_JNI_END_CLASS

		class TakePhotoResultEx : public TakePhotoResult
		{
		public:
			void setResult(String path, int fd, RotationMode rotation, sl_bool flipHorz, sl_bool flipVert)
			{
				flagSuccess = sl_false;
				Memory mem;
				if (path.isNotEmpty()) {
					filePath = path;
					mem = File::readAllBytes(path);
				} else {
					File file(fd);
					mem = file.readAllBytes();
					fileContent = mem;
					file.release();
				}
				if (mem.isNull()) {
					return;
				}
				FlipMode flip;
				if (flipHorz) {
					if (flipVert) {
						flip = FlipMode::Both;
					} else {
						flip = FlipMode::Horizontal;
					}
				} else {
					if (flipVert) {
						flip = FlipMode::Vertical;
					} else {
						flip = FlipMode::None;
					}
				}
				NormalizeRotateAndFlip(rotation, flip);
				if (rotation == RotationMode::Rotate0 && flip == FlipMode::None) {
					flagSuccess = sl_true;
				} else {
					filePath.setNull();
					fileContent.setNull();
					if (mem.isNotNull()) {
						Ref<Image> image = Image::loadFromMemory(mem);
						if (image.isNotNull()) {
							drawable = image->rotate(rotation, flip);
							if (drawable.isNotNull()) {
								flagSuccess = sl_true;
							}
						}
					}
				}
			}
		};

		class TakePhotoContext
		{
		public:
			Mutex lock;
			Function<void(TakePhotoResult&)> callback;

			void run(TakePhoto& takePhoto, sl_bool flagCamera)
			{
				Function<void(TakePhotoResult&)> oldCallback;
				MutexLocker locker(&lock);
				if (callback.isNotNull()) {
					oldCallback = callback;
					callback.setNull();
				}
				jobject context = Android::getCurrentContext();
				if (context) {
					callback = takePhoto.onComplete;
					locker.unlock();
					if (oldCallback.isNotNull()) {
						TakePhotoResult result;
						oldCallback(result);
					}
					JniLocal<jstring> jpath = Jni::getJniString(takePhoto.outputFilePath);
					JTakePhoto::open.call(sl_null, context, flagCamera, jpath.get());
				} else {
					locker.unlock();
					if (oldCallback.isNotNull()) {
						TakePhotoResult result;
						oldCallback(result);
					}
					if (takePhoto.onComplete.isNotNull()) {
						TakePhotoResult result;
						takePhoto.onComplete(result);
					}
				}
			}

			void onComplete(String filePath, int fd, RotationMode rotation, sl_bool flipHorz, sl_bool flipVert, sl_bool flagCancel)
			{
				Function<void(TakePhotoResult&)> callback;
				{
					MutexLocker locker(&lock);
					callback = this->callback;
					this->callback.setNull();
				}
				if (callback.isNotNull()) {
					TakePhotoResultEx result;
					if (!flagCancel && (filePath.isNotEmpty() || fd != (int)SLIB_FILE_INVALID_HANDLE)) {
						result.setResult(filePath, fd, rotation, flipHorz, flipVert);
					}
					result.flagCancel = flagCancel;
					callback(result);
				}
			}
		};

		SLIB_SAFE_STATIC_GETTER(TakePhotoContext, GetTakePhotoContext);

		static void RunTakePhoto(TakePhoto& takePhoto, sl_bool flagCamera)
		{
			TakePhotoContext* p = GetTakePhotoContext();
			if (p) {
				p->run(takePhoto, flagCamera);
			}
		}

		void JNICALL OnCompleteTakePhoto(JNIEnv* env, jobject _this, jstring filePath, jint fd, jint rotation, jboolean flipHorz, jboolean flipVert, jboolean flagCancel)
		{
			TakePhotoContext* p = GetTakePhotoContext();
			if (p) {
				p->onComplete(Jni::getString(filePath), (int)fd, (slib::RotationMode)rotation, flipHorz, flipVert, flagCancel);
			}
		}

	}

	void TakePhoto::takeFromCamera()
	{
		RunTakePhoto(*this, sl_true);
	}

	void TakePhoto::chooseFromLibrary()
	{
		RunTakePhoto(*this, sl_false);
	}


	void PhotoKit::saveImage(const SaveImageParam& param)
	{
		Application::grantPermissions(AppPermissions::WriteExternalStorage, [param]() {
			if (Application::checkPermissions(AppPermissions::WriteExternalStorage)) {
				Memory content;
				if (param.image.isNotNull()) {
					Ref<Image> image = param.image->toImage();
					if (image.isNotNull()) {
						content = image->saveJpeg();
					}
				} else if (param.content.isNotNull()) {
					content = param.content;
				}
				if (content.isNotNull()) {
					JniLocal<jobject> dir = android::Context::getPicturesDir(Android::getCurrentContext());
					String path = java::File::getAbsolutePath(dir);
					if (path.isNotEmpty()) {
						path += "/" + Time::now().format("%04y-%02m-%02d_%02H%02M%02S.jpg");
						if (File::writeAllBytes(path, content)) {
							param.onComplete(path);
							return;
						}
					}
				}
				param.onComplete(sl_null);
			} else {
				param.onComplete(sl_null);
			}
		});
	}

}

#endif
