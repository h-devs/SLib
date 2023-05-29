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

#ifndef CHECKHEADER_SLIB_NETWORK_DNS
#define CHECKHEADER_SLIB_NETWORK_DNS

#include "socket_address.h"
#include "async.h"

#include "../core/string.h"
#include "../core/hash_map.h"

/********************************************************************
	DNS Specification from RFC 1035, RFC 1034, RFC 2535

- Messages sent using UDP/TCP user server port 53 (decimal)

- Message Format
    +---------------------+
    |        Header       |
    +---------------------+
    |       Question      | the question for the name server
    +---------------------+
    |        Answer       | RRs answering the question
    +---------------------+
    |      Authority      | RRs pointing toward an authority
    +---------------------+
    |      Additional     | RRs holding additional information
    +---------------------+

- Header Format

                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA| Z|AD|CD|   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

- Quetion Record
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                     QNAME                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QTYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QCLASS                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

- RR (Resource Record) Format
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                                               /
    /                      NAME                     /
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TTL                      |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   RDLENGTH                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
    /                     RDATA                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

- Name Format
	Label.Label.Label. ... .Label

	Domain names in messages are expressed in terms of a sequence of labels.
	Each label is represented as a one octet length field followed by that
	number of octets.  Since every domain name ends with the null label of
	the root, a domain name is terminated by a length byte of zero.  The
	high order two bits of every length octet must be zero, and the
	remaining six bits of the length field limit the label to 63 octets or
	less.

- Message compression
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    | 1  1|                OFFSET                   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	In this scheme, an entire domain name or a list of labels at
	the end of a domain name is replaced with a pointer to a prior occurance
	of the same name.

********************************************************************/

#define SLIB_NETWORK_DNS_PORT 53
#define SLIB_NETWORK_DNS_LABEL_MAX_LENGTH 63
#define SLIB_NETWORK_DNS_NAME_MAX_LENGTH 255
#define SLIB_NETWORK_DNS_RECORD_HEADER_MAX_LENGTH 263

namespace slib
{

	enum class DnsOpcode
	{
		Query = 0,
		InverseQuery = 1,
		ServerStatusRequest = 2
	};

	enum class DnsResponseCode
	{
		NoError = 0,
		FormatError = 1, // The name server was unable to interpret the query
		ServerFailure = 2, // The name server was unable to process this query due to a problem with the name server
		NameError = 3, // Meaningful only for responses from an authoritative name server, this code signifies that the domain name referenced in the query does not exist
		NotImplemented = 4, // The name server does not support the requested kind of query
		Refused = 5 // The name server refuses to perform the specified operation for policy reasons
	};

	enum class DnsRecordType
	{
		None = 0,
		A = 1, // a host address (IPv4)
		NS = 2, // an authoritative name server
		MD = 3, // a mail destination (Obsolete - use MX)
		MF = 4, // a mail forwarder (Obsolete - use MX)
		CNAME = 5, // the canonical name for an alias
		SOA = 6, // marks the start of a zone of authority
		MB = 7, // a mailbox domain name (EXPERIMENTAL)
		MG = 8, // a mail group member (EXPERIMENTAL)
		MR = 9, // a mail rename domain name (EXPERIMENTAL)
		Null = 10, // a null RR (EXPERIMENTAL)
		WKS = 11, // a well known service description
		PTR = 12, // a domain name pointer
		HINFO = 13, // host information
		MINFO = 14, // mailbox or mail list information
		MX = 15, // mail exchange
		TXT = 16, // text strings
		AAAA = 28, // a host address (IPv6)
		Question_AXFR = 252, // A request for a transfer of an entire zone
		Question_MAILB = 253, // A request for mailbox-related records (MB, MG or MR)
		Question_MAILA = 254, // A request for mail agent RRs (Obsolete - see MX)
		Question_ALL = 255 // A request for all records
	};

#ifdef IN
#undef IN
#endif

	enum class DnsClass
	{
		IN = 1, // the Internet
		CS = 2, // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
		CH = 3, // the CHAOS class
		HS = 4, // Hesiod [Dyer 87]
		Question_ANY = 255 // any class
	};

	class SLIB_EXPORT DnsHeader
	{
	public:
		sl_uint16 getId() const;

		void setId(sl_uint16 id);

		sl_bool isQuestion() const;

		void setQuestion(sl_bool flag);

		// 4 bits
		DnsOpcode getOpcode() const;

		// 4 bits
		void setOpcode(DnsOpcode opcode);

		// Authoritative Answer (only valid in Response)
		sl_bool isAA() const;

		// Authoritative Answer (only valid in Response)
		void setAA(sl_bool flag);

		// TrunCation: this message was truncated due to length greater than that permitted on the transmission channel
		sl_bool isTC() const;

		// TrunCation: this message was truncated due to length greater than that permitted on the transmission channel
		void setTC(sl_bool flag);

		// Recursion Desired: it directs the name server to pursue the query recursively
		sl_bool isRD() const;

		// Recursion Desired: it directs the name server to pursue the query recursively
		void setRD(sl_bool flag);

		// Recursion Available: denotes whether recursive query support is available in the name server (valid in response)
		sl_bool isRA() const;

		// Recursion Available: denotes whether recursive query support is available in the name server (valid in response)
		void setRA(sl_bool flag);

		// Authentic Data: all the data included in the answer and authority portion of the response has been authenticated by the server (valid in response)
		sl_bool isAD() const;

		// Authentic Data: all the data included in the answer and authority portion of the response has been authenticated by the server (valid in response)
		void setAD(sl_bool flag);

		// Checking Disabled: Pending (non-authenticated) data is acceptable to the resolver sending the query
		sl_bool isCD() const;

		// Checking Disabled: Pending (non-authenticated) data is acceptable to the resolver sending the query
		void setCD(sl_bool flag);

		// 4 bits
		DnsResponseCode getResponseCode() const;

		// 4 bits
		void setResponseCode(DnsResponseCode code);

		// QDCOUNT
		sl_uint16 getQuestionCount() const;

		// QDCOUNT
		void setQuestionCount(sl_uint16 count);

		// ANCOUNT
		sl_uint16 getAnswerCount() const;

		// ANCOUNT
		void setAnswerCount(sl_uint16 count);

		// NSCOUNT
		sl_uint16 getAuthorityCount() const;

		// NSCOUNT
		void setAuthorityCount(sl_uint16 count);

		// ARCOUNT
		sl_uint16 getAdditionalCount() const;

		// ARCOUNT
		void setAdditionalCount(sl_uint16 count);

	private:
		sl_uint8 _id[2];
		sl_uint8 _flags[2];
		sl_uint8 _totalQuestions[2]; // QDCOUNT
		sl_uint8 _totalAnswers[2]; // ANCOUNT
		sl_uint8 _totalAuthorities[2]; // NSCOUNT
		sl_uint8 _totalAdditionals[2];  // ARCOUNT

	};

	class SLIB_EXPORT DnsRecord
	{
	public:
		DnsRecord();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DnsRecord)

	public:
		const String& getName() const;

		void setName(const String& name);

		DnsRecordType getType() const;

		void setType(DnsRecordType type);

		DnsClass getClass() const;

		void setClass(DnsClass cls);

	protected:
		static sl_uint32 _parseName(String& nameOut, const void* buf, sl_uint32 offset, sl_uint32 sizeName);

		static sl_uint32 _buildName(const String& nameIn, void* buf, sl_uint32 offset, sl_uint32 sizeName);

		sl_uint32 _parseHeader(const void* buf, sl_uint32 offset, sl_uint32 size);

		sl_uint32 _buildHeader(void* buf, sl_uint32 offset, sl_uint32 size);

	private:
		String _name;
		DnsRecordType _type;
		DnsClass _class;

	};

	class SLIB_EXPORT DnsQuestionRecord : public DnsRecord
	{
	public:
		DnsQuestionRecord();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DnsQuestionRecord)

	public:
		sl_uint32 parseRecord(const void* buf, sl_uint32 offset, sl_uint32 size);

		sl_uint32 buildRecord(void* buf, sl_uint32 offset, sl_uint32 size);

	};

	class SLIB_EXPORT DnsResponseRecord : public DnsRecord
	{
	public:
		DnsResponseRecord();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DnsResponseRecord)

	public:
		sl_uint32 getTTL() const;

		void setTTL(sl_uint32 TTL);

		sl_uint16 getDataLength() const;

		sl_uint16 getDataOffset() const;

		sl_uint32 parseRecord(const void* buf, sl_uint32 offset, sl_uint32 size);

		sl_uint32 buildRecord(void* buf, sl_uint32 offset, sl_uint32 size, const void* data, sl_uint16 sizeData);

		// A 32 bit Internet address
		IPv4Address parseData_A() const;

		//A 32 bit Internet address
		sl_uint32 buildRecord_A(void* buf, sl_uint32 offset, sl_uint32 size, const IPv4Address& addr);

		// A <domain-name> which specifies the canonical or primary	name for the owner. The owner name is an alias
		String parseData_CNAME() const;

		// A <domain-name> which specifies the canonical or primary	name for the owner. The owner name is an alias
		sl_uint32 buildRecord_CNAME(void* buf, sl_uint32 offset, sl_uint32 size, const String& cname);

		// A <domain-name> which specifies a host which should be authoritative for the specified class and domain
		String parseData_NS() const;

		// A <domain-name> which specifies a host which should be authoritative for the specified class and domain
		sl_uint32 buildRecord_NS(void* buf, sl_uint32 offset, sl_uint32 size, const String& nameServer);

		// A 128 bit Internet address
		IPv6Address parseData_AAAA() const;

		//A 128 bit Internet address
		sl_uint32 buildRecord_AAAA(void* buf, sl_uint32 offset, sl_uint32 size, const IPv6Address& addr);

		//  A <domain-name> which specifies a host which should be authoritative for the specified class and domain.
		String parseData_PTR() const;

		// A <domain-name> which specifies a host which should be authoritative for the specified class and domain.
		sl_uint32 buildRecord_PTR(void* buf, sl_uint32 offset, sl_uint32 size, const String& dname);

		String toString() const;

	private:
		sl_uint32 m_TTL;
		sl_uint16 _dataLength;
		sl_uint16 _dataOffset;
		const sl_uint8* _message;
		sl_uint32 _messageLength;

	};

	class SLIB_EXPORT DnsPacket
	{
	public:
		DnsPacket();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DnsPacket)

	public:
		sl_bool flagQuestion;

		sl_uint16 id;

		struct Question
		{
			String name;
			DnsRecordType type;
		};
		List<Question> questions;

		struct Address
		{
			String name;
			IPAddress address;
		};
		List<Address> addresses;

		struct Alias
		{
			String name;
			String alias;
		};
		List<Alias> aliases;

		struct NameServer
		{
			String name;
			String server;
		};
		List<NameServer> nameServers;

		struct NamePointer
		{
			String name;
			String pointer;
		};
		List<NamePointer> pointers;

	public:
		sl_bool parsePacket(const void* packet, sl_uint32 len);

		static Memory buildQuestionPacket(sl_uint16 id, const String& host);

		static Memory buildHostAddressAnswerPacket(sl_uint16 id, const String& hostName, const IPv4Address& hostAddress);

	};


	class DnsClient;

	class SLIB_EXPORT DnsClientParam
	{
	public:
		Function<void(DnsClient*, const SocketAddress&, const DnsPacket&)> onAnswer;

		Ref<AsyncIoLoop> ioLoop;

	public:
		DnsClientParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DnsClientParam)

	};

	class SLIB_EXPORT DnsClient : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		DnsClient();

		~DnsClient();

	public:
		static Ref<DnsClient> create(const DnsClientParam& param);

	public:
		void sendQuestion(const SocketAddress& serverAddress, const String& hostName);

		void sendQuestion(const IPv4Address& serverIp, const String& hostName);

	protected:
		void _onReceiveFrom(AsyncUdpSocket* socket, const SocketAddress& address, void* data, sl_uint32 sizeReceive);

		void _onAnswer(const SocketAddress& serverAddress, const DnsPacket& packet);

	protected:
		Ref<AsyncUdpSocket> m_udp;
		sl_uint16 m_idLast;

		Function<void(DnsClient*, const SocketAddress&, const DnsPacket&)> m_onAnswer;

	};

	class DnsServer;

	class SLIB_EXPORT ResolveDnsHostParam
	{
	public:
		// in
		SocketAddress clientAddress;

		// in
		String hostName;

		// out
		IPv4Address hostAddress;

		// out
		sl_bool flagIgnoreRequest;

		// in, out
		SocketAddress forwardAddress;

	public:
		ResolveDnsHostParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ResolveDnsHostParam)

	};

	class SLIB_EXPORT DnsServerParam
	{
	public:
		sl_uint16 port;

		sl_bool flagProxy;

		SocketAddress defaultForwardAddress;

		sl_bool flagAutoStart;

		Ref<AsyncIoLoop> ioLoop;

		Function<void(DnsServer*, ResolveDnsHostParam&)> onResolve;
		Function<void(DnsServer*, const String& hostName, const IPAddress& hostAddress)> onCache;

	public:
		DnsServerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DnsServerParam)

	public:
		void parse(const Json& config);

	};

	class SLIB_EXPORT DnsServer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		DnsServer();

		~DnsServer();

	public:
		static Ref<DnsServer> create(const DnsServerParam& param);

	public:
		void release();

		void start();

		sl_bool isRunning();

	protected:
		void _processReceivedDnsQuestion(const SocketAddress& clientAddress, sl_uint16 id, const String& hostName);

		void _processReceivedDnsAnswer(const DnsPacket& packet);

		void _processReceivedProxyQuestion(const SocketAddress& clientAddress, void* data, sl_uint32 size);

		void _processReceivedProxyAnswer(void* data, sl_uint32 size);

		void _sendPacket(const SocketAddress& targetAddress, const MemoryView& packet);

	protected:
		void _onReceiveFrom(AsyncUdpSocket* socket, const SocketAddress& address, void* data, sl_uint32 sizeReceive);

		void _onResolve(ResolveDnsHostParam& param);

		void _onCache(const String& hostName, const IPAddress& hostAddress);

	private:
		sl_bool m_flagInit;
		sl_bool m_flagRunning;

		Ref<AsyncUdpSocket> m_socket;
		sl_bool m_flagProxy;

		SocketAddress m_defaultForwardAddress;

		sl_uint16 m_lastForwardId;

		struct ForwardElement
		{
			SocketAddress clientAddress;
			sl_uint16 requestedId;
			String requestedHostName;
		};
		CHashMap<sl_uint16, ForwardElement> m_mapForward;

		Function<void(DnsServer*, ResolveDnsHostParam&)> m_onResolve;
		Function<void(DnsServer*, const String& hostName, const IPAddress& hostAddress)> m_onCache;

	};

}

#endif
