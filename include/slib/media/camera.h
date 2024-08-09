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

#ifndef CHECKHEADER_SLIB_MEDIA_CAMERA
#define CHECKHEADER_SLIB_MEDIA_CAMERA

#include "video_capture.h"

#include "../graphics/image.h"
#include "../core/function.h"
#include "../core/queue.h"

namespace slib
{

	enum class CameraFlashMode
	{
		Auto = 0,
		On = 1,
		Off = 2
	};

	enum class CameraFocusMode
	{
		None = 0,
		ContinuousAutoFocus = 1,
		SmoothContinuousAutoFocus = 2
	};

	enum class CameraTorchMode
	{
		Auto = 0,
		On = 1,
		Off = 2,
	};

	class SLIB_EXPORT CameraInfo
	{
	public:
		String id;
		String name;
		String description;

	public:
		CameraInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CameraInfo)

	};

	class SLIB_EXPORT CameraParam : public VideoCaptureParam
	{
	public:
		String deviceId;

		sl_uint32 preferedFrameWidth;
		sl_uint32 preferedFrameHeight;
		BitmapFormat preferedFrameFormat;

	public:
		CameraParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CameraParam)

	public:
		void setFrontCamera();

		sl_bool isFrontCamera();


		void setBackCamera();

		sl_bool isBackCamera();

	};

	class SLIB_EXPORT Camera : public VideoCapture
	{
		SLIB_DECLARE_OBJECT

	public:
		Camera();

		~Camera();

	public:
		class TakePictureResult
		{
		public:
			sl_bool flagSuccess;

			RotationMode rotation;
			FlipMode flip;

		public:
			TakePictureResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TakePictureResult)

		public:
			Ref<Image> getImage();

			Ref<Drawable> getDrawable();

			Memory getJpeg();

			void setFrame(const VideoCaptureFrame& frame);

			void setJpeg(const Memory& jpeg);

		protected:
			const VideoCaptureFrame* frame;
			Memory jpeg;
		};

		class TakePictureParam
		{
		public:
			CameraFlashMode flashMode;

			Function<void(TakePictureResult& frame)> onComplete;

		public:
			TakePictureParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TakePictureParam)
		};

		virtual void takePicture(const TakePictureParam& param);

		virtual void setFocusMode(CameraFocusMode mode);

		virtual void autoFocus();

		// x, y: 0~1
		virtual void autoFocusOnPoint(sl_real x, sl_real y);

		virtual sl_bool isAdjustingFocus();

		virtual sl_bool isTorchActive();

		virtual void setTorchMode(CameraTorchMode mode, float level = 1.0f);

	public:
		static Ref<Camera> create(const CameraParam& param);

		static List<CameraInfo> getCameras();

		static sl_bool isMobileDeviceTorchActive();

		static void setMobileDeviceTorchMode(CameraTorchMode mode, float level = 1.0f);

		static sl_bool isEnabled();

		static void requestAccess(const Function<void(sl_bool flagGranted)>& callback);

		static void requestAccess();

#ifdef SLIB_PLATFORM_IS_MACOS
		static void resetAccess(const StringParam& appBundleId);
#endif

	protected:
		void onCaptureVideoFrame(VideoCaptureFrame& frame) override;

	protected:
		Queue<TakePictureParam> m_queueTakePictureRequests;

	};

}

#endif
