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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY
#define CHECKHEADER_SLIB_CORE_MEMORY

#include "memory_view.h"
#include "ref.h"

namespace slib
{

	class Memory;
	class String;
	class String16;
	class String32;
	class MemoryBuffer;
	class SerializeBuffer;

	class SLIB_EXPORT MemoryData : public MemoryView
	{
	public:
		Ref<CRef> ref;

	public:
		SLIB_CONSTEXPR MemoryData() {}

		SLIB_CONSTEXPR MemoryData(const void* data, sl_size size): MemoryView(data, size) {}

		template <class REF>
		MemoryData(const void* data, sl_size size, REF&& _ref) noexcept: MemoryView(data, size), ref(Forward<REF>(_ref)) {}

		MemoryData(const Memory& memory) noexcept;

		MemoryData(Memory&& memory) noexcept;

		template <sl_size N>
		MemoryData(const char(&s)[N]) noexcept: MemoryView(s, N-1) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MemoryData)

	public:
		MemoryData& operator=(const Memory& memory) noexcept;

		MemoryData& operator=(Memory&& memory) noexcept;

	public:
		Memory getMemory() const noexcept;

		void setMemory(const Memory& memory) noexcept;

		void setMemory(Memory&& memory) noexcept;

		Memory sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

	};

	class CMemory : public CRef
	{
		SLIB_DECLARE_OBJECT

	public:
		void* data;
		sl_size size;

	public:
		CMemory(const void* data, sl_size size) noexcept;

		~CMemory() noexcept;

	public:
		CMemory(const CMemory& other) = delete;

		CMemory(CMemory&& other) = delete;

		CMemory& operator=(const CMemory& other) = delete;

		CMemory& operator=(CMemory&& other) = delete;

	public:
		virtual sl_bool isResizable() noexcept;

		virtual sl_bool setSize(sl_size size) noexcept;

		virtual CRef* getRef() noexcept;

		virtual String getString() noexcept;

		virtual String16 getString16() noexcept;

		virtual String32 getString32() noexcept;

	public:
		String toString() override;

		sl_bool toJsonString(StringBuffer& buf) override;

		sl_bool toJsonBinary(MemoryBuffer& buf) override;

	public:
		CMemory* sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) noexcept;

		sl_size read(sl_size offsetSource, sl_size size, void* dst) noexcept;

		sl_size write(sl_size offsetTarget, sl_size size, const void* src) noexcept;

		sl_size copy(sl_size offsetTarget, const CMemory* source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) noexcept;

		CMemory* duplicate() noexcept;

		sl_bool serialize(MemoryBuffer* output) noexcept;

	};

	template <> class Atomic<Memory>;

	class SLIB_EXPORT Memory
	{
	public:
		Ref<CMemory> ref;
		SLIB_REF_WRAPPER_NO_OP(Memory, CMemory)

	public:
		static Memory create(sl_size count) noexcept;

		static Memory create(const void* buf, sl_size size) noexcept;

		static Memory createResizable(sl_size count) noexcept;

		static Memory createResizable(const void* buf, sl_size size) noexcept;

		static Memory createNoCopy(const void* buf, sl_size size) noexcept;

		static Memory createStatic(const void* buf, sl_size size) noexcept;

		template <class T>
		static Memory createStatic(const void* buf, sl_size size, T* ref) noexcept
		{
			return _createStatic(buf, size, ref);
		}

		template <class T>
		static Memory createStatic(const void* buf, sl_size size, const Ref<T>& ref) noexcept
		{
			return _createStatic(buf, size, ref.get());
		}

		template <class T>
		static Memory createStatic(const void* buf, sl_size size, Ref<T>&& ref) noexcept
		{
			return _createStaticMove(buf, size, &ref);
		}

		template <sl_size N>
		static Memory createStatic(const char(&s)[N]) noexcept
		{
			return createStatic(s, N - 1);
		}

		static Memory createFromString(const String& str) noexcept;
		static Memory createFromString(String&& str) noexcept;
		static Memory createFromString(const String16& str) noexcept;
		static Memory createFromString(String16&& str) noexcept;
		static Memory createFromString(const String32& str) noexcept;
		static Memory createFromString(String32&& str) noexcept;

		static Memory createFromExtendedJson(const Json& json, sl_uint32* pOutSubType = sl_null);

	public:
		void* getData() const noexcept;

		sl_size getSize() const noexcept;

		sl_bool setSize(sl_size size) noexcept;

		CRef* getRef() const noexcept;

		sl_bool isResizable() const noexcept;

	public:
		Memory sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size read(sl_size offsetSource, sl_size size, void* bufDst) const noexcept;

		sl_size write(sl_size offsetTarget, sl_size size, const void* bufSrc) const noexcept;

		sl_size copy(sl_size offsetTarget, const Memory& source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size copy(const Memory& source, sl_size offset = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		Memory duplicate() const noexcept;

		sl_bool getData(MemoryData& data) const noexcept;

	public:
		Memory operator+(const Memory& other) const noexcept;

		SLIB_CONSTEXPR explicit operator sl_bool() const
		{
			return ref.ptr != sl_null;
		}

		SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS(Memory)
		SLIB_DECLARE_CLASS_SERIALIZE_MEMBERS

		sl_bool serialize(MemoryBuffer* output) const;
		sl_bool deserialize(SerializeBuffer* input);

	private:
		static Memory _createStatic(const void* buf, sl_size size, CRef* ref) noexcept;
		static Memory _createStaticMove(const void* buf, sl_size size, void* pRef) noexcept;

	};

	template <>
	class SLIB_EXPORT Atomic<Memory>
	{
	public:
		AtomicRef<CMemory> ref;
		SLIB_ATOMIC_REF_WRAPPER_NO_OP(CMemory)

	};

	typedef Atomic<Memory> AtomicMemory;

}

#endif
