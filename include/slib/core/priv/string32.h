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

#ifndef CHECKHEADER_SLIB_CORE_STRING32
#define CHECKHEADER_SLIB_CORE_STRING32

/**
 * @addtogroup core
 *  @{
 */

namespace slib
{

	class SLIB_EXPORT StringContainer32
	{
	public:
		typedef String32 StringType;

	public:
		sl_char32* data;
		sl_size len;
		sl_size hash;
		sl_uint32 type;
		volatile sl_reg ref;

	public:
		sl_reg increaseReference() noexcept;

		sl_reg decreaseReference() noexcept;

	};

	/**
	 * @class String32
	 * @brief String32 class provides an extensive set of APIs for working with strings, including method for comparing, searching, and modifying strings.
	 */
	class SLIB_EXPORT String32
	{
	public:
		typedef StringContainer32 Container;
		typedef sl_char32 Char;
		typedef StringView32 StringViewType;
#ifdef SLIB_SUPPORT_STD_TYPES
		typedef std::u32string StdString;
#endif

	private:
		Container* m_container;

	public:
		/**
		 * Initializes as a null string.
		 */
		SLIB_CONSTEXPR String32(): m_container(sl_null) {}
		SLIB_CONSTEXPR String32(sl_null_t): m_container(sl_null) {}
		SLIB_CONSTEXPR String32(Container* container): m_container(container) {}

		/**
		 * Constructors
		 */
		String32(String32&& src) noexcept
		{
			m_container = src.m_container;
			src.m_container = sl_null;
		}

		String32(const String32& src) noexcept;
		String32(AtomicString32&& src) noexcept;
		String32(const AtomicString32& src) noexcept;
		String32(const StringView32& src) noexcept;

		/**
		 * Destructor
		 */
		~String32();

	public:

		/**
		 * Fill the string with `nRepeatCount` consecutive copies of charactor `ch`
		 */
		String32(sl_char32 ch, sl_size nRepeatCount) noexcept;

		/**
		 * Copies the null-terminated character sequence pointed by `str`.
		 */
		String32(const sl_char32* str) noexcept;

		/**
		 * Copies the first `length` characters from the array of characters pointed by `str`
		 */
		String32(const sl_char32* str, sl_reg length) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Initialize from `std::u32string`.
		 * This does not copy the data of the string, but keep the reference to the original string.
		 */
		String32(const std::u32string& str) noexcept;
		String32(std::u32string&& str) noexcept;
#endif

	public:
		/**
		 * Create a string of `len` characters.
		 */
		static String32 allocate(sl_size len) noexcept;

		/**
		 * Creates a string copying other string
		 */
		static String32 create(const String& str) noexcept;
		static String32 create(const String16& str) noexcept;
		static String32 create(const String32& str) noexcept;
		static String32 create(const StringView& str) noexcept;
		static String32 create(const StringView16& str) noexcept;
		static String32 create(const StringView32& str) noexcept;

		/**
		 * Creates a string from the array of characters pointed by `str`
		 */
		static String32 create(const char* str, sl_reg length = -1) noexcept;
		static String32 create(const wchar_t* str, sl_reg length = -1) noexcept;
		static String32 create(const char16_t* str, sl_reg length = -1) noexcept;
		static String32 create(const char32_t* str, sl_reg length = -1) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Create a string copying from std strings.
		 */
		static String32 create(const std::u32string& str) noexcept;
		static String32 create(const std::string& str) noexcept;
		static String32 create(const std::wstring& str) noexcept;
		static String32 create(const std::u16string& str) noexcept;
#endif

		/**
		 * Creates a string pointing the `str` as the content, without copying the data.
		 * `str` should not be freed while the returned string is being used.
		 */
		template <sl_size N> static String32 fromStatic(const sl_char32 (&str)[N]) noexcept
		{
			return fromStatic(str, N - 1);
		}

		static String32 fromStatic(const sl_char32* str, sl_reg len) noexcept;

		/**
		 * Creates a string pointing the `str` as the content, without copying the data.
		 * `ref` should be used to keep the alive of the string content.
		 */
		static String32 fromRef(CRef* ref, const sl_char32* str, sl_size len) noexcept;

		/**
		 * Creates a string pointing the `mem` as the UTF-32 content, without copying the data.
		 */
		static String32 fromMemory(const Memory& mem) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-8 text.
		 *
		 * @param[in] text string buffer containing the UTF-8 text
		 * @param[in] len length of the `text`. negative value means that `text` is null terminated.
		 */
		static String32 fromUtf8(const void* text, sl_reg len = -1) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-8 text in `mem`.
		 */
		static String32 fromUtf8(const MemoryView& mem) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-16 text.
		 *
		 * @param[in] text string buffer containing the UTF-16 text
		 * @param[in] len length of the `text`. negative value means that `text` is null terminated.
		 */
		static String32 fromUtf16(const sl_char16* text, sl_reg len = -1) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-16 Big Endian text.
		 *
		 * @param[in] text string buffer containing the UTF-16 Big Endian text
		 * @param[in] size size (in bytes) of the `text`
		 */
		static String32 fromUtf16BE(const void* text, sl_size size) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-16 Big Endian text in `mem`.
		 */
		static String32 fromUtf16BE(const MemoryView& mem) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-16 Little Endian text.
		 *
		 * @param[in] text string buffer containing the UTF-16 Little Endian text
		 * @param[in] size size (in bytes) of the `text`
		 */
		static String32 fromUtf16LE(const void* text, sl_size size) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-16 Little Endian text in `mem`.
		 */
		static String32 fromUtf16LE(const MemoryView& mem) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-32 text.
		 *
		 * @param[in] text string buffer containing the UTF-32 text
		 * @param[in] len length of the `text`. negative value means that `text` is null terminated.
		 */
		static String32 fromUtf32(const sl_char32* text, sl_reg len = -1) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-8, UTF-16BE, or UTF-16LE text.
		 * This function detects the encoding type from the first 3 bytes of `text`.
		 *
		 * @param[in] text string buffer containing the unicode text.
		 * @param[in] size size (in bytes) of the `text`
		 */
		static String32 fromUtf(const void* text, sl_size size) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-8, UTF-16BE, or UTF-16LE text in `mem`.
		 * This function detects the encoding type from the first 3 bytes of the text.
		 */
		static String32 fromUtf(const MemoryView& mem) noexcept;

		/**
		 * Creates a string copying the characters from `text` encoded by `charset`.
		 */
		static String32 decode(Charset charset, const void* text, sl_size size);

		/**
		 * Creates a string copying the characters from text in `mem`, encoded by `charset`.
		 */
		static String32 decode(Charset charset, const MemoryView& mem);


		static const String32& from(const String32& str) noexcept
		{
			return str;
		}

		static String32&& from(String32&& str) noexcept
		{
			return Move(str);
		}

		static String32 from(const String& str) noexcept;
		static String32 from(const AtomicString& str) noexcept;
		static String32 from(const String16& str) noexcept;
		static String32 from(const AtomicString16& str) noexcept;
		static String32 from(const AtomicString32& str) noexcept;
		static String32 from(const StringView& str) noexcept;
		static String32 from(const StringView16& str) noexcept;
		static String32 from(const StringView32& str) noexcept;
		static String32 from(const StringParam& str) noexcept;
		static String32 from(const char* str, sl_reg length = -1) noexcept;
		static String32 from(const wchar_t* str, sl_reg length = -1) noexcept;
		static String32 from(const char16_t* str, sl_reg length = -1) noexcept;
		static String32 from(const char32_t* str, sl_reg length = -1) noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		static String32 from(const std::string& str) noexcept;
		static String32 from(const std::wstring& str) noexcept;
		static String32 from(const std::u16string& str) noexcept;
		static String32 from(const std::u32string& str) noexcept;
#endif
		static String32 from(signed char value) noexcept;
		static String32 from(unsigned char value) noexcept;
		static String32 from(short value) noexcept;
		static String32 from(unsigned short value) noexcept;
		static String32 from(int value) noexcept;
		static String32 from(unsigned int value) noexcept;
		static String32 from(long value) noexcept;
		static String32 from(unsigned long value) noexcept;
		static String32 from(sl_int64 value) noexcept;
		static String32 from(sl_uint64 value) noexcept;
		static String32 from(float value) noexcept;
		static String32 from(double value) noexcept;
		static String32 from(sl_bool value) noexcept;
		static String32 from(const Time& value) noexcept;
		static String32 from(const Variant& var) noexcept;

	public:
		/**
		 * @return null string.
		 */
		static const String32& null() noexcept
		{
			return *(reinterpret_cast<String32 const*>(&(priv::string::g_null32)));
		}

		/**
		 * @return empty string.
		 */
		static const String32& getEmpty() noexcept
		{
			return *(reinterpret_cast<String32 const*>(&(priv::string::g_empty32)));
		}

		/**
		 * @return empty string if this string is null. otherwise returns this string.
		 */
		const String32& getNotNull() const noexcept
		{
			if (!m_container) {
				return *(reinterpret_cast<String32 const*>(&(priv::string::g_empty32)));
			}
			return *this;
		}

		/**
		 * @return `true` if this string is null.
		 */
		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return !m_container;
		}

		/**
		 * @return `true` if this string is not null.
		 */
		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return m_container != sl_null;
		}

		/**
		 * @return `true` if this string is empty.
		 */
		sl_bool isEmpty() const noexcept
		{
			return !(m_container && m_container->len);
		}

		/**
		 * @return `true` if this string is not empty.
		 */
		sl_bool isNotEmpty() const noexcept
		{
			return m_container && m_container->len;
		}

		/**
		 * Sets this string as a null.
		 */
		void setNull() noexcept;

		/**
		 * Sets this string as an empty.
		 */
		void setEmpty() noexcept;

	public:
		/**
		 * @return string content.
		 */
		sl_char32* getData() const noexcept
		{
			if (m_container) {
				return m_container->data;
			} else {
				return (sl_char32*)((void*)(U""));
			}
		}

		/**
		 * @return string content and length.
		 */
		sl_char32* getData(sl_size& outLength) const noexcept
		{
			if (m_container) {
				outLength = m_container->len;
				return m_container->data;
			} else {
				outLength = 0;
				return (sl_char32*)((void*)(U""));
			}
		}

		sl_char32* getNullTerminatedData(sl_size& outLength, String32& outStringConverted) const noexcept;

		/**
		 * @return string length.
		 */
		SLIB_CONSTEXPR sl_size getLength() const
		{
			return m_container ? m_container->len : 0;
		}

		/**
		 * @return the hash code.
		 */
		sl_size getHashCode() const noexcept;
		static sl_size getHashCode(const sl_char32* str, sl_reg len = -1) noexcept;

		/**
		 * @return the hash code ignoring the case.
		 */
		sl_size getHashCode_IgnoreCase() const noexcept;
		static sl_size getHashCode_IgnoreCase(const sl_char32* str, sl_reg len = -1) noexcept;

		/**
		 * Sets the string length.
		 *
		 * Don't use for null or empty string
		 */
		void setLength(sl_size len) noexcept;

		/**
		 * Sets the hash code.
		 *
		 * Don't use for null or empty string
		 */
		void setHashCode(sl_size hash) noexcept;

		/**
		 * @return the first character
		 */
		sl_char32 getFirst() const noexcept;

		/**
		 * @return the last character
		 */
		sl_char32 getLast() const noexcept;

		/**
		 * @return the character at `index` in string.
		 */
		sl_char32 getAt(sl_reg index) const noexcept;

		/**
		 * Sets the character at `index` in string.
		 * @return `true` on success.
		 */
		sl_bool setAt(sl_reg index, sl_char32 ch) noexcept;

		sl_char32 operator[](sl_size index) const noexcept;

		sl_char32& operator[](sl_size index) noexcept;

		explicit operator sl_bool() const noexcept
		{
			return isNotEmpty();
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Convert this string to std::u32string.
		 */
		std::u32string toStd() const noexcept;
#endif

	public:
		/**
		 * String assignment
		 */
		String32& operator=(String32&& other) noexcept;
		String32& operator=(const String32& other) noexcept;
		String32& operator=(AtomicString32&& other) noexcept;
		String32& operator=(const AtomicString32& other) noexcept;
		String32& operator=(const StringView32& other) noexcept;
		String32& operator=(sl_null_t) noexcept;
		String32& operator=(const sl_char32* sz) noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		String32& operator=(const std::u32string& other) noexcept;
		String32& operator=(std::u32string&& other) noexcept;
#endif

	public:
		PRIV_SLIB_DECLARE_STRING_OPS(String32)

	public:
		/**
		 * @return true if this string is equal to the specified string.
		 */
		sl_bool equals(const String32& other) const noexcept;
		sl_bool equals(const StringView32& other) const noexcept;
		sl_bool equals(const sl_char32* sz) const noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool equals(const std::u32string& other) const noexcept;
#endif
		sl_bool equals_IgnoreCase(const StringView32& other) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView32& other) const noexcept;
		sl_compare_result compare_IgnoreCase(const StringView32& other) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * This functions stops searching on the index of `len-1` and returns 0.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView32& other, sl_size len) const noexcept;
		sl_compare_result compare_IgnoreCase(const StringView32& other, sl_size len) const noexcept;

	public:
		/**
		 * @return duplicated string.
		 */
		String32 duplicate() const noexcept;

		/**
		* @return null terminated string.
		*/
		String32 toNullTerminated() const noexcept;

		/**
		 * @return memory containing string content.
		 */
		Memory toMemory() const noexcept;

		/**
		 * Fills Utf8 characters to the provided buffer
		 */
		sl_size getUtf8(sl_char8* utf8, sl_size len) const noexcept;

		/**
		 * Fills Utf8 characters to the provided buffer
		 */
		sl_bool getUtf8(StringStorage& output) const noexcept;

		/**
		 * Converts to Utf8 and Returns a Memory containing the Utf8 characters and null at last
		 */
		Memory toUtf8() const noexcept;

		/**
		 * Encodes using `charset` and Returns Memory containing the encoded bytes
		 */
		Memory encode(Charset charset) const;

		/**
		 * @return a substring of this string.
		 */
		String32 substring(sl_reg start, sl_reg end = -1) const noexcept;

		/**
		 * @return a string containing a specified number of characters from the left side of this string.
		 */
		String32 left(sl_reg len) const noexcept;

		/**
		 * @return a string containing a specified number of characters from the right side of this string.
		 */
		String32 right(sl_reg len) const noexcept;

		/**
		 * @return a string that contains a specified number of characters starting from a specified position in this string.
		 */
		String32 mid(sl_reg start, sl_reg len) const noexcept;

		/**
		 * @return the index within this string of the first occurrence of the specified character, starting the search at `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg indexOf(sl_char32 ch, sl_reg start = 0) const noexcept;
		sl_reg indexOf(const StringView32& str, sl_reg start = 0) const noexcept;
		sl_reg indexOf_IgnoreCase(const StringView32& str, sl_reg start = 0) const noexcept;

		/**
		 * @return the index within this string of the last occurrence of the specified character, searching backwards from `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg lastIndexOf(sl_char32 ch, sl_reg start = -1) const noexcept;
		sl_reg lastIndexOf(const StringView32& str, sl_reg start = -1) const noexcept;
		sl_reg lastIndexOf_IgnoreCase(const StringView32& str, sl_reg start = -1) const noexcept;

		/**
		 * @return `true` if this string starts with the specified character.
		 */
		sl_bool startsWith(sl_char32 ch) const noexcept;
		sl_bool startsWith(const StringView32& str) const noexcept;
		sl_bool startsWith_IgnoreCase(const StringView32& str) const noexcept;

		/**
		 * @return `true` if this string ends with the specified character.
		 */
		sl_bool endsWith(sl_char32 ch) const noexcept;
		sl_bool endsWith(const StringView32& str) const noexcept;
		sl_bool endsWith_IgnoreCase(const StringView32& str) const noexcept;

		/**
		 * @return `true` if the specified character occurs within this string.
		 */
		sl_bool contains(sl_char32 ch) const noexcept;
		sl_bool contains(const StringView32& str) const noexcept;
		sl_bool contains_IgnoreCase(const StringView32& str) const noexcept;

		/**
		* @return the total count of the specified character occurs within this string.
		*/
		sl_size countOf(sl_char32 ch) const noexcept;
		sl_size countOf(const StringView32& str) const noexcept;
		sl_size countOf_IgnoreCase(const StringView32& str) const noexcept;

		/**
		 * Converts the characters of this string to uppercase.
		 */
		void makeUpper() noexcept;

		/**
		 * Converts the characters of this string to lowercase.
		 */
		void makeLower() noexcept;

		/**
		 * @return a copy of this string converted to uppercase.
		 */
		String32 toUpper() const noexcept;

		/**
		 * @return a copy of this string converted to lowercase.
		 */
		String32 toLower() const noexcept;

		/**
		* Replaces each character of this string that matches the given `pattern` with the given `replacement`. if `replacement` is given as zero, then the matched chracters will be removed.
		*/
		String32 replaceAll(const StringView32& pattern, const StringView32& replacement) const noexcept;
		String32 replaceAll(sl_char32 pattern, sl_char32 replacement) const noexcept;

		/**
		* Removes all characters that matches the given `pattern`
		*/
		String32 removeAll(const StringView32& pattern) const noexcept;
		String32 removeAll(sl_char32 pattern) const noexcept;

		/**
		 * Copy this string and then removes whitespaces from both ends of the new string.
		 */
		String32 trim() const noexcept;

		/**
		 * Copy this string and then removes whitespaces from the left of the new string.
		 */
		String32 trimLeft() const noexcept;

		/**
		 * Copy this string and then removes whitespaces from the right of the new string.
		 */
		String32 trimRight() const noexcept;

		/**
		* Copy this string and then removes CR/LF from both ends of the new string.
		*/
		String32 trimLine() const noexcept;

		/**
		* Reverse the characters on this string
		*/
		void makeReverse() noexcept;

		/**
		* Returns reversed copy of this string
		*/
		String32 reverse() const noexcept;

		/**
		 * Splits this string into the list of strings by the `pattern` separator.
		 */
		List<String32> split(const StringView32& pattern, sl_reg nMaxSplit = -1) const noexcept;
		List<String32> split(sl_char32 pattern, sl_reg nMaxSplit = -1) const noexcept;

		/**
		 * Join all strings in the list
		 */
		static String32 join(const String32* strings, sl_size count, const StringView32& delimiter) noexcept;
		static String32 join(const String32* strings, sl_size count) noexcept;
		static String32 join(const StringView32* strings, sl_size count, const StringView32& delimiter) noexcept;
		static String32 join(const StringView32* strings, sl_size count) noexcept;
		static String32 join(const StringParam* strings, sl_size count, const StringView32& delimiter) noexcept;
		static String32 join(const StringParam* strings, sl_size count) noexcept;
		static String32 join(const ListParam<String32>& list, const StringView32& delimiter) noexcept;
		static String32 join(const ListParam<String32>& list) noexcept;
		static String32 join(const ListParam<StringView32>& list, const StringView32& delimiter) noexcept;
		static String32 join(const ListParam<StringView32>& list) noexcept;
		static String32 join(const ListParam<StringParam>& list, const StringView32& delimiter) noexcept;
		static String32 join(const ListParam<StringParam>& list) noexcept;

		/**
		 * Concatenates strings
		 */
		template <class... ARGS>
		static String32 concat(const StringParam& s1, const StringParam& s2, ARGS&&... args) noexcept;
		static String32 concat(const StringParam& s1, const StringParam& s2) noexcept;

	public:
		/**
		* Convert the string to integer of the specified radix.
		*
		* @param[in] radix Numerical base used to represent the integer.  Use 10 for the decimal system. Use 0 to let the function determine the radix by the prefix (`0x`: Hexadecimal, `0`: Octal)
		* @param[out] value Pointer to the result output
		* @param[in] str String containing the integer
		* @param[in] posBegin Zero-based position of the integer
		* @param[in] posEnd Maximum length of the input string. Parser will stop at this position or at null found before this position.
		*
		* @return the position after the integer if a valid integer is found at position of `posBegin`, otherwise returns `SLIB_PARSE_ERROR`.
		*/
		static sl_reg parseInt32(sl_int32 radix, sl_int32* value, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseInt64(sl_int32 radix, sl_int64* value, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseUint32(sl_int32 radix, sl_uint32* value, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseUint64(sl_int32 radix, sl_uint64* value, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

		/**
		* Convert this string to integer of the specified radix.
		*
		* @param[in] radix Numerical base used to represent the integer.  Use 10 for the decimal system. Use 0 to let the function determine the radix by the prefix (`0x`: Hexadecimal, `0`: Octal)
		* @param[out] value Pointer to the result output
		*
		* @return `true` if this string is valid integer
		*/
		sl_bool parseInt32(sl_int32 radix, sl_int32* value) const noexcept;
		sl_bool parseInt32(sl_int32* value) const noexcept;
		sl_bool parseInt64(sl_int32 radix, sl_int64* value) const noexcept;
		sl_bool parseInt64(sl_int64* value) const noexcept;
		sl_bool parseUint32(sl_int32 radix, sl_uint32* value) const noexcept;
		sl_bool parseUint32(sl_uint32* value) const noexcept;
		sl_bool parseUint64(sl_int32 radix, sl_uint64* value) const noexcept;
		sl_bool parseUint64(sl_uint64* value) const noexcept;
		sl_bool parseInt(sl_int32 radix, sl_reg* value) const noexcept;
		sl_bool parseInt(sl_reg* value) const noexcept;
		sl_bool parseSize(sl_int32 radix, sl_size* value) const noexcept;
		sl_bool parseSize(sl_size* value) const noexcept;

		/**
		* Convert this string to integer of the specified radix.
		*
		* @param[in] radix Numerical base used to represent the integer.  Use 10 for the decimal system. Use 0 to let the function determine the radix by the prefix (`0x`: Hexadecimal, `0`: Octal)
		* @param[in] def Default return value for the non-integer string
		*
		* @return Result integer if the conversion is successful, otherwise returns `def`
		*/
		sl_int32 parseInt32(sl_int32 radix = 10, sl_int32 def = 0) const noexcept;
		sl_int64 parseInt64(sl_int32 radix = 10, sl_int64 def = 0) const noexcept;
		sl_reg parseInt(sl_int32 radix = 10, sl_reg def = 0) const noexcept;
		sl_uint32 parseUint32(sl_int32 radix = 10, sl_uint32 def = 0) const noexcept;
		sl_uint64 parseUint64(sl_int32 radix = 10, sl_uint64 def = 0) const noexcept;
		sl_size parseSize(sl_int32 radix = 10, sl_size def = 0) const noexcept;

		/**
		* Convert the string to a float number value
		*
		* @param[out] value Pointer to the result output
		* @param[in] str String to convert
		* @param[in] posBegin Zero-based position of the number
		* @param[in] posEnd Maximum length of the input string. Parser will stop at this position or at null found before this position.
		*
		* @return the position after the number if a valid number is found at position of `posBegin`, otherwise returns `SLIB_PARSE_ERROR`.
		*/
		static sl_reg parseFloat(float* value, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseDouble(double* value, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

		/**
		* Convert this string to a float number value.
		*
		* @param[out] value Pointer to the result output
		*
		* @return `true` if the conversion is success
		*/
		sl_bool parseFloat(float* value) const noexcept;
		sl_bool parseDouble(double* value) const noexcept;

		/**
		* Convert this string to a float number value.
		*
		* @param[in] def Default return value on failure
		*
		* @return Result value if the conversion is successful, otherwise returns `def`
		*/
		float parseFloat(float def = 0) const noexcept;
		double parseDouble(double def = 0) const noexcept;

		/**
		* Convert the string (`str`) to a boolean value.
		* "yes", "YES", "Yes", "true", "TRUE" and "True" are converted to `true`.
		* "no", "NO", "No", "false", "FALSE" and "False" are converted to `false`.
		*
		* @param[out] value Pointer to the result output
		* @param[in] str String to convert
		* @param[in] posBegin Zero-based position of the boolean keyword
		* @param[in] posEnd Maximum length of the input string. Parser will stop at this position or at null found before this position.
		*
		* @return the position after the boolean keyword if a valid keyword is found at position of `posBegin`, otherwise returns `SLIB_PARSE_ERROR`.
		*/
		static sl_reg parseBoolean(sl_bool* value, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

		/**
		* Convert this string to a boolean value.
		* "yes", "YES", "Yes", "true", "TRUE" and "True" are converted to `true`.
		* "no", "NO", "No", "false", "FALSE" and "False" are converted to `false`.
		*
		* @param[out] value Pointer to the result output
		*
		* @return `true` if the conversion is success
		*/
		sl_bool parseBoolean(sl_bool* value) const noexcept;

		/**
		* Convert this string to a boolean value.
		* "yes", "YES", "Yes", "true", "TRUE" and "True" are converted to `true`.
		* "no", "NO", "No", "false", "FALSE" and "False" are converted to `false`.
		*
		* @param[in] def Default return value on failure
		*
		* @return Result value if the conversion is successful, otherwise returns `def`
		*/
		sl_bool parseBoolean(sl_bool def = sl_false) const noexcept;

		/**
		* Parses the hex string and writes the bytes to `output`. Format example, "a1a1a1a1" is converted to 4 bytes of 0xA1.
		*
		* @param[out] output Pointer to the output buffer.
		* @param[in] str The string containing the hex string.
		* @param[in] posBegin The zero-based index of the first character in this string object.
		* @param[in] posEnd Maximum length of the input string. Parser will stop at this position or at null found before this position.
		*
		* @return the position after the boolean keyword if a valid keyword is found at position of `posBegin`, otherwise returns `SLIB_PARSE_ERROR`.
		*/
		static sl_reg parseHexString(void* output, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

		/**
		* Parses this hex string and writes the bytes to `output`. Format example, "a1a1a1a1" is converted to 4 bytes of 0xA1.
		*
		* @param[out] output Pointer to the output buffer.
		*
		* @return `true` if the conversion is success
		*/
		sl_bool parseHexString(void* output) const noexcept;

		/**
		* Parses this hex string and returns hex data. Format example, "a1a1a1a1" is converted to 4 bytes of 0xA1.
		*
		* @return parsed hex data
		*/
		Memory parseHexString() const noexcept;

		/**
		* @return the string representation of integer argument.
		*
		* @param value The integer to be parsed.
		* @param radix This would be used to convert integer into String. radix:2 ~ 64.
		* @param minWidth Minimum width of the result.
		* @param flagUpperCase if flagUpperCase is true, converts string to an uppercase string. flagUpperCase only works if radix <=36 (0~9, a~z)
		*/
		static String32 fromInt32(sl_int32 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String32 fromUint32(sl_uint32 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String32 fromInt64(sl_int64 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String32 fromUint64(sl_uint64 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String32 fromInt(sl_reg value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String32 fromSize(sl_size value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;

		/**
		* @return the string representation of the float argument.
		*
		* @param value The float value to be parsed.
		* @param precision The number of characters in decimal. Negative values is ignored and this parameter has not effect.
		* @param flagZeroPadding If flagZeroPadding is true, zeros are used to pad the field instead of space characters.
		* @param minWidthIntegral The minimum number of characters in integral field.
		*/
		static String32 fromFloat(float value, sl_int32 precision = -1, sl_bool flagZeroPadding = sl_false, sl_uint32 minWidthIntegral = 1) noexcept;
		static String32 fromDouble(double value, sl_int32 precision = -1, sl_bool flagZeroPadding = sl_false, sl_uint32 minWidthIntegral = 1) noexcept;

		/**
		 * @return the string representation of the memory address.
		 *
		 * @param pointer The memory address to be parsed.
		 */
		static String32 fromPointerValue(const void* pointer) noexcept;

		/**
		 * @return the string representation of the boolean argument.
		 *
		 * @param value The boolean value to be parsed.
		 */
		static String32 fromBoolean(sl_bool value) noexcept;

		/**
		 * @return the converted hex string from the buffer.
		 *
		 * @param data The buffer to be converted.
		 * @param size Size of the buffer.
		 * @param flagUseLowerChar uses a-f (`true`) or A-F (`false`) for encoding
		 */
		static String32 makeHexString(const void* data, sl_size size, sl_bool flagUseLowerChar = sl_true) noexcept;

		/**
		 * @return the converted hex string from the buffer.
		 *
		 * @param mem The buffer to be converted.
		 * @param flagUseLowerChar uses a-f (`true`) or A-F (`false`) for encoding
		 */
		static String32 makeHexString(const MemoryView& mem, sl_bool flagUseLowerChar = sl_true) noexcept;

		/**
		 * Returns the formatted string from the format string and arbitrary list of arguments.
		 *
		 * String formatting is smiliar with Java Formatter
		 *
		 * https://docs.oracle.com/javase/7/docs/api/java/util/Formatter.html
		 *
		 * %[argument_index$][flags][width][.precision]conversion
		 *
		 * @param strFormat The buffer containing the format string, this supports the conversion specifiers, length modifiers, and flags.
		 *
		 */
		static String32 format(const StringView32& strFormat) noexcept;
		template <class... ARGS>
		static String32 format(const StringView32& strFormat, ARGS&&... args) noexcept;
		static String32 formatBy(const StringView32& strFormat, const Variant* params, sl_size nParams) noexcept;
		static String32 formatBy(const StringView32& strFormat, const ListParam<Variant>& params) noexcept;
		template <class... ARGS>
		static String32 format(const Locale& locale, const StringView32& strFormat, ARGS&&... args) noexcept;
		static String32 formatBy(const Locale& locale, const StringView32& strFormat, const Variant* params, sl_size nParams) noexcept;
		static String32 formatBy(const Locale& locale, const StringView32& strFormat, const ListParam<Variant>& params) noexcept;

	private:
		void _replaceContainer(Container* container) noexcept;

	public:
		friend class Atomic<String32>;

	};


	template <>
	class SLIB_EXPORT Atomic<String32>
	{
	public:
		typedef StringContainer32 Container;
		typedef sl_char32 Char;
		typedef StringView32 StringViewType;
#ifdef SLIB_SUPPORT_STD_TYPES
		typedef std::u32string StdString;
#endif

	private:
		Container* m_container;
		SpinLock m_lock;

	public:
		/**
		 * Initialize as a null string.
		 */
		SLIB_CONSTEXPR Atomic(): m_container(sl_null) {}
		SLIB_CONSTEXPR Atomic(sl_null_t): m_container(sl_null) {}

		/**
		 * Constructors
		 */
		Atomic(String32&& src) noexcept
		{
			m_container = src.m_container;
			src.m_container = sl_null;
		}

		Atomic(const String32& src) noexcept;
		Atomic(Atomic&& src) noexcept;
		Atomic(const Atomic& src) noexcept;
		Atomic(const StringView32& src) noexcept;

		/**
		 * Destructor
		 */
		~Atomic();

	public:
		/**
		 * Fill the string with `nRepeatCount` consecutive copies of charactor `ch`
		 */
		Atomic(sl_char32 ch, sl_size nRepeatCount) noexcept;

		/**
		 * Copies the null-terminated character sequence pointed by `str`.
		 */
		Atomic(const sl_char32* str) noexcept;

		/**
		 * Copies the first `length` characters from the array of characters pointed by `str`
		 */
		Atomic(const sl_char32* str, sl_reg length) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Initialize from `std::u32string`.
		 * This does not copy the data of the string, but keep the reference to the original string.
		 */
		Atomic(const std::u32string& str) noexcept;
		Atomic(std::u32string&& str) noexcept;
#endif

	public:
		/**
		 * @return `true` if this string is null.
		 */
		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return !m_container;
		}

		/**
		 * @return `true` if this string is not null.
		 */
		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return m_container != sl_null;
		}

		/**
		 * Sets this string as a null.
		 */
		void setNull() noexcept;

		/**
		 * Sets this string as an empty.
		 */
		void setEmpty() noexcept;

		/**
		 * make this string as a null and returns the original string.
		 */
		String32 release() noexcept;

	public:
		/**
		 * String assignment
		 */
		Atomic& operator=(String32&& other) noexcept;
		Atomic& operator=(const String32& other) noexcept;
		Atomic& operator=(Atomic&& other) noexcept;
		Atomic& operator=(const Atomic& other) noexcept;
		Atomic& operator=(const StringView32& other) noexcept;
		Atomic& operator=(sl_null_t) noexcept;
		Atomic& operator=(const sl_char32* sz) noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic& operator=(const std::u32string& other) noexcept;
		Atomic& operator=(std::u32string&& other) noexcept;
#endif

	private:
		Container* _retainContainer() const noexcept;
		Container* _releaseContainer() noexcept;
		void _replaceContainer(Container* other) noexcept;

		friend class String32;
	};

}

/// @}

#endif
