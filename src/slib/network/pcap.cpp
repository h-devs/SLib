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

#include "slib/network/pcap.h"

#include "slib/network/socket_address.h"
#include "slib/io/file.h"
#include "slib/system/process.h"
#include "slib/system/system.h"
#include "slib/core/timer.h"
#include "slib/core/log.h"
#include "slib/core/safe_static.h"

#ifdef SLIB_PLATFORM_IS_WIN32
#	include "slib/platform.h"
#	include "slib/dl/common/pcap.h"
#	define PCAP_DYNAMIC_LIB
#else
#	include "slib/io/pipe_event.h"
#	ifdef SLIB_PLATFORM_IS_LINUX_DESKTOP
#		include "slib/dl/common/pcap.h"
#		define PCAP_DYNAMIC_LIB
#	else
#		include "pcap/pcap.h"
#	endif
#	include <sys/socket.h>
#endif

#define TAG "Pcap"
#ifdef SLIB_DEBUG
#define LOG(...) SLIB_LOG(TAG, __VA_ARGS__)
#else
#define LOG(...)
#endif
#define LOG_ERROR(...) SLIB_LOG_ERROR(TAG, __VA_ARGS__)

#define MAX_PACKET_SIZE 65535

#ifndef SLIB_PLATFORM_IS_WIN32
#define PCAP_BREAK_SIGNAL SIGUSR1
#endif

namespace slib
{

	namespace
	{

		static sl_uint64 g_tickForAnyPcap = 0;

		class PcapImpl : public Pcap
		{
		public:
			pcap_t* m_handle = sl_null;
			Ref<Thread> m_threadReceive;
#ifdef SLIB_PLATFORM_IS_WIN32
			Ref<Thread> m_threadSend;
			Ref<Event> m_eventSend;
			Mutex m_lockSend;
			pcap_send_queue* m_queueSend = sl_null;
#endif

			sl_bool m_flagInit = sl_false;
			sl_uint64 m_lastPacketTickForAnyPcap = 0;

		public:
			PcapImpl()
			{
			}

			~PcapImpl()
			{
				release();
			}

		public:
			static Ref<PcapImpl> create(const PcapParam& param)
			{
#ifdef PCAP_DYNAMIC_LIB
				if (!(pcap_findalldevs)) {
					return sl_null;
				}
#endif
				StringCstr name(param.deviceName);
				if (name.isEmpty()) {
					name = "any";
				}
				pcap_t* handle = sl_null;
				int iRet = createHandle(handle, name.getData(), param);
				if (iRet >= 0) {
					return create(handle, param, name);
				} else {
					if (iRet == PCAP_ERROR_NO_SUCH_DEVICE) {
#ifdef SLIB_PLATFORM_IS_WIN32
						if (name.startsWith("{")) {
							String _name = "\\Device\\NPF_" + name;
							iRet = createHandle(handle, _name.getData(), param);
							if (iRet >= 0) {
								return create(handle, param, _name);
							} else {
								if (iRet != PCAP_ERROR_NO_SUCH_DEVICE) {
									return sl_null;
								}
							}
						}
#endif
						char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
						pcap_if_t* devs = NULL;
						int ret = pcap_findalldevs(&devs, errBuf);
						if (!ret && devs) {
							pcap_if_t* dev = devs;
							while (dev) {
								if (name == dev->description) {
									iRet = createHandle(handle, dev->name, param);
									if (iRet >= 0) {
										return create(handle, param, dev->name);
									} else {
										if (iRet != PCAP_ERROR_NO_SUCH_DEVICE) {
											return sl_null;
										}
									}
								}
								dev = dev->next;
							}
							pcap_freealldevs(devs);
						}
						LOG_ERROR("Failed to find device: %s", name);
					}
				}
				return sl_null;
			}

			static Ref<PcapImpl> create(pcap_t* handle, const PcapParam& param, const StringView& deviceName)
			{
				Ref<PcapImpl> ret = new PcapImpl;
				if (ret.isNotNull()) {
					if (ret->_initWithParam(param, deviceName)) {
						ret->m_handle = handle;
						if (param.flagAutoStart) {
							ret->start();
						}
						return ret;
					} else {
						LOG_ERROR("Failed to create thread");
					}
				}
				pcap_close(handle);
				return sl_null;
			}

			sl_bool _initWithParam(const PcapParam& param, const StringView& deviceName)
			{
				Pcap::_initWithParam(param);
				if (m_deviceName.getData() != deviceName.getData()) {
					m_deviceName = deviceName;
				}
				m_threadReceive = Thread::create(SLIB_FUNCTION_MEMBER(this, runReceive));
				if (m_threadReceive.isNull()) {
					return sl_false;
				}
#ifdef SLIB_PLATFORM_IS_WIN32
				m_threadSend = Thread::create(SLIB_FUNCTION_MEMBER(this, runSend));
				if (m_threadSend.isNull()) {
					return sl_false;
				}
				m_eventSend = Event::create();
				if (m_eventSend.isNull()) {
					return sl_false;
				}
				m_queueSend = pcap_sendqueue_alloc(param.bufferSize);
				if (!m_queueSend) {
					return sl_false;
				}
				m_threadSend->setPriority(param.threadPriority);
#endif
				m_threadReceive->setPriority(param.threadPriority);
				m_flagInit = sl_true;
				return sl_true;
			}

			static int createHandle(pcap_t*& handle, const StringView& name, const PcapParam& param)
			{
#ifdef SLIB_PLATFORM_IS_LINUX_DESKTOP
				if (!(slib::pcap::getApi_pcap_create())) {
					return PCAP_ERROR;
				}
#endif
				char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
#ifdef SLIB_PLATFORM_IS_WIN32
				if (name.startsWith('{')) {
					String _name = StringView::literal("\\Device\\NPF_") + name;
					handle = pcap_create(_name.getData(), errBuf);
				} else {
					handle = pcap_create(name.getData(), errBuf);
				}
#else
				handle = pcap_create(name.getData(), errBuf);
#endif
				if (handle) {
					pcap_set_snaplen(handle, MAX_PACKET_SIZE);
					pcap_set_buffer_size(handle, (int)(param.bufferSize));
#ifdef SLIB_PLATFORM_IS_WIN32
					pcap_setuserbuffer(handle, (int)(param.bufferSize));
#endif
					pcap_set_promisc(handle, (int)(param.flagPromiscuous));
					pcap_set_timeout(handle, (int)(param.timeout));
					pcap_set_immediate_mode(handle, (int)(param.flagImmediate));
					pcap_setnonblock(handle, 1, errBuf);
					int iRet = pcap_activate(handle);
					if (iRet < 0) {
						if (iRet != PCAP_ERROR_NO_SUCH_DEVICE) {
							LOG_ERROR("Failed to activate: %s", name);
						}
						pcap_close(handle);
						handle = sl_null;
					}
					return iRet;
				} else {
					LOG_ERROR(errBuf);
				}
				return PCAP_ERROR;
			}

			void release() override
			{
				ObjectLocker lock(this);
				if (!m_flagInit) {
					return;
				}
				m_flagInit = sl_false;

				if (m_threadReceive.isNotNull()) {
					m_threadReceive->finishAndWait();
					m_threadReceive.setNull();
				}
#ifdef SLIB_PLATFORM_IS_WIN32
				if (m_threadSend.isNotNull()) {
					m_threadSend->finishAndWait();
					m_threadSend.setNull();
				}
				if (m_queueSend) {
					pcap_sendqueue_destroy(m_queueSend);
				}
#endif
				pcap_close(m_handle);
			}

			void start() override
			{
				ObjectLocker lock(this);
				if (!m_flagInit) {
					return;
				}
				if (m_threadReceive.isNotNull()) {
					m_threadReceive->start();
				}
#ifdef SLIB_PLATFORM_IS_WIN32
				if (m_threadSend.isNotNull()) {
					m_threadSend->start();
				}
#endif
			}

			sl_bool isRunning() override
			{
				if (m_threadReceive.isNotNull()) {
					return m_threadReceive->isRunning();
				}
				return sl_false;
			}

			static void onHandlePacket(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
			{
				PcapImpl* pcap = (PcapImpl*)user;
				if (pcap->increaseReference() > 1) {
					NetCapturePacket packet;
					packet.data = (sl_uint8*)bytes;
					packet.length = (sl_uint32)(h->len);
					sl_uint32 caplen = (sl_uint32)(h->caplen);
					if (packet.length > caplen) {
						packet.length = caplen;
					}
					sl_uint64 t = h->ts.tv_sec;
					t = t * 1000000 + h->ts.tv_usec;
					packet.time = t;
					pcap->m_lastPacketTickForAnyPcap = g_tickForAnyPcap;
					pcap->_onCapturePacket(packet);
					pcap->decreaseReference();
				} else {
					pcap->_decreaseReference();
				}
			}

			void runReceive()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
#ifdef SLIB_PLATFORM_IS_WIN32
				HANDLE hEvent = pcap_getevent(m_handle);
				if (!hEvent) {
					return;
				}
				Ref<Event> ev = Win32::createEvent(hEvent, sl_false);
				if (ev.isNull()) {
					return;
				}
#else
				int fd = pcap_get_selectable_fd(m_handle);
				if (fd == -1) {
					return;
				}
				Ref<PipeEvent> ev = PipeEvent::create();
				if (ev.isNull()) {
					return;
				}
#endif
				while (thread->isNotStopping()) {
					int result = pcap_dispatch(m_handle, -1, &onHandlePacket, (u_char*)this);
					if (result < 0) {
						if (result == -1) {
							if (increaseReference() > 1) {
								_onError();
								decreaseReference();
							} else {
								_decreaseReference();
							}
						}
						break;
					} else if (!result) {
						if (thread->isStopping()) {
							break;
						}
#ifdef SLIB_PLATFORM_IS_WIN32
						ev->wait(10000);
#else
						ev->waitReadFd(fd, 10000);
#endif
					}
				}
			}

#ifdef SLIB_PLATFORM_IS_WIN32
			void runSend()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				while (thread->isNotStopping()) {
					{
						MutexLocker lock(&m_lockSend);
						pcap_sendqueue_transmit(m_handle, m_queueSend, 0);
						m_queueSend->len = 0;
					}
					m_eventSend->wait();
				}
			}
#endif

			NetworkCaptureType getType() override
			{
				sl_uint32 dt = pcap_datalink(m_handle);
				switch (dt) {
					case DLT_RAW:
						return NetworkCaptureType::Raw;
					default:
						return (NetworkCaptureType)dt;
				}
			}

			sl_bool setType(NetworkCaptureType _type) override
			{
				sl_uint32 type = (sl_uint32)_type;
				if (_type == NetworkCaptureType::Raw) {
					type = DLT_RAW;
				}
				int ret = pcap_set_datalink(m_handle, type);
				return !ret;
			}

			sl_bool sendPacket(const void* data, sl_uint32 size) override
			{
				if (!m_flagInit) {
					return sl_false;
				}
#ifdef SLIB_PLATFORM_IS_WIN32
				pcap_pkthdr hdr = { 0 };
				hdr.caplen = size;
				hdr.len = size;
				MutexLocker lock(&m_lockSend);
				int ret = pcap_sendqueue_queue(m_queueSend, &hdr, (const sl_uint8*)data);
				if (!ret) {
					lock.unlock();
					m_eventSend->set();
					return sl_true;
				}
#else
				int ret = pcap_sendpacket(m_handle, (const sl_uint8*)data, size);
				if (!ret) {
					return sl_true;
				}
#endif
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

	}
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PcapDeviceInfo)

	PcapDeviceInfo::PcapDeviceInfo() : flagLoopback(sl_false), flagUp(sl_false), flagRunning(sl_false), flagWireless(sl_false), connectionStatus(PcapConnectionStatus::Unknown)
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PcapParam)

	PcapParam::PcapParam()
	{
		timeout = 0; // no timeout specified
		flagImmediate = sl_true;
		bufferSize = 0x200000; // 2MB
		threadPriority = ThreadPriority::Normal;
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
		return Ref<Pcap>::cast(PcapImpl::create(param));
	}

	namespace
	{
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
	}

	List<PcapDeviceInfo> Pcap::getAllDevices()
	{
#ifdef PCAP_DYNAMIC_LIB
		if (!(pcap_findalldevs)) {
			return sl_null;
		}
#endif
		List<PcapDeviceInfo> ret;
		char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
		pcap_if_t* devs = NULL;
		int iRet = pcap_findalldevs(&devs, errBuf);
		if (!iRet && devs) {
			pcap_if_t* dev = devs;
			while (dev) {
				PcapDeviceInfo item;
				ParseDeviceInfo(dev, item);
				ret.add_NoLock(item);
				dev = dev->next;
			}
			pcap_freealldevs(devs);
		} else {
			LOG_ERROR(errBuf);
		}
		return ret;
	}

	sl_bool Pcap::findDevice(const StringView& name, PcapDeviceInfo& _out)
	{
#ifdef PCAP_DYNAMIC_LIB
		if (!(pcap_findalldevs)) {
			return sl_false;
		}
#endif
		if (name.isEmpty()) {
			return sl_false;
		}
		char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
		pcap_if_t* devs = NULL;
		int iRet = pcap_findalldevs(&devs, errBuf);
		if (!iRet && devs) {
			pcap_if_t* dev = devs;
			while (dev) {
				if (name == dev->name || name == dev->description) {
					ParseDeviceInfo(dev, _out);
					return sl_true;
				}
#ifdef SLIB_PLATFORM_IS_WIN32
				if (name.startsWith("{") && StringView(dev->name).startsWith("\\Device\\NPF_{")) {
					if (name == dev->name + 12) {
						ParseDeviceInfo(dev, _out);
						return sl_true;
					}
				}
#endif
				dev = dev->next;
			}
			pcap_freealldevs(devs);
		} else {
			LOG_ERROR(errBuf);
		}
		return sl_false;
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
		return sl_true;
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

	namespace
	{
		static void OnUpdateTick(Timer*)
		{
			g_tickForAnyPcap = System::getTickCount64();
		}

		class AnyDevicePcapImpl : public AnyDevicePcap
		{
		public:
			AnyDevicePcapParam m_param;
			CList< Ref<PcapImpl> > m_devices;
			AtomicRef<Timer> m_timerAddDevices;
			AtomicRef<Timer> m_timerUpdateTick;

		public:
			AnyDevicePcapImpl()
			{
			}

			~AnyDevicePcapImpl()
			{
				release();
			}

		public:
			static Ref<AnyDevicePcapImpl> create(const AnyDevicePcapParam& param)
			{
				Ref<AnyDevicePcapImpl> ret = new AnyDevicePcapImpl;
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
				m_timerUpdateTick.setNull();
				ObjectLocker lock(&m_devices);
				ListElements< Ref<PcapImpl> > devices(m_devices);
				for (sl_size i = 0; i < devices.count; i++) {
					devices[i]->release();
				}
				m_devices.removeAll_NoLock();
			}

			void start() override
			{
				ObjectLocker lock(this);
				if (m_timerAddDevices.isNotNull()) {
					return;
				}
				if (m_param.deviceTimeout) {
					OnUpdateTick(sl_null);
					m_timerUpdateTick = Timer::start(&OnUpdateTick, 1000);
				}
				onAddDevices(sl_null);
				m_timerAddDevices = Timer::start(SLIB_FUNCTION_MEMBER(this, onAddDevices), 10000);
			}

			sl_bool isRunning() override
			{
				return m_devices.getCount() > 0;
			}

			NetworkCaptureType getType() override
			{
				return NetworkCaptureType::Any;
			}

			sl_bool sendPacket(const void* frame, sl_uint32 size) override
			{
				return sl_false;
			}

			List< Ref<Pcap> > getDevices() override
			{
				return reinterpret_cast<CList< Ref<Pcap> >*>(m_devices.duplicate());
			}

		protected:
			Ref<NetCapture> findDevice(const StringParam& _name)
			{
				ListLocker< Ref<PcapImpl> > devices(m_devices);
				if (devices.count) {
					StringData name(_name);
					for (sl_size i = 0; i < devices.count; i++) {
						Ref<PcapImpl>& capture = devices[i];
						if (capture->getDeviceName() == name) {
							return capture;
						}
					}
				}
				return sl_null;
			}

			void checkDevices()
			{
				ListElements< Ref<PcapImpl> > devices(List< Ref<PcapImpl> >(m_devices.duplicate()));
				for (sl_size i = 0; i < devices.count; i++) {
					Ref<PcapImpl>& capture = devices[i];
					sl_bool flagRemove = sl_false;
					if (!(capture->isRunning())) {
						flagRemove = sl_true;
					} else if (m_param.deviceTimeout) {
						if (g_tickForAnyPcap > capture->m_lastPacketTickForAnyPcap + m_param.deviceTimeout) {
							flagRemove = sl_true;
						}
					}
					if (flagRemove) {
						removeDevice(capture);
					}
				}
			}

			void removeDevice(NetCapture* device)
			{
				if (!device) {
					return;
				}
				LOG("Removed device from any capture: %s", device->getDeviceName());
				device->release();
				m_devices.remove(device);
			}

			void addDevice(pcap_if_t* dev)
			{
				if ((dev->flags & PCAP_IF_LOOPBACK) || Base::equalsString(dev->name, "any")) {
					return;
				}
				Ref<NetCapture> capture = findDevice(dev->name);
				sl_uint32 status = (sl_uint32)(dev->flags & PCAP_IF_CONNECTION_STATUS);
				if (status == PCAP_IF_CONNECTION_STATUS_DISCONNECTED) {
					removeDevice(capture.get());
					return;
				}
				if (status != PCAP_IF_CONNECTION_STATUS_CONNECTED && !(dev->flags & PCAP_IF_UP)) {
					removeDevice(capture.get());
					return;
				}
				if (capture.isNotNull()) {
					return;
				}
				PcapParam param = m_param;
				param.deviceName = dev->name;
				param.onError = SLIB_BIND_WEAKREF(void(NetCapture*), this, onErrorDevice, param.onError);
				param.flagAutoStart = sl_true;
				Ref<PcapImpl> pcap = PcapImpl::create(param);
				if (pcap.isNotNull()) {
					m_devices.add_NoLock(Move(pcap));
					if (dev->description && dev->description[0]) {
						LOG("Added device to any capture: %s (%s)", param.deviceName, dev->description);
					} else {
						LOG("Added device to any capture: %s", param.deviceName);
					}
				} else {
					if (dev->description && dev->description[0]) {
						LOG("Failed to add device to any capture: %s (%s)", param.deviceName);
					} else {
						LOG("Failed to add device to any capture: %s (%s)", param.deviceName, dev->description);
					}
				}
			}

			void onAddDevices(Timer*)
			{
#ifdef PCAP_DYNAMIC_LIB
				if (!(pcap_findalldevs)) {
					return;
				}
#endif
				checkDevices();

				char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
				pcap_if_t* devs = NULL;
				int ret = pcap_findalldevs(&devs, errBuf);
				if (!ret && devs) {
					pcap_if_t* dev = devs;
					while (dev) {
						addDevice(dev);
						dev = dev->next;
					}
					pcap_freealldevs(devs);
				}
			}

			void onErrorDevice(const Function<void(NetCapture*)>& onError, NetCapture* capture)
			{
				auto thiz = ToWeakRef(this);
				Dispatch::dispatch([this, thiz, capture]() {
					auto ref = ToRef(thiz);
					if (ref.isNull()) {
						return;
					}
					removeDevice(capture);
				});
				onError(capture);
			}

		};
	}
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AnyDevicePcapParam)

	AnyDevicePcapParam::AnyDevicePcapParam()
	{
		deviceTimeout = 0;
	}

	SLIB_DEFINE_OBJECT(AnyDevicePcap, Pcap)

	AnyDevicePcap::AnyDevicePcap()
	{
	}

	AnyDevicePcap::~AnyDevicePcap()
	{
	}

	Ref<AnyDevicePcap> AnyDevicePcap::create(const AnyDevicePcapParam& param)
	{
		return AnyDevicePcapImpl::create(param);
	}

	Ref<Pcap> Pcap::createAny(const AnyDevicePcapParam& param)
	{
		return Ref<Pcap>::cast(AnyDevicePcapImpl::create(param));
	}

}
