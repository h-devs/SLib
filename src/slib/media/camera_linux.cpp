/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "slib/media/camera.h"

#include <unistd.h>
#include <errno.h>

#include "v4l/videodev2.h"

#define MAX_CAMERAS 8
#define DEFAULT_BUFFER_COUNT 4

#define TAG "Camera_v4l2"

namespace slib
{

	namespace priv
	{
		namespace camera
		{
			
			static void LogError(const String& error)
			{
				Log(TAG, error);
			}

			struct FORMAT_MAPPING
			{
				__u32 v4l_fmt;
				BitmapFormat fmt;
			} g_formatMappings = {
				{ V4L2_PIX_FMT_BGR24, BitmapFormat::BGR },
				{ V4L2_PIX_FMT_RGB24, BitmapFormat::RGB },
				{ V4L2_PIX_FMT_YUV32, BitmapFormat::YUVA }
			};

			static sl_bool SetFormat(int handle, __u32 fmt, __u32 width, __u32 height, v4l2_format& format)
			{
				Base::zeroMemory(&format, sizeof(format));
				format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				format.fmt.pix.pixelformat = fmt;
				format.fmt.pix.field = V4L2_FIELD_ANY;
				format.fmt.pix.width = width;
				format.fmt.pix.height = height;
				int iRet = ioctl(handle, VIDIOC_S_FMT, &format);
				if (iRet != -1) {
					if (format.fmt.pix.pixelformat == fmt) {
						return sl_true;
					}
				}
				return sl_false;
			}

			static BitmapFormat GetBitmapFormat(__u32 fmt)
			{
				sl_size n = CountOfArray(g_formatMappings);
				for (sl_size i = 0; i < n; i++) {
					if (g_formatMappings[i].v4l_fmt == fmt) {
						return g_formatMappings[i].fmt;
					}
				}
				return BitmapFormat::None;
			}

			struct CaptureBuffer
			{
				void* start;
				sl_uint32 length;
			};

			static List<CaptureBuffer> AllocateBuffers(int handle, Memory& bufFrame)
			{
				v4l2_requestbuffers req;
				Base::zeroMemory(&req, sizeof(req));
				req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				req.memory = V4L2_MEMORY_MMAP;

				sl_uint32 n = DEFAULT_BUFFER_COUNT;
				while (n > 0) {
					req.count = (__u32)n;
					int iRet = ioctl(handle, VIDIOC_REQBUFS, &req);
					if (iRet == -1) {
						if (errno == EINVAL) {
							LogError("Not support memory mapping");
						} else {
							LogError("Failed to allocate request buffers");
						}
						return sl_null;
					} else {
						break;
					}
					n--;
				}
				if (!n) {
					LogError("Insufficient buffer memory");
					return sl_null;
				}

				List<CaptureBuffer> ret;
				
				sl_uint32 sizeMax = 0;
				sl_uint32 i;

				for (i = 0; i < n; i++) {
					v4l2_buffer buf;
					Base::zeroMemory(&buf, sizeof(buf));
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					buf.index = (__u32)i;
					int iRet = ioctl(handle, VIDIOC_QUERYBUF, &buf);
					if (iRet == -1) {
						LogError("Failed to query buffer");
						return sl_null;
					}
					CaptureBuffer cbuf;
					cbuf.length = (sl_uint32)(buf.length);
					if (cbuf.length > sizeMax) {
						sizeMax = cbuf.length;
					}
					cbuf.start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, handle, buf.m.offset);
					if (cbuf.start == MAP_FAILED) {
						LogError("Failed mmap");
						return sl_null;
					}
					ret.add_NoLock(cbuf);
				}

				bufFrame = Memory::create(sizeMax);
				if (bufFrame.isNull()) {
					return sl_null;
				}

				for (i = 0; i < n; i++) {
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					buf.index = (__u32)i;
					int iRet = ioctl(handle, VIDIOC_QBUF, &buf);
					if (iRet == -1) {
						LogError("Failed to queue buffer");
						return sl_null;
					}
				}
				return ret;
			}

			class CameraImpl : public Camera
			{
			public:
				int m_handle;

				v4l2_capability m_cap;
				v4l2_format m_format;
				BitmapFormat m_bitmapFormat;

				List<CaptureBuffer> m_buffers;
				CaptureBuffer* m_pBuffers;
				sl_uint32 m_nBuffers;
				Memory m_bufFrame;

				Ref<Thread> m_threadCapture;

			public:
				CameraImpl()
				{
					m_handle = -1;
				}

				~CameraImpl()
				{
					release();
				}

			public:
				static List<CameraInfo> _queryDevices()
				{
					List<CameraInfo> ret;
					for (sl_uint32 deviceNo = 0; deviceNo < MAX_CAMERAS; deviceNo++) {
						String id = String::fromUint32(deviceNo);
						String path = "/dev/video" + id;
						int handle = ::open(path.getData(), O_RDONLY);
						if (handle != -1) {
							v4l2_capability cap;
							Base::zeroMemory(&cap, sizeof(cap));
							int iRet = ioctl(handle, VIDIOC_QUERYCAP, &cap);
							if (iRet != -1 && cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
								CameraInfo info;
								info.id = id;
								info.name = path;
								info.description = String::fromUtf8(cap.card, Base::getStringLength(cap.card, sizeof(cap.card)));
								ret.add_NoLock(info);
							}
							::close(handle);
						}
					}
					return ret;
				}

				static Ref<CameraImpl> _create(const CameraParam& param)
				{
					String path = "/dev/video" + param.deviceId;
					int handle = ::open(path.getData(), O_RDWR | O_NONBLOCK, 0);
					if (handle != -1) {
						v4l2_capability cap;
						Base::zeroMemory(&cap, sizeof(cap));
						int iRet = ioctl(handle, VIDIOC_QUERYCAP, &cap);
						if (iRet != -1 && cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
							int deviceIndex;
							iRet = ioctl(handle, VIDIOC_G_INPUT, &deviceIndex);
							if (iRet != -1) {
								v4l2_input input;
								Base::zeroMemory(&input, sizeof(input));
								input.index = deviceIndex;
								iRet = ioctl(handle, VIDIOC_ENUMINPUT, &input);
								if (iRet != -1) {
									v4l2_format format;
									Base::zeroMemory(&format, sizeof(format));
									format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
									iRet = ioctl(handle, VIDIOC_G_FMT, &format);
									if (iRet != -1) {
										__u32 width = format.fmt.pix.width;
										__u32 height = format.fmt.pix.height;
										BitmapFormat bitmapFormat = GetBitmapFormat(format.fmt.pix.pixelformat);
										if (bitmapFormat == BitmapFormat::None) {
											sl_size n = CountOfArray(g_formatMappings);
											for (sl_size i = 0; i < n; i++) {
												if (SetFormat(handle, g_formatMappings[i].v4l_fmt, width, height, format) {
													bitmapFormat = g_formatMappings[i].fmt;
													break;
												}
											}
										}
										if (bitmapFormat != BitmapFormat::None) {
											Memory bufFrame;
											List<CaptureBuffer> buffers = AllocateBuffers(handle, bufFrame);
											if (buffers.isNotEmpty()) {
												__u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
												iRet = ioctl(handle, VIDIOC_STREAMON, &type);
												if (iRet != -1) {
													Ref<CameraImpl> ret = new CameraImpl;
													if (ret.isNotNull()) {
														ret->m_handle = handle;
														ret->m_cap = cap;
														ret->m_format = format;
														ret->m_bitmapFormat = bitmapFormat;
														ret->m_pBuffers = buffers.getData();
														ret->m_nBuffers = (sl_uint32)(buffers.getCount());
														ret->m_buffers = Move(buffers);
														ret->m_bufFrame = Move(bufFrame);
														ret->_init(param);
														if (param.flagAutoStart) {
															ret->start();
														}
														return ret;
													}
												} else {
													LogError("Failed to start streaming");
												}
											}
										} else {
											LogError("Cannot support video format");
										}
									} else {
										LogError("Failed to get format");
									}
								} else {
									LogError("Failed to get video device index");
								}
							} else {
								if (iRet != -1) {
									LogError("Device does not support video capture capability");
								} else {
									LogError("Failed to get device capability");
								}
							}
						}
						::close(handle);
					}
					return sl_null;
				}

				void release() override
				{
					ObjectLocker lock(this);
					stop();
					if (m_handle != -1) {
						::close(m_handle);
						m_handle = -1;
					}
				}
				
				sl_bool isOpened() override
				{
					return m_handle != =1;
				}
				
				void start() override
				{
					ObjectLocker lock(this);
					if (m_threadCapture.isNull()) {
						m_threadCapture = Thread::start(SLIB_FUNCTION_WEAKREF(CameraImpl, _run, this));
					}
				}
				
				void stop() override
				{
					ObjectLocker lock(this);
					if (m_threadCapture.isNotNull()) {
						m_threadCapture->finishAndWait();
						m_threadCapture.setNull();
					}
				}
				
				sl_bool isRunning() override
				{
					return m_threadCapture.isNotNull();
				}

				sl_bool _runStep()
				{
					int handle = m_handle;
					
					v4l2_buffer buf;
					Base::zeroMemory(&buf, sizeof(buf));
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;

					int iRet = ioctl(handle, VIDIOC_DQBUF, &buf);
					if (iRet == -1) {
						switch (errno) {
							case EAGAIN:
								return sl_true;
							case EIO:
								if (!(buf.flags & (V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_DONE))) {
									iRet = ioctl(handle, VIDIOC_QBUF, &buf);
									if (iRet == -1) {
										LogError("Failed to queue buffer #1");
										return sl_false;
									}
								}
								return sl_true;
						}
						LogError("Failed to dequeue buffer");
						return sl_false;
					}

					if (buf.index >= m_nBuffers) {
						LogError("Invalid buffer index");
						return sl_false;
					}

					void* pData = m_bufFrame.getData();
					Base::copyMemory(pData, m_pBuffers[buf.index].start, m_pBuffers[buf.index].length);
					
					VideoCaptureFrame frame;
					frame.image.width = (sl_uint32)(m_format.fmt.pix.width);
					frame.image.height = (sl_uint32)(m_format.fmt.pix.height);
					frame.image.format = m_bitmapFormat;
					frame.image.data = pData;
					onCaptureVideoFrame(frame);

					iRet = ioctl(handle, VIDIOC_QBUF, &buf);
					if (iRet == -1) {
						LogError("Failed to queue buffer #2");
						return sl_false;
					}
					return sl_true;
				}

				void _run()
				{
					Ref<Thread> thread = Thread::getCurrent();
					if (thread.isNull()) {
						return;
					}
					if (thread->isNotStopping()) {
						if (!(_runStep())) {
							return;
						}
						Thread::sleep(10);
					}
				}
				
			};

		}
	}

	using namespace priv::camera;

	Ref<Camera> Camera::create(const CameraParam& param)
	{
		return priv::camera::CameraImpl::_create(param);
	}

	List<CameraInfo> Camera::getCamerasList()
	{
		return priv::camera::CameraImpl::_queryDevices();
	}

}

#endif
