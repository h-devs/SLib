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

#include "ref.h"
#include "default_members.h"

namespace slib
{

	class Memory;
	class String;
	class String16;
	class MemoryBuffer;
	class DeserializeBuffer;

	class SLIB_EXPORT MemoryData
	{
	public:
		void* data;
		sl_size size;
		Ref<Referable> ref;

	public:
		MemoryData() noexcept;

		MemoryData(const void* _data, sl_size _size) noexcept;

		template <class REF>
		MemoryData(const void* _data, sl_size _size, REF&& _ref) noexcept: data((void*)_data), size(_size), ref(Forward<REF>(_ref)) {}

		MemoryData(const Memory& memory) noexcept;

		MemoryData(Memory&& memory) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MemoryData)

	public:
		MemoryData& operator=(const Memory& memory) noexcept;

		MemoryData& operator=(Memory&& memory) noexcept;

	public:
		Memory getMemory() const noexcept;

		Memory sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

	};

	class CMemory : public Referable
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

		virtual Referable* getRef() noexcept;

		virtual String getString() noexcept;

		virtual String16 getString16() noexcept;

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

	template <>
	class SLIB_EXPORT Atomic<Memory>
	{
	public:
		AtomicRef<CMemory> ref;
		SLIB_ATOMIC_REF_WRAPPER_NO_OP(CMemory)

	public:
		sl_size getSize() const noexcept;

		Memory sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size read(sl_size offsetSource, sl_size size, void* bufDst) const noexcept;

		sl_size write(sl_size offsetTarget, sl_size size, const void* bufSrc) const noexcept;

		sl_size copy(sl_size offsetTarget, const Memory& source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size copy(const Memory& source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		Memory duplicate() const noexcept;

		sl_bool getData(MemoryData& data) const noexcept;
		
	public:
		SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS_NO_OP(Memory)

	};
	
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

		static Memory createFromString(const String& str) noexcept;
		static Memory createFromString(String&& str) noexcept;

		static Memory createFromString16(const String16& str) noexcept;
		static Memory createFromString16(String16&& str) noexcept;

		static Memory createFromExtendedJson(const Json& json, sl_uint32* pOutSubType = sl_null);

	public:
		void* getData() const noexcept;

		sl_size getSize() const noexcept;

		sl_bool setSize(sl_size size) noexcept;

		Referable* getRef() const noexcept;

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
		SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS_NO_OP(Memory)
		SLIB_DECLARE_CLASS_SERIALIZE_MEMBERS

		sl_bool serialize(MemoryBuffer* output) const;
		sl_bool deserialize(DeserializeBuffer* input);

	private:
		static Memory _createStatic(const void* buf, sl_size size, Referable* ref) noexcept;
		static Memory _createStaticMove(const void* buf, sl_size size, void* pRef) noexcept;

	};
	
	typedef Atomic<Memory> AtomicMemory;
	
	SLIB_DECLARE_DEFAULT_COMPARE_OPERATORS(Memory)
	Memory operator+(const Memory& a, const Memory& b) noexcept;
	
}

#endif
