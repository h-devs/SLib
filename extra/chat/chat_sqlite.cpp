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

#include "chat.h"

#include "../file_encrypt/chacha.h"

#include <slib/db/sqlite.h>

namespace slib
{

	namespace {

		class Room : public CRef
		{
		public:
			Ref<DatabaseStatement> smtInsertMessage;
			Ref<DatabaseStatement> smtGetMessagesFrom;
			Ref<DatabaseStatement> smtGetMessagesTo;

			ChatRoom data;

		};

		class DatabaseImpl : public ChatClientDatabase
		{
		public:
			Ref<SQLite> m_db;

			HashMap< String, Ref<Room> > m_mapRooms;
			HashMap<String, sl_uint64> m_mapSenderIdAndIndex;
			HashMap<sl_uint64, String> m_mapSenderIndexAndId;

			Ref<DatabaseStatement> m_smtInsertSenderId;
			Ref<DatabaseStatement> m_smtGetContacts;
			Ref<DatabaseStatement> m_smtGetContact;
			Ref<DatabaseStatement> m_smtAddContact;

		public:
			static Ref<DatabaseImpl> create(const String& dbPath, const String& encryptionKey)
			{
				Ref<DatabaseImpl> ret = new DatabaseImpl;
				if (ret.isNotNull()) {
					SQLiteParam param;
					param.path = dbPath;
					param.encryption = new ChaChaFileEncryption(encryptionKey);
					Ref<SQLite> db = SQLite::open(param);
					if (db.isNotNull()) {
						ret->m_db = db;
						if (ret->initialize()) {
							return ret;
						}
					}
				}
				return sl_null;
			}

			sl_bool initialize()
			{
				if (m_db->isTableExisting("t_sender_index")) {
					Ref<DatabaseCursor> cursor = m_db->query("SELECT rowid, f_sender_id FROM t_sender_index");
					if (cursor.isNotNull()) {
						while (cursor->moveNext()) {
							sl_uint64 index = cursor->getUint64(0);
							String id = cursor->getString(1);
							m_mapSenderIndexAndId.put_NoLock(index, id);
							m_mapSenderIdAndIndex.put_NoLock(id, index);
						}
					}
				} else {
					StringParam p;

					if (m_db->execute("CREATE TABLE IF NOT EXISTS t_sender_index (f_sender_id TEXT NOT NULL UNIQUE);") < 0) {
						return sl_false;
					}
				}
				m_smtInsertSenderId = m_db->prepareStatement("INSERT INTO t_sender_index (f_sender_id) VALUES (?)");
				if (m_smtInsertSenderId.isNull()) {
					return sl_false;
				}
				return sl_true;
			}

			List<ChatContact> getContacts() override
			{
				return sl_null;
			}

			sl_bool getContact(const String& userId, ChatContact& outContact) override
			{
				return sl_false;
			}

			sl_bool addContact(const ChatContact& contact) override
			{
				return sl_false;
			}

			void updateContact(const ChatContact& contact) override
			{

			}

			void removeContact(const String& userId) override
			{

			}

			List<ChatRoom> getRooms() override
			{
				return sl_null;
			}

			sl_bool getRoom(const String& roomId, ChatRoom& outRoom) override
			{
				return sl_false;
			}

			sl_bool addRoom(const ChatRoom& room) override
			{
				return sl_false;
			}

			void updateRoom(const ChatRoom& room) override
			{

			}

			void removeRoom(const String& roomId) override
			{

			}

			static String getRoomTableName(const String& roomId)
			{
				return "t_room_" + roomId;
			}

			Ref<Room> createRoom(const String& roomId)
			{
				String tableName = getRoomTableName(roomId);
				Ref<Room> room = m_mapRooms.getValue_NoLock(roomId);
				if (room.isNull()) {
					if (!(m_db->isTableExisting(tableName))) {
						if (m_db->execute("CREATE TABLE IF NOT EXISTS " + tableName + " (f_sender_index BIGINT, f_msg_id BIGINT, f_time BIGINT, f_type INTEGER, f_encrypted BOOLEAN, f_inlined BOOLEAN, f_text TEXT, f_content BLOB)") < 0) {
							return sl_null;
						}
						if (m_db->execute("CREATE INDEX IF NOT EXISTS " + tableName + "_index ON " + tableName + " (f_msg_id)") < 0) {
							return sl_null;
						}
						if (m_db->execute("CREATE UNIQUE INDEX IF NOT EXISTS " + tableName + "_uindex ON " + tableName + " (f_msg_id, f_sender_index)") < 0) {
							return sl_null;
						}
					}
					room = new Room;
					room->smtInsertMessage = m_db->prepareStatement("INSERT INTO " + tableName + "(f_sender_index, f_msg_id, f_time, f_type, f_encrypted, f_inlined, f_text, f_content) VALUES (?,?,?,?,?,?,?,?)");
					if (room->smtInsertMessage.isNull()) {
						return sl_null;
					}
					String strSqlGetMessagesPrefix = "SELECT f_sender_index, f_msg_id, f_time, f_type, f_encrypted, f_inlined, f_text, f_content FROM " + tableName;
					String strSqlGetMessagesSuffix = " ORDER BY f_msg_id DESC LIMIT ?";
					room->smtGetMessagesFrom = m_db->prepareStatement(strSqlGetMessagesPrefix + " WHERE f_msg_id>=?" + strSqlGetMessagesSuffix);
					room->smtGetMessagesTo = m_db->prepareStatement(strSqlGetMessagesPrefix + " WHERE f_msg_id<=?" + strSqlGetMessagesSuffix);
					if (room->smtInsertMessage.isNull()) {
						return sl_null;
					}
					m_mapRooms.put_NoLock(roomId, room);
				}
				return room;
			}

			sl_bool addMessage(const String& roomId, const ChatMessage& message) override
			{
				if (roomId.isEmpty() || message.senderId.isEmpty()) {
					return sl_false;
				}
				ObjectLocker lock(this);
				Ref<Room> room = createRoom(roomId);
				if (room.isNull()) {
					return sl_false;
				}
				sl_uint64 senderIndex;
				if (!(m_mapSenderIdAndIndex.get_NoLock(message.senderId, &senderIndex))) {
					if (m_smtInsertSenderId->execute(message.senderId) <= 0) {
						return sl_false;
					}
					senderIndex = m_db->getLastInsertRowId();
					m_mapSenderIdAndIndex.put_NoLock(message.senderId, senderIndex);
					m_mapSenderIndexAndId.put_NoLock(senderIndex, message.senderId);
				}
				if (room->smtInsertMessage->execute(senderIndex, message.messageId, message.time, (int)(message.contentType), message.flagEncrypted, message.flagInlined, message.text, message.content) > 0) {
					return sl_true;
				}
				return sl_false;
			}

			List<ChatMessage> getMessages(const String& roomId, sl_bool flagFrom, sl_uint64 base, sl_uint32 countLimit)
			{
				if (roomId.isEmpty()) {
					return sl_null;
				}
				String tableName = getRoomTableName(roomId);
				ObjectLocker lock(this);
				Ref<Room> room = m_mapRooms.getValue_NoLock(roomId);
				if (room.isNull()) {
					return sl_null;
				}
				Ref<DatabaseCursor> cursor;
				if (flagFrom) {
					cursor = room->smtGetMessagesFrom->query(base, countLimit);
				} else {
					cursor = room->smtGetMessagesTo->query(base, countLimit);
				}
				if (cursor.isNotNull()) {
					List<ChatMessage> list;
					while (cursor->moveNext()) {
						ChatMessage message;
						message.senderId = m_mapSenderIndexAndId.getValue(cursor->getUint64(0));
						if (message.senderId.isNotEmpty()) {
							message.messageId = cursor->getUint64(1);
							message.time = cursor->getTime(2);
							message.contentType = (ChatContentType)(cursor->getUint32(3));
							message.flagEncrypted = cursor->getBoolean(4);
							message.flagInlined = cursor->getBoolean(5, sl_true);
							message.text = cursor->getString(6);
							message.content = cursor->getBlob(7);
							list.add_NoLock(message);
						}
					}
					return list;
				}
				return sl_null;
			}

			List<ChatMessage> getMessagesFrom(const String& roomId, sl_uint64 start, sl_uint32 countLimit) override
			{
				return getMessages(roomId, sl_true, start, countLimit);
			}

			List<ChatMessage> getMessagesTo(const String& roomId, sl_uint64 end, sl_uint32 countLimit) override
			{
				return getMessages(roomId, sl_false, end, countLimit);
			}

		};

	}

	Ref<ChatClientDatabase> ChatClientDatabase::createSQLite(const String& dbPath, const String& encryptionKey)
	{
		return DatabaseImpl::create(dbPath, encryptionKey);
	}

}

