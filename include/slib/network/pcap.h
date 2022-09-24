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

#ifndef CHECKHEADER_SLIB_NETWORK_PCAP
#define CHECKHEADER_SLIB_NETWORK_PCAP

#include "capture.h"

#include "ip_address.h"
#include "mac_address.h"

namespace slib
{

	enum class PcapConnectionStatus
	{
		Unknown = 0,
		Connected = 1,
		Disconnected = 2,
		NotApplicable = 3
	};

	class SLIB_EXPORT PcapDeviceInfo
	{
	public:
		String name;
		String description;
		sl_bool flagLoopback;
		sl_bool flagUp;
		sl_bool flagRunning;
		sl_bool flagWireless;
		PcapConnectionStatus connectionStatus;

		List<IPv4Address> ipv4Addresses;
		List<IPv6Address> ipv6Addresses;

	public:
		PcapDeviceInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PcapDeviceInfo)

	};

	class SLIB_EXPORT PcapParam : public NetCaptureParam
	{
	public:
		sl_uint32 timeoutRead; // read timeout, in milliseconds
		sl_bool flagImmediate; // immediate mode
		sl_uint32 sizeBuffer; // buffer size

	public:
		PcapParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PcapParam)

	};

	class Pcap : public NetCapture
	{
		SLIB_DECLARE_OBJECT

	public:
		Pcap();

		~Pcap();

	public:
		static Ref<Pcap> create(const PcapParam& param);

		static List<PcapDeviceInfo> getAllDevices();

		static sl_bool findDevice(const StringView& name, PcapDeviceInfo& _out);
		
		static Ref<Pcap> createAny(const PcapParam& param);


		static sl_bool isAllowedNonRoot(const StringParam& executablePath);

		static sl_bool isAllowedNonRoot();

		static void allowNonRoot(const StringParam& executablePath);

		static void allowNonRoot();

	};

	class AnyDevicePcap : public Pcap
	{
		SLIB_DECLARE_OBJECT

	public:
		AnyDevicePcap();

		~AnyDevicePcap();

	public:
		static Ref<AnyDevicePcap> create(const PcapParam& param);

	public:
		virtual List< Ref<Pcap> > getDevices() = 0;

	};

}

#endif
