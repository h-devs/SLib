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

#include "slib/network/pcap.h"
#include "slib/network/npcap.h"

#if defined(SLIB_PLATFORM_IS_UNIX) || defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/network/socket_address.h"

#include "slib/core/thread.h"
#include "slib/core/timer.h"
#include "slib/core/process.h"
#include "slib/core/file.h"
#include "slib/core/system.h"
#include "slib/core/log.h"
#include "slib/core/safe_static.h"

#define TAG "Pcap"

#define USE_WINPCAP 0

#if defined(SLIB_PLATFORM_IS_WIN32)
#	if !USE_WINPCAP
#		define USE_STATIC_NPCAP
#		include "pcap/pcap.h"
#	else
#		include "winpcap/pcap/pcap.h"
#	endif
#else
#	if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
#		include "slib/network/dl/linux/pcap.h"
#	else
#		include "pcap/pcap.h"
#	endif
#	include <sys/socket.h>
#	include <signal.h>
#	include <pthread.h>
#endif

#define MAX_PACKET_SIZE 65535

#ifdef USE_STATIC_NPCAP
void InitNpcap();
void FreeNpcap();
#endif
#define NPCAP_DRIVER_NAME "NPCAP"

#define PCAP_BREAK_SIGNAL SIGUSR1

namespace slib
{

	namespace priv
	{
		namespace pcap
		{

#ifdef USE_STATIC_NPCAP

			class NpcapEnv
			{
			public:
				NpcapEnv()
				{
					InitNpcap();
				}

				~NpcapEnv()
				{
					FreeNpcap();
				}
			};

			SLIB_SAFE_STATIC_GETTER(NpcapEnv, GetEnv)
#endif

			class PcapImpl : public Pcap
			{
			public:
				pcap_t* m_handle;
				
				Ref<Thread> m_thread;

				sl_bool m_flagInit;
				sl_bool m_flagRunning;

#if !defined(SLIB_PLATFORM_IS_WIN32)
				pthread_t m_pthread;
#endif
				
			public:
				PcapImpl()
				{
					m_flagInit = sl_false;
					m_flagRunning = sl_false;
#if !defined(SLIB_PLATFORM_IS_WIN32)
					m_pthread = 0;
#endif
				}
				
				~PcapImpl()
				{
					release();
				}
				
			public:
				static Ref<PcapImpl> create(const PcapParam& param)
				{
					StringCstr name = param.deviceName;
					if (name.isEmpty()) {
						name = "any";
					}

					char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
				
#if USE_WINPCAP
					pcap_t* handle = pcap_open_live(name.getData(), MAX_PACKET_SIZE, param.flagPromiscuous, param.timeoutRead, errBuf);
					if (handle) {
						if (pcap_setbuff(handle, param.sizeBuffer) == 0) {
							Ref<PcapImpl> ret = new PcapImpl;
							if (ret.isNotNull()) {
								ret->_initWithParam(param);
								ret->m_handle = handle;
								ret->m_thread = Thread::create(SLIB_FUNCTION_MEMBER(PcapImpl, _run, ret.get()));
								if (ret->m_thread.isNotNull()) {
									ret->m_flagInit = sl_true;
									if (param.flagAutoStart) {
										ret->start();
									}
									return ret;
								} else {
									LogError(TAG, "Failed to create thread");
								}
							} 
						} else {
							LogError(TAG, "Set Buffer Size Failed");
						}
						pcap_close(handle);
					} else {
						LogError(TAG, errBuf);
					}
#else
					pcap_t* handle = pcap_create(name.getData(), errBuf);
					if (handle) {
						if (pcap_set_snaplen(handle, MAX_PACKET_SIZE) == 0) {
							if (pcap_set_buffer_size(handle, param.sizeBuffer) == 0) {
								if (pcap_set_promisc(handle, param.flagPromiscuous) == 0) {
									if (pcap_set_timeout(handle, param.timeoutRead) == 0) {
										if (pcap_set_immediate_mode(handle, param.flagImmediate) == 0) {
											int iRet = pcap_activate(handle);
											if (iRet == 0 || iRet == PCAP_WARNING || iRet == PCAP_WARNING_PROMISC_NOTSUP || iRet == PCAP_WARNING_TSTAMP_TYPE_NOTSUP) {
												Ref<PcapImpl> ret = new PcapImpl;
												if (ret.isNotNull()) {
													ret->_initWithParam(param);
													ret->m_handle = handle;
													ret->m_thread = Thread::create(SLIB_FUNCTION_MEMBER(PcapImpl, _run, ret.get()));
													if (ret->m_thread.isNotNull()) {
														ret->m_flagInit = sl_true;
														if (param.flagAutoStart) {
															ret->start();
														}
														return ret;
													} else {
														LogError(TAG, "Failed to create thread");
													}
												}
											} else {
												LogError(TAG, "Activate Failed");
											}
										} else {
											LogError(TAG, "Set Immediate-Mode Failed");
										}
									} else {
										LogError(TAG, "Set Timeout Failed");
									}
								} else {
									LogError(TAG, "Set Promiscuous Mode Failed");
								}
							} else {
								LogError(TAG, "Set Buffer Size Failed");
							}
						} else {
							LogError(TAG, "Set Snaplen Failed");
						}
						pcap_close(handle);
					} else {
						LogError(TAG, errBuf);
					}
#endif
					return sl_null;
				}
				
				void release() override
				{
					ObjectLocker lock(this);
					if (!m_flagInit) {
						return;
					}
					m_flagInit = sl_false;

					m_flagRunning = sl_false;

					if (m_thread.isNotNull()) {
						m_thread->finish();
					}

					pcap_breakloop(m_handle);

#if defined(SLIB_PLATFORM_IS_WIN32)
					HANDLE hEvent = pcap_getevent(m_handle);
					if (hEvent) {
						SetEvent(hEvent);
					}
#else
					pthread_kill(m_pthread, PCAP_BREAK_SIGNAL);
#endif

					if (m_thread.isNotNull()) {
						m_thread->finishAndWait();
						m_thread.setNull();
					}
					
					pcap_close(m_handle);
				}

				void start() override
				{
					ObjectLocker lock(this);
					if (!m_flagInit) {
						return;
					}

					if (m_flagRunning) {
						return;
					}
					if (m_thread.isNotNull()) {
						m_flagRunning = sl_true;
						if (!(m_thread->start())) {
							m_flagRunning = sl_false;
						}
					}
				}
				
				sl_bool isRunning() override
				{
					return m_flagRunning;
				}

				static void _handler(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
				{
					PcapImpl* pcap = (PcapImpl*)user;

					NetCapturePacket packet;
					packet.data = (sl_uint8*)bytes;
					packet.length = h->len;
					if (packet.length > h->caplen) {
						packet.length = h->caplen;
					}
					sl_uint64 t = h->ts.tv_sec;
					t = t * 1000000 + h->ts.tv_usec;
					packet.time = t;

					pcap->_onCapturePacket(packet);
				}

				static void _handlerBreakSignal(int signum)
				{
				}
				
				void _run()
				{
#if !defined(SLIB_PLATFORM_IS_WIN32)
					m_pthread = pthread_self();
					struct sigaction sa;
					Base::zeroMemory(&sa, sizeof(sa));
					sigemptyset(&sa.sa_mask);
					sa.sa_handler = _handlerBreakSignal;
					sigaction(PCAP_BREAK_SIGNAL, &sa, NULL);
#endif
					while (m_flagRunning) {
						int result = pcap_dispatch(m_handle, -1, &_handler, (u_char*)this);
						if (result < 0) {
							if (result == -1) {
								Ref<PcapImpl> thiz = this; // Protection for releasing during callback
								_onError();
							}
							break;
						}
					}
					m_flagRunning = sl_false;
				}

				NetworkLinkDeviceType getLinkType() override
				{
					sl_uint32 dt = pcap_datalink(m_handle);
					return (NetworkLinkDeviceType)dt;
				}

				sl_bool setLinkType(sl_uint32 type) override
				{
					int ret = pcap_set_datalink(m_handle, type);
					return ret == 0;
				}

				sl_bool sendPacket(const void* buf, sl_uint32 size) override
				{
					if (m_flagInit) {
						int ret = pcap_sendpacket(m_handle, (const sl_uint8*)buf, size);
						if (ret == 0) {
							return sl_true;
						}
					}
					return sl_false;
				}
				
				String getErrorMessage() override
				{
					if (m_flagInit) {
						char* err = pcap_geterr(m_handle);
						if (err) {
							return err;
						}
					}
					return sl_null;
				}

			};

			static void ParseDeviceInfo(pcap_if_t* dev, PcapDeviceInfo& _out)
			{
				_out.name = String::fromUtf8(dev->name);
				_out.description = String::fromUtf8(dev->description);
				if (_out.description.isEmpty()) {
					_out.description = _out.name;
				}

				_out.flagLoopback = dev->flags & PCAP_IF_LOOPBACK;
				_out.flagUp = dev->flags & PCAP_IF_UP;
				_out.flagRunning = dev->flags & PCAP_IF_RUNNING;
				_out.flagWireless = dev->flags & PCAP_IF_WIRELESS;
				switch (dev->flags & PCAP_IF_CONNECTION_STATUS) {
					case PCAP_IF_CONNECTION_STATUS_CONNECTED:
						_out.connectionStatus = PcapConnectionStatus::Connected;
						break;
					case PCAP_IF_CONNECTION_STATUS_DISCONNECTED:
						_out.connectionStatus = PcapConnectionStatus::Disconnected;
						break;
					case PCAP_IF_CONNECTION_STATUS_NOT_APPLICABLE:
						_out.connectionStatus = PcapConnectionStatus::NotApplicable;
						break;
					default:
						_out.connectionStatus = PcapConnectionStatus::Unknown;
						break;
				}

				pcap_addr* addr = dev->addresses;
				SocketAddress sa;

				List<IPv4Address> addrs4;
				List<IPv6Address> addrs6;
				while (addr) {
					if (addr->addr) {
						switch (addr->addr->sa_family) {
							case AF_INET:
								sa.setSystemSocketAddress(addr->addr);
								addrs4.add_NoLock(sa.ip.getIPv4());
								break;
							case AF_INET6:
								sa.setSystemSocketAddress(addr->addr);
								addrs6.add_NoLock(sa.ip.getIPv6());
								break;
							}
					}
					addr = addr->next;
				}
				_out.ipv4Addresses = addrs4;
				_out.ipv6Addresses = addrs6;
			}

			class AnyPcap : public Pcap
			{
			public:
				PcapParam m_param;
				CList< Ref<PcapImpl> > m_devices;
				AtomicRef<Timer> m_timerAddDevices;

			public:
				AnyPcap()
				{
				}

				~AnyPcap()
				{
					release();
				}

			public:
				static Ref<AnyPcap> create(const PcapParam& param)
				{
					Ref<AnyPcap> ret = new AnyPcap;
					if (ret.isNotNull()) {
						ret->_initWithParam(param);
						ret->m_param = param;
						if (param.flagAutoStart) {
							ret->start();
						}
						return ret;
					}
					return sl_null;
				}

			public:
				void release() override
				{
					Ref<Timer> timer = Move(m_timerAddDevices);
					if (timer.isNotNull()) {
						timer->stopAndWait();
					}
					m_devices.removeAll();
				}

				void start() override
				{
					ObjectLocker lock(this);
					if (m_timerAddDevices.isNotNull()) {
						return;
					}
					onAddDevices(sl_null);
					m_timerAddDevices = Timer::start(SLIB_FUNCTION_MEMBER(AnyPcap, onAddDevices, this), 10000);
				}

				sl_bool isRunning() override
				{
					return m_devices.getCount() > 0;
				}

				NetworkLinkDeviceType getLinkType() override
				{
					return NetworkLinkDeviceType::Raw;
				}

				sl_bool sendPacket(const void*, sl_uint32) override
				{
					return sl_false;
				}

			protected:
				void onAddDevices(Timer*)
				{
					char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
					pcap_if_t* devs = NULL;
					int ret = pcap_findalldevs(&devs, errBuf);
					if (ret == 0 && devs) {
						pcap_if_t* dev = devs;
						while (dev) {
							if (!(dev->flags & PCAP_IF_LOOPBACK) && !(Base::equalsString(dev->name, "any"))) {
								sl_uint32 status = (sl_uint32)(dev->flags & PCAP_IF_CONNECTION_STATUS);
								if (status == PCAP_IF_CONNECTION_STATUS_CONNECTED || ((dev->flags & PCAP_IF_UP) && status == PCAP_IF_CONNECTION_STATUS_UNKNOWN)) {
									ListLocker< Ref<PcapImpl> > devices(m_devices);
									sl_bool flagFound = sl_false;
									for (sl_size i = 0; i < devices.count; i++) {
										if (devices[i]->getDeviceName() == dev->name) {
											flagFound = sl_true;
											break;
										}
									}
									if (!flagFound) {
										PcapParam param = m_param;
										param.deviceName = dev->name;
										param.onError = SLIB_BIND_WEAKREF(void(NetCapture*), AnyPcap, onErrorDevice, this, param.onError);
										param.flagAutoStart = sl_true;
										Ref<PcapImpl> pcap = PcapImpl::create(param);
										if (pcap.isNotNull()) {
											m_devices.add_NoLock(Move(pcap));
											if (dev->description && dev->description[0]) {
												SLIB_LOG(TAG, "Added device to any capture: %s (%s)", param.deviceName, dev->description);
											} else {
												SLIB_LOG(TAG, "Added device to any capture: %s", param.deviceName);
											}
										} else {
											if (dev->description && dev->description[0]) {
												SLIB_LOG(TAG, "Failed to add device to any capture: %s (%s)", param.deviceName);
											} else {
												SLIB_LOG(TAG, "Failed to add device to any capture: %s (%s)", param.deviceName, dev->description);
											}
										}
									}
								}
							}
							dev = dev->next;
						}
						pcap_freealldevs(devs);
					}
				}

				void onErrorDevice(const Function<void(NetCapture*)>& onError, NetCapture* capture)
				{
					m_devices.removeValues(capture);
					SLIB_LOG(TAG, "Removed device from any capture: %s", capture->getDeviceName());
					onError(capture);
				}

			};

		}
	}

	using namespace priv::pcap;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PcapDeviceInfo)

	PcapDeviceInfo::PcapDeviceInfo(): flagLoopback(sl_false), flagUp(sl_false), flagRunning(sl_false), flagWireless(sl_false), connectionStatus(PcapConnectionStatus::Unknown)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PcapParam)

	PcapParam::PcapParam()
	{
		timeoutRead = 0; // no timeout specified
		flagImmediate = sl_true;
		sizeBuffer = 0x200000; // 2MB (16Mb)
	}


	SLIB_DEFINE_OBJECT(Pcap, NetCapture)

	Pcap::Pcap()
	{
	}

	Pcap::~Pcap()
	{
	}

	Ref<Pcap> Pcap::create(const PcapParam& param)
	{
#ifdef USE_STATIC_NPCAP
		if (!(GetEnv())) {
			return sl_null;
		}
#endif
		return Ref<Pcap>::from(PcapImpl::create(param));
	}

	List<PcapDeviceInfo> Pcap::getAllDevices()
	{
#ifdef USE_STATIC_NPCAP
		if (!(GetEnv())) {
			return sl_null;
		}
#endif

		List<PcapDeviceInfo> list;
		char errBuf[PCAP_ERRBUF_SIZE] = { 0 };

		pcap_if_t* devs = NULL;
		int ret = pcap_findalldevs(&devs, errBuf);
		if (ret == 0 && devs) {
			pcap_if_t* dev = devs;
			while (dev) {
				PcapDeviceInfo item;
				ParseDeviceInfo(dev, item);
				list.add_NoLock(item);
				dev = dev->next;
			}
			pcap_freealldevs(devs);
		} else {
			LogError(TAG, errBuf);
		}
		return list;
	}

	sl_bool Pcap::findDevice(const String& name, PcapDeviceInfo& _out)
	{
#ifdef USE_STATIC_NPCAP
		if (!(GetEnv())) {
			return sl_false;
		}
#endif

		char errBuf[PCAP_ERRBUF_SIZE] = { 0 };

		pcap_if_t* devs = NULL;
		int ret = pcap_findalldevs(&devs, errBuf);
		if (ret == 0 && devs) {
			pcap_if_t* dev = devs;
			while (dev) {
				if (name == dev->name || name == dev->description) {
					ParseDeviceInfo(dev, _out);
					return sl_true;
				}
				dev = dev->next;
			}
			pcap_freealldevs(devs);
		} else {
			LogError(TAG, errBuf);
		}
		return sl_false;
	}

	Ref<Pcap> Pcap::createAny(const PcapParam& param)
	{
		return Ref<Pcap>::from(AnyPcap::create(param));
	}

	sl_bool Pcap::isAllowedNonRoot(const StringParam& executablePath)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		return sl_true;
#elif defined(SLIB_PLATFORM_IS_MACOS)
		ListElements<String> files(File::getFiles("/dev"));
		for (sl_size i = 0; i < files.count; i++) {
			String& file = files[i];
			if (file.startsWith("bp")) {
				String path = "/dev/" + file;
				FileAttributes attrs = File::getAttributes(path);
				if ((attrs & FileAttributes::AllAccess) != FileAttributes::AllAccess) {
					return sl_false;
				}
			}
		}
		return sl_false;
#elif defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
		return File::equalsCap(executablePath, "cap_net_admin,cap_net_raw=eip");
#else
		return sl_false;
#endif
	}

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	sl_bool Pcap::isAllowedNonRoot()
	{
		return isAllowedNonRoot(System::getApplicationPath());
	}
#else
	sl_bool Pcap::isAllowedNonRoot()
	{
		return isAllowedNonRoot(sl_null);
	}
#endif

	void Pcap::allowNonRoot(const StringParam &executablePath)
	{
#if defined(SLIB_PLATFORM_IS_MACOS)
		if (Process::isCurrentProcessAdmin()) {
			ListElements<String> files(File::getFiles("/dev"));
			for (sl_size i = 0; i < files.count; i++) {
				String& file = files[i];
				if (file.startsWith("bp")) {
					String path = "/dev/" + file;
					FileAttributes attrs = File::getAttributes(path);
					if ((attrs & FileAttributes::AllAccess) != FileAttributes::AllAccess) {
						File::setAttributes(path, FileAttributes::AllAccess);
					}
				}
			}
		} else {
			Process::runAsAdmin("chmod", "777", "/dev/bp*");
		}
#elif defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
		if (Process::isCurrentProcessAdmin()) {
			File::setCap(executablePath, "cap_net_admin,cap_net_raw=eip");
		} else {
			Process::runAsAdmin("setcap", "cap_net_admin,cap_net_raw=eip", executablePath);
		}
#endif
	}

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	void Pcap::allowNonRoot()
	{
		allowNonRoot(System::getApplicationPath());
	}
#else
	void Pcap::allowNonRoot()
	{
		allowNonRoot(sl_null);
	}
#endif


	ServiceState Npcap::getDriverState()
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		return ServiceManager::getState(NPCAP_DRIVER_NAME);
#else
		return ServiceState::None;
#endif
	}

	sl_bool Npcap::startDriver()
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		return ServiceManager::start(NPCAP_DRIVER_NAME);
#else
		return sl_false;
#endif
	}
	
#ifndef SLIB_PLATFORM_IS_WIN32
	sl_bool Npcap::install()
	{
		return sl_false;
	}

	sl_bool Npcap::uninstall()
	{
		return sl_false;
	}
#endif

}

#else

namespace slib
{

	Ref<Pcap> Pcap::create(const PcapParam& param)
	{
		return sl_null;
	}

	List<PcapDeviceInfo> Pcap::getAllDevices()
	{
		return sl_null;
	}

	sl_bool Pcap::findDevice(const String& name, PcapDeviceInfo& _out)
	{
		return sl_false;
	}

	Ref<Pcap> Pcap::createAny(const PcapParam& param)
	{
		return sl_null;
	}

	ServiceState Npcap::getDriverState()
	{
		return ServiceState::None;
	}

	sl_bool Npcap::startDriver()
	{
		return sl_false;
	}

	sl_bool Npcap::install()
	{
		return sl_false;
	}

	sl_bool Npcap::uninstall()
	{
		return sl_false;
	}

}

#endif
