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

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_UNIX) && !defined(SLIB_PLATFORM_IS_MOBILE)

#include "slib/network/tap.h"

#include "slib/core/thread.h"
#include "slib/core/system.h"
#include "slib/core/file.h"
#include "slib/core/handle_ptr.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>

namespace slib
{

	namespace priv
	{
		namespace tap
		{

			class TapImpl : public Tap
			{
			public:
				int m_handle;

			public:
				TapImpl()
				{
				}

				~TapImpl()
				{
					close();
				}

			public:
				static Ref<TapImpl> open(const StringParam& _deviceName)
				{
					StringCstr deviceName(_deviceName);
					const char* szDeviceName;
					if (deviceName.isEmpty()) {
						szDeviceName = "tap";
					} else {
						szDeviceName = deviceName.getData();
					}

					int handle = ::open("/dev/net/tun", O_RDWR);
					if (handle != -1) {
						HandlePtr<File>(handle)->setNonBlocking();
						ifreq ifr;
						Base::zeroMemory(&ifr, sizeof(ifr));
						ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
						Base::copyString(ifr.ifr_name, szDeviceName, IFNAMSIZ);
						if (ioctl(handle, TUNSETIFF, &ifr) >= 0) {
							Ref<TapImpl> tap = new TapImpl;
							if (tap.isNotNull()) {
								tap->m_handle = handle;
								tap->m_deviceName = szDeviceName;
								tap->m_interfaceName = szDeviceName;
								return tap;
							}
						}

					}
					return sl_null;
				}

				void _close() override
				{
					::close(m_handle);
				}

				sl_int32 read(void* buf, sl_uint32 size) override
				{
					if (m_flagOpened) {
						for (;;) {
							sl_int32 n = (sl_int32)(::read(m_handle, buf, size));
							if (n > 0) {
								return n;
							}
							if (n) {
								int err = errno;
								if (err != EAGAIN && err != EWOULDBLOCK && err != EINTR) {
									return SLIB_IO_ERROR;
								}
							}
							pollfd fd;
							Base::zeroMemory(&fd, sizeof(fd));
							fd.fd = m_handle;
							fd.events = POLLIN | POLLPRI | POLLERR | POLLHUP;
							int iRet = poll(&fd, 1, 10);
							if (iRet < 0) {
								break;
							}
							if (Thread::isStoppingCurrent()) {
								return SLIB_IO_WOULD_BLOCK;
							}
						}
					}
					return SLIB_IO_ERROR;
				}

				sl_int32 write(const void* buf, sl_uint32 size) override
				{
					if (m_flagOpened) {
						for (;;) {
							sl_int32 n = (sl_int32)(::write(m_handle, buf, size));
							if (n >= 0) {
								return n;
							}
							int err = errno;
							if (err != EAGAIN && err != EWOULDBLOCK && err != EINTR) {
								return SLIB_IO_ERROR;
							}
							pollfd fd;
							Base::zeroMemory(&fd, sizeof(fd));
							fd.fd = m_handle;
							fd.events = POLLOUT | POLLERR | POLLHUP;
							int iRet = poll(&fd, 1, 10);
							if (iRet < 0) {
								break;
							}
							if (Thread::isStoppingCurrent()) {
								return SLIB_IO_WOULD_BLOCK;
							}
						}
					}
					return SLIB_IO_ERROR;
				}

				sl_bool setIpAddress(const StringParam& _ip, const StringParam& _mask) override
				{

					StringCstr ip(_ip);
					StringCstr mask(_mask);

					sl_bool bRet = sl_false;

					int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
					if (fd >= 0) {

						ifreq req;
						Base::zeroMemory(&req, sizeof(req));
						Base::copyString(req.ifr_name, m_interfaceName.getData(), IFNAMSIZ);

						req.ifr_addr.sa_family = AF_INET;
						inet_pton(AF_INET, ip.getData(), &(((sockaddr_in*)&(req.ifr_addr))->sin_addr));

						if(ioctl(fd, SIOCSIFADDR, &req) != -1) {

							if (((sockaddr_in*)&(req.ifr_addr))->sin_addr.s_addr) {
								if (mask.isNotEmpty()) {

									inet_pton(AF_INET, mask.getData(), &(((sockaddr_in*)&(req.ifr_addr))->sin_addr));

									if(ioctl(fd, SIOCSIFNETMASK, &req) != -1) {

										if(ioctl(fd, SIOCGIFFLAGS, &req) != -1) {
											req.ifr_flags |= (IFF_UP | IFF_RUNNING);
											if(ioctl(fd, SIOCSIFFLAGS, &req) != -1) {
												bRet = sl_true;
											}
										}

									}
								}
							}
						}

						::close(fd);
					}

					return bRet;
				}

			};

		}
	}

	using namespace priv::tap;

	Ref<Tap> Tap::open(const StringParam& deviceName)
	{
		return Ref<Tap>::from(TapImpl::open(deviceName));
	}

	ServiceState Tap::getDriverState()
	{
		return ServiceManager::getState("tap0901");
	}

	sl_bool Tap::install()
	{
		return sl_true;
	}

	sl_bool Tap::uninstall()
	{
		return sl_false;
	}

}

#endif
