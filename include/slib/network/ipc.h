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

#ifndef CHECKHEADER_SLIB_NETWORK_IPC
#define CHECKHEADER_SLIB_NETWORK_IPC

#include "definition.h"

#include "../data/data_container.h"
#include "../io/memory_output.h"
#include "../io/async_stream.h"

namespace slib
{

	typedef DataContainer IPCMessage;
	typedef IPCMessage IPCRequestMessage;
	typedef IPCMessage IPCResponseMessage;

	class SLIB_EXPORT IPCRequestParam
	{
	public:
		StringParam targetName;
		sl_bool flagGlobal;
		IPCRequestMessage message;

		Ref<AsyncIoLoop> ioLoop;
		Ref<Dispatcher> dispatcher;

		sl_int32 timeout; // In milliseconds
		sl_bool flagSelfAlive; // default: true

		sl_uint32 maximumMessageSize;
		sl_uint32 messageSegmentSize;

		Function<void(IPCResponseMessage&)> onResponse;

	public:
		IPCRequestParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(IPCRequestParam)

	};

	class SLIB_EXPORT IPCRequest : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		IPCRequest();

		~IPCRequest();

	public:
		sl_bool initialize(Ref<AsyncStream>&&, const IPCRequestParam&);

		sl_bool initialize(Ref<AsyncStream>&&, const IPCRequestParam&, sl_int64 tickEnd);

	protected:
		void onError();

		void onResponse(IPCResponseMessage& response);

		void sendRequest();

		void onSentRequest(AsyncStream*, sl_bool flagError);

		void receiveResponse();

		void onReceiveResponse(AsyncStream*, Memory& data, sl_bool flagError);

	protected:
		Ref<AsyncStream> m_stream;
		sl_bool m_flagSelfAlive;
		Ref<Dispatcher> m_dispatcher;
		Function<void(IPCResponseMessage&)> m_onResponse;

		Memory m_requestData;
		Memory m_responseData;
		sl_int64 m_tickEnd;
		sl_int32 m_maximumResponseSize;
		sl_uint32 m_messageSegmentSize;

		sl_int32 m_nCountFinish;

	};

	class SLIB_EXPORT IPCServerParam
	{
	public:
		StringParam name;
		sl_bool flagGlobal;
		Ref<AsyncIoLoop> ioLoop;
		Ref<Dispatcher> dispatcher; // usually ThreadPool

		sl_uint32 maximumMessageSize;
		sl_uint32 messageSegmentSize;
		sl_bool flagAcceptOtherUsers; // default: true

		Function<void(IPCRequestMessage&, IPCResponseMessage&)> onReceiveMessage;

	public:
		IPCServerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(IPCServerParam)

	};

	class SLIB_EXPORT IPCServer : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		IPCServer();

		~IPCServer();

	protected:
		sl_bool initialize(const IPCServerParam& param);

		void startStream(AsyncStream*);

		void receiveRequest(AsyncStream*);

		void onReceiveRequest(AsyncStream*, Memory& data, sl_bool flagError);

		void processRequest(AsyncStream*, const Memory& data);

		void sendResponse(AsyncStream*, const Memory& data);

		void onSentResponse(AsyncStream*, sl_bool flagError);

	protected:
		Ref<AsyncIoLoop> m_ioLoop;
		Ref<Dispatcher> m_dispatcher;
		sl_uint32 m_maximumMessageSize;
		sl_uint32 m_messageSegmentSize;
		sl_bool m_flagAcceptOtherUsers;
		Function<void(IPCRequestMessage&, IPCResponseMessage&)> m_onReceiveMessage;

		HashMap< AsyncStream*, Ref<AsyncStream> > m_streams;

	};

	class SLIB_EXPORT IPC
	{
	public:
		typedef IPCRequest Request;
		typedef IPCRequestParam RequestParam;
		typedef IPCRequestMessage RequestMessage;
		typedef IPCResponseMessage ResponseMessage;
		typedef IPCServer Server;
		typedef IPCServerParam ServerParam;

	public:
		static Ref<Request> sendMessage(const RequestParam& param);

		static Ref<Request> sendMessage(const StringParam& targetName, const RequestMessage& request, const Function<void(ResponseMessage&)>& callbackResponse);

		static sl_bool sendMessageSynchronous(const RequestParam& param, ResponseMessage& response);

		static sl_bool sendMessageSynchronous(const StringParam& targetName, const RequestMessage& request, ResponseMessage& response, sl_int32 timeout = -1);

		static Ref<Server> createServer(const ServerParam& param);

	};

	class SocketIPC
	{
	public:
		typedef IPCRequest Request;
		typedef IPCRequestParam RequestParam;
		typedef IPCRequestMessage RequestMessage;
		typedef IPCResponseMessage ResponseMessage;
		typedef IPCServer Server;
		typedef IPCServerParam ServerParam;

	public:
		static Ref<Request> sendMessage(const RequestParam& param);

		static Ref<Request> sendMessage(const StringParam& targetName, const RequestMessage& message, const Function<void(ResponseMessage&)>& callbackResponse);

		static sl_bool sendMessageSynchronous(const RequestParam& param, ResponseMessage& response);

		static sl_bool sendMessageSynchronous(const StringParam& targetName, const RequestMessage& request, ResponseMessage& response, sl_int32 timeout = -1);

		static Ref<Server> createServer(const ServerParam& param);

	};

}

#endif
