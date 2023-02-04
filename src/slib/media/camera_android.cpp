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

#include "slib/media/definition.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/media/camera.h"

#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"
#include "slib/platform.h"

namespace slib
{

	namespace {

		SLIB_JNI_BEGIN_CLASS(JCameraInfo, "slib/android/camera/SCameraInfo")
			SLIB_JNI_STRING_FIELD(id);
			SLIB_JNI_STRING_FIELD(name);
		SLIB_JNI_END_CLASS

		SLIB_JNI_BEGIN_CLASS(JCamera, "slib/android/camera/SCamera")

			SLIB_JNI_STATIC_METHOD(getCameras, "getCameras", "(Landroid/app/Activity;)[Lslib/android/camera/SCameraInfo;");
			SLIB_JNI_STATIC_METHOD(create, "create", "(Landroid/app/Activity;Ljava/lang/String;J)Lslib/android/camera/SCamera;");
			SLIB_JNI_STATIC_METHOD(isMobileDeviceTorchActive, "isMobileDeviceTorchActive", "(Landroid/app/Activity;)Z");
			SLIB_JNI_STATIC_METHOD(setMobileDeviceTorchMode, "setMobileDeviceTorchMode", "(Landroid/app/Activity;IF)V");

			SLIB_JNI_METHOD(release, "release", "()V");
			SLIB_JNI_METHOD(setSettings, "setSettings", "(II)V");
			SLIB_JNI_METHOD(start, "start", "()V");
			SLIB_JNI_METHOD(stop, "stop", "()V");
			SLIB_JNI_METHOD(isRunning, "isRunning", "()Z");
			SLIB_JNI_METHOD(takePicture, "takePicture", "(I)V");
			SLIB_JNI_METHOD(setFocusMode, "setFocusMode", "(I)V");
			SLIB_JNI_METHOD(autoFocus, "autoFocus", "()V");
			SLIB_JNI_METHOD(autoFocusOnPoint, "autoFocusOnPoint", "(FF)V");
			SLIB_JNI_METHOD(isTorchActive, "isTorchActive", "()Z");
			SLIB_JNI_METHOD(setTorchMode, "setTorchMode", "(IF)V");

		SLIB_JNI_END_CLASS

		class CameraImpl;

		typedef CHashMap<jlong, WeakRef<CameraImpl> > CameraMap;

		SLIB_SAFE_STATIC_GETTER(CameraMap, GetCameraMap)

		class CameraImpl : public Camera
		{
		public:
			JniGlobal<jobject> m_camera;

		public:
			CameraImpl()
			{
			}

			~CameraImpl()
			{
				release();
			}

		public:
			static Ref<CameraImpl> _create(const CameraParam& param)
			{
				jobject context = Android::getCurrentContext();
				if (!context) {
					return sl_null;
				}
				CameraMap* cameraMap = GetCameraMap();
				if (!cameraMap) {
					return sl_null;
				}
				Ref<CameraImpl> ret = new CameraImpl();
				if (ret.isNotNull()) {
					jlong instance = (jlong)(ret.get());
					cameraMap->put(instance, ret);
					JniLocal<jstring> jid = Jni::getJniString(param.deviceId);
					JniGlobal<jobject> camera = JCamera::create.callObject(sl_null, context, jid.get(), instance);
					if (camera.isNotNull()) {
						JCamera::setSettings.call(camera, param.preferedFrameWidth, param.preferedFrameHeight);
						ret->_init(param);
						ret->m_camera = Move(camera);
						if (param.flagAutoStart) {
							ret->start();
						}
						return ret;
					}
				}
				return sl_null;
			}

			static Ref<CameraImpl> get(jlong instance)
			{
				CameraMap* cameraMap = GetCameraMap();
				if (!cameraMap) {
					return sl_null;
				}
				WeakRef<CameraImpl> camera;
				cameraMap->get(instance, &camera);
				return camera;
			}

			void release() override
			{
				ObjectLocker lock(this);

				jobject jcamera = m_camera.get();
				if (!jcamera) {
					return;
				}

				JCamera::release.call(jcamera);
				m_camera.setNull();

				CameraMap* cameraMap = GetCameraMap();
				if (cameraMap) {
					cameraMap->remove((jlong)this);
				}
			}

			sl_bool isOpened() override
			{
				return m_camera.isNotNull();
			}

			void start() override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					JCamera::start.call(jcamera);
				}
			}

			void stop() override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					JCamera::stop.call(jcamera);
				}
			}

			sl_bool isRunning() override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					return JCamera::isRunning.callBoolean(jcamera) != 0;
				}
				return sl_false;
			}

			static RotationMode _getRotation(jint rotation)
			{
				switch (rotation) {
					case 90:
						return RotationMode::Rotate90;
					case 180:
						return RotationMode::Rotate180;
					case 270:
						return RotationMode::Rotate270;
					default:
						return RotationMode::Rotate0;
				}
			}

			static FlipMode _getFlip(jint flip)
			{
				switch (flip) {
					case 1:
						return FlipMode::Horizontal;
					case 2:
						return FlipMode::Vertical;
					case 3:
						return FlipMode::Both;
					default:
						return FlipMode::None;
				}
			}

			Memory m_memFrame;

			void _onFrame(jbyteArray jdata, jint width, jint height, jint orientation, jint flip)
			{
				sl_uint32 size = Jni::getArrayLength(jdata);
				Memory mem = m_memFrame;
				if (mem.isNotNull()) {
					if (mem.getSize() != size) {
						mem.setNull();
					}
				}
				if (mem.isNull()) {
					mem = Memory::create(size);
					if (mem.isNull()) {
						return;
					}
					m_memFrame = mem;
				}
				Jni::getByteArrayRegion(jdata, 0, size, (jbyte*)(mem.getData()));
				VideoCaptureFrame frame;
				frame.image.width = (sl_uint32)(width);
				frame.image.height = (sl_uint32)(height);
				frame.image.format = BitmapFormat::YUV_NV21;
				frame.image.data = mem.getData();
				frame.image.pitch = 0;
				frame.image.ref = mem.ref;
				frame.rotation = _getRotation(orientation);
				frame.flip = _getFlip(flip);
				onCaptureVideoFrame(frame);
			}

			void _onFrame2(jint width, jint height, jobject Y, jobject U, jobject V, int rowStrideY, int rowStrideUV, int pixelStrideUV, jint orientation, jint flip)
			{
				JNIEnv* env = Jni::getCurrent();
				if (!env) {
					return;
				}

				sl_uint8* pY = (sl_uint8*)(env->GetDirectBufferAddress(Y));
				sl_uint8* pU = (sl_uint8*)(env->GetDirectBufferAddress(U));
				sl_uint8* pV = (sl_uint8*)(env->GetDirectBufferAddress(V));
				sl_uint32 nY = (sl_uint32)(env->GetDirectBufferCapacity(Y));
				sl_uint32 nU = (sl_uint32)(env->GetDirectBufferCapacity(U));
				sl_uint32 nV = (sl_uint32)(env->GetDirectBufferCapacity(V));

				if (pU == pV + 1) {
					nV = Math::max(nV, 1 + nU);
					nU = 0;
				} else if (pU + 1 == pV) {
					nU = Math::max(nU, 1 + nV);
					nV = 0;
				}
				sl_uint32 size = nY + nU + nV;

				Memory mem = m_memFrame;
				if (mem.isNotNull()) {
					if (mem.getSize() != size) {
						mem.setNull();
					}
				}
				if (mem.isNull()) {
					mem = Memory::create(size);
					if (mem.isNull()) {
						return;
					}
					m_memFrame = mem;
				}

				sl_uint8* data = (sl_uint8*)(mem.getData());
				Base::copyMemory(data, pY, nY);
				if (nU) {
					Base::copyMemory(data + nY, pU, nU);
				}
				if (nV) {
					Base::copyMemory(data + nY + nU, pV, nV);
				}

				VideoCaptureFrame frame;
				frame.image.width = (sl_uint32)(width);
				frame.image.height = (sl_uint32)(height);
				frame.image.format = BitmapFormat::YUV_I420;
				frame.image.pitch = rowStrideY;
				frame.image.pitch1 = rowStrideUV;
				frame.image.sampleStride1 = pixelStrideUV;
				frame.image.pitch2 = rowStrideUV;
				frame.image.sampleStride2 = pixelStrideUV;
				frame.rotation = _getRotation(orientation);
				frame.flip = _getFlip(flip);

				frame.image.data = data;
				frame.image.ref = mem.ref;
				frame.image.data1 = data + nY;
				frame.image.data2 = data + nY + nU;
				if (pU == pV + 1) {
					frame.image.data1 = (char*)(frame.image.data2) + 1;
				} else if (pU + 1 == pV) {
					frame.image.data2 = (char*)(frame.image.data1) + 1;
				}

				onCaptureVideoFrame(frame);
			}

			void takePicture(const CameraTakePictureParam& param) override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (!jcamera || !isRunning()) {
					CameraTakePictureResult result;
					param.onComplete(result);
					return;
				}
				m_queueTakePictureRequests.push(param);
				JCamera::takePicture.call(jcamera, (jint)((int)(param.flashMode)));
			}

			void _onPicture(jbyteArray jdata, jint orientation, jint flip) {
				CameraTakePictureParam param;
				if (m_queueTakePictureRequests.pop(&param)) {
					CameraTakePictureResult result;
					if (jdata) {
						sl_uint32 size = Jni::getArrayLength(jdata);
						Memory mem = Memory::create(size);
						if (mem.isNotNull()) {
							Jni::getByteArrayRegion(jdata, 0, size, (jbyte*)(mem.getData()));
							result.flagSuccess = sl_true;
							result.setJpeg(mem);
							result.rotation = _getRotation(orientation);
							result.flip = _getFlip(flip);
						}
					}
					param.onComplete(result);
				}
			}

			void _onPicture2(jobject jdata, jint orientation, jint flip) {
				CameraTakePictureParam param;
				if (m_queueTakePictureRequests.pop(&param)) {
					CameraTakePictureResult result;
					if (jdata) {
						JNIEnv* env = Jni::getCurrent();
						if (env) {
							sl_uint8* data = (sl_uint8*)(env->GetDirectBufferAddress(jdata));
							sl_uint32 size = env->GetDirectBufferCapacity(jdata);
							Memory mem = Memory::createStatic(data, size);
							if (mem.isNotNull()) {
								result.flagSuccess = sl_true;
								result.setJpeg(mem);
								result.rotation = _getRotation(orientation);
								result.flip = _getFlip(flip);
							}
						}
					}
					param.onComplete(result);
				}
			}

			void onCaptureVideoFrame(VideoCaptureFrame& frame) override
			{
				VideoCapture::onCaptureVideoFrame(frame);
			}

			void setFocusMode(CameraFocusMode mode) override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					JCamera::setFocusMode.call(jcamera, (jint)((int)mode));
				}
			}

			void autoFocus() override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					JCamera::autoFocus.call(jcamera);
				}
			}

			void autoFocusOnPoint(sl_real x, sl_real y) override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					JCamera::autoFocusOnPoint.call(jcamera, (jfloat)x, (jfloat)y);
				}
			}

			sl_bool isTorchActive() override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					return JCamera::isTorchActive.callBoolean(jcamera) != 0;
				}
				return sl_false;
			}

			void setTorchMode(CameraTorchMode mode, float level) override
			{
				ObjectLocker lock(this);
				jobject jcamera = m_camera.get();
				if (jcamera) {
					JCamera::setTorchMode.call(jcamera, (jint)((int)mode), (jfloat)level);
				}
			}

		};


		SLIB_JNI_BEGIN_CLASS_SECTION(JCamera)

			SLIB_JNI_NATIVE_IMPL(nativeOnFrame, "nativeOnFrame", "(J[BIIII)V", void, jlong instance, jbyteArray data, jint width, jint height, jint orientation, jint flip)
			{
				Ref<CameraImpl> camera = CameraImpl::get(instance);
				if (camera.isNotNull()) {
					camera->_onFrame(data, width, height, orientation, flip);
				}
			}

			SLIB_JNI_NATIVE_IMPL(nativeOnFrame2, "nativeOnFrame2", "(JIILjava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;IIIII)V", void, jlong instance, jint width, jint height, jobject Y, jobject U, jobject V, int rowStrideY, int rowStrideUV, int pixelStrideUV, jint orientation, jint flip)
			{
				Ref<CameraImpl> camera = CameraImpl::get(instance);
				if (camera.isNotNull()) {
					camera->_onFrame2(width, height, Y, U, V, rowStrideY, rowStrideUV, pixelStrideUV, orientation, flip);
				}
			}

			SLIB_JNI_NATIVE_IMPL(nativeOnPicture, "nativeOnPicture", "(J[BII)V", void, jlong instance, jbyteArray data, jint orientation, jint flip)
			{
				Ref<CameraImpl> camera = CameraImpl::get(instance);
				if (camera.isNotNull()) {
					camera->_onPicture(data, orientation, flip);
				}
			}

			SLIB_JNI_NATIVE_IMPL(nativeOnPicture2, "nativeOnPicture2", "(JLjava/nio/ByteBuffer;II)V", void, jlong instance, jobject data, jint orientation, jint flip)
			{
				Ref<CameraImpl> camera = CameraImpl::get(instance);
				if (camera.isNotNull()) {
					camera->_onPicture2(data, orientation, flip);
				}
			}

		SLIB_JNI_END_CLASS_SECTION

	}

	Ref<Camera> Camera::create(const CameraParam& param)
	{
		return CameraImpl::_create(param);
	}

	List<CameraInfo> Camera::getCameras()
	{
		jobject context = Android::getCurrentContext();
		if (!context) {
			return sl_null;
		}
		List<CameraInfo> ret;
		JniLocal<jobjectArray> arr = JCamera::getCameras.callObject(sl_null, context);
		sl_uint32 len = Jni::getArrayLength(arr);
		for (sl_uint32 i = 0; i < len; i++) {
			JniLocal<jobject> jinfo = Jni::getObjectArrayElement(arr, i);
			if (jinfo.isNotNull()) {
				CameraInfo info;
				info.id = JCameraInfo::id.get(jinfo);
				info.name = JCameraInfo::name.get(jinfo);
				ret.add_NoLock(info);
			}
		}
		return ret;
	}

	sl_bool Camera::isMobileDeviceTorchActive()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			return JCamera::isMobileDeviceTorchActive.callBoolean(sl_null, context);
		}
		return sl_false;
	}

	void Camera::setMobileDeviceTorchMode(CameraTorchMode mode, float level)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JCamera::setMobileDeviceTorchMode.call(sl_null, context, (jint)((int)mode), (jfloat)level);
		}
	}

}

#endif
