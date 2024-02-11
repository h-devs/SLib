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

#ifndef CHECKHEADER_SLIB_CORE_STRING_BUFFER
#define CHECKHEADER_SLIB_CORE_STRING_BUFFER

#include "string.h"
#include "queue.h"


/**
 * @addtogroup core
 *  @{
 */
namespace slib
{

	/** @class StringBuffer
	 * @brief Queue containing string items. StringBuffer is not thread-safe.
	 */
	class SLIB_EXPORT StringBuffer
	{
	public:
		typedef String StringType;
		typedef sl_char8 Char;

	public:
		StringBuffer() noexcept;

		~StringBuffer() noexcept;

	public:
		/**
		 * @return total length of all string items.
		 */
		sl_size getLength() const noexcept;

		/**
		 * @return `true` if this buffer is empty.
		 */
		sl_bool isEmpty() const noexcept;

		/**
		 * @return `true` if this buffer is not empty.
		 */
		sl_bool isNotEmpty() const noexcept;

		/**
		 * @return the first character in the buffer.
		 */
		sl_char8 getFirstChar() const noexcept;

		/**
		 * @return the last character in the buffer.
		 */
		sl_char8 getLastChar() const noexcept;

		/**
		 * @return the character at `index` in string.
		 */
		sl_char8 getCharAt(sl_size index) const noexcept;

		/**
		 * Add string object to the queue.
		 */
		sl_bool add(const String& str) noexcept;
		sl_bool add(String&& str) noexcept;

		/**
		 * Add string represented by StringStorage struct to the queue.
		 */
		sl_bool add(const StringStorage& str) noexcept;

		/**
		 * Add string pointed by buf to the queue.
		 */
		sl_bool addStatic(const sl_char8* buf, sl_size length) noexcept;

		template <sl_size N>
		sl_bool addStatic(const sl_char8 (&s)[N]) noexcept
		{
			return addStatic(s, N - 1);
		}

		/**
		 * Add all string items in buf to the current object and then clear buf.
		 */
		void link(StringBuffer& buf) noexcept;

		/**
		 * Clears all items.
		 */
		void clear() noexcept;

		/**
		 * Merge all items and returns the merged string.
		 */
		String merge() const noexcept;

		/**
		 * Merge all items and returns it as memory.
		 */
		Memory mergeToMemory() const noexcept;

	private:
		LinkedQueue<StringStorage> m_queue;
		sl_size m_len;

	};


	/** @class StringBuffer16
	 * @brief Queue containing String16 items. StringBuffer16 is not thread-safe.
	 */
	class SLIB_EXPORT StringBuffer16
	{
	public:
		typedef String16 StringType;
		typedef sl_char16 Char;

	public:
		StringBuffer16() noexcept;

		~StringBuffer16() noexcept;

	public:
		/**
		 * @return total length of all string items.
		 */
		sl_size getLength() const noexcept;

		/**
		 * @return `true` if this buffer is empty.
		 */
		sl_bool isEmpty() const noexcept;

		/**
		 * @return `true` if this buffer is not empty.
		 */
		sl_bool isNotEmpty() const noexcept;

		/**
		 * @return the first character in the buffer.
		 */
		sl_char16 getFirstChar() const noexcept;

		/**
		 * @return the last character in the buffer.
		 */
		sl_char16 getLastChar() const noexcept;

		/**
		 * @return the character at `index` in string.
		 */
		sl_char16 getCharAt(sl_size index) const noexcept;

		/**
		 * Add string object to the queue.
		 */
		sl_bool add(const String16& str) noexcept;
		sl_bool add(String16&& str) noexcept;

		/**
		 * Add string represented by StringStorage struct to the queue.
		 */
		sl_bool add(const StringStorage& str) noexcept;

		/**
		 * Add string pointed by buf to the queue.
		 */
		sl_bool addStatic(const sl_char16* buf, sl_size length) noexcept;

		template <sl_size N>
		sl_bool addStatic(const sl_char16 (&s)[N]) noexcept
		{
			return addStatic(s, N - 1);
		}

		/**
		 * Add all string items in buf to the current object and then clear buf.
		 */
		void link(StringBuffer16& buf) noexcept;

		/**
		 * Clears all items.
		 */
		void clear() noexcept;

		/**
		 * Merge all items and returns the merged string.
		 */
		String16 merge() const noexcept;

		/**
		 * Merge all items and returns it as memory.
		 */
		Memory mergeToMemory() const noexcept;

	private:
		LinkedQueue<StringStorage> m_queue;
		sl_size m_len;

	};


	/** @class StringBuffer32
	* @brief Queue containing String32 items. StringBuffer32 is not thread-safe.
	*/
	class SLIB_EXPORT StringBuffer32
	{
	public:
		typedef String32 StringType;
		typedef sl_char32 Char;

	public:
		StringBuffer32() noexcept;

		~StringBuffer32() noexcept;

	public:
		/**
		 * @return total length of all string items.
		 */
		sl_size getLength() const noexcept;
		
		/**
		 * @return `true` if this buffer is empty.
		 */
		sl_bool isEmpty() const noexcept;

		/**
		 * @return `true` if this buffer is not empty.
		 */
		sl_bool isNotEmpty() const noexcept;

		/**
		 * @return the first character in the buffer.
		 */
		sl_char32 getFirstChar() const noexcept;

		/**
		 * @return the last character in the buffer.
		 */
		sl_char32 getLastChar() const noexcept;

		/**
		 * @return the character at `index` in string.
		 */
		sl_char32 getCharAt(sl_size index) const noexcept;

		/**
		 * Add string object to the queue.
		 */
		sl_bool add(const String32& str) noexcept;
		sl_bool add(String32&& str) noexcept;

		/**
		 * Add string represented by StringStorage struct to the queue.
		 */
		sl_bool add(const StringStorage& str) noexcept;

		/**
		 * Add string pointed by buf to the queue.
		 */
		sl_bool addStatic(const sl_char32* buf, sl_size length) noexcept;

		template <sl_size N>
		sl_bool addStatic(const sl_char32(&s)[N]) noexcept
		{
			return addStatic(s, N - 1);
		}

		/**
		 * Add all string items in buf to the current object and then clear buf.
		 */
		void link(StringBuffer32& buf) noexcept;

		/**
		 * Clears all items.
		 */
		void clear() noexcept;

		/**
		 * Merge all items and returns the merged string.
		 */
		String32 merge() const noexcept;

		/**
		 * Merge all items and returns it as memory.
		 */
		Memory mergeToMemory() const noexcept;

	private:
		LinkedQueue<StringStorage> m_queue;
		sl_size m_len;

	};

	template <class CharType>
	struct StringBufferTypeFromCharType;

	template <>
	struct StringBufferTypeFromCharType<sl_char8> { typedef StringBuffer Type; };

	template <>
	struct StringBufferTypeFromCharType<sl_char16> { typedef StringBuffer16 Type; };

	template <>
	struct StringBufferTypeFromCharType<sl_char32> { typedef StringBuffer32 Type; };

}

/// @}

#endif
