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

#ifndef CHECKHEADER_SLIB_CORE_STRING16
#define CHECKHEADER_SLIB_CORE_STRING16

/**
 * @addtogroup core
 *  @{
 */

namespace slib
{

	class SLIB_EXPORT StringContainer16
	{
	public:
		typedef String16 StringType;

	public:
		sl_char16* sz;
		sl_size len;
		sl_size hash;
		sl_uint32 type;
		volatile sl_reg ref;
		
	public:
		sl_reg increaseReference() noexcept;
		
		sl_reg decreaseReference() noexcept;
		
	};
	
	/**
	 * @class String16
	 * @brief String16 class provides an extensive set of APIs for working with strings, including method for comparing, searching, and modifying strings.
	 */
	class SLIB_EXPORT String16
	{
	public:
		typedef StringContainer16 Container;
		typedef sl_char16 Char;
		typedef StringView16 StringViewType;
#ifdef SLIB_SUPPORT_STD_TYPES
		typedef std::u16string StdString;
#endif

	private:
		Container* m_container;
		
	public:
		/**
		 * Initializes as a null string.
		 */
		SLIB_CONSTEXPR String16(): m_container(sl_null) {}
		SLIB_CONSTEXPR String16(sl_null_t): m_container(sl_null) {}
		SLIB_CONSTEXPR String16(Container* container) : m_container(container) {}

		/**
		 * Constructors
		 */
		String16(String16&& src) noexcept
		{
			m_container = src.m_container;
			src.m_container = sl_null;
		}

		String16(const String16& src) noexcept;
		String16(const AtomicString16& src) noexcept;
		String16(const StringView16& src) noexcept;

		/**
		 * Destructor
		 */
		~String16();
		
	public:
		/**
		 * Fill the string with `nRepeatCount` consecutive copies of charactor `ch`
		 */
		String16(sl_char16 ch, sl_size nRepeatCount) noexcept;
		
		/**
         * Copies the null-terminated character sequence pointed by `str`.
		 */
		String16(const sl_char16* str) noexcept;

		/**
		 * Copies the first `length` characters from the array of characters pointed by `str`
		 */
		String16(const sl_char16* str, sl_reg length) noexcept;
		
#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Initialize from `std::u16string`.
		 * This does not copy the data of the string, but keep the reference to the original string.
		 */
		String16(const std::u16string& str) noexcept;
		String16(std::u16string&& str) noexcept;
#endif

	public:
		/**
		 * Create a string of `len` characters.
		 */
		static String16 allocate(sl_size len) noexcept;
		
		/**
		 * Creates a string copying other string
		 */
		static String16 create(const String& str) noexcept;
		static String16 create(const String16& str) noexcept;
		static String16 create(const String32& str) noexcept;
		static String16 create(const StringView& str) noexcept;
		static String16 create(const StringView16& str) noexcept;
		static String16 create(const StringView32& str) noexcept;

		/**
		 * Creates a string from the array of characters pointed by `str`
		 */
		static String16 create(const char* str, sl_reg length = -1) noexcept;
		static String16 create(const wchar_t* str, sl_reg length = -1) noexcept;
		static String16 create(const char16_t* str, sl_reg length = -1) noexcept;
		static String16 create(const char32_t* str, sl_reg length = -1) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Create a string copying from std strings.
		 */
		static String16 create(const std::u16string& str) noexcept;
		static String16 create(const std::string& str) noexcept;
		static String16 create(const std::wstring& str) noexcept;
		static String16 create(const std::u32string& str) noexcept;
#endif

		/**
		 * Creates a string pointing the `str` as the content, without copying the data.
		 * `str` should not be freed while the returned string is being used.
		 */
		template <sl_size N> static String16 fromStatic(const sl_char16 (&str)[N]) noexcept
		{
			return fromStatic(str, N - 1);
		}

		static String16 fromStatic(const sl_char16* str, sl_reg len) noexcept;
		
		/**
		 * Creates a string pointing the `str` as the content, without copying the data.
		 * `ref` should be used to keep the alive of the string content.
		 */
		static String16 fromRef(Referable* ref, const sl_char16* str, sl_size len) noexcept;
		
		/**
		 * Creates a string pointing the `mem` as the UTF-16 content, without copying the data.
		 */
		static String16 fromMemory(const Memory& mem) noexcept;

		/**
		 * Creates a string copying the characters from the UTF-8 text.
		 *
		 * @param[in] text string buffer containing the UTF-8 text
		 * @param[in] len length of the `text`. negative value means that `text` is null terminated.
		 */
		static String16 fromUtf8(const void* text, sl_reg len = -1) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-8 text in `mem`.
		 */
		static String16 fromUtf8(const Memory& mem) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-16 text.
		 *
		 * @param[in] text string buffer containing the UTF-16 text
		 * @param[in] len length of the `text`. negative value means that `text` is null terminated.
		 */
		static String16 fromUtf16(const sl_char16* text, sl_reg len = -1) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-16 Big Endian text.
		 *
		 * @param[in] text string buffer containing the UTF-16 Big Endian text
		 * @param[in] size size (in bytes) of the `text`
		 */
		static String16 fromUtf16BE(const void* text, sl_size size) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-16 Big Endian text in `mem`.
		 */
		static String16 fromUtf16BE(const Memory& mem) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-16 Little Endian text.
		 *
		 * @param[in] text string buffer containing the UTF-16 Little Endian text
		 * @param[in] size size (in bytes) of the `text`
		 */
		static String16 fromUtf16LE(const void* text, sl_size size) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-16 Little Endian text in `mem`.
		 */
		static String16 fromUtf16LE(const Memory& mem) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-32 text.
		 *
		 * @param[in] text string buffer containing the UTF-32 text
		 * @param[in] len length of the `text`. negative value means that `text` is null terminated.
		 */
		static String16 fromUtf32(const sl_char32* text, sl_reg len = -1) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-8, UTF-16BE, or UTF-16LE text.
		 * This function detects the encoding type from the first 3 bytes of `text`.
		 *
		 * @param[in] text string buffer containing the unicode text.
		 * @param[in] size size (in bytes) of the `text`
		 */
		static String16 fromUtf(const void* text, sl_size size) noexcept;
		
		/**
		 * Creates a string copying the characters from the UTF-8, UTF-16BE, or UTF-16LE text in `mem`.
		 * This function detects the encoding type from the first 3 bytes of the text.
		 */
		static String16 fromUtf(const Memory& mem) noexcept;
		
		/**
		 * Creates a string copying the characters from `text` encoded by `charset`.
		 */
		static String16 decode(Charset charset, const void* text, sl_size size);
		
		/**
		 * Creates a string copying the characters from text in `mem`, encoded by `charset`.
		 */
		static String16 decode(Charset charset, const Memory& mem);


		static const String16& from(const String16& str) noexcept
		{
			return str;
		}

		static String16&& from(String16&& str) noexcept
		{
			return Move(str);
		}

		static String16 from(const String& str) noexcept;
		static String16 from(const AtomicString& str) noexcept;
		static String16 from(const AtomicString16& str) noexcept;
		static String16 from(const String32& str) noexcept;
		static String16 from(const AtomicString32& str) noexcept;
		static String16 from(const StringView& str) noexcept;
		static String16 from(const StringView16& str) noexcept;
		static String16 from(const StringView32& str) noexcept;
		static String16 from(const StringParam& str) noexcept;
		static String16 from(const char* str, sl_reg length = -1) noexcept;
		static String16 from(const wchar_t* str, sl_reg length = -1) noexcept;
		static String16 from(const char16_t* str, sl_reg length = -1) noexcept;
		static String16 from(const char32_t* str, sl_reg length = -1) noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		static String16 from(const std::string& str) noexcept;
		static String16 from(const std::wstring& str) noexcept;
		static String16 from(const std::u16string& str) noexcept;
		static String16 from(const std::u32string& str) noexcept;
#endif
		static String16 from(signed char value) noexcept;
		static String16 from(unsigned char value) noexcept;
		static String16 from(short value) noexcept;
		static String16 from(unsigned short value) noexcept;
		static String16 from(int value) noexcept;
		static String16 from(unsigned int value) noexcept;
		static String16 from(long value) noexcept;
		static String16 from(unsigned long value) noexcept;
		static String16 from(sl_int64 value) noexcept;
		static String16 from(sl_uint64 value) noexcept;
		static String16 from(float value) noexcept;
		static String16 from(double value) noexcept;
		static String16 from(sl_bool value) noexcept;
		static String16 from(const Time& value) noexcept;
		static String16 from(const Json& json) noexcept;
		static String16 from(const Variant& var) noexcept;
		
	public:
		/**
		 * @return null string.
		 */
		static const String16& null() noexcept
		{
			return *(reinterpret_cast<String16 const*>(&(priv::string::g_null16)));
		}
		
		/**
		 * @return empty string.
		 */
		static const String16& getEmpty() noexcept
		{
			return *(reinterpret_cast<String16 const*>(&(priv::string::g_empty16)));
		}
		
		/**
		 * @return empty string if this string is null. otherwise returns this string.
		 */
		const String16& getNotNull() const noexcept
		{
			if (!m_container) {
				return *(reinterpret_cast<String16 const*>(&(priv::string::g_empty16)));
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
		SLIB_CONSTEXPR sl_bool isEmpty() const
		{
			return !(m_container && m_container->len);
		}
		
		/**
		 * @return `true` if this string is not empty.
		 */
		SLIB_CONSTEXPR sl_bool isNotEmpty() const
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
		sl_char16* getData() const noexcept
		{
			if (m_container) {
				return m_container->sz;
			} else {
				return (sl_char16*)((void*)(u""));
			}
		}
		
		/**
		 * @return string content and length.
		 */
		sl_char16* getData(sl_size& outLength) const noexcept
		{
			if (m_container) {
				outLength = m_container->len;
				return m_container->sz;
			} else {
				outLength = 0;
				return (sl_char16*)((void*)(u""));
			}
		}
		
		sl_char16* getNullTerminatedData(sl_size& outLength, String16& outStringConverted) const noexcept;

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
		static sl_size getHashCode(const sl_char16* str, sl_reg len = -1) noexcept;

		/**
		 * @return the hash code ignoring the case.
		 */
		sl_size getHashCodeIgnoreCase() const noexcept;
		static sl_size getHashCodeIgnoreCase(const sl_char16* str, sl_reg len = -1) noexcept;

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
		 * @return the character at `index` in string.
		 */
		sl_char16 getAt(sl_reg index) const noexcept;
		
		/**
		 * Sets the character at `index` in string.
		 * @return `true` on success.
		 */
		sl_bool setAt(sl_reg index, sl_char16 ch) noexcept;
		
		sl_char16 operator[](sl_size index) const noexcept;
		
		sl_char16& operator[](sl_size index) noexcept;

		SLIB_CONSTEXPR explicit operator sl_bool() const
		{
			return m_container != sl_null;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Convert this string to std::u16string.
		 */
		std::u16string toStd() const noexcept;
#endif

	public:
		/**
		 * String assignment
		 */
		String16& operator=(String16&& other) noexcept;
		String16& operator=(const String16& other) noexcept;
		String16& operator=(const AtomicString16& other) noexcept;
		String16& operator=(const StringView16& other) noexcept;
		String16& operator=(sl_null_t) noexcept;
		String16& operator=(const sl_char16* sz) noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		String16& operator=(const std::u16string& other) noexcept;
		String16& operator=(std::u16string&& other) noexcept;
#endif
		
		String16& operator+=(String16&& other) noexcept;
		String16& operator+=(const String16& other) noexcept;
		
		template <class T>
		String16& operator+=(const T& other) noexcept
		{
			return *this = *this + other;
		}

	public:
		PRIV_SLIB_DECLARE_STRING_OPS(String16)

	public:
		/**
		 * @return true if this string is equal to the specified string.
		 */
		sl_bool equals(const String16& other) const noexcept;
		sl_bool equals(const StringView16& other) const noexcept;
		sl_bool equals(const sl_char16* sz) const noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool equals(const std::u16string& other) const noexcept;
#endif

		/**
		 * Compares this string to the specified string.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView16& other) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * This functions stops searching on the index of `len-1` and returns 0.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView16& other, sl_size len) const noexcept;
	
		/**
		 * @return true if this string is equal to the specified string ignoring the case.
		 */
		sl_bool equalsIgnoreCase(const StringView16& other) const noexcept;

		/**
		 * Compares this string to the specified string ignoring the case.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compareIgnoreCase(const StringView16& other) const noexcept;
		
	public:
		/**
		 * @return duplicated string.
		 */
		String16 duplicate() const noexcept;

		/**
		* @return null terminated string.
		*/
		String16 toNullTerminated() const noexcept;

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
		String16 substring(sl_reg start, sl_reg end = -1) const noexcept;
		
		/**
		 * @return a string containing a specified number of characters from the left side of this string.
		 */
		String16 left(sl_reg len) const noexcept;
		
		/**
		 * @return a string containing a specified number of characters from the right side of this string.
		 */
		String16 right(sl_reg len) const noexcept;
		
		/**
		 * @return a string that contains a specified number of characters starting from a specified position in this string.
		 */
		String16 mid(sl_reg start, sl_reg len) const noexcept;
		
		/**
		 * @return the index within this string of the first occurrence of the specified character, starting the search at `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg indexOf(sl_char16 ch, sl_reg start = 0) const noexcept;
		
		/**
		 * @return the index within this string of the first occurrence of the specified string, starting the search at `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg indexOf(const StringView16& str, sl_reg start = 0) const noexcept;
		
		/**
		 * @return the index within this string of the last occurrence of the specified character, searching backwards from `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg lastIndexOf(sl_char16 ch, sl_reg start = -1) const noexcept;
		
		/**
		 * @return the index within this string of the last occurrence of the specified string, searching backwards from `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg lastIndexOf(const StringView16& str, sl_reg start = -1) const noexcept;
		
		/**
		 * @return `true` if this string starts with the specified character.
		 */
		sl_bool startsWith(sl_char16 ch) const noexcept;
		
		/**
		 * @return `true` if this string starts with the specified string.
		 */
		sl_bool startsWith(const StringView16& str) const noexcept;
		
		/**
		 * @return `true` if this string ends with the specified character.
		 */
		sl_bool endsWith(sl_char16 ch) const noexcept;
		
		/**
		 * @return `true` if this string ends with the specified string.
		 */
		sl_bool endsWith(const StringView16& str) const noexcept;
		
		/**
		 * @return `true` if the specified character occurs within this string.
		 */
		sl_bool contains(sl_char16 ch) const noexcept;
		
		/**
		 * @return `true` if the specified substring occurs within this string.
		 */
		sl_bool contains(const StringView16& str) const noexcept;

		/**
		* @return the total count of the specified character occurs within this string.
		*/
		sl_size countOf(sl_char16 ch) const noexcept;

		/**
		* @return the total count of the specified substring occurs within this string.
		*/
		sl_size countOf(const StringView16& str) const noexcept;

		/**
		 * Converts the characters of this string to uppercase.
		 */
		void makeUpper() noexcept;
		
		/**
		 * Converts the characters of this string to lowercase.
		 */
		void makeLower() noexcept;
		
		/**
		 * @return a copy of the specified string converted to uppercase.
		 */
		static String16 toUpper(const sl_char16* str, sl_reg len = -1) noexcept;
		
		/**
		 * @return a copy of the specified string converted to lowercase.
		 */
		static String16 toLower(const sl_char16* str, sl_reg len = -1) noexcept;
		
		/**
		 * @return a copy of this string converted to uppercase.
		 */
		String16 toUpper() const noexcept;
		
		/**
		 * @return a copy of this string converted to lowercase.
		 */
		String16 toLower() const noexcept;

		/**
		* Replaces each character of this string that matches the given `pattern` with the given `replacement`. if `replacement` is given as zero, then the matched chracters will be removed.
		*/
		String16 replaceAll(sl_char16 pattern, sl_char16 replacement) const noexcept;

		/**
		 * Replaces each substring of this string that matches the given `pattern` with the given `replacement`.
		 */
		String16 replaceAll(const StringView16& pattern, const StringView16& replacement) const noexcept;

		/**
		* Removes all characters that matches the given `pattern`
		*/
		String16 removeAll(sl_char16 pattern) const noexcept;

		/**
		* Removes all characters that matches the given `pattern`
		*/
		String16 removeAll(const StringView16& pattern) const noexcept;

		/**
		 * Copy this string and then removes whitespaces from both ends of the new string.
		 */
		String16 trim() const noexcept;
		
		/**
		 * Copy this string and then removes whitespaces from the left of the new string.
		 */
		String16 trimLeft() const noexcept;
		
		/**
		 * Copy this string and then removes whitespaces from the right of the new string.
		 */
		String16 trimRight() const noexcept;

		/**
		* Copy this string and then removes CR/LF from both ends of the new string.
		*/
		String16 trimLine() const noexcept;

		/**
		 * Splits this string into the list of strings by the `pattern` separator.
		 */
		List<String16> split(const StringView16& pattern) const noexcept;
		
		/**
		 * Join all strings in the list
		 */
		static String16 join(const String16* strings, sl_size count, const StringView16& delimiter) noexcept;
		static String16 join(const String16* strings, sl_size count) noexcept;
		static String16 join(const StringView16* strings, sl_size count, const StringView16& delimiter) noexcept;
		static String16 join(const StringView16* strings, sl_size count) noexcept;
		static String16 join(const StringParam* strings, sl_size count, const StringView16& delimiter) noexcept;
		static String16 join(const StringParam* strings, sl_size count) noexcept;
		static String16 join(const ListParam<String16>& list, const StringView16& delimiter) noexcept;
		static String16 join(const ListParam<String16>& list) noexcept;
		static String16 join(const ListParam<StringView16>& list, const StringView16& delimiter) noexcept;
		static String16 join(const ListParam<StringView16>& list) noexcept;
		static String16 join(const ListParam<StringParam>& list, const StringView16& delimiter) noexcept;
		static String16 join(const ListParam<StringParam>& list) noexcept;

		/**
		 * Concatenates strings
		 */
		template <class... ARGS>
		static String16 concat(const StringParam& s1, const StringParam& s2, ARGS&&... args) noexcept;
		static String16 concat(const StringParam& s1, const StringParam& s2) noexcept;

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
		static sl_reg parseInt32(sl_int32 radix, sl_int32* value, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseInt64(sl_int32 radix, sl_int64* value, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseUint32(sl_int32 radix, sl_uint32* value, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseUint64(sl_int32 radix, sl_uint64* value, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

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
		static sl_reg parseFloat(float* value, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;
		static sl_reg parseDouble(double* value, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

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
		static sl_reg parseBoolean(sl_bool* value, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

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
		static sl_reg parseHexString(void* output, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

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
		static String16 fromInt32(sl_int32 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String16 fromUint32(sl_uint32 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String16 fromInt64(sl_int64 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String16 fromUint64(sl_uint64 value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String16 fromInt(sl_reg value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;
		static String16 fromSize(sl_size value, sl_uint32 radix = 10, sl_uint32 minWidth = 0, sl_bool flagUpperCase = sl_false) noexcept;

		/**
		* @return the string representation of the float argument.
		*
		* @param value The float value to be parsed.
		* @param precision The number of characters in decimal. Negative values is ignored and this parameter has not effect.
		* @param flagZeroPadding If flagZeroPadding is true, zeros are used to pad the field instead of space characters.
		* @param minWidthIntegral The minimum number of characters in integral field.
		*/
		static String16 fromFloat(float value, sl_int32 precision = -1, sl_bool flagZeroPadding = sl_false, sl_uint32 minWidthIntegral = 1) noexcept;
		static String16 fromDouble(double value, sl_int32 precision = -1, sl_bool flagZeroPadding = sl_false, sl_uint32 minWidthIntegral = 1) noexcept;
		
		/**
		 * @return the string representation of the memory address.
		 *
		 * @param pointer The memory address to be parsed.
		 */
		static String16 fromPointerValue(const void* pointer) noexcept;
		
		/**
		 * @return the string representation of the boolean argument.
		 *
		 * @param value The boolean value to be parsed.
		 */
		static String16 fromBoolean(sl_bool value) noexcept;
		
		/**
		 * @return the converted hex string from the buffer.
		 *
		 * @param data The buffer to be converted.
		 * @param size Size of the buffer.
		 * @param flagUseLowerChar uses a-f (`true`) or A-F (`false`) for encoding
		 */
		static String16 makeHexString(const void* data, sl_size size, sl_bool flagUseLowerChar = sl_true) noexcept;
		
		/**
		 * @return the converted hex string from the buffer.
		 *
		 * @param mem The buffer to be converted.
		 * @param flagUseLowerChar uses a-f (`true`) or A-F (`false`) for encoding
		 */
		static String16 makeHexString(const Memory& mem, sl_bool flagUseLowerChar = sl_true) noexcept;
		
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
		static String16 format(const StringView16& strFormat) noexcept;
		template <class... ARGS>
		static String16 format(const StringView16& strFormat, ARGS&&... args) noexcept;
		static String16 formatBy(const StringView16& strFormat, const Variant* params, sl_size nParams) noexcept;
		static String16 formatBy(const StringView16& strFormat, const ListParam<Variant>& params) noexcept;
		template <class... ARGS>
		static String16 format(const Locale& locale, const StringView16& strFormat, ARGS&&... args) noexcept;
		static String16 formatBy(const Locale& locale, const StringView16& strFormat, const Variant* params, sl_size nParams) noexcept;
		static String16 formatBy(const Locale& locale, const StringView16& strFormat, const ListParam<Variant>& params) noexcept;

	private:
		void _replaceContainer(Container* container) noexcept;
		
	public:
		friend class Atomic<String16>;
		
	};
	
	
	template <>
	class SLIB_EXPORT Atomic<String16>
	{
	public:
		typedef StringContainer16 Container;
		typedef sl_char16 Char;
		typedef StringView16 StringViewType;
#ifdef SLIB_SUPPORT_STD_TYPES
		typedef std::u16string StdString;
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
		Atomic(String16&& src) noexcept
		{
			m_container = src.m_container;
			src.m_container = sl_null;
		}

		Atomic(const String16& src) noexcept;
		Atomic(const AtomicString16& src) noexcept;
		Atomic(const StringView16& src) noexcept;
		
		/**
		 * Destructor
		 */
		~Atomic();
		
	public:
		/**
		 * Fill the string with `nRepeatCount` consecutive copies of charactor `ch`
		 */
		Atomic(sl_char16 ch, sl_size nRepeatCount) noexcept;
		
		/**
		 * Copies the null-terminated character sequence pointed by `str`.
		 */
		Atomic(const sl_char16* str) noexcept;

		/**
		 * Copies the first `length` characters from the array of characters pointed by `str`
		 */
		Atomic(const sl_char16* str, sl_reg length) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		/**
		 * Initialize from `std::u16string`.
		 * This does not copy the data of the string, but keep the reference to the original string.
		 */
		Atomic(const std::u16string& str) noexcept;
		Atomic(std::u16string&& str) noexcept;
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
		
	public:
		/**
		 * String assignment
		 */
		AtomicString16& operator=(String16&& other) noexcept;
		AtomicString16& operator=(const String16& other) noexcept;
		AtomicString16& operator=(const AtomicString16& other) noexcept;
		AtomicString16& operator=(const StringView16& other) noexcept;
		AtomicString16& operator=(sl_null_t) noexcept;
		AtomicString16& operator=(const sl_char16* str) noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		AtomicString16& operator=(const std::u16string& other) noexcept;
		AtomicString16& operator=(std::u16string&& other) noexcept;
#endif
		
	private:
		Container* _retainContainer() const noexcept;
		void _replaceContainer(Container* other) noexcept;

		friend class String16;
	};
	
}

/// @}


#define SLIB_STATIC_STRING16(name, str) \
		auto& _static_string_content_##name = SLIB_UNICODE(str); \
		static slib::StringContainer16 _static_string_container_##name = {(sl_char16*)_static_string_content_##name, (sizeof(_static_string_content_##name)/2)-1, 0, 0, -1}; \
		static slib::StringContainer16* _static_string_##name = &_static_string_container_##name; \
		static const slib::String16& name = *(reinterpret_cast<slib::String16*>(&_static_string_##name));

#define SLIB_RETURN_STRING16(str) { SLIB_STATIC_STRING16(strRetTemp16, str) return strRetTemp16; }

#endif
