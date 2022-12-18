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

#ifndef CHECKHEADER_SLIB_NETWORK_TCPIP_FRAGMENT
#define CHECKHEADER_SLIB_NETWORK_TCPIP_FRAGMENT

#include "tcpip.h"

#include "../core/memory.h"
#include "../data/expiring_map.h"

namespace slib
{

	class SLIB_EXPORT IPv4Fragment
	{
	public:
		sl_uint16 offset;
		Memory data;

	public:
		IPv4Fragment();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(IPv4Fragment)

	};

	class SLIB_EXPORT IPv4FragmentedPacket : public Referable
	{
	public:
		Memory header;
		List<IPv4Fragment> fragments;

	public:
		IPv4FragmentedPacket();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(IPv4FragmentedPacket)

	};

	class SLIB_EXPORT IPv4Fragmentation
	{
	public:
		IPv4Fragmentation();

		~IPv4Fragmentation();

	public:
		void setupExpiringDuration(sl_uint32 ms, const Ref<DispatchLoop>& loop);

		void setupExpiringDuration(sl_uint32 ms);

		static sl_bool isNeededReassembly(const IPv4Packet* packet);

		Memory reassemble(const IPv4Packet* packet);

		static sl_bool isNeededFragmentation(const IPv4Packet* packet, sl_uint16 mtu = 1500);

		static List<Memory> makeFragments(const IPv4Packet* packet, sl_uint16 mtu = 1500);

	protected:
		ExpiringMap< IPv4PacketIdentifier, Ref<IPv4FragmentedPacket> > m_packets;

	};

}

#endif
