/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include "chat_client.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ChatClientParam)

	ChatClientParam::ChatClientParam()
	{
	}


	SLIB_DEFINE_OBJECT(ChatClient, Object)

	ChatClient::ChatClient()
	{
	}

	ChatClient::~ChatClient()
	{
	}

	Ref<ChatClient> ChatClient::create(const ChatClientParam& param)
	{
		if (param.myId.isEmpty() || param.chatView.isNull() || param.database.isNull()) {
			return sl_null;
		}
		Ref<ChatClient> ret = new ChatClient;
		if (ret->initialize(param)) {
			return ret;
		}
		return sl_null;
	}

	sl_bool ChatClient::initialize(const ChatClientParam& param)
	{
		m_myId = param.myId;
		m_roomsView = param.roomsView;
		m_chatView = param.chatView;
		m_database = param.database;
		m_service = param.service;
		return sl_true;
	}

	void ChatClient::sendMessage(const String& receiverId, const String& message)
	{

	}

	void ChatClient::dispatchReceiveMessage(const String& roomId, ChatMessage& message)
	{

	}

}
