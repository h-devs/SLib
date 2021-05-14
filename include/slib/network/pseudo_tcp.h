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

#ifndef CHECKHEADER_SLIB_NETWORK_PSEUDO_TCP
#define CHECKHEADER_SLIB_NETWORK_PSEUDO_TCP

#include "definition.h"

#include "../core/linked_list.h"
#include "../core/unique_ptr.h"

namespace slib
{

	enum class PseudoTcpWriteResult
	{
		Success = 0,
		TooLarge,
		Fail
	};

	enum class PseudoTcpState
	{
		Listen,
		SentSyn,
		ReceivedSyn,
		Established,
		Closed
	};

	enum class PseudoTcpSendFlags
	{
		None = 0,
		DelayedAck,
		ImmediateAck
	};

	enum class PseudoTcpShutdownType
	{
		None = 0,
		Graceful,
		Forceful
	};

	enum class PseudoTcpError
	{
		None = 0,
		WouldBlock,
		InvalidState,
		NotConnected,
		ConnectionAborted,
		ConnectionReset
	};

	class PseudoTcp;

	namespace priv
	{
		namespace pseudo_tcp
		{

			class Segment
			{
			public:
				sl_uint32 conv, seq, ack;
				sl_uint8 flags;
				sl_uint16 wnd;
				const char* data;
				sl_uint32 len;
				sl_uint32 tsval, tsecr;
			};

			class SSegment
			{
			public:
				SSegment(sl_uint32 s, sl_uint32 l, sl_bool c) : seq(s), len(l), xmit(0), bCtrl(c) {}

			public:
				sl_uint32 seq, len;
				sl_uint8 xmit;
				sl_bool bCtrl;
			};

			class RSegment
			{
			public:
				sl_uint32 seq, len;
			};

			typedef CLinkedList<SSegment> SList;
			
			typedef CLinkedList<RSegment> RList;
			
			class LockedFifoBuffer
			{
			public:
				LockedFifoBuffer(sl_size size);
				~LockedFifoBuffer();

			public:
				sl_size getBuffered() const;
				sl_bool setCapacity(sl_size size);
				sl_bool readOffset(void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_read);
				sl_bool writeOffset(const void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_written);
				sl_bool read(void* buffer, sl_size bytes, sl_size* bytes_read);
				sl_bool write(const void* buffer, sl_size bytes, sl_size* bytes_written);
				void consumeReadData(sl_size size);
				void consumeWriteBuffer(sl_size size);
				sl_bool getWriteRemaining(sl_size* size) const;

			private:
				sl_bool readOffsetLocked(void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_read);
				sl_bool writeOffsetLocked(const void* buffer, sl_size bytes, sl_size offset, sl_size* bytes_written);

			private:
				UniquePtr<char[]> m_buf; // the allocated buffer
				sl_size m_lenBuf; // size of the allocated buffer
				sl_size m_lenData; // amount of readable data in the buffer
				sl_size m_posRead; // offset to the readable data
				Mutex m_lock;
			};

		}
	}

	class SLIB_EXPORT IPseudoTcpNotify
	{
	public:
		IPseudoTcpNotify();

		virtual ~IPseudoTcpNotify();

	public:
		virtual void onTcpOpen(PseudoTcp* tcp) = 0;

		virtual void onTcpReadable(PseudoTcp* tcp) = 0;

		virtual void onTcpWriteable(PseudoTcp* tcp) = 0;

		virtual void onTcpClosed(PseudoTcp* tcp, PseudoTcpError error) = 0;

		// write the packet onto the network
		virtual PseudoTcpWriteResult writeTcpPacket(PseudoTcp* tcp, const void* buf, sl_size len) = 0;

	};

	class SLIB_EXPORT PseudoTcp
	{
	public:
		PseudoTcp(IPseudoTcpNotify* notify, sl_uint32 conv);

		~PseudoTcp();

	public:
		static sl_uint32 now();

		sl_bool connect();

		sl_int32 receive(void* buf, sl_size len);

		sl_int32 send(const void* buf, sl_size len);

		void close(sl_bool bForce);

		PseudoTcpError getError();

		PseudoTcpState getState();

	public:
		// Call this when the PMTU changes.
		void notifyMTU(sl_uint16 mtu);

		// Call this based on `timeout` value returned from `getNextClock`. It's ok to call this too frequently.
		void notifyClock(sl_uint32 now);

		// Call this whenever a packet arrives. Returns true if the packet was processed successfully.
		sl_bool notifyPacket(const void* buf, sl_size len);

		// Call this to determine the next time `notifyClock` should be called. Returns false if the socket is ready to be destroyed.
		sl_bool getNextClock(sl_uint32 now, sl_uint32& timeout);

	public:
		sl_bool isNoDelay(); // Whether to enable Nagle's algorithm (0: off)

		void setNoDelay(sl_bool flag);

		sl_uint32 getAckDelay(); // he Delayed ACK timeout (0: off)

		void setAckDelay(sl_uint32 delay);

		sl_uint32 getReceiveBufferSize();

		// call before `connect()`
		void setReceiveBufferSize(sl_uint32 size);

		sl_uint32 getSendBufferSize();

		// call before `connect()`
		void setSendBufferSize(sl_uint32 size);

		// current congestion window in bytes.
		sl_uint32 getCongestionWindow() const;

		// Returns amount of data in bytes that has been sent, but haven't been acknowledged.
		sl_uint32 getBytesInFlight() const;

		// Returns number of bytes that were written in buffer and haven't been sent.
		sl_uint32 getBytesBufferedNotSent() const;

		// Returns current round-trip time estimate in milliseconds.
		sl_uint32 getRoundTripTimeEstimate() const;

	protected:
		sl_uint32 queue(const char* data, sl_uint32 len, sl_bool bCtrl);

		PseudoTcpWriteResult buildPacket(sl_uint32 seq, sl_uint8 flags, sl_uint32 offset, sl_uint32 len);

		sl_bool parsePacket(const sl_uint8* buffer, sl_uint32 size);

		void attemptSend(PseudoTcpSendFlags sflags = PseudoTcpSendFlags::None);

		void closedown(PseudoTcpError err = PseudoTcpError::None);

		sl_bool clock_check(sl_uint32 now, sl_uint32& nTimeout);

		sl_bool process(priv::pseudo_tcp::Segment& seg);

		sl_bool transmit(Link<priv::pseudo_tcp::SSegment>* seg, sl_uint32 now);

		void adjustMTU();

	protected:
		sl_bool isReceiveBufferFull() const;

		void disableWindowScale();

		void queueConnectMessage();

		void parseOptions(const char* data, sl_uint32 len);

		void applyOption(char kind, const char* data, sl_uint32 len);

		void applyWindowScaleOption(sl_uint8 scale_factor);

		void resizeSendBuffer(sl_uint32 new_size);

		void resizeReceiveBuffer(sl_uint32 new_size);

	protected:
		IPseudoTcpNotify* m_notify;
		PseudoTcpShutdownType m_shutdown;
		PseudoTcpError m_error;

		// TCB data
		PseudoTcpState m_state;
		sl_uint32 m_conv;
		sl_bool m_bReadEnable, m_bWriteEnable, m_bOutgoing;
		sl_uint32 m_lasttraffic;

		// Incoming data
		priv::pseudo_tcp::RList m_rlist;
		sl_uint32 m_rbuf_len, m_rcv_nxt, m_rcv_wnd, m_lastrecv;
		sl_uint8 m_rwnd_scale;  // Window scale factor.
		priv::pseudo_tcp::LockedFifoBuffer m_rbuf;

		// Outgoing data
		priv::pseudo_tcp::SList m_slist;
		sl_uint32 m_sbuf_len, m_snd_nxt, m_snd_wnd, m_lastsend, m_snd_una;
		sl_uint8 m_swnd_scale;  // Window scale factor.
		priv::pseudo_tcp::LockedFifoBuffer m_sbuf;

		// Maximum segment size, estimated protocol level, largest segment sent
		sl_uint32 m_mss, m_msslevel, m_largest, m_mtu_advise;
		// Retransmit timer
		sl_uint32 m_rto_base;

		// Timestamp tracking
		sl_uint32 m_ts_recent, m_ts_lastack;

		// Round-trip calculation
		sl_uint32 m_rx_rttvar, m_rx_srtt, m_rx_rto;

		// Congestion avoidance, Fast retransmit/recovery, Delayed ACKs
		sl_uint32 m_ssthresh, m_cwnd;
		sl_uint8 m_dup_acks;
		sl_uint32 m_recover;
		sl_uint32 m_t_ack;

		// Configuration options
		sl_bool m_use_nagling;
		sl_uint32 m_ack_delay;

		// This is used by unit tests to test backward compatibility of PseudoTcp implementations that don't support window scaling.
		sl_bool m_support_wnd_scale;

	};

}

#endif
