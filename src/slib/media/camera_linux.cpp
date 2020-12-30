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

#include "slib/core/thread.h"
#include "slib/core/time.h"
#include "slib/core/log.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "v4l/videodev2.h"

#define MAX_CAMERAS 8
#define DEFAULT_BUFFER_COUNT 4

#define TAG "Camera"
#define LOG_DEBUG(...) SLIB_LOG_DEBUG(TAG, ##__VA_ARGS__)
#define LOG_ERROR(...) SLIB_LOG_ERROR(TAG, ##__VA_ARGS__)

namespace slib
{

	namespace priv
	{
		namespace camera
		{
			
			struct FORMAT_MAPPING
			{
				__u32 v4l_fmt;
				BitmapFormat fmt;
			} g_formatMappings[] = {
				{ V4L2_PIX_FMT_BGR24, BitmapFormat::BGR },
				{ V4L2_PIX_FMT_RGB24, BitmapFormat::RGB },
				{ V4L2_PIX_FMT_YUV32, BitmapFormat::YUVA },
				{ V4L2_PIX_FMT_YUYV, BitmapFormat::YUYV },
				{ V4L2_PIX_FMT_UYVY, BitmapFormat::UYVY },
				{ V4L2_PIX_FMT_NV12, BitmapFormat::YUV_NV12 },
				{ V4L2_PIX_FMT_NV21, BitmapFormat::YUV_NV21 }
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

			static void ReleaseBuffers(int handle, CaptureBuffer* buffers, sl_uint32 nBuffers)
			{
				for (sl_uint32 i = 0; i < nBuffers; i++) {
					munmap(buffers[i].start, buffers[i].length);
				}
				v4l2_requestbuffers req;
				Base::zeroMemory(&req, sizeof(req));
				req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				req.memory = V4L2_MEMORY_MMAP;
				req.count = 0;
				ioctl(handle, VIDIOC_REQBUFS, &req);
			}

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
							LOG_ERROR("Not support memory mapping");
						} else {
							LOG_ERROR("Failed to allocate request buffers");
						}
						return sl_null;
					} else {
						break;
					}
					n--;
				}
				if (!n) {
					LOG_ERROR("Insufficient buffer memory");
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
						LOG_ERROR("Failed to query buffer");
						ReleaseBuffers(handle, ret.getData(), (sl_uint32)(ret.getCount()));
						return sl_null;
					}
					CaptureBuffer cbuf;
					cbuf.length = (sl_uint32)(buf.length);
					if (cbuf.length > sizeMax) {
						sizeMax = cbuf.length;
					}
					cbuf.start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, handle, buf.m.offset);
					if (cbuf.start == MAP_FAILED) {
						LOG_ERROR("Failed mmap");
						ReleaseBuffers(handle, ret.getData(), (sl_uint32)(ret.getCount()));
						return sl_null;
					}
					ret.add_NoLock(cbuf);
				}

				bufFrame = Memory::create(sizeMax);
				if (bufFrame.isNull()) {
					LOG_ERROR("Failed to allocate memory");
					ReleaseBuffers(handle, ret.getData(), (sl_uint32)(ret.getCount()));
					return sl_null;
				}

				for (i = 0; i < n; i++) {
					v4l2_buffer buf;
					Base::zeroMemory(&buf, sizeof(buf));
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					buf.index = (__u32)i;
					int iRet = ioctl(handle, VIDIOC_QBUF, &buf);
					if (iRet == -1) {
						LOG_ERROR("Failed to queue buffer");
						ReleaseBuffers(handle, ret.getData(), (sl_uint32)(ret.getCount()));
						return sl_null;
					}
				}
				return ret;
			}

			static void FlushBuffers(int handle)
			{
				v4l2_buffer buf;
				for (sl_uint32 i = 0; i < DEFAULT_BUFFER_COUNT; i++) {
					Base::zeroMemory(&buf, sizeof(buf));
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					int iRet = ioctl(handle, VIDIOC_DQBUF, &buf);
					if (iRet == -1) {
						return;
					}
					iRet = ioctl(handle, VIDIOC_QBUF, &buf);
				}
			}

			static void ReleaseCapture(int handle, CaptureBuffer* buffers, sl_uint32 nBuffers)
			{
				__u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				ioctl(handle, VIDIOC_STREAMOFF, &type);
				ReleaseBuffers(handle, buffers, nBuffers);
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
								info.description = String::fromUtf8(cap.card, Base::getStringLength((sl_char8*)(cap.card), sizeof(cap.card)));
								ret.add_NoLock(info);
							}
							::close(handle);
						}
					}
					return ret;
				}

				static Ref<CameraImpl> _create(const CameraParam& param)
				{
					int handle;
					if (param.deviceId.isNotNull() && param.deviceId != "FRONT" && param.deviceId != "BACK") {
						String path = "/dev/video" + param.deviceId;
						handle = ::open(path.getData(), O_RDWR | O_NONBLOCK, 0);
					} else {
						struct stat st;
						if (0 == stat("/dev/video", &st)) {
							handle = ::open("/dev/video", O_RDWR | O_NONBLOCK, 0);
						} else {
							handle = ::open("/dev/video0", O_RDWR | O_NONBLOCK, 0);
						}
					}
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
										__u32 pixelFormatOriginal = format.fmt.pix.pixelformat;
										BitmapFormat bitmapFormat = GetBitmapFormat(pixelFormatOriginal);
										if (bitmapFormat == BitmapFormat::None) {
											LOG_DEBUG("Not supported video format: %c%c%c%c, Trying other video formats supported by driver.", SLIB_GET_BYTE0(pixelFormatOriginal), SLIB_GET_BYTE1(pixelFormatOriginal), SLIB_GET_BYTE2(pixelFormatOriginal), SLIB_GET_BYTE3(pixelFormatOriginal));
											for (sl_uint32 i = 0; ; i++) {
												v4l2_fmtdesc fd;
												Base::zeroMemory(&fd, sizeof(fd));
												fd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
												fd.index = (__u32)i;
												iRet = ioctl(handle, VIDIOC_ENUM_FMT, &fd);
												if (iRet == -1) {
													break;
												}
												LOG_DEBUG("Checking video format: %c%c%c%c", SLIB_GET_BYTE0(fd.pixelformat), SLIB_GET_BYTE1(fd.pixelformat), SLIB_GET_BYTE2(fd.pixelformat), SLIB_GET_BYTE3(fd.pixelformat));
												bitmapFormat = GetBitmapFormat(fd.pixelformat);
												if (bitmapFormat != BitmapFormat::None) {
													if (SetFormat(handle, fd.pixelformat, width, height, format)) {
														break;
													} else {
														LOG_DEBUG("Cannot support video format: %c%c%c%c", SLIB_GET_BYTE0(fd.pixelformat), SLIB_GET_BYTE1(fd.pixelformat), SLIB_GET_BYTE2(fd.pixelformat), SLIB_GET_BYTE3(fd.pixelformat));
													}
												}
											}
										}
										if (bitmapFormat != BitmapFormat::None) {
											LOG_DEBUG("Selected video format: %c%c%c%c", SLIB_GET_BYTE0(format.fmt.pix.pixelformat), SLIB_GET_BYTE1(format.fmt.pix.pixelformat), SLIB_GET_BYTE2(format.fmt.pix.pixelformat), SLIB_GET_BYTE3(format.fmt.pix.pixelformat));
											Memory bufFrame;
											List<CaptureBuffer> buffers = AllocateBuffers(handle, bufFrame);
											if (buffers.isNotEmpty()) {
												Ref<CameraImpl> ret = new CameraImpl;
												if (ret.isNotNull()) {
													__u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
													iRet = ioctl(handle, VIDIOC_STREAMON, &type);
													if (iRet != -1) {
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
													} else {
														LOG_ERROR("Failed to start streaming");
													}
												}
											}
										} else {
											LOG_ERROR("Cannot support video format: %c%c%c%c", SLIB_GET_BYTE0(pixelFormatOriginal), SLIB_GET_BYTE1(pixelFormatOriginal), SLIB_GET_BYTE2(pixelFormatOriginal), SLIB_GET_BYTE3(pixelFormatOriginal));
										}
									} else {
										LOG_ERROR("Failed to get format");
									}
								} else {
									LOG_ERROR("Failed to get video device index");
								}
							} else {
								if (iRet != -1) {
									LOG_ERROR("Device does not support video capture capability");
								} else {
									LOG_ERROR("Failed to get device capability");
								}
							}
						}
						::close(handle);
					}
					return sl_null;
				}

				void release() override
				{
					if (m_handle == -1) {
						return;
					}
					ObjectLocker lock(this);
					stop();
					if (m_handle != -1) {
						ReleaseCapture(m_handle, m_pBuffers, m_nBuffers);
						::close(m_handle);
						m_handle = -1;
					}
				}
				
				sl_bool isOpened() override
				{
					return m_handle != 1;
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
						int err = errno;
						switch (err) {
							case EAGAIN:
								return sl_true;
							case EIO:
								if (!(buf.flags & (V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_DONE))) {
									iRet = ioctl(handle, VIDIOC_QBUF, &buf);
									if (iRet == -1) {
										LOG_ERROR("Failed to queue buffer #1");
										return sl_false;
									}
								}
								return sl_true;
						}
						LOG_ERROR("Failed to dequeue buffer");
						return sl_false;
					}

					if (buf.index >= m_nBuffers) {
						LOG_ERROR("Invalid buffer index");
						return sl_false;
					}

					void* pData = m_bufFrame.getData();
					sl_uint32 size = m_pBuffers[buf.index].length;
					Base::copyMemory(pData, m_pBuffers[buf.index].start, size);

					VideoCaptureFrame frame;
					frame.image.width = (sl_uint32)(m_format.fmt.pix.width);
					frame.image.height = (sl_uint32)(m_format.fmt.pix.height);
					if (frame.image.width && frame.image.height) {
						frame.image.format = m_bitmapFormat;
						frame.image.data = pData;
						if (BitmapFormats::getPlanesCount(m_bitmapFormat) == 1) {
							frame.image.pitch = size / frame.image.height;
						}
						if (frame.image.getTotalSize() <= size) {
							onCaptureVideoFrame(frame);
						}
					}

					iRet = ioctl(handle, VIDIOC_QBUF, &buf);
					if (iRet == -1) {
						LOG_ERROR("Failed to queue buffer #2");
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
					FlushBuffers(m_handle);
					TimeCounter t;
					while (thread->isNotStopping()) {
						if (!(_runStep())) {
							return;
						}
						sl_uint32 dt = (sl_uint32)(t.getElapsedMilliseconds());
						if (dt < 30) {
							Thread::sleep(30 - dt);
						}
						t.reset();
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
