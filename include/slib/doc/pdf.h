/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DOC_PDF
#define CHECKHEADER_SLIB_DOC_PDF

#include "definition.h"

#include "../core/variant.h"

namespace slib
{

	enum class PdfObjectType
	{
		Undefined = 0,
		Null = 1,
		Boolean = 2,
		Uint = 3,
		Int = 4,
		Float = 5,
		String = 6,
		Name = 7,
		Array = 8,
		Dictionary = 9,
		Stream = 10,
		Reference = 11
	};

	class PdfObject;
	class PdfStream;
	class PdfPage;
	class PdfDocument;

	typedef HashMap<String, PdfObject> PdfDictionary;
	typedef List<PdfObject> PdfArray;

	class SLIB_EXPORT PdfReference
	{
	public:
		sl_uint32 objectNumber;
		sl_uint32 generation;

	public:
		PdfReference() noexcept {}

		SLIB_CONSTEXPR PdfReference(sl_uint32 _num): objectNumber(_num), generation(0) {}

		SLIB_CONSTEXPR PdfReference(sl_uint32 _num, sl_uint32 _gen): objectNumber(_num), generation(_gen) {}

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(PdfReference)

	public:
		SLIB_CONSTEXPR sl_bool operator==(const PdfReference& other) const
		{
			return objectNumber == other.objectNumber && generation == other.generation;
		}

	};

	class SLIB_EXPORT PdfName
	{
	public:
		SLIB_CONSTEXPR PdfName() {}

		PdfName(const String& _name) noexcept: value(_name) {}
		PdfName(String&& _name) noexcept: value(Move(_name)) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfName)

	public:
		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return value.isNull();
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return value.isNotNull();
		}

	public:
		String value;

	};

	class SLIB_EXPORT PdfObject
	{
	public:
		PdfObject() noexcept {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfObject)

	public:
		PdfObject(sl_null_t) noexcept: m_var(sl_null, (sl_uint8)(PdfObjectType::Null)) {}

		PdfObject(sl_bool v) noexcept;

		PdfObject(sl_int32 v) noexcept;
		PdfObject(sl_uint32 v) noexcept;
		PdfObject(float v) noexcept;

		PdfObject(const String& v) noexcept;
		PdfObject(String&& v) noexcept;

		PdfObject(const PdfArray& v) noexcept;
		PdfObject(PdfArray&& v) noexcept;

		PdfObject(const PdfDictionary& v) noexcept;
		PdfObject(PdfDictionary&& v) noexcept;

		PdfObject(const Ref<PdfStream>& v) noexcept;
		PdfObject(Ref<PdfStream>&& v) noexcept;

		PdfObject(const PdfName& v) noexcept;
		PdfObject(PdfName&& v) noexcept;

		PdfObject(const PdfReference& v) noexcept;

	public:
		const Variant& getVariant() const noexcept
		{
			return m_var;
		}

		Variant& getVariant() noexcept
		{
			return m_var;
		}

		SLIB_CONSTEXPR PdfObjectType getType() const
		{
			return (PdfObjectType)(m_var.getTag());
		}

		SLIB_CONSTEXPR sl_bool isUndefined() const
		{
			return m_var.isUndefined();
		}

		SLIB_CONSTEXPR sl_bool isNotUndefined() const
		{
			return m_var.isNotUndefined();
		}

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return m_var.isNull();
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return m_var.isNotNull();
		}

		sl_bool getBoolean() const noexcept;

		sl_bool getBoolean(sl_bool& _out) const noexcept;

		sl_uint32 getUint() const noexcept;

		sl_bool getUint(sl_uint32& _out) const noexcept;

		sl_int32 getInt() const noexcept;

		sl_bool getInt(sl_int32& _out) const noexcept;

		float getFloat() const noexcept;

		sl_bool getFloat(float& _out) const noexcept;

		sl_bool isNumeric() const noexcept;

		const String& getString() const noexcept;

		const String& getName() const noexcept;

		sl_bool equalsName(const StringView& name) const noexcept;

		const PdfArray& getArray() const noexcept;

		const PdfDictionary& getDictionary() const noexcept;

		const Ref<PdfStream>& getStream() const noexcept;

		Memory getStreamContent() const noexcept;

		PdfReference getReference() const noexcept;

		sl_bool getReference(PdfReference& _out) const noexcept;

	private:
		Variant m_var;

	};

	class SLIB_EXPORT PdfStream : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfDictionary properties;
		Memory content;

	public:
		PdfStream() noexcept;

		~PdfStream();

	public:
		PdfObject getProperty(const String& name) noexcept;

		Memory getOriginalContent() noexcept;

	};

	class PdfPageTreeItem : public Referable
	{
	public:
		WeakRef<PdfPageTreeItem> parent;
		PdfDictionary attributes;
		sl_bool flagPage;

	public:
		PdfPageTreeItem() noexcept;

		~PdfPageTreeItem();

	public:
		PdfObject getAttribute(const String& name) noexcept;

	};

	class SLIB_EXPORT PdfPage : public PdfPageTreeItem
	{
		SLIB_DECLARE_OBJECT

	public:
		WeakRef<PdfDocument> document;

	public:
		PdfPage() noexcept;

		~PdfPage();

	public:
		Memory getContent() noexcept;

	};

	class SLIB_EXPORT PdfDocument : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		sl_uint32 fileSize;

		sl_uint8 majorVersion;
		sl_uint8 minorVersion;
		PdfDictionary lastTrailer;
		PdfDictionary encrypt;
		PdfDictionary catalog;

	public:
		PdfDocument();

		~PdfDocument();

	public:
		sl_bool openFile(const StringParam& filePath);

		PdfObject getObject(const PdfReference& ref);

		sl_uint32 getPagesCount();

		Ref<PdfPage> getPage(sl_uint32 index);

		sl_bool isEncrypted();

		static sl_bool isEncryptedFile(const StringParam& path);

		sl_bool isAuthenticated();

		sl_bool setUserPassword(const StringView& password);
		
	private:
		Ref<Referable> m_parser;

		friend class PdfPage;

	};

}

#endif
