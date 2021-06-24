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

#ifndef CHECKHEADER_SLIB_NETWORK_ICMP
#define CHECKHEADER_SLIB_NETWORK_ICMP

/********************************************************************
				
				INTERNET CONTROL MESSAGE PROTOCOL

				https://tools.ietf.org/html/rfc792

	
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|     Type      |     Code      |          Checksum             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|						Rest of Header						    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|     Data ...
	+-+-+-+-+-

 - Types

 * Echo/Echo Reply Type

		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Type      |     Code      |          Checksum             |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|           Identifier          |        Sequence Number        |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Data ...
		+-+-+-+-+-

		Type
			0 = Echo Reply
			8 = Echo

		Code = 0

 * Destination Unreachable Message Type

		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Type      |     Code      |          Checksum             |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|                             unused                            |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|      Internet Header + 64 bits of Original Data Datagram      |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		Type = 3

		Code
			0 = net unreachable;
			1 = host unreachable;
			2 = protocol unreachable;
			3 = port unreachable;
			4 = fragmentation needed and DF set;
			5 = source route failed.

 * Redirect Message
		
		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Type      |     Code      |          Checksum             |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|                 Gateway Internet Address                      |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|      Internet Header + 64 bits of Original Data Datagram      |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		Type =	5

		Code
			0 = Redirect datagrams for the Network.
			1 = Redirect datagrams for the Host.
			2 = Redirect datagrams for the Type of Service and Network.
			3 = Redirect datagrams for the Type of Service and Host.

 * Time Exceeded Message
		
		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Type      |     Code      |          Checksum             |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|                             unused                            |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|      Internet Header + 64 bits of Original Data Datagram      |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		Type = 11

		Code
			0 = time to live exceeded in transit;
			1 = fragment reassembly time exceeded.


 * Parameter Problem Message

		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Type      |     Code      |          Checksum             |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|    Pointer    |                   unused                      |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|      Internet Header + 64 bits of Original Data Datagram      |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		Type = 12

		Code = 0


 * Timestamp or Timestamp Reply Message
		
		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Type      |      Code     |          Checksum             |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|           Identifier          |        Sequence Number        |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Originate Timestamp                                       |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Receive Timestamp                                         |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|     Transmit Timestamp                                        |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		Type
			13 = timestamp message;
			14 = timestamp reply message.

		Code = 0

********************************************************************/

#include "constants.h"
#include "ip_address.h"

namespace slib
{

	enum class IcmpType
	{
		EchoReply = 0,
		DestinationUnreachable = 3,
		Redirect = 5,
		Echo = 8,
		TimeExceeded = 11,
		ParameterProblem = 12,
		Timestamp = 13,
		TimestampReply = 14,
		AddressMaskRequest = 17,
		AddressMaskReply = 18
	};
	
	class SLIB_EXPORT IcmpHeaderFormat
	{
	public:
		IcmpType getType() const;
		
		void setType(IcmpType type);
		
		sl_uint8 getCode() const;
		
		void setCode(sl_uint8 code);
		
		sl_uint16 getChecksum() const;
		
		void setChecksum(sl_uint16 checksum);
		
	public:
		void updateChecksum(sl_uint32 sizeICMP);
		
		sl_bool checkChecksum(sl_uint32 sizeICMP) const;
		
		sl_bool check(sl_uint32 sizeICMP) const;
		
	public:
		sl_uint16 getEchoIdentifier() const;
		
		void setEchoIdentifier(sl_uint16 id);
		
		sl_uint16 getEchoSequenceNumber() const;
		
		void setEchoSequenceNumber(sl_uint16 sn);
		
		IPv4Address getRedirectGatewayAddress() const;
		
		void setRedirectGatewayAddress(const IPv4Address& address);
		
		sl_uint8 getParameterProblemPointer() const;
		
		void setParameterProblemPointer(sl_uint8 pointer);
		
		sl_uint16 getTimestampIdentifier() const;
		
		void setTimestampIdentifier(sl_uint16 id);
		
		sl_uint16 getTimestampSequenceNumber() const;
		
		void setTimestampSequenceNumber(sl_uint16 sn);

		sl_uint16 getAddressMaskIdentifier() const;

		void setAddressMaskIdentifier(sl_uint16 id);

		sl_uint16 getAddressMaskSequenceNumber() const;

		void setAddressMaskSequenceNumber(sl_uint16 sn);

		sl_uint16 getNextHopMTU() const;

		void setNextHopMTU(sl_uint16 mtu);

		const sl_uint8* getContent() const;
		
		sl_uint8* getContent();
		
	private:
		sl_uint8 _type;
		sl_uint8 _code;
		sl_uint8 _checksum[2];
		sl_uint8 _rest[4];
		
	};
	
	class SLIB_EXPORT IcmpEchoAddress
	{
	public:
		IPv4Address ip;
		sl_uint16 identifier;
		sl_uint16 sequenceNumber;
		
	public:
		sl_compare_result compare(const IcmpEchoAddress& other) const noexcept;

		sl_bool equals(const IcmpEchoAddress& other) const noexcept;
		
		sl_size getHashCode() const noexcept;
		
	public:
		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS
		
	};
	
}

#endif
