/*
*   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#include "pseudo_tcp.h"

#include <slib/math/math.h>
#include <slib/io/memory_reader.h>
#include <slib/io/memory_writer.h>
#include <slib/core/mio.h>
#include <slib/core/system.h>
#include <slib/core/hash_map.h>
#include <slib/core/assert.h>
#include <slib/core/scoped_buffer.h>

/*
	This code is ported from WebRTC source

	https://webrtc.googlesource.com/src/+/refs/heads/master/p2p/base/pseudo_tcp.h
	https://webrtc.googlesource.com/src/+/refs/heads/master/p2p/base/pseudo_tcp.cc
*/

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_NORMAL 1
#define LOG_LEVEL_VERBOSE 2

#ifdef SLIB_DEBUG
#define LOG_LEVEL LOG_LEVEL_NONE
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define PSEUDO_KEEPALIVE 0

#define TAG "PseudoTcp"

#if LOG_LEVEL >= LOG_LEVEL_NORMAL
#include "slib/core/log.h"
#define LOG_NORMAL(...) SLIB_LOG(TAG, __VA_ARGS__)
#else
#define LOG_NORMAL(...)
#endif
#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
#define LOG_VERBOSE(...) SLIB_LOG(TAG, __VA_ARGS__)
#else
#define LOG_VERBOSE(...)
#endif

#define NOT_REACHED(...)

//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |                      Conversation Number                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |                        Sequence Number                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |                     Acknowledgment Number                     |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |               |   |U|A|P|R|S|F|                               |
// 12 |    Control    |   |R|C|S|S|Y|I|            Window             |
//    |               |   |G|K|H|T|N|N|                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |                       Timestamp sending                       |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 20 |                      Timestamp receiving                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 24 |                             data                              |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

namespace slib
{

	namespace {

		// Standard MTUs
		static const sl_uint16 PACKET_MAXIMUMS[] = {
			65535, // Theoretical maximum, Hyperchannel
			32000, // Nothing
			17914, // 16Mb IBM Token Ring
			8166, // IEEE 802.4
			// 4464, // IEEE 802.5 (4Mb max)
			4352,  // FDDI
			// 2048, // Wideband Network
			2002,  // IEEE 802.5 (4Mb recommended)
			// 1536, // Expermental Ethernet Networks
			// 1500, // Ethernet, Point-to-Point (default)
			1492, // IEEE 802.3
			1006, // SLIP, ARPANET
			// 576, // X.25 Networks
			// 544, // DEC IP Portal
			// 512, // NETBIOS
			508, // IEEE 802/Source-Rt Bridge, ARCNET
			296, // Point-to-Point (low delay)
			// 68, // Official minimum
			0, // End of list marker
		};

		const sl_uint32 MAX_PACKET = 65535;
		// Note: we removed lowest level because packet overhead was larger!
		const sl_uint32 MIN_PACKET = 296;

		const sl_uint32 IP_HEADER_SIZE = 20;  // (+ up to 40 bytes of options?)
		const sl_uint32 UDP_HEADER_SIZE = 8;
		// TODO(?): Make JINGLE_HEADER_SIZE transparent to this code?
		const sl_uint32 JINGLE_HEADER_SIZE = 64;  // when relay framing is in use

		// Default size for receive and send buffer.
		const sl_uint32 DEFAULT_RCV_BUF_SIZE = 60 * 1024;
		const sl_uint32 DEFAULT_SND_BUF_SIZE = 90 * 1024;

		const sl_uint32 HEADER_SIZE = 24;
		const sl_uint32 PACKET_OVERHEAD = HEADER_SIZE + UDP_HEADER_SIZE + IP_HEADER_SIZE + JINGLE_HEADER_SIZE;

		const sl_uint32 MIN_RTO = 250; // 250 ms (RFC1122, Sec 4.2.3.1 "fractions of a second")
		const sl_uint32 DEF_RTO = 3000; // 3 seconds (RFC1122, Sec 4.2.3.1)
		const sl_uint32 MAX_RTO = 60000; // 60 seconds
		const sl_uint32 DEF_ACK_DELAY = 100; // 100 milliseconds

		const sl_uint8 FLAG_CTL = 0x02;
		const sl_uint8 FLAG_RST = 0x04;

		const sl_uint8 CTL_CONNECT = 0;

		// TCP options
		const sl_uint8 TCP_OPT_EOL = 0; // End of list.
		const sl_uint8 TCP_OPT_NOOP = 1; // No-op.
		const sl_uint8 TCP_OPT_MSS = 2; // Maximum segment size.
		const sl_uint8 TCP_OPT_WND_SCALE = 3; // Window scale factor.

		const long DEFAULT_TIMEOUT = 4000; // If there are no pending clocks, wake up every 4 seconds
		const long CLOSED_TIMEOUT = 60 * 1000; // If the connection is closed, once per minute

#if PSEUDO_KEEPALIVE
		// !?! Rethink these times
		const sl_uint32 IDLE_PING = 20 * 1000; // 20 seconds (note: WinXP SP2 firewall udp timeout is 90 seconds)
		const sl_uint32 IDLE_TIMEOUT = 90 * 1000; // 90 seconds;
#endif

		SLIB_INLINE static void LongToBytes(sl_uint32 val, void* buf)
		{
			MIO::writeUint32BE(buf, val);
		}

		SLIB_INLINE static void ShortToBytes(sl_uint16 val, void* buf)
		{
			MIO::writeUint16BE(buf, val);
		}

		SLIB_INLINE static sl_uint32 BytesToLong(const void* buf)
		{
			return MIO::readUint32BE(buf);
		}

		SLIB_INLINE static sl_uint16 BytesToShort(const void* buf)
		{
			return MIO::readUint16BE(buf);
		}

		SLIB_INLINE static sl_int32 TimeDiff32(sl_uint32 later, sl_uint32 earlier)
		{
			return later - earlier;
		}
	}

	struct PseudoTcp::Segment
	{
	public:
		sl_uint32 conv, seq, ack;
		sl_uint8 flags;
		sl_uint16 wnd;
		const char* data;
		sl_uint32 len;
		sl_uint32 tsval, tsecr;

	};

	class PseudoTcp::SSegment
	{
	public:
		SSegment(sl_uint32 s, sl_uint32 l, sl_bool c) : seq(s), len(l), xmit(0), bCtrl(c) {}

	public:
		sl_uint32 seq, len;
		sl_uint8 xmit;
		sl_bool bCtrl;

	};

	class PseudoTcp::RSegment
	{
	public:
		sl_uint32 seq;
		sl_uint32 len;

	};

	class PseudoTcp::LockedFifoBuffer
	{
	public:
		LockedFifoBuffer(sl_size size): m_buf(new char[size]), m_lenBuf(size), m_lenData(0), m_posRead(0)
		{
		}

	public:
		sl_size getBuffered() const
		{
			MutexLocker locker(&m_lock);
			return m_lenData;
		}

		sl_bool setCapacity(sl_size size)
		{
			MutexLocker locker(&m_lock);
			if (m_lenData > size) {
				return sl_false;
			}

			if (size != m_lenBuf) {
				char* buffer = new char[size];
				const sl_size copy = m_lenData;
				const sl_size tail_copy = Math::min(copy, m_lenBuf - m_posRead);
				Base::copyMemory(buffer, &m_buf[m_posRead], tail_copy);
				Base::copyMemory(buffer + tail_copy, &m_buf[0], copy - tail_copy);
				m_buf = buffer;
				m_posRead = 0;
				m_lenBuf = size;
			}

			return sl_true;
		}

		sl_bool readOffset(void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_read)
		{
			MutexLocker locker(&m_lock);
			return readOffsetLocked(buffer, bytes, offset, bytes_read);
		}

		sl_bool writeOffset(const void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_written)
		{
			MutexLocker locker(&m_lock);
			return writeOffsetLocked(buffer, bytes, offset, bytes_written);
		}

		sl_bool read(void* buffer, sl_size bytes, sl_size* bytes_read)
		{
			MutexLocker locker(&m_lock);
			sl_size copy = 0;
			if (!readOffsetLocked(buffer, bytes, 0, &copy)) {
				return sl_false;
			}

			// If read was successful then adjust the read position and number of bytes buffered.
			m_posRead = (m_posRead + copy) % m_lenBuf;
			m_lenData -= copy;
			if (bytes_read) {
				*bytes_read = copy;
			}

			return sl_true;
		}

		sl_bool write(const void* buffer, sl_size bytes, sl_size* bytes_written)
		{
			MutexLocker locker(&m_lock);

			sl_size copy = 0;
			if (!writeOffsetLocked(buffer, bytes, 0, &copy)) {
				return sl_false;
			}

			// If write was successful then adjust the number of readable bytes.
			m_lenData += copy;
			if (bytes_written) {
				*bytes_written = copy;
			}

			return sl_true;
		}

		void consumeReadData(sl_size size)
		{
			MutexLocker locker(&m_lock);
			SLIB_ASSERT(size <= m_lenData);
			m_posRead = (m_posRead + size) % m_lenBuf;
			m_lenData -= size;
		}

		void consumeWriteBuffer(sl_size size)
		{
			MutexLocker locker(&m_lock);
			SLIB_ASSERT(size <= m_lenBuf - m_lenData);
			m_lenData += size;
		}

		sl_bool getWriteRemaining(sl_size* size) const
		{
			MutexLocker locker(&m_lock);
			*size = m_lenBuf - m_lenData;
			return sl_true;
		}

	private:
		sl_bool readOffsetLocked(void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_read)
		{
			if (offset >= m_lenData) {
				return sl_false;
			}

			const sl_size available = m_lenData - offset;
			const sl_size read_position = (m_posRead + offset) % m_lenBuf;
			const sl_size copy = Math::min(bytes, available);
			const sl_size tail_copy = Math::min(copy, m_lenBuf - read_position);
			char* const p = static_cast<char*>(buffer);
			Base::copyMemory(p, &m_buf[read_position], tail_copy);
			Base::copyMemory(p + tail_copy, &m_buf[0], copy - tail_copy);

			if (bytes_read) {
				*bytes_read = copy;
			}

			return sl_true;
		}

		sl_bool writeOffsetLocked(const void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_written)
		{
			if (m_lenData + offset >= m_lenBuf) {
				return sl_false;
			}

			const sl_size available = m_lenBuf - m_lenData - offset;
			const sl_size write_position = (m_posRead + m_lenData + offset) % m_lenBuf;
			const sl_size copy = Math::min(bytes, available);
			const sl_size tail_copy = Math::min(copy, m_lenBuf - write_position);
			const char* const p = static_cast<const char*>(buffer);
			Base::copyMemory(&m_buf[write_position], p, tail_copy);
			Base::copyMemory(&m_buf[0], p + tail_copy, copy - tail_copy);

			if (bytes_written) {
				*bytes_written = copy;
			}

			return sl_true;
		}

	private:
		UniquePtr<char[]> m_buf; // the allocated buffer
		sl_size m_lenBuf; // size of the allocated buffer
		sl_size m_lenData; // amount of readable data in the buffer
		sl_size m_posRead; // offset to the readable data
		Mutex m_lock;
	};


	IPseudoTcpNotify::IPseudoTcpNotify()
	{
	}

	IPseudoTcpNotify::~IPseudoTcpNotify()
	{
	}


	PseudoTcp::PseudoTcp(IPseudoTcpNotify* notify, sl_uint32 conv):
		m_notify(notify),
		m_shutdown(PseudoTcpShutdownType::None),
		m_error(PseudoTcpError::None),
		m_rlist(new CLinkedList<RSegment>),
		m_rbuf_len(DEFAULT_RCV_BUF_SIZE),
		m_rbuf(new LockedFifoBuffer(m_rbuf_len)),
		m_slist(new CLinkedList<SSegment>),
		m_sbuf_len(DEFAULT_SND_BUF_SIZE),
		m_sbuf(new LockedFifoBuffer(m_sbuf_len))
	{
		SLIB_ASSERT(m_rbuf_len + MIN_PACKET < m_sbuf_len);

		sl_uint32 now = PseudoTcp::now();

		m_state = PseudoTcpState::Listen;
		m_conv = conv;
		m_rcv_wnd = m_rbuf_len;
		m_rwnd_scale = m_swnd_scale = 0;
		m_snd_nxt = 0;
		m_snd_wnd = 1;
		m_snd_una = m_rcv_nxt = 0;
		m_bReadEnable = sl_true;
		m_bWriteEnable = sl_false;
		m_t_ack = 0;

		m_msslevel = 0;
		m_largest = 0;

		SLIB_ASSERT(MIN_PACKET > PACKET_OVERHEAD);
		m_mss = MIN_PACKET - PACKET_OVERHEAD;
		m_mtu_advise = MAX_PACKET;

		m_rto_base = 0;

		m_cwnd = 2 * m_mss;
		m_ssthresh = m_rbuf_len;
		m_lastrecv = m_lastsend = m_lasttraffic = now;
		m_bOutgoing = sl_false;

		m_dup_acks = 0;
		m_recover = 0;

		m_ts_recent = m_ts_lastack = 0;

		m_rx_rto = DEF_RTO;
		m_rx_srtt = m_rx_rttvar = 0;

		m_use_nagling = sl_true;
		m_ack_delay = DEF_ACK_DELAY;
		m_support_wnd_scale = sl_true;
	}

	PseudoTcp::~PseudoTcp()
	{
	}

	sl_uint32 PseudoTcp::now()
	{
		return (sl_uint32)(System::getHighResolutionTickCount());
	}

	sl_bool PseudoTcp::connect()
	{
		if (m_state != PseudoTcpState::Listen) {
			m_error = PseudoTcpError::InvalidState;
			return sl_false;
		}

		m_state = PseudoTcpState::SentSyn;

		LOG_NORMAL("connect: changed state to SentSyn");

		queueConnectMessage();
		attemptSend();

		return sl_true;
	}

	void PseudoTcp::notifyMTU(sl_uint16 mtu)
	{
		m_mtu_advise = mtu;
		if (m_state == PseudoTcpState::Established) {
			adjustMTU();
		}
	}

	void PseudoTcp::notifyClock(sl_uint32 now)
	{
		if (m_state == PseudoTcpState::Closed) {
			return;
		}

		// Check if it's time to retransmit a segment
		if (m_rto_base && (TimeDiff32(m_rto_base + m_rx_rto, now) <= 0)) {
			if (m_slist->isEmpty()) {
				NOT_REACHED()
			} else {

				LOG_NORMAL("notifyClock: timeout retransmit: rto=%s, rto_base=%s, now=%s, dup_acks=%s", m_rx_rto, m_rto_base, now, m_dup_acks);

				// retransmit segments
				if (!transmit(m_slist->getFront(), now)) {
					closedown(PseudoTcpError::ConnectionAborted);
					return;
				}

				sl_uint32 nInFlight = m_snd_nxt - m_snd_una;
				m_ssthresh = Math::max(nInFlight / 2, 2 * m_mss);

				m_cwnd = m_mss;

				// Back off retransmit timer.  Note: the limit is lower when connecting.
				sl_uint32 rto_limit = (m_state < PseudoTcpState::Established) ? DEF_RTO : MAX_RTO;
				m_rx_rto = Math::min(rto_limit, m_rx_rto * 2);
				m_rto_base = now;
			}
		}

		// Check if it's time to probe closed windows
		if ((m_snd_wnd == 0) && (TimeDiff32(m_lastsend + m_rx_rto, now) <= 0)) {
			if (TimeDiff32(now, m_lastrecv) >= 15000) {
				closedown(PseudoTcpError::ConnectionAborted);
				return;
			}

			// probe the window
			buildPacket(m_snd_nxt - 1, 0, 0, 0);
			m_lastsend = now;

			// back off retransmit timer
			m_rx_rto = Math::min(MAX_RTO, m_rx_rto * 2);
		}

		// Check if it's time to send delayed acks
		if (m_t_ack && (TimeDiff32(m_t_ack + m_ack_delay, now) <= 0)) {
			buildPacket(m_snd_nxt, 0, 0, 0);
		}

#if PSEUDO_KEEPALIVE
		// Check for idle timeout
		if ((m_state == PseudoTcpState::Established) && (TimeDiff32(m_lastrecv + IDLE_TIMEOUT, now) <= 0)) {
			closedown(PseudoTcpError::ConnectionAborted);
			return;
		}

		// Check for ping timeout (to keep udp mapping open)
		if ((m_state == PseudoTcpState::Established) && (TimeDiff32(m_lasttraffic + (m_bOutgoing ? IDLE_PING * 3 / 2 : IDLE_PING), now) <= 0)) {
			buildPacket(m_snd_nxt, 0, 0, 0);
		}
#endif  // PSEUDO_KEEPALIVE
	}

	sl_bool PseudoTcp::notifyPacket(const void* buffer, sl_size len)
	{
		if (len > MAX_PACKET) {
			LOG_NORMAL("notifyPacket: packet too large");
			return sl_false;
		}
		return parsePacket(reinterpret_cast<const sl_uint8*>(buffer), sl_uint32(len));
	}

	sl_bool PseudoTcp::getNextClock(sl_uint32 now, sl_uint32& timeout)
	{
		return clock_check(now, timeout);
	}

	sl_bool PseudoTcp::isNoDelay()
	{
		return !m_use_nagling;
	}

	void PseudoTcp::setNoDelay(sl_bool flag)
	{
		m_use_nagling = !flag;
	}

	sl_uint32 PseudoTcp::getAckDelay()
	{
		return m_ack_delay;
	}

	void PseudoTcp::setAckDelay(sl_uint32 delay)
	{
		m_ack_delay = delay;
	}

	sl_uint32 PseudoTcp::getReceiveBufferSize()
	{
		return m_rbuf_len;
	}

	void PseudoTcp::setReceiveBufferSize(sl_uint32 size)
	{
		SLIB_ASSERT(m_state == PseudoTcpState::Listen);
		resizeReceiveBuffer(size);
	}

	sl_uint32 PseudoTcp::getSendBufferSize()
	{
		return m_sbuf_len;
	}

	void PseudoTcp::setSendBufferSize(sl_uint32 size)
	{
		SLIB_ASSERT(m_state == PseudoTcpState::Listen);
		resizeSendBuffer(size);
	}

	sl_uint32 PseudoTcp::getCongestionWindow() const
	{
		return m_cwnd;
	}

	sl_uint32 PseudoTcp::getBytesInFlight() const
	{
		return m_snd_nxt - m_snd_una;
	}

	sl_uint32 PseudoTcp::getBytesBufferedNotSent() const
	{
		return static_cast<sl_uint32>(m_snd_una + m_sbuf->getBuffered() - m_snd_nxt);
	}

	sl_uint32 PseudoTcp::getRoundTripTimeEstimate() const
	{
		return m_rx_srtt;
	}

	sl_int32 PseudoTcp::receive(void* buffer, sl_size len)
	{
		if (m_state != PseudoTcpState::Established) {
			m_error = PseudoTcpError::NotConnected;
			return SLIB_IO_ERROR;
		}

		sl_size read = 0;
		if (!m_rbuf->read(buffer, len, &read)) {
			m_bReadEnable = sl_true;
			m_error = PseudoTcpError::WouldBlock;
			return SLIB_IO_WOULD_BLOCK;
		}

		sl_size available_space = 0;
		m_rbuf->getWriteRemaining(&available_space);

		if (sl_uint32(available_space) - m_rcv_wnd >= Math::min<sl_uint32>(m_rbuf_len / 2, m_mss)) {
			// TODO(jbeda): !?! Not sure about this was closed business
			sl_bool bWasClosed = (m_rcv_wnd == 0);
			m_rcv_wnd = static_cast<sl_uint32>(available_space);

			if (bWasClosed) {
				attemptSend(PseudoTcpSendFlags::ImmediateAck);
			}
		}

		return static_cast<sl_int32>(read);
	}

	sl_int32 PseudoTcp::send(const void* _buffer, sl_size len)
	{
		const char* buffer = (const char*)_buffer;

		if (m_state != PseudoTcpState::Established) {
			m_error = PseudoTcpError::NotConnected;
			return SLIB_IO_ERROR;
		}

		sl_size available_space = 0;
		m_sbuf->getWriteRemaining(&available_space);

		if (!available_space) {
			m_bWriteEnable = sl_true;
			m_error = PseudoTcpError::WouldBlock;
			return SLIB_IO_WOULD_BLOCK;
		}

		int written = queue(buffer, sl_uint32(len), sl_false);
		attemptSend();
		return written;
	}

	void PseudoTcp::close(sl_bool force)
	{
		LOG_VERBOSE("close: force=%s", force);
		m_shutdown = force ? PseudoTcpShutdownType::Forceful : PseudoTcpShutdownType::Graceful;
	}

	PseudoTcpError PseudoTcp::getError()
	{
		return m_error;
	}

	PseudoTcpState PseudoTcp::getState()
	{
		return m_state;
	}

	sl_uint32 PseudoTcp::queue(const char* data, sl_uint32 len, sl_bool bCtrl)
	{
		sl_size available_space = 0;
		m_sbuf->getWriteRemaining(&available_space);

		if (len > static_cast<sl_uint32>(available_space)) {
			SLIB_ASSERT(!bCtrl);
			len = static_cast<sl_uint32>(available_space);
		}

		// We can concatenate data if the last segment is the same type (control v. regular data), and has not been transmitted yet
		if (!m_slist->isEmpty() && (m_slist->getBack()->value.bCtrl == bCtrl) && (m_slist->getBack()->value.xmit == 0)) {
			m_slist->getBack()->value.len += len;
		} else {
			SSegment sseg(static_cast<sl_uint32>(m_snd_una + m_sbuf->getBuffered()), len, bCtrl);
			m_slist->pushBack_NoLock(sseg);
		}

		sl_size written = 0;
		m_sbuf->write(data, len, &written);
		return static_cast<sl_uint32>(written);
	}


	/*
		Creates a packet and submits it to the network. This method can either send payload or just an ACK packet.

		seq: the sequence number of this packet.
		flags: the flags for sending this packet.
		offset: the offset to read from `m_sbuf`.
		len: the number of bytes to read from `m_sbuf` as payload. If this
		value: if 0 then this is an ACK packet, otherwise this packet has payload.
	*/
	PseudoTcpWriteResult PseudoTcp::buildPacket(
		sl_uint32 seq,
		sl_uint8 flags,
		sl_uint32 offset,
		sl_uint32 len)
	{
		SLIB_ASSERT(HEADER_SIZE + len <= MAX_PACKET);
		SLIB_SCOPED_BUFFER(sl_uint8, 4096, buffer, len + HEADER_SIZE)
		if (!buffer) {
			return PseudoTcpWriteResult::Fail;
		}

		sl_uint32 now = PseudoTcp::now();

		LongToBytes(m_conv, buffer);
		LongToBytes(seq, buffer + 4);
		LongToBytes(m_rcv_nxt, buffer + 8);
		buffer[12] = 0;
		buffer[13] = flags;
		ShortToBytes(static_cast<sl_uint16>(m_rcv_wnd >> m_rwnd_scale), buffer + 14);

		// Timestamp computations
		LongToBytes(now, buffer + 16);
		LongToBytes(m_ts_recent, buffer + 20);
		m_ts_lastack = m_rcv_nxt;

		if (len) {
			sl_size bytes_read = 0;
			sl_bool result = m_sbuf->readOffset(buffer + HEADER_SIZE, len, offset, &bytes_read);
			SLIB_UNUSED(result);
			SLIB_ASSERT(result);
			SLIB_ASSERT(static_cast<sl_uint32>(bytes_read) == len);
		}

		LOG_VERBOSE("buildPacket: conv=%s, flg=%s, seq=%s:%s, ack=%s, wnd=%s, ts=%s, tsr=%s, len=%s", m_conv, flags, seq, seq + len, m_rcv_nxt, m_rcv_wnd, (now % 10000), (m_ts_recent % 10000), len);

		PseudoTcpWriteResult wres = m_notify->writeTcpPacket(this, reinterpret_cast<char*>(buffer), len + HEADER_SIZE);

		// Note: When len is 0, this is an ACK packet.  We don't read the return value for those, and thus we won't retry.  So go ahead and treat the packet as a success (basically simulate as if it were dropped), which will prevent our timers from being messed up.
		if ((wres != PseudoTcpWriteResult::Success) && (0 != len)) {
			return wres;
		}

		m_t_ack = 0;
		if (len > 0) {
			m_lastsend = now;
		}
		m_lasttraffic = now;
		m_bOutgoing = sl_true;

		return PseudoTcpWriteResult::Success;
	}

	sl_bool PseudoTcp::parsePacket(const sl_uint8* buffer, sl_uint32 size)
	{
		if (size < HEADER_SIZE) {
			return sl_false;
		}

		Segment seg;

		seg.conv = BytesToLong(buffer);
		seg.seq = BytesToLong(buffer + 4);
		seg.ack = BytesToLong(buffer + 8);
		seg.flags = buffer[13];
		seg.wnd = BytesToShort(buffer + 14);

		seg.tsval = BytesToLong(buffer + 16);
		seg.tsecr = BytesToLong(buffer + 20);

		seg.data = reinterpret_cast<const char*>(buffer) + HEADER_SIZE;
		seg.len = size - HEADER_SIZE;

		LOG_VERBOSE("parsePacket: conv=%s, flg=%s, seq=%s:%s, ack=%s, wnd=%s, ts=%s, tsr=%s, len=%s", seg.conv, seg.flags, seg.seq, seg.seq + seg.len, seg.ack, seg.wnd, (seg.tsval % 10000), (seg.tsecr % 10000), seg.len);

		return process(seg);
	}

	sl_bool PseudoTcp::clock_check(sl_uint32 now, sl_uint32& nTimeout)
	{
		if (m_shutdown == PseudoTcpShutdownType::Forceful) {
			return sl_false;
		}

		if ((m_shutdown == PseudoTcpShutdownType::Graceful) && ((m_state != PseudoTcpState::Established) || ((m_sbuf->getBuffered() == 0) && (m_t_ack == 0)))) {
			return sl_false;
		}

		if (m_state == PseudoTcpState::Closed) {
			nTimeout = CLOSED_TIMEOUT;
			return sl_true;
		}

		nTimeout = DEFAULT_TIMEOUT;

		if (m_t_ack) {
			nTimeout = Math::min<sl_int32>(nTimeout, TimeDiff32(m_t_ack + m_ack_delay, now));
		}
		if (m_rto_base) {
			nTimeout = Math::min<sl_int32>(nTimeout, TimeDiff32(m_rto_base + m_rx_rto, now));
		}
		if (m_snd_wnd == 0) {
			nTimeout = Math::min<sl_int32>(nTimeout, TimeDiff32(m_lastsend + m_rx_rto, now));
		}
#if PSEUDO_KEEPALIVE
		if (m_state == PseudoTcpState::Established) {
			nTimeout = Math::min<sl_int32>(nTimeout, TimeDiff32(m_lasttraffic + (m_bOutgoing ? IDLE_PING * 3 / 2 : IDLE_PING), now));
		}
#endif
		return sl_true;
	}

	sl_bool PseudoTcp::process(Segment& seg)
	{
		// If this is the wrong conversation, send a reset!?! (with the correct conversation?)
		if (seg.conv != m_conv) {
			// if ((seg.flags & FLAG_RST) == 0) {
			//  buildPacket(tcb, seg.ack, 0, FLAG_RST, 0, 0);
			// }
			LOG_NORMAL("process: wrong conversation");
			return sl_false;
		}

		sl_uint32 now = PseudoTcp::now();
		m_lasttraffic = m_lastrecv = now;
		m_bOutgoing = sl_false;

		if (m_state == PseudoTcpState::Closed) {
			// !?! send reset?
			LOG_NORMAL("process: closed");
			return sl_false;
		}

		// Check if this is a reset segment
		if (seg.flags & FLAG_RST) {
			closedown(PseudoTcpError::ConnectionReset);
			return sl_false;
		}

		// Check for control data
		sl_bool bConnect = sl_false;
		if (seg.flags & FLAG_CTL) {
			if (seg.len == 0) {
				LOG_NORMAL("process: Missing control code");
				return sl_false;
			} else if (seg.data[0] == CTL_CONNECT) {
				bConnect = sl_true;
				// TCP options are in the remainder of the payload after CTL_CONNECT.
				parseOptions(&seg.data[1], seg.len - 1);
				if (m_state == PseudoTcpState::Listen) {
					m_state = PseudoTcpState::ReceivedSyn;
					LOG_NORMAL("process: changed state to ReceivedSyn");
					// m_notify->associate(addr);
					queueConnectMessage();
				} else if (m_state == PseudoTcpState::SentSyn) {
					m_state = PseudoTcpState::Established;
					LOG_NORMAL("process: changed state to Established");
					adjustMTU();
					if (m_notify) {
						m_notify->onTcpOpen(this);
					}
				}
			} else {
				LOG_NORMAL("process: Unknown control code: %s", seg.data[0]);
				return sl_false;
			}
		}

		// Update timestamp
		if ((seg.seq <= m_ts_lastack) && (m_ts_lastack < seg.seq + seg.len)) {
			m_ts_recent = seg.tsval;
		}

		// Check if this is a valuable ack
		if ((seg.ack > m_snd_una) && (seg.ack <= m_snd_nxt)) {
			// Calculate round-trip time
			if (seg.tsecr) {
				sl_int32 rtt = TimeDiff32(now, seg.tsecr);
				if (rtt >= 0) {
					if (m_rx_srtt == 0) {
						m_rx_srtt = rtt;
						m_rx_rttvar = rtt / 2;
					} else {
						sl_uint32 unsigned_rtt = static_cast<sl_uint32>(rtt);
						sl_uint32 abs_err = unsigned_rtt > m_rx_srtt
							? unsigned_rtt - m_rx_srtt
							: m_rx_srtt - unsigned_rtt;
						m_rx_rttvar = (3 * m_rx_rttvar + abs_err) / 4;
						m_rx_srtt = (7 * m_rx_srtt + rtt) / 8;
					}
					m_rx_rto = Math::clamp(m_rx_srtt + Math::max<sl_uint32>(1, 4 * m_rx_rttvar), MIN_RTO, MAX_RTO);
					LOG_VERBOSE("process: rtt=%s, srtt=%s, rto=%s", rtt, m_rx_srtt, m_rx_rto);
				} else {
					LOG_NORMAL("process: rtt < 0");
				}
			}

			m_snd_wnd = static_cast<sl_uint32>(seg.wnd) << m_swnd_scale;

			sl_uint32 nAcked = seg.ack - m_snd_una;
			m_snd_una = seg.ack;

			m_rto_base = (m_snd_una == m_snd_nxt) ? 0 : now;

			m_sbuf->consumeReadData(nAcked);

			for (sl_uint32 nFree = nAcked; nFree > 0;) {
				SLIB_ASSERT(!m_slist->isEmpty());
				if (nFree < m_slist->getFront()->value.len) {
					m_slist->getFront()->value.len -= nFree;
					nFree = 0;
				} else {
					if (m_slist->getFront()->value.len > m_largest) {
						m_largest = m_slist->getFront()->value.len;
					}
					nFree -= m_slist->getFront()->value.len;
					m_slist->popFront_NoLock();
				}
			}

			if (m_dup_acks >= 3) {
				if (m_snd_una >= m_recover) {  // NewReno
					sl_uint32 nInFlight = m_snd_nxt - m_snd_una;
					m_cwnd = Math::min(m_ssthresh, nInFlight + m_mss);  // (Fast Retransmit)
					LOG_NORMAL("process: exit recovery");
					m_dup_acks = 0;
				} else {
					LOG_NORMAL("process: recovery retransmit");
					if (!transmit(m_slist->getFront(), now)) {
						closedown(PseudoTcpError::ConnectionAborted);
						return sl_false;
					}
					m_cwnd += m_mss - Math::min(nAcked, m_cwnd);
				}
			} else {
				m_dup_acks = 0;
				// Slow start, congestion avoidance
				if (m_cwnd < m_ssthresh) {
					m_cwnd += m_mss;
				} else {
					m_cwnd += Math::max<sl_uint32>(1, m_mss * m_mss / m_cwnd);
				}
			}
		} else if (seg.ack == m_snd_una) {
			// !?! Note, tcp says don't do this... but otherwise how does a closed window become open?
			m_snd_wnd = static_cast<sl_uint32>(seg.wnd) << m_swnd_scale;

			// Check duplicate acks
			if (seg.len > 0) {
				// it's a dup ack, but with a data payload, so don't modify m_dup_acks
			} else if (m_snd_una != m_snd_nxt) {
				m_dup_acks += 1;
				if (m_dup_acks == 3) {  // (Fast Retransmit)
					LOG_NORMAL("process: enter recovery");
					LOG_NORMAL("process: recovery retransmit");
					if (!transmit(m_slist->getFront(), now)) {
						closedown(PseudoTcpError::ConnectionAborted);
						return sl_false;
					}
					m_recover = m_snd_nxt;
					sl_uint32 nInFlight = m_snd_nxt - m_snd_una;
					m_ssthresh = Math::max(nInFlight / 2, 2 * m_mss);
					m_cwnd = m_ssthresh + 3 * m_mss;
				} else if (m_dup_acks > 3) {
					m_cwnd += m_mss;
				}
			} else {
				m_dup_acks = 0;
			}
		}

		// !?! A bit hacky
		if ((m_state == PseudoTcpState::ReceivedSyn) && !bConnect) {
			m_state = PseudoTcpState::Established;
			LOG_NORMAL("process: changed state to Established");
			adjustMTU();
			if (m_notify) {
				m_notify->onTcpOpen(this);
			}
		}

		// If we make room in the send queue, notify the user. The goal it to make sure we always have at least enough data to fill the window. We'd like to notify the app when we are halfway to that point.
		const sl_uint32 kIdealRefillSize = (m_sbuf_len + m_rbuf_len) / 2;
		if (m_bWriteEnable && static_cast<sl_uint32>(m_sbuf->getBuffered()) < kIdealRefillSize) {
			m_bWriteEnable = sl_false;
			if (m_notify) {
				m_notify->onTcpWriteable(this);
			}
		}

		// Conditions were acks must be sent:
		// 1) Segment is too old (they missed an ACK) (immediately)
		// 2) Segment is too new (we missed a segment) (immediately)
		// 3) Segment has data (so we need to ACK!) (delayed) so the only time we don't need to ACK, is an empty segment that points to rcv_nxt!

		PseudoTcpSendFlags sflags = PseudoTcpSendFlags::None;
		if (seg.seq != m_rcv_nxt) {
			sflags = PseudoTcpSendFlags::ImmediateAck; // (Fast Recovery)
		} else if (seg.len != 0) {
			if (m_ack_delay == 0) {
				sflags = PseudoTcpSendFlags::ImmediateAck;
			} else {
				sflags = PseudoTcpSendFlags::DelayedAck;
			}
		}
#if LOG_LEVEL >= LOG_LEVEL_NORMAL
		if (sflags == PseudoTcpSendFlags::ImmediateAck) {
			if (seg.seq > m_rcv_nxt) {
				LOG_NORMAL("process: too new");
			} else if (seg.seq + seg.len <= m_rcv_nxt) {
				LOG_NORMAL("process: too old");
			}
		}
#endif

		// Adjust the incoming segment to fit our receive buffer
		if (seg.seq < m_rcv_nxt) {
			sl_uint32 nAdjust = m_rcv_nxt - seg.seq;
			if (nAdjust < seg.len) {
				seg.seq += nAdjust;
				seg.data += nAdjust;
				seg.len -= nAdjust;
			} else {
				seg.len = 0;
			}
		}

		sl_size available_space = 0;
		m_rbuf->getWriteRemaining(&available_space);

		if ((seg.seq + seg.len - m_rcv_nxt) > static_cast<sl_uint32>(available_space)) {
			sl_uint32 nAdjust = seg.seq + seg.len - m_rcv_nxt - static_cast<sl_uint32>(available_space);
			if (nAdjust < seg.len) {
				seg.len -= nAdjust;
			} else {
				seg.len = 0;
			}
		}

		sl_bool bIgnoreData = (seg.flags & FLAG_CTL) || (m_shutdown != PseudoTcpShutdownType::None);
		sl_bool bNewData = sl_false;

		if (seg.len > 0) {
			sl_bool bRecover = sl_false;
			if (bIgnoreData) {
				if (seg.seq == m_rcv_nxt) {
					m_rcv_nxt += seg.len;
					// If we received a data segment out of order relative to a control segment, then we wrote it into the receive buffer at an offset (see "writeOffset") below. So we need to advance the position in the buffer to avoid corrupting data. See bugs.webrtc.org/9208
					// We advance the position in the buffer by N bytes by acting like we wrote N bytes and then immediately read them. We can only do this if there's not already data ready to read, but this should always be true in the problematic scenario, since control frames are always sent first in the stream.
					if (m_rbuf->getBuffered() == 0) {
						m_rbuf->consumeWriteBuffer(seg.len);
						m_rbuf->consumeReadData(seg.len);
						// After shifting the position in the buffer, we may have out-of-order packets ready to be recovered.
						bRecover = sl_true;
					}
				}
			} else {
				sl_uint32 nOffset = seg.seq - m_rcv_nxt;

				if (!m_rbuf->writeOffset(seg.data, seg.len, nOffset, sl_null)) {
					// Ignore incoming packets outside of the receive window.
					return sl_false;
				}

				if (seg.seq == m_rcv_nxt) {
					m_rbuf->consumeWriteBuffer(seg.len);
					m_rcv_nxt += seg.len;
					m_rcv_wnd -= seg.len;
					bNewData = sl_true;
					// May be able to recover packets previously received out-of-order now.
					bRecover = sl_true;
				} else {
					LOG_NORMAL("process: saving %s bytes (%s->%s)", seg.len, seg.seq, seg.seq + seg.len);
					RSegment rseg;
					rseg.seq = seg.seq;
					rseg.len = seg.len;
					auto it = m_rlist->getFront();
					while (it && (it->value.seq < rseg.seq)) {
						it = it->next;
					}
					m_rlist->insertBefore(it, rseg);
				}
			}
			if (bRecover) {
				auto it = m_rlist->getFront();
				while (it && (it->value.seq <= m_rcv_nxt)) {
					if (it->value.seq + it->value.len > m_rcv_nxt) {
						sflags = PseudoTcpSendFlags::ImmediateAck;  // (Fast Recovery)
						sl_uint32 nAdjust = (it->value.seq + it->value.len) - m_rcv_nxt;
						LOG_NORMAL("process: Recovered %s bytes (%s->%s", nAdjust, m_rcv_nxt, m_rcv_nxt + nAdjust);
						m_rbuf->consumeWriteBuffer(nAdjust);
						m_rcv_nxt += nAdjust;
						m_rcv_wnd -= nAdjust;
						bNewData = sl_true;
					}
					it = m_rlist->removeAt(it);
				}
			}
		}

		attemptSend(sflags);

		// If we have new data, notify the user
		if (bNewData && m_bReadEnable) {
			m_bReadEnable = sl_false;
			if (m_notify) {
				m_notify->onTcpReadable(this);
			}
		}

		return sl_true;
	}

	sl_bool PseudoTcp::transmit(Link<SSegment>* _seg, sl_uint32 now)
	{
		SSegment& seg = _seg->value;

		if (seg.xmit >= ((m_state == PseudoTcpState::Established) ? 15 : 30)) {
			LOG_VERBOSE("transmit: too many retransmits");
			return sl_false;
		}

		sl_uint32 nTransmit = Math::min(seg.len, m_mss);

		while (sl_true) {
			sl_uint32 seq = seg.seq;
			sl_uint8 flags = (seg.bCtrl ? FLAG_CTL : 0);

			if (seq < m_snd_una) {
				break;
			}
			PseudoTcpWriteResult wres = buildPacket(seq, flags, seq - m_snd_una, nTransmit);

			if (wres == PseudoTcpWriteResult::Success) {
				break;
			}

			if (wres == PseudoTcpWriteResult::Fail) {
				LOG_VERBOSE("transmit: packet failed");
				return sl_false;
			}

			SLIB_ASSERT(wres == PseudoTcpWriteResult::TooLarge);

			while (sl_true) {
				if (PACKET_MAXIMUMS[m_msslevel + 1] == 0) {
					LOG_VERBOSE("transmit: MTU too small");
					return sl_false;
				}
				// !?! We need to break up all outstanding and pending packets and then retransmit!?!
				m_mss = PACKET_MAXIMUMS[++m_msslevel] - PACKET_OVERHEAD;
				m_cwnd = 2 * m_mss;  // I added this... haven't researched actual formula
				if (m_mss < nTransmit) {
					nTransmit = m_mss;
					break;
				}
			}
			LOG_NORMAL("transmit: Adjusting mss to %s bytes", m_mss);
		}

		if (nTransmit < seg.len) {
			LOG_NORMAL("transmit: mss reduced to %s", m_mss);

			SSegment subseg(seg.seq + nTransmit, seg.len - nTransmit, seg.bCtrl);
			// subseg.tstamp = seg->tstamp;
			subseg.xmit = seg.xmit;
			seg.len = nTransmit;

			m_slist->insertAfter(_seg, subseg);
		}

		if (seg.xmit == 0) {
			m_snd_nxt += seg.len;
		}
		seg.xmit += 1;
		// seg->tstamp = now;
		if (m_rto_base == 0) {
			m_rto_base = now;
		}

		return sl_true;
	}

	void PseudoTcp::attemptSend(PseudoTcpSendFlags sflags)
	{

		sl_uint32 now = PseudoTcp::now();

		if (TimeDiff32(now, m_lastsend) > static_cast<long>(m_rx_rto)) {
			m_cwnd = m_mss;
		}

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
		sl_bool bFirst = sl_true;
#endif

		while (sl_true) {
			sl_uint32 cwnd = m_cwnd;
			if ((m_dup_acks == 1) || (m_dup_acks == 2)) {  // Limited Transmit
				cwnd += m_dup_acks * m_mss;
			}
			sl_uint32 nWindow = Math::min(m_snd_wnd, cwnd);
			sl_uint32 nInFlight = m_snd_nxt - m_snd_una;
			sl_uint32 nUseable = (nInFlight < nWindow) ? (nWindow - nInFlight) : 0;

			sl_size snd_buffered = m_sbuf->getBuffered();
			sl_uint32 nAvailable = Math::min(static_cast<sl_uint32>(snd_buffered) - nInFlight, m_mss);

			if (nAvailable > nUseable) {
				if (nUseable * 4 < nWindow) {
					// RFC 813 - avoid SWS
					nAvailable = 0;
				} else {
					nAvailable = nUseable;
				}
			}

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
			if (bFirst) {
				sl_size available_space = 0;
				m_sbuf->getWriteRemaining(&available_space);

				bFirst = sl_false;
				LOG_VERBOSE("attemptSend: cwnd=%s, nWindow=%s, nInFlight=%s, nAvailable=%s, nQueued=%s, nEmpty=%s, ssthresh=%s", m_cwnd, nWindow, nInFlight, nAvailable, snd_buffered, available_space, m_ssthresh);
			}
#endif

			if (nAvailable == 0) {
				if (sflags == PseudoTcpSendFlags::None) {
					return;
				}
				// If this is an immediate ack, or the second delayed ack
				if ((sflags == PseudoTcpSendFlags::ImmediateAck) || m_t_ack) {
					buildPacket(m_snd_nxt, 0, 0, 0);
				} else {
					m_t_ack = PseudoTcp::now();
				}
				return;
			}

			// Nagle's algorithm.
			// If there is data already in-flight, and we haven't a full segment of data ready to send then hold off until we get more to send, or the in-flight data is acknowledged.
			if (m_use_nagling && (m_snd_nxt > m_snd_una) && (nAvailable < m_mss)) {
				return;
			}

			// Find the next segment to transmit
			auto it = m_slist->getFront();
			while (it->value.xmit > 0) {
				it = it->next;
				SLIB_ASSERT(it != sl_null);
			}
			auto seg = &(it->value);
			// If the segment is too large, break it into two
			if (seg->len > nAvailable) {
				SSegment subseg(seg->seq + nAvailable, seg->len - nAvailable, seg->bCtrl);
				seg->len = nAvailable;
				m_slist->insertAfter(it, subseg);
			}

			if (!transmit(it, now)) {
				LOG_VERBOSE("attemptSend: transmit failed");
				// TODO(?): consider closing socket
				return;
			}

			sflags = PseudoTcpSendFlags::None;
		}
	}

	void PseudoTcp::closedown(PseudoTcpError err)
	{
		LOG_NORMAL("closedown: changed state to Closed");
		m_state = PseudoTcpState::Closed;
		if (m_notify) {
			m_notify->onTcpClosed(this, err);
		}
	}

	void PseudoTcp::adjustMTU()
	{
		// Determine our current mss level, so that we can adjust appropriately later
		for (m_msslevel = 0; PACKET_MAXIMUMS[m_msslevel + 1] > 0; ++m_msslevel) {
			if (static_cast<sl_uint16>(PACKET_MAXIMUMS[m_msslevel]) <= m_mtu_advise) {
				break;
			}
		}
		m_mss = m_mtu_advise - PACKET_OVERHEAD;
		// !?! Should we reset m_largest here?
		LOG_NORMAL("Adjusting mss to %s bytes", m_mss);
		// Enforce minimums on ssthresh and cwnd
		m_ssthresh = Math::max(m_ssthresh, 2 * m_mss);
		m_cwnd = Math::max(m_cwnd, m_mss);
	}

	// This method is used in test only to query receive buffer state.
	sl_bool PseudoTcp::isReceiveBufferFull() const
	{
		sl_size available_space = 0;
		m_rbuf->getWriteRemaining(&available_space);
		return !available_space;
	}

	// This method is only used in tests, to disable window scaling support for testing backward compatibility.
	void PseudoTcp::disableWindowScale()
	{
		m_support_wnd_scale = sl_false;
	}

	// Queue the connect message with TCP options.
	void PseudoTcp::queueConnectMessage()
	{
		char _buf[32];
		MemoryWriter buf(_buf, sizeof(_buf));
		buf.writeUint8(CTL_CONNECT);
		if (m_support_wnd_scale) {
			buf.writeUint8(TCP_OPT_WND_SCALE);
			buf.writeUint8(1);
			buf.writeUint8(m_rwnd_scale);
		}
		m_snd_wnd = static_cast<sl_uint32>(buf.getPosition());
		queue(_buf, static_cast<sl_uint32>(buf.getPosition()), sl_true);
	}

	// Parse TCP options in the header.
	void PseudoTcp::parseOptions(const char* data, sl_uint32 len)
	{
		CHashMap<sl_uint8, sl_bool> options_specified;

		// See http://www.freesoft.org/CIE/Course/Section4/8.htm for parsing the options list.

		MemoryReader buf(data, len);

		while (buf.getRemainedSize()) {

			sl_uint8 kind = TCP_OPT_EOL;
			buf.readUint8(&kind);

			if (kind == TCP_OPT_EOL) {
				// End of option list.
				break;
			} else if (kind == TCP_OPT_NOOP) {
				// No op.
				continue;
			}

			// Length of this option.
			SLIB_ASSERT(len != 0);
			sl_uint8 opt_len = 0;
			buf.readUint8(&opt_len);

			// Content of this option.
			if (opt_len <= buf.getRemainedSize()) {
				applyOption(kind, data + buf.getPosition(), opt_len);
				buf.skip(opt_len);
			} else {
				LOG_NORMAL("parseOptions: Invalid option length received");
				return;
			}
			options_specified.put_NoLock(kind, sl_true);
		}

		if (!(options_specified.find_NoLock(TCP_OPT_WND_SCALE))) {
			LOG_NORMAL("parseOptions: Peer doesn't support window scaling");
			if (m_rwnd_scale > 0) {
				// Peer doesn't support TCP options and window scaling. Revert receive buffer size to default value.
				resizeReceiveBuffer(DEFAULT_RCV_BUF_SIZE);
				m_swnd_scale = 0;
			}
		}
	}

	// Apply a TCP option that has been read from the header.
	void PseudoTcp::applyOption(char kind, const char* data, sl_uint32 len)
	{
		if (kind == TCP_OPT_MSS) {
			LOG_NORMAL("applyOption: Peer specified MSS option which is not supported.");
			// TODO(?): Implement.
		} else if (kind == TCP_OPT_WND_SCALE) {
			// Window scale factor.
			// http://www.ietf.org/rfc/rfc1323.txt
			if (len != 1) {
				LOG_NORMAL("applyOption: Invalid window scale option received.");
				return;
			}
			applyWindowScaleOption(data[0]);
		}
	}

	// Apply window scale option.
	void PseudoTcp::applyWindowScaleOption(sl_uint8 scale_factor)
	{
		m_swnd_scale = scale_factor;
	}

	// Resize the send buffer with `new_size` in bytes.
	void PseudoTcp::resizeSendBuffer(sl_uint32 new_size)
	{
		m_sbuf_len = new_size;
		m_sbuf->setCapacity(new_size);
	}

	// Resize the receive buffer with `new_size` in bytes. This call adjusts window scale factor `m_swnd_scale` accordingly.
	void PseudoTcp::resizeReceiveBuffer(sl_uint32 new_size)
	{
		sl_uint8 scale_factor = 0;

		// Determine the scale factor such that the scaled window size can fit in a 16-bit unsigned integer.
		while (new_size > 0xFFFF) {
			++scale_factor;
			new_size >>= 1;
		}

		// Determine the proper size of the buffer.
		new_size <<= scale_factor;
		sl_bool result = m_rbuf->setCapacity(new_size);
		SLIB_UNUSED(result);

		// Make sure the new buffer is large enough to contain data in the old buffer. This should always be sl_true because this method is called either before connection is established or when peers are exchanging connect messages.
		SLIB_ASSERT(result);
		m_rbuf_len = new_size;
		m_rwnd_scale = scale_factor;
		m_ssthresh = new_size;

		sl_size available_space = 0;
		m_rbuf->getWriteRemaining(&available_space);
		m_rcv_wnd = static_cast<sl_uint32>(available_space);
	}

}
