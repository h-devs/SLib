/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_STRING_VIEW
#define CHECKHEADER_SLIB_CORE_STRING_VIEW

namespace slib
{

	class SLIB_EXPORT StringView
	{
	public:
		typedef sl_char8 Char;
		typedef String StringType;
#ifdef SLIB_SUPPORT_STD_TYPES
		typedef std::string StdString;
#endif

	public:
		StringView() noexcept: data(sl_null), length(0) {}

		StringView(sl_null_t) noexcept: data(sl_null), length(0) {}

		StringView(const StringView& other) noexcept: data(other.data), length(other.length) {}

	public:
		StringView(const String& value) noexcept;

		StringView(const sl_char8* sz) noexcept;

		StringView(const sl_char8* str, sl_reg length) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		StringView(const std::string& str) noexcept;
#endif

	public:
		StringView& operator=(sl_null_t) noexcept;

		StringView& operator=(const StringView& other) noexcept;

		StringView& operator=(const String& value) noexcept;

		StringView& operator=(const sl_char8* sz) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		StringView& operator=(const std::string& str) noexcept;
#endif

		sl_char8 operator[](sl_size index) const noexcept
		{
			return data[index];
		}

		sl_char8& operator[](sl_size index) noexcept
		{
			return data[index];
		}

		explicit operator sl_bool() const noexcept
		{
			return !(isEmpty());
		}

	public:
		template <sl_size N>
		static StringView literal(const sl_char8 (&s)[N]) noexcept
		{
			return StringView(s, N - 1);
		}

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return !data;
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return data != sl_null;
		}

		sl_bool isEmpty() const noexcept
		{
			if (length) {
				if (length > 0) {
					return sl_false;
				}
				return !(*data);
			}
			return sl_true;
		}

		sl_bool isNotEmpty() const noexcept
		{
			return !(isEmpty());
		}

		SLIB_CONSTEXPR sl_char8* getUnsafeData() const
		{
			return data;
		}

		SLIB_CONSTEXPR sl_reg getUnsafeLength() const
		{
			return length;
		}

		sl_char8* getData() const noexcept
		{
			if (data) {
				return data;
			} else {
				return (sl_char8*)((void*)(""));
			}
		}

		sl_char8* getData(sl_size& outLength) const noexcept;

		sl_size getLength() const noexcept;

		sl_size getHashCode() const noexcept;

		sl_size getHashCodeIgnoreCase() const noexcept;

		static const StringView& null() noexcept;

		static const StringView& getEmpty() noexcept;

		void setNull() noexcept;

	public:
		PRIV_SLIB_DECLARE_STRING_VIEW_OPS(StringView)

	public:
		/**
		 * @return true if this string is equal to the specified string.
		 */
		sl_bool equals(const StringView& other) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * This functions stops searching on the index of `len-1` and returns 0.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView& other, sl_size len) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView& other) const noexcept;

		/**
		 * @return true if this string is equal to the specified string ignoring the case.
		 */
		sl_bool equalsIgnoreCase(const StringView& other) const noexcept;

		/**
		 * Compares this string to the specified string ignoring the case.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compareIgnoreCase(const StringView& other) const noexcept;

		/**
		 * @return a view of the substring
		 */
		StringView substring(sl_reg start, sl_reg end = -1) const noexcept;

		/**
		 * @return a view of the left substring
		 */
		StringView left(sl_reg len) const noexcept;

		/**
		 * @return a view of the right substring
		 */
		StringView right(sl_reg len) const noexcept;

		/**
		 * @return a view of the substring
		 */
		StringView mid(sl_reg start, sl_reg len) const noexcept;

		/**
		 * @return the index within this string of the first occurrence of the specified character, starting the search at `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg indexOf(const StringView& str, sl_reg start = 0) const noexcept;
		sl_reg indexOf(sl_char8 ch, sl_reg start = 0) const noexcept;

		/**
		 * @return the index within this string of the last occurrence of the specified character, searching backwards from `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg lastIndexOf(const StringView& str, sl_reg start = -1) const noexcept;
		sl_reg lastIndexOf(sl_char8 ch, sl_reg start = -1) const noexcept;

		/**
		 * @return `true` if this string starts with the specified character.
		 */
		sl_bool startsWith(const StringView& str) const noexcept;
		sl_bool startsWith(sl_char8 ch) const noexcept;

		/**
		 * @return `true` if this string ends with the specified character.
		 */
		sl_bool endsWith(const StringView& str) const noexcept;
		sl_bool endsWith(sl_char8 ch) const noexcept;

		/**
		 * @return `true` if the specified character occurs within this string.
		 */
		sl_bool contains(const StringView& str) const noexcept;
		sl_bool contains(sl_char8 ch) const noexcept;

		/**
		* @return the total count of the specified character occurs within this string.
		*/
		sl_size countOf(const StringView& str) const noexcept;
		sl_size countOf(sl_char8 ch) const noexcept;

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
		String toUpper() const noexcept;

		/**
		 * @return a copy of this string converted to lowercase.
		 */
		String toLower() const noexcept;

		/**
		 * Replaces each character of this string that matches the given `pattern` with the given `replacement`. if `replacement` is given as zero, then the matched chracters will be removed.
		 */
		String replaceAll(const StringView& pattern, const StringView& replacement) const noexcept;
		String replaceAll(sl_char8 pattern, sl_char8 replacement) const noexcept;

		/**
		* Removes all characters that matches the given `pattern`
		*/
		String removeAll(const StringView& pattern) const noexcept;
		String removeAll(sl_char8 pattern) const noexcept;

		/**
		 * a view of the substring that removed whitespaces from both ends of the new string.
		 */
		StringView trim() const noexcept;

		/**
		 * a view of the substring that removed whitespaces from the left of the new string.
		 */
		StringView trimLeft() const noexcept;

		/**
		 * a view of the substring that removed whitespaces from the right of the new string.
		 */
		StringView trimRight() const noexcept;

		/**
		* Copy this string and then removes CR/LF from both ends of the new string.
		*/
		StringView trimLine() const noexcept;

		/**
		* Reverse the characters on this string
		*/
		void makeReverse() noexcept;

		/**
		* Returns reversed copy of this string
		*/
		String reverse() const noexcept;

		/**
		 * Splits this string into the list of strings by the `pattern` separator.
		 */
		List<StringView> split(const StringView& pattern, sl_reg nMaxSplit = -1) const noexcept;
		List<StringView> split(sl_char8 pattern, sl_reg nMaxSplit = -1) const noexcept;

		/**
		 * Convert this string to integer of the specified radix.
		 *
		 * @param[in] radix Numerical base used to represent the integer. Use 10 for the decimal system. Use 0 to let the function determine the radix by the prefix (`0x`: Hexadecimal, `0`: Octal)
		 * @param[out] value Pointer to the result output
		 *
		 * @return `true` if this string is valid integer
		 */
		sl_bool parseInt32(sl_int32 radix, sl_int32* value) const noexcept;
		sl_bool parseInt32(sl_int32* value) const noexcept;
		sl_bool parseUint32(sl_int32 radix, sl_uint32* value) const noexcept;
		sl_bool parseUint32(sl_uint32* value) const noexcept;
		sl_bool parseInt64(sl_int32 radix, sl_int64* value) const noexcept;
		sl_bool parseInt64(sl_int64* value) const noexcept;
		sl_bool parseUint64(sl_int32 radix, sl_uint64* value) const noexcept;
		sl_bool parseUint64(sl_uint64* value) const noexcept;
		sl_bool parseInt(sl_int32 radix, sl_reg* value) const noexcept;
		sl_bool parseInt(sl_reg* value) const noexcept;
		sl_bool parseSize(sl_int32 radix, sl_size* value) const noexcept;
		sl_bool parseSize(sl_size* value) const noexcept;

		/**
		 * Convert this string to integer of the specified radix.
		 *
		 * @param[in] radix Numerical base used to represent the integer. Use 10 for the decimal system. Use 0 to let the function determine the radix by the prefix (`0x`: Hexadecimal, `0`: Octal)
		 * @param[in] def Default return value for the non-integer string
		 *
		 * @return Result integer if the conversion is successful, otherwise returns `def`
		 */
		sl_int32 parseInt32(sl_int32 radix = 10, sl_int32 def = 0) const noexcept;
		sl_uint32 parseUint32(sl_int32 radix = 10, sl_uint32 def = 0) const noexcept;
		sl_int64 parseInt64(sl_int32 radix = 10, sl_int64 def = 0) const noexcept;
		sl_uint64 parseUint64(sl_int32 radix = 10, sl_uint64 def = 0) const noexcept;
		sl_reg parseInt(sl_int32 radix = 10, sl_reg def = 0) const noexcept;
		sl_size parseSize(sl_int32 radix = 10, sl_size def = 0) const noexcept;

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

	protected:
		sl_char8* data;
		mutable sl_reg length;

	};


	class SLIB_EXPORT StringView16
	{
	public:
		typedef sl_char16 Char;
		typedef String16 StringType;
#ifdef SLIB_SUPPORT_STD_TYPES
		typedef std::u16string StdString;
#endif

	public:
		StringView16() noexcept: data(sl_null), length(0) {}

		StringView16(sl_null_t) noexcept: data(sl_null), length(0) {}

		StringView16(const StringView16& other) noexcept: data(other.data), length(other.length) {}

	public:
		StringView16(const String16& value) noexcept;

		StringView16(const sl_char16* sz) noexcept;

		StringView16(const sl_char16* str, sl_reg length) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		StringView16(const std::u16string& str) noexcept;
#endif

	public:
		StringView16& operator=(sl_null_t) noexcept;

		StringView16& operator=(const StringView16& other) noexcept;

		StringView16& operator=(const String16& value) noexcept;

		StringView16& operator=(const sl_char16* sz) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		StringView16& operator=(const std::u16string& str) noexcept;
#endif

		sl_char16 operator[](sl_size index) const noexcept
		{
			return data[index];
		}

		sl_char16& operator[](sl_size index) noexcept
		{
			return data[index];
		}

		explicit operator sl_bool() const noexcept
		{
			return !(isEmpty());
		}

	public:
		template <sl_size N>
		static StringView16 literal(const sl_char16 (&s)[N]) noexcept
		{
			return StringView16(s, N - 1);
		}

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return !data;
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return data != sl_null;
		}

		sl_bool isEmpty() const noexcept
		{
			if (length) {
				if (length > 0) {
					return sl_false;
				}
				return !(*data);
			}
			return sl_true;
		}

		sl_bool isNotEmpty() const noexcept
		{
			return !(isEmpty());
		}

		SLIB_CONSTEXPR sl_char16* getUnsafeData() const
		{
			return data;
		}

		SLIB_CONSTEXPR sl_reg getUnsafeLength() const
		{
			return length;
		}

		sl_char16* getData() const noexcept
		{
			if (data) {
				return data;
			} else {
				return (sl_char16*)((void*)(u""));
			}
		}

		sl_char16* getData(sl_size& outLength) const noexcept;

		sl_size getLength() const noexcept;

		sl_size getHashCode() const noexcept;

		sl_size getHashCodeIgnoreCase() const noexcept;

		static const StringView16& null() noexcept;

		static const StringView16& getEmpty() noexcept;

		void setNull() noexcept;

	public:
		PRIV_SLIB_DECLARE_STRING_VIEW_OPS(StringView16)

	public:
		/**
		 * @return true if this string is equal to the specified string.
		 */
		sl_bool equals(const StringView16& other) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * This functions stops searching on the index of `len-1` and returns 0.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView16& other, sl_size len) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView16& other) const noexcept;

		/**
		 * @return true if this string is equal to the specified string ignoring the case.
		 */
		sl_bool equalsIgnoreCase(const StringView16& other) const noexcept;

		/**
		 * Compares this string to the specified string ignoring the case.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compareIgnoreCase(const StringView16& other) const noexcept;

		/**
		 * @return a view of the substring
		 */
		StringView16 substring(sl_reg start, sl_reg end = -1) const noexcept;

		/**
		 * @return a view of the left substring
		 */
		StringView16 left(sl_reg len) const noexcept;

		/**
		 * @return a view of the right substring
		 */
		StringView16 right(sl_reg len) const noexcept;

		/**
		 * @return a view of the substring
		 */
		StringView16 mid(sl_reg start, sl_reg len) const noexcept;

		/**
		 * @return the index within this string of the first occurrence of the specified character, starting the search at `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg indexOf(const StringView16& str, sl_reg start = 0) const noexcept;
		sl_reg indexOf(sl_char16 ch, sl_reg start = 0) const noexcept;

		/**
		 * @return the index within this string of the last occurrence of the specified character, searching backwards from `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg lastIndexOf(const StringView16& str, sl_reg start = -1) const noexcept;
		sl_reg lastIndexOf(sl_char16 ch, sl_reg start = -1) const noexcept;

		/**
		 * @return `true` if this string starts with the specified character.
		 */
		sl_bool startsWith(const StringView16& str) const noexcept;
		sl_bool startsWith(sl_char16 ch) const noexcept;

		/**
		 * @return `true` if this string ends with the specified character.
		 */
		sl_bool endsWith(const StringView16& str) const noexcept;
		sl_bool endsWith(sl_char16 ch) const noexcept;

		/**
		 * @return `true` if the specified character occurs within this string.
		 */
		sl_bool contains(const StringView16& str) const noexcept;
		sl_bool contains(sl_char16 ch) const noexcept;

		/**
		* @return the total count of the specified character occurs within this string.
		*/
		sl_size countOf(const StringView16& str) const noexcept;
		sl_size countOf(sl_char16 ch) const noexcept;

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
		String16 toUpper() const noexcept;

		/**
		 * @return a copy of this string converted to lowercase.
		 */
		String16 toLower() const noexcept;

		/**
		* Replaces each character of this string that matches the given `pattern` with the given `replacement`. if `replacement` is given as zero, then the matched chracters will be removed.
		*/
		String16 replaceAll(const StringView16& pattern, const StringView16& replacement) const noexcept;
		String16 replaceAll(sl_char16 pattern, sl_char16 replacement) const noexcept;

		/**
		* Removes all characters that matches the given `pattern`
		*/
		String16 removeAll(const StringView16& pattern) const noexcept;
		String16 removeAll(sl_char16 pattern) const noexcept;

		/**
		 * a view of the substring that removed whitespaces from both ends of the new string.
		 */
		StringView16 trim() const noexcept;

		/**
		 * a view of the substring that removed whitespaces from the left of the new string.
		 */
		StringView16 trimLeft() const noexcept;

		/**
		 * a view of the substring that removed whitespaces from the right of the new string.
		 */
		StringView16 trimRight() const noexcept;

		/**
		* Copy this string and then removes CR/LF from both ends of the new string.
		*/
		StringView16 trimLine() const noexcept;

		/**
		* Reverse the characters on this string
		*/
		void makeReverse() noexcept;

		/**
		* Returns reversed copy of this string
		*/
		String16 reverse() const noexcept;

		/**
		 * Splits this string into the list of strings by the `pattern` separator.
		 */
		List<StringView16> split(const StringView16& pattern, sl_reg nMaxSplit = -1) const noexcept;
		List<StringView16> split(sl_char16 pattern, sl_reg nMaxSplit = -1) const noexcept;

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
		sl_bool parseUint32(sl_int32 radix, sl_uint32* value) const noexcept;
		sl_bool parseUint32(sl_uint32* value) const noexcept;
		sl_bool parseInt64(sl_int32 radix, sl_int64* value) const noexcept;
		sl_bool parseInt64(sl_int64* value) const noexcept;
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
		sl_uint32 parseUint32(sl_int32 radix = 10, sl_uint32 def = 0) const noexcept;
		sl_int64 parseInt64(sl_int32 radix = 10, sl_int64 def = 0) const noexcept;
		sl_uint64 parseUint64(sl_int32 radix = 10, sl_uint64 def = 0) const noexcept;
		sl_reg parseInt(sl_int32 radix = 10, sl_reg def = 0) const noexcept;
		sl_size parseSize(sl_int32 radix = 10, sl_size def = 0) const noexcept;

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

	protected:
		sl_char16* data;
		mutable sl_reg length;

	};


	class SLIB_EXPORT StringView32
	{
	public:
		typedef sl_char32 Char;
		typedef String32 StringType;
#ifdef SLIB_SUPPORT_STD_TYPES
		typedef std::u32string StdString;
#endif

	public:
		StringView32() noexcept: data(sl_null), length(0) {}

		StringView32(sl_null_t) noexcept: data(sl_null), length(0) {}

		StringView32(const StringView32& other) noexcept: data(other.data), length(other.length) {}

	public:
		StringView32(const String32& value) noexcept;

		StringView32(const sl_char32* sz) noexcept;

		StringView32(const sl_char32* str, sl_reg length) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		StringView32(const std::u32string& str) noexcept;

#endif

	public:
		StringView32& operator=(sl_null_t) noexcept;

		StringView32& operator=(const StringView32& other) noexcept;

		StringView32& operator=(const String32& value) noexcept;

		StringView32& operator=(const sl_char32* sz) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		StringView32& operator=(const std::u32string& str) noexcept;
#endif

		sl_char32 operator[](sl_size index) const noexcept
		{
			return data[index];
		}

		sl_char32& operator[](sl_size index) noexcept
		{
			return data[index];
		}

		explicit operator sl_bool() const noexcept
		{
			return !(isEmpty());
		}

	public:
		template <sl_size N>
		static StringView32 literal(const sl_char32 (&s)[N]) noexcept
		{
			return StringView32(s, N - 1);
		}

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return !data;
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return data != sl_null;
		}

		sl_bool isEmpty() const noexcept
		{
			if (length) {
				if (length > 0) {
					return sl_false;
				}
				return !(*data);
			}
			return sl_true;
		}

		sl_bool isNotEmpty() const noexcept
		{
			return !(isEmpty());
		}

		SLIB_CONSTEXPR sl_char32* getUnsafeData() const
		{
			return data;
		}

		SLIB_CONSTEXPR sl_reg getUnsafeLength() const
		{
			return length;
		}

		sl_char32* getData() const noexcept
		{
			if (data) {
				return data;
			} else {
				return (sl_char32*)((void*)(U""));
			}
		}

		sl_char32* getData(sl_size& outLength) const noexcept;

		sl_size getLength() const noexcept;

		sl_size getHashCode() const noexcept;

		sl_size getHashCodeIgnoreCase() const noexcept;

		static const StringView32& null() noexcept;

		static const StringView32& getEmpty() noexcept;

		void setNull() noexcept;

	public:
		PRIV_SLIB_DECLARE_STRING_VIEW_OPS(StringView32)

	public:
		/**
		 * @return true if this string is equal to the specified string.
		 */
		sl_bool equals(const StringView32& other) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * This functions stops searching on the index of `len-1` and returns 0.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView32& other, sl_size len) const noexcept;

		/**
		 * Compares this string to the specified string.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compare(const StringView32& other) const noexcept;

		/**
		 * @return true if this string is equal to the specified string ignoring the case.
		 */
		sl_bool equalsIgnoreCase(const StringView32& other) const noexcept;

		/**
		 * Compares this string to the specified string ignoring the case.
		 * @return signed integral indicating the relation between the strings.
		 */
		sl_compare_result compareIgnoreCase(const StringView32& other) const noexcept;

		/**
		 * @return a view of the substring
		 */
		StringView32 substring(sl_reg start, sl_reg end = -1) const noexcept;

		/**
		 * @return a view of the left substring
		 */
		StringView32 left(sl_reg len) const noexcept;

		/**
		 * @return a view of the right substring
		 */
		StringView32 right(sl_reg len) const noexcept;

		/**
		 * @return a view of the substring
		 */
		StringView32 mid(sl_reg start, sl_reg len) const noexcept;

		/**
		 * @return the index within this string of the first occurrence of the specified character, starting the search at `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg indexOf(const StringView32& str, sl_reg start = 0) const noexcept;
		sl_reg indexOf(sl_char32 ch, sl_reg start = 0) const noexcept;

		/**
		 * @return the index within this string of the last occurrence of the specified character, searching backwards from `start` index.
		 * @return -1 if no occurrence is found.
		 */
		sl_reg lastIndexOf(const StringView32& str, sl_reg start = -1) const noexcept;
		sl_reg lastIndexOf(sl_char32 ch, sl_reg start = -1) const noexcept;

		/**
		 * @return `true` if this string starts with the specified character.
		 */
		sl_bool startsWith(const StringView32& str) const noexcept;
		sl_bool startsWith(sl_char32 ch) const noexcept;

		/**
		 * @return `true` if this string ends with the specified character.
		 */
		sl_bool endsWith(const StringView32& str) const noexcept;
		sl_bool endsWith(sl_char32 ch) const noexcept;

		/**
		 * @return `true` if the specified character occurs within this string.
		 */
		sl_bool contains(const StringView32& str) const noexcept;
		sl_bool contains(sl_char32 ch) const noexcept;

		/**
		* @return the total count of the specified character occurs within this string.
		*/
		sl_size countOf(const StringView32& str) const noexcept;
		sl_size countOf(sl_char32 ch) const noexcept;

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
		 * a view of the substring that removed whitespaces from both ends of the new string.
		 */
		StringView32 trim() const noexcept;

		/**
		 * a view of the substring that removed whitespaces from the left of the new string.
		 */
		StringView32 trimLeft() const noexcept;

		/**
		 * a view of the substring that removed whitespaces from the right of the new string.
		 */
		StringView32 trimRight() const noexcept;

		/**
		* Copy this string and then removes CR/LF from both ends of the new string.
		*/
		StringView32 trimLine() const noexcept;

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
		List<StringView32> split(const StringView32& pattern, sl_reg nMaxSplit = -1) const noexcept;
		List<StringView32> split(sl_char32 pattern, sl_reg nMaxSplit = -1) const noexcept;

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
		sl_bool parseUint32(sl_int32 radix, sl_uint32* value) const noexcept;
		sl_bool parseUint32(sl_uint32* value) const noexcept;
		sl_bool parseInt64(sl_int32 radix, sl_int64* value) const noexcept;
		sl_bool parseInt64(sl_int64* value) const noexcept;
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
		sl_uint32 parseUint32(sl_int32 radix = 10, sl_uint32 def = 0) const noexcept;
		sl_int64 parseInt64(sl_int32 radix = 10, sl_int64 def = 0) const noexcept;
		sl_uint64 parseUint64(sl_int32 radix = 10, sl_uint64 def = 0) const noexcept;
		sl_reg parseInt(sl_int32 radix = 10, sl_reg def = 0) const noexcept;
		sl_size parseSize(sl_int32 radix = 10, sl_size def = 0) const noexcept;

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

	protected:
		sl_char32* data;
		mutable sl_reg length;

	};

}

#endif
