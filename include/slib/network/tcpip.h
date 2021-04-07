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

#ifndef CHECKHEADER_SLIB_NETWORK_TCPIP
#define CHECKHEADER_SLIB_NETWORK_TCPIP

#include "constants.h"
#include "ip_address.h"

/********************************************************************
					IPv4 Header from RFC 791

	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|Version|  IHL  |Type of Service|          Total Length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|         Identification        |Flags|      Fragment Offset    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|  Time to Live |    Protocol   |         Header Checksum       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Source Address                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Destination Address                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Options                    |    Padding    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


- by RFC 2474 and RFC 3168, TOS(Type Of Service) field is redefined as following
	0   1   2   3   4   5   6   7
	+---+---+---+---+---+---+---+---+
	|         DSCP          |  ECN  |


- Protocol is defined in RFC 790



			TCP Header from RFC 793, RFC 3168, RFC 3540

	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Source Port          |       Destination Port        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                        Sequence Number                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Acknowledgment Number                      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|  Data |Reser|N|C|E|U|A|P|R|S|F|                               |
	| Offset| ved |S|W|C|R|C|S|S|Y|I|            Window             |
	|       |     | |R|E|G|K|H|T|N|N|                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |         Urgent Pointer        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Options                    |    Padding    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                             data                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



					UDP Header from RFC 768

				0      7 8     15 16    23 24    31
				+--------+--------+--------+--------+
				|     Source      |   Destination   |
				|      Port       |      Port       |
				+--------+--------+--------+--------+
				|                 |                 |
				|     Length      |    Checksum     |
				+--------+--------+--------+--------+
				|
				|          data octets ...
				+---------------- ...

 
 
 
             IPv6 Header from RFC 2460, RFC 8200
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |Version| Traffic Class |           Flow Label                  |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |         Payload Length        |  Next Header  |   Hop Limit   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 +                                                               +
 |                                                               |
 +                         Source Address                        +
 |                                                               |
 +                                                               +
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                               |
 +                                                               +
 |                                                               |
 +                      Destination Address                      +
 |                                                               |
 +                                                               +
 |                                                               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

********************************************************************/

namespace slib
{
	class SLIB_EXPORT TCP_IP
	{
	public:
		static sl_uint16 calculateOneComplementSum(const void* data, sl_size size, sl_uint32 add = 0);

		static sl_uint16 calculateChecksum(const void* data, sl_size size);

	};

	class SLIB_EXPORT IPv4Packet
	{
	public:
		enum
		{
			HeaderSizeBeforeOptions = 20
		};

	public:
		// 4 bits, version is 4 for IPv4
		sl_uint8 getVersion() const
		{
			return _versionAndHeaderLength >> 4;
		}
		
		// 4 bits, version is 4 for IPv4
		void setVersion(sl_uint8 version = 4)
		{
			_versionAndHeaderLength = (_versionAndHeaderLength & 0x0F) | (version << 4);
		}

		// 4 bits, get the count of 32bit words for the header including options and padding
		sl_uint8 getHeaderLength() const
		{
			return _versionAndHeaderLength & 0x0F;
		}
		
		// 4 bits, set the count of 32bit words for the header including options and padding (5 if no option is included)
		void setHeaderLength(sl_uint8 length = 5)
		{
			_versionAndHeaderLength = (_versionAndHeaderLength & 0xF0) | (length & 0x0F);
		}
		
		// header size in bytes
		sl_uint8 getHeaderSize() const
		{
			return (_versionAndHeaderLength & 0x0F) << 2;
		}
		
		// header size in bytes
		void setHeaderSize(sl_uint8 size)
		{
			setHeaderLength((size + 3) >> 2);
		}
		
		// 8 bits, TOS is deprecated and replaced with DSCP&ECN
		sl_uint8 getTypeOfService() const
		{
			return m_TOS_DSCP_ECN;
		}
		
		// 8 bits, TOS is deprecated and replaced with DSCP&ECN
		void setTypeOfService(sl_uint8 TOS)
		{
			m_TOS_DSCP_ECN = TOS;
		}
		
		// 6 bits
		sl_uint8 getDSCP() const
		{
			return ((m_TOS_DSCP_ECN >> 2) & 0x3F);
		}
		
		// 6 bits
		void setDSCP(sl_uint8 DSCP)
		{
			m_TOS_DSCP_ECN = (sl_uint8)((m_TOS_DSCP_ECN & 3) | ((DSCP & 0x3F) << 2));
		}
		
		// 2 bits
		sl_uint8 getECN() const
		{
			return (m_TOS_DSCP_ECN & 3);
		}
		
		// 2 bits
		void setECN(sl_uint8 ECN)
		{
			m_TOS_DSCP_ECN = (sl_uint8)((m_TOS_DSCP_ECN & 0xFC) | (ECN & 3));
		}
		
		// 16 bits, total size (including header and data) in bytes
		sl_uint16 getTotalSize() const
		{
			return ((sl_uint16)(_totalLength[0]) << 8) | ((sl_uint16)(_totalLength[1]));
		}
		
		// 16 bits, total size (including header and data) in bytes
		void setTotalSize(sl_uint16 size)
		{
			_totalLength[0] = (sl_uint8)(size >> 8);
			_totalLength[1] = (sl_uint8)(size);
		}

		// 16 bits
		sl_uint16 getIdentification() const
		{
			return ((sl_uint16)(_identification[0]) << 8) | ((sl_uint16)(_identification[1]));
		}
		
		// 16 bits
		void setIdentification(sl_uint16 identification)
		{
			_identification[0] = (sl_uint8)(identification >> 8);
			_identification[1] = (sl_uint8)(identification);
		}

		// true = Do not fragment, false = Fragment
		sl_bool isDF() const
		{
			return (_flagsAndFragmentOffset[0] & 0x40) != 0;
		}
		
		// true = Do not fragment, false = Fragment
		void setDF(sl_bool flag)
		{
			_flagsAndFragmentOffset[0] = (sl_uint8)((_flagsAndFragmentOffset[0] & 0xBF) | (flag ? 0x40 : 0));
		}

		// true = More Fragment, false = Last Fragment
		sl_bool isMF() const
		{
			return (_flagsAndFragmentOffset[0] & 0x20) != 0;
		}
		
		// true = More Fragment, false = Last Fragment
		void setMF(sl_bool flag)
		{
			_flagsAndFragmentOffset[0] = (sl_uint8)((_flagsAndFragmentOffset[0] & 0xDF) | (flag ? 0x20 : 0));
		}
		
		// 13 bits, the fragment offset measured in units of 8 octets (64 bits)
		sl_uint16 getFragmentOffset() const
		{
			return (((sl_uint16)(_flagsAndFragmentOffset[0] & 0x1F)) << 8) | _flagsAndFragmentOffset[1];
		}
		
		// 13 bits, the fragment offset measured in units of 8 octets (64 bits)
		void setFragmentOffset(sl_uint16 offset)
		{
			_flagsAndFragmentOffset[1] = (sl_uint8)offset;
			_flagsAndFragmentOffset[0] = (sl_uint8)((_flagsAndFragmentOffset[0] & 0xE0) | ((offset >> 8) & 0x1F));
		}
		
		// Time To Live
		sl_uint8 getTTL() const
		{
			return _timeToLive;
		}
		
		// Time To Live
		void setTTL(sl_uint8 TTL)
		{
			_timeToLive = TTL;
		}
		
		NetworkInternetProtocol getProtocol() const
		{
			return (NetworkInternetProtocol)_protocol;
		}
		
		void setProtocol(NetworkInternetProtocol protocol)
		{
			_protocol = (sl_uint8)(protocol);
		}
		
		sl_uint16 getChecksum() const
		{
			return ((sl_uint16)(_headerChecksum[0]) << 8) | ((sl_uint16)(_headerChecksum[1]));
		}
		
		void setChecksum(sl_uint16 checksum)
		{
			_headerChecksum[0] = (sl_uint8)(checksum >> 8);
			_headerChecksum[1] = (sl_uint8)(checksum);
		}
		
		IPv4Address getSourceAddress() const
		{
			return {_sourceIp[0], _sourceIp[1], _sourceIp[2], _sourceIp[3]};
		}
		
		void setSourceAddress(const IPv4Address& address)
		{
			_sourceIp[0] = address.a;
			_sourceIp[1] = address.b;
			_sourceIp[2] = address.c;
			_sourceIp[3] = address.d;
		}
		
		IPv4Address getDestinationAddress() const
		{
			return {_destinationIp[0], _destinationIp[1], _destinationIp[2], _destinationIp[3]};
		}
		
		void setDestinationAddress(const IPv4Address& address)
		{
			_destinationIp[0] = address.a;
			_destinationIp[1] = address.b;
			_destinationIp[2] = address.c;
			_destinationIp[3] = address.d;
		}
		
		const sl_uint8* getOptions() const
		{
			return (const sl_uint8*)(this) + sizeof(IPv4Packet);
		}
		
		sl_uint8* getOptions()
		{
			return (sl_uint8*)(this) + sizeof(IPv4Packet);
		}
		
		const sl_uint8* getContent() const
		{
			return (const sl_uint8*)(this) + getHeaderSize();
		}
		
		sl_uint8* getContent()
		{
			return (sl_uint8*)(this) + getHeaderSize();
		}
		
		sl_uint16 getContentSize() const
		{
			return getTotalSize() - getHeaderSize();
		}
	
public:
		void updateChecksum();
		
		sl_bool checkChecksum() const;

		// used in TCP/UDP protocol
		sl_uint16 getChecksumForContent(const void* content, sl_uint16 size) const;

#ifdef check
#undef check
#endif
		
		static sl_bool check(const void* packet, sl_size sizePacket);

		static sl_bool checkSize(const void* packet, sl_size sizePacket);
		
		static sl_bool checkHeader(const void* packet, sl_size sizePacket);
		
		static sl_bool checkHeaderSize(const void* packet, sl_size sizePacket);

		sl_bool getPortsForTcpUdp(sl_uint16& src, sl_uint16& dst) const;

	private:
		sl_uint8 _versionAndHeaderLength;
		sl_uint8 m_TOS_DSCP_ECN;
		sl_uint8 _totalLength[2];
		sl_uint8 _identification[2];
		sl_uint8 _flagsAndFragmentOffset[2];
		sl_uint8 _timeToLive;
		sl_uint8 _protocol;
		sl_uint8 _headerChecksum[2];
		sl_uint8 _sourceIp[4];
		sl_uint8 _destinationIp[4];
		// options and padding
		
	};

	class SLIB_EXPORT IPv6Packet
	{
	public:
		enum
		{
			HeaderSize = 40
		};
		
	public:
		// 4 bits, version is 6 for IPv6
		sl_uint8 getVersion() const
		{
			return _version_TrafficClass_FlowLabel[0] >> 4;
		}
		
		// 4 bits, version is 6 for IPv6
		void setVersion(sl_uint8 version = 6)
		{
			_version_TrafficClass_FlowLabel[0] = (_version_TrafficClass_FlowLabel[0] & 0x0F) | (version << 4);
		}
		
		sl_uint8 getTrafficClass() const
		{
			return ((_version_TrafficClass_FlowLabel[0] & 0x0F) << 4) | (_version_TrafficClass_FlowLabel[1] >> 4);
		}
		
		void setTrafficClass(sl_uint8 value)
		{
			_version_TrafficClass_FlowLabel[0] = (_version_TrafficClass_FlowLabel[0] & 0xF0) | (value >> 4);
			_version_TrafficClass_FlowLabel[1] = (_version_TrafficClass_FlowLabel[1] & 0x0F) | ((value & 0x0F) << 4);
		}
		
		sl_uint32 getFlowLabel() const
		{
			return ((sl_uint32)(_version_TrafficClass_FlowLabel[1] & 0x0F) << 16) | ((sl_uint32)(_version_TrafficClass_FlowLabel[2]) << 8) | _version_TrafficClass_FlowLabel[3];
		}
		
		void setFlowLabel(sl_uint32 value)
		{
			_version_TrafficClass_FlowLabel[1] = (_version_TrafficClass_FlowLabel[0] & 0xF0) | (sl_uint8)((value >> 16) & 0x0F);
			_version_TrafficClass_FlowLabel[2] = (sl_uint8)(value >> 8);
			_version_TrafficClass_FlowLabel[3] = (sl_uint8)(value);
		}
		
		sl_uint16 getPayloadLength() const
		{
			return ((sl_uint16)(_payloadLength[0]) << 8) | ((sl_uint16)(_payloadLength[1]));
		}
		
		void setPayloadLength(sl_uint16 length)
		{
			_payloadLength[0] = (sl_uint8)(length >> 8);
			_payloadLength[1] = (sl_uint8)(length);
		}
		
		NetworkInternetProtocol getNextHeader() const
		{
			return (NetworkInternetProtocol)(_nextHeader);
		}
		
		void setNextHeader(NetworkInternetProtocol protocol)
		{
			_nextHeader = (sl_uint8)(protocol);
		}
		
		sl_uint8 getHopLimit() const
		{
			return _hopLimit;
		}
		
		void setHopLimit(sl_uint8 limit)
		{
			_hopLimit = limit;
		}
		
		IPv6Address getSourceAddress() const
		{
			return IPv6Address(_sourceAddress);
		}
		
		void setSourceAddresss(const IPv6Address& address)
		{
			address.getBytes(_sourceAddress);
		}
		
		IPv6Address getDestinationAddress() const
		{
			return IPv6Address(_destinationAddress);
		}
		
		void setDestinationAddresss(const IPv6Address& address)
		{
			address.getBytes(_destinationAddress);
		}
		
		const sl_uint8* getContent() const
		{
			return (const sl_uint8*)(this) + HeaderSize;
		}
		
		sl_uint8* getContent()
		{
			return (sl_uint8*)(this) + HeaderSize;
		}

	public:
		// used in TCP/UDP protocol
		sl_uint16 getChecksumForContent(const void* content, sl_uint32 size) const;
		
		static sl_bool check(const void* packet, sl_size sizePacket);
		
		static sl_bool checkHeader(const void* packet, sl_size sizePacket);
		
	private:
		sl_uint8 _version_TrafficClass_FlowLabel[4];
		sl_uint8 _payloadLength[2];
		sl_uint8 _nextHeader;
		sl_uint8 _hopLimit;
		sl_uint8 _sourceAddress[16];
		sl_uint8 _destinationAddress[16];
		
	};

	
	class SLIB_EXPORT TcpSegment
	{
	public:
		enum
		{
			HeaderSizeBeforeOptions = 20
		};
		
	public:
		sl_uint16 getSourcePort() const
		{
			return ((sl_uint16)(_sourcePort[0]) << 8) | ((sl_uint16)(_sourcePort[1]));
		}
		
		void setSourcePort(sl_uint16 port)
		{
			_sourcePort[0] = (sl_uint8)(port >> 8);
			_sourcePort[1] = (sl_uint8)(port);
		}
		
		sl_uint16 getDestinationPort() const
		{
			return ((sl_uint16)(_destinationPort[0]) << 8) | ((sl_uint16)(_destinationPort[1]));
		}
		
		void setDestinationPort(sl_uint16 port)
		{
			_destinationPort[0] = (sl_uint8)(port >> 8);
			_destinationPort[1] = (sl_uint8)(port);
		}
		
		sl_uint32 getSequenceNumber() const
		{
			return ((sl_uint32)(_sequenceNumber[0]) << 24) | ((sl_uint32)(_sequenceNumber[1]) << 16) | ((sl_uint32)(_sequenceNumber[2]) << 8) | ((sl_uint32)(_sequenceNumber[3]));
		}
		
		void setSequenceNumber(sl_uint32 num)
		{
			_sequenceNumber[0] = (sl_uint8)(num >> 24);
			_sequenceNumber[1] = (sl_uint8)(num >> 16);
			_sequenceNumber[2] = (sl_uint8)(num >> 8);
			_sequenceNumber[3] = (sl_uint8)(num);
		}

		sl_uint32 getAcknowledgmentNumber() const
		{
			return ((sl_uint32)(_acknowledgmentNumber[0]) << 24) | ((sl_uint32)(_acknowledgmentNumber[1]) << 16) | ((sl_uint32)(_acknowledgmentNumber[2]) << 8) | ((sl_uint32)(_acknowledgmentNumber[3]));
		}
		
		void setAcknowledgmentNumber(sl_uint32 num)
		{
			_acknowledgmentNumber[0] = (sl_uint8)(num >> 24);
			_acknowledgmentNumber[1] = (sl_uint8)(num >> 16);
			_acknowledgmentNumber[2] = (sl_uint8)(num >> 8);
			_acknowledgmentNumber[3] = (sl_uint8)(num);
		}
		
		// 4 bits, the size of the TCP header in 32-bit words
		sl_uint8 getHeaderLength() const
		{
			return _dataOffsetAndFlags[0] >> 4;
		}
		
		// 4 bits, the size of the TCP header in 32-bit words
		void setHeaderLength(sl_uint8 length = 5)
		{
			_dataOffsetAndFlags[0] = (sl_uint8)((_dataOffsetAndFlags[0] & 0x0F) | (length << 4));
		}

		// header size in bytes
		sl_uint8 getHeaderSize() const
		{
			return (_dataOffsetAndFlags[0] >> 4) << 2;
		}
		
		// header size in bytes
		void setHeaderSize(sl_uint8 size)
		{
			setHeaderLength((size + 3) >> 2);
		}

		sl_bool isNS() const
		{
			return (_dataOffsetAndFlags[0] & 1) != 0;
		}
		
		void setNS(sl_bool flag)
		{
			_dataOffsetAndFlags[0] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xFE) | (flag ? 1 : 0));
		}
		
		sl_bool isCWR() const
		{
			return (_dataOffsetAndFlags[1] & 0x80) != 0;
		}
		
		void setCWR(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0x7F) | (flag ? 0x80 : 0));
		}
		
		sl_bool isECE() const
		{
			return (_dataOffsetAndFlags[1] & 0x40) != 0;
		}
		
		void setECE(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xBF) | (flag ? 0x40 : 0));
		}
		
		sl_bool isURG() const
		{
			return (_dataOffsetAndFlags[1] & 0x20) != 0;
		}
		
		void setURG(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xDF) | (flag ? 0x20 : 0));
		}
		
		sl_bool isACK() const
		{
			return (_dataOffsetAndFlags[1] & 0x10) != 0;
		}
		
		void setACK(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xEF) | (flag ? 0x10 : 0));
		}

		sl_bool isPSH() const
		{
			return (_dataOffsetAndFlags[1] & 0x08) != 0;
		}
		
		void setPSH(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xF7) | (flag ? 0x08 : 0));
		}
		
		sl_bool isRST() const
		{
			return (_dataOffsetAndFlags[1] & 0x04) != 0;
		}
		
		void setRST(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xFB) | (flag ? 0x04 : 0));
		}
		
		sl_bool isSYN() const
		{
			return (_dataOffsetAndFlags[1] & 0x02) != 0;
		}
		
		void setSYN(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xFD) | (flag ? 0x02 : 0));
		}

		sl_bool isFIN() const
		{
			return (_dataOffsetAndFlags[1] & 0x01) != 0;
		}
		
		void setFIN(sl_bool flag)
		{
			_dataOffsetAndFlags[1] = (sl_uint8)((_dataOffsetAndFlags[0] & 0xFE) | (flag ? 0x01 : 0));
		}
		
		sl_uint16 getWindowSize() const
		{
			return ((sl_uint16)(_windowSize[0]) << 8) | ((sl_uint16)(_windowSize[1]));
		}
		
		void setWindowSize(sl_uint16 size)
		{
			_windowSize[0] = (sl_uint8)(size >> 8);
			_windowSize[1] = (sl_uint8)(size);
		}
		
		sl_uint16 getChecksum() const
		{
			return ((sl_uint16)(_checksum[0]) << 8) | ((sl_uint16)(_checksum[1]));
		}
		
		void setChecksum(sl_uint16 checksum)
		{
			_checksum[0] = (sl_uint8)(checksum >> 8);
			_checksum[1] = (sl_uint8)(checksum);
		}

		sl_uint16 getUrgentPointer() const
		{
			return ((sl_uint16)(_urgentPointer[0]) << 8) | ((sl_uint16)(_urgentPointer[1]));
		}
		
		void setUrgentPointer(sl_uint16 urgentPointer)
		{
			_urgentPointer[0] = (sl_uint8)(urgentPointer >> 8);
			_urgentPointer[1] = (sl_uint8)(urgentPointer);
		}
		
		const sl_uint8* getOptions() const
		{
			return (const sl_uint8*)(this) + HeaderSizeBeforeOptions;
		}
		
		sl_uint8* getOptions()
		{
			return (sl_uint8*)(this) + HeaderSizeBeforeOptions;
		}
		
		const sl_uint8* getContent() const
		{
			return (const sl_uint8*)(this) + getHeaderSize();
		}
		
		sl_uint8* getContent()
		{
			return (sl_uint8*)(this) + getHeaderSize();
		}

	public:
		sl_bool checkSize(sl_size sizeTcp) const;
		
		void updateChecksum(const IPv4Packet* ipv4, sl_size sizeTcp);
		
		sl_bool checkChecksum(const IPv4Packet* ipv4, sl_size sizeTcp) const;

		sl_bool check(IPv4Packet* ip, sl_size sizeTcp) const;

		void updateChecksum(const IPv6Packet* ipv4, sl_size sizeTcp);
		
		sl_bool checkChecksum(const IPv6Packet* ipv4, sl_size sizeTcp) const;
		
		sl_bool check(IPv6Packet* ip, sl_size sizeTcp) const;

	private:
		sl_uint8 _sourcePort[2];
		sl_uint8 _destinationPort[2];
		sl_uint8 _sequenceNumber[4];
		sl_uint8 _acknowledgmentNumber[4];
		sl_uint8 _dataOffsetAndFlags[2];
		sl_uint8 _windowSize[2];
		sl_uint8 _checksum[2];
		sl_uint8 _urgentPointer[2];
		// options and padding
		
	};


	class SLIB_EXPORT UdpDatagram
	{
	public:
		enum
		{
			HeaderSize = 8
		};
		
	public:
		sl_uint16 getSourcePort() const
		{
			return ((sl_uint16)(_sourcePort[0]) << 8) | ((sl_uint16)(_sourcePort[1]));
		}
		
		void setSourcePort(sl_uint16 port)
		{
			_sourcePort[0] = (sl_uint8)(port >> 8);
			_sourcePort[1] = (sl_uint8)(port);
		}
		
		sl_uint16 getDestinationPort() const
		{
			return ((sl_uint16)(_destinationPort[0]) << 8) | ((sl_uint16)(_destinationPort[1]));
		}
		
		void setDestinationPort(sl_uint16 port)
		{
			_destinationPort[0] = (sl_uint8)(port >> 8);
			_destinationPort[1] = (sl_uint8)(port);
		}
		
		// including header and data
		sl_uint16 getTotalSize() const
		{
			return ((sl_uint16)(_length[0]) << 8) | ((sl_uint16)(_length[1]));
		}
		
		// including header and data
		void setTotalSize(sl_uint16 size)
		{
			_length[0] = (sl_uint8)(size >> 8);
			_length[1] = (sl_uint8)(size);
		}

		sl_uint16 getChecksum() const
		{
			return ((sl_uint16)(_checksum[0]) << 8) | ((sl_uint16)(_checksum[1]));
		}
		
		void setChecksum(sl_uint16 checksum)
		{
			_checksum[0] = (sl_uint8)(checksum >> 8);
			_checksum[1] = (sl_uint8)(checksum);
		}
		
		const sl_uint8* getContent() const
		{
			return (const sl_uint8*)(this) + HeaderSize;
		}
		
		sl_uint8* getContent()
		{
			return (sl_uint8*)(this) + HeaderSize;
		}
		
		sl_uint16 getContentSize() const
		{
			return getTotalSize() - HeaderSize;
		}
		
	public:
		sl_bool checkSize(sl_size sizeUdp) const;
		
		void updateChecksum(const IPv4Packet* ipv4);
		
		sl_bool checkChecksum(const IPv4Packet* ipv4) const;
		
		sl_bool check(IPv4Packet* ipv4, sl_size sizeUdp) const;

		void updateChecksum(const IPv6Packet* ipv6);
		
		sl_bool checkChecksum(const IPv6Packet* ipv6) const;
		
		sl_bool check(IPv6Packet* ipv6, sl_size sizeUdp) const;

	private:
		sl_uint8 _sourcePort[2];
		sl_uint8 _destinationPort[2];
		sl_uint8 _length[2];
		sl_uint8 _checksum[2];
		
	};


	class SLIB_EXPORT IPv4PacketIdentifier
	{
	public:
		IPv4Address source;
		IPv4Address destination;
		sl_uint16 identification;
		NetworkInternetProtocol protocol;

	public:
		sl_bool operator==(const IPv4PacketIdentifier& other) const;

	public:
		sl_size getHashCode() const;
		
	};

	template <>
	class Compare<IPv4PacketIdentifier>
	{
	public:
		sl_compare_result operator()(const IPv4PacketIdentifier& a, const IPv4PacketIdentifier& b) const;
		
	};

	template <>
	class Hash<IPv4PacketIdentifier>
	{
	public:
		sl_size operator()(const IPv4PacketIdentifier& v) const;
		
	};
	
}

#endif
