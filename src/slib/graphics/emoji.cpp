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

#include "slib/graphics/emoji.h"

#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace priv
	{

		namespace noto_emoji
		{
			extern const char32_t* emojis[];
		}

		using namespace noto_emoji;

		namespace emoji
		{

			class StaticContext
			{
			public:
				CHashMap<String16, sl_bool> mapEmojis;
				CHashMap<sl_char32, sl_bool> mapEmojiFirstChars;
				CHashMap< sl_char32, List<String16> > mapEmojiList;

			public:
				StaticContext()
				{
					for (sl_uint32 index = 0; ; index++) {
						const sl_char32* s = emojis[index];
						if (s) {
							mapEmojiFirstChars.add_NoLock(s[0], sl_true);
							String16 str = String16::create(s);
							mapEmojis.add_NoLock(str, sl_true);
							auto ret = mapEmojiList.emplace_NoLock(s[0], sl_null);
							if (ret.node) {
								ret.node->value.add_NoLock(str);
							}
						} else {
							break;
						}
					}
				}

			public:
				sl_size getEmojiLength(const sl_char16* s, sl_size len)
				{
					sl_char32 ch;
					sl_size pos = 0;
					if (!(Charsets::getUnicode(ch, s, len, pos))) {
						return 0;
					}
					sl_size ret = 0;
					ListElements<String16> list(mapEmojiList.getValue_NoLock(ch));
					for (sl_size i = 0; i < list.count; i++) {
						String16& str = list[i];
						sl_size lenStr = str.getLength();
						if (lenStr <= len && lenStr > ret) {
							if (Base::equalsMemory(str.getData(), s, lenStr << 1)) {
								ret = lenStr;
							}
						}
					}
					return ret;
				}

			};

			SLIB_SAFE_STATIC_GETTER(StaticContext, GetStaticContext)

		}

	}

	using namespace priv::emoji;

	sl_bool Emoji::isEmoji(const String16& str)
	{
		StaticContext* context = GetStaticContext();
		if (context) {
			return context->mapEmojis.getValue_NoLock(str);
		}
		return sl_false;
	}

	sl_bool Emoji::isEmoji(sl_char32 ch)
	{
		StaticContext* context = GetStaticContext();
		if (context) {
			return context->mapEmojiFirstChars.getValue_NoLock(ch);
		}
		return sl_false;
	}

	sl_size Emoji::getEmojiLength(const sl_char16* s, sl_size len)
	{
		StaticContext* context = GetStaticContext();
		if (context) {
			return context->getEmojiLength(s, len);
		}
		return sl_false;
	}

}
