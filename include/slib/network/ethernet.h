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

#ifndef CHECKHEADER_SLIB_NETWORK_ETHERNET
#define CHECKHEADER_SLIB_NETWORK_ETHERNET

#include "constants.h"
#include "mac_address.h"

#include "../core/hash_map.h"

/*
	Ethernet II Frame (Layer 2)

	https://en.wikipedia.org/wiki/Ethernet_frame#Ethernet_II

	Header (14 bytes)
		0~5: Destination MAC Address
		6~11: Source MAC Address
		12~13: EtherType (Protocol)
		14~: Payload (46~1500 bytes)
*/

namespace slib
{

	class SLIB_EXPORT EthernetFrame
	{
	public:
		enum
		{
			HeaderSize = 14
		};
		
	public:
		MacAddress getDestinationAddress()
		{
			return {_macDestination[0], _macDestination[1], _macDestination[2], _macDestination[3], _macDestination[4], _macDestination[5]};
		}
		
		void setDestinationAddress(const MacAddress& address)
		{
			_macDestination[0] = address.m[0];
			_macDestination[1] = address.m[1];
			_macDestination[2] = address.m[2];
			_macDestination[3] = address.m[3];
			_macDestination[4] = address.m[4];
			_macDestination[5] = address.m[5];
		}
		
		
		MacAddress getSourceAddress()
		{
			return {_macSource[0], _macSource[1], _macSource[2], _macSource[3], _macSource[4], _macSource[5]};
		}
		
		void setSourceAddress(const MacAddress& address)
		{
			_macSource[0] = address.m[0];
			_macSource[1] = address.m[1];
			_macSource[2] = address.m[2];
			_macSource[3] = address.m[3];
			_macSource[4] = address.m[4];
			_macSource[5] = address.m[5];
		}
		
		
		NetworkLinkProtocol getProtocol() const
		{
			return (NetworkLinkProtocol)(((sl_uint16)(_etherType[0]) << 8) | ((sl_uint16)(_etherType[1])));
		}
		
		void setProtocol(NetworkLinkProtocol _type)
		{
			sl_uint32 type = (sl_uint32)_type;
			_etherType[0] = (sl_uint8)(type >> 8);
			_etherType[1] = (sl_uint8)(type);
		}
		
		
		const sl_uint8* getContent() const
		{
			return ((const sl_uint8*)this) + HeaderSize;
		}
		
		sl_uint8* getContent()
		{
			return ((sl_uint8*)this) + HeaderSize;
		}
		
	private:
		sl_uint8 _macDestination[6];
		sl_uint8 _macSource[6];
		sl_uint8 _etherType[2];
		
	};
	
	
	class SLIB_EXPORT EthernetMacTable : public Object
	{
	public:
		EthernetMacTable();
		
		~EthernetMacTable();
		
	public:
		void add(const IPv4Address& ip, const MacAddress& mac);
		
		sl_bool getMacAddress(const IPv4Address& ip, MacAddress* _out = sl_null);
		
		void parseEthernetFrame(const void* frame, sl_uint32 sizeFrame, sl_bool flagUseSource, sl_bool flagUseDestination);
		
	protected:
		CHashMap<IPv4Address, MacAddress> m_table;
		
	};
	
}

#endif
