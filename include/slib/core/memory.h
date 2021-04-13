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

	class SLIB_EXPORT MemoryData
	{
	public:
		void* data;
		sl_size size;
		Ref<Referable> refer;

	public:
		MemoryData() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MemoryData)

	public:
		Memory getMemory() const noexcept;

		Memory sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

	};

	class CMemory : public Referable
	{
		SLIB_DECLARE_OBJECT

	private:
		CMemory() noexcept;

		CMemory(const void* data, sl_size size, Referable* refer, sl_bool flagStatic) noexcept;

		~CMemory() noexcept;

	public:
		CMemory(const CMemory& other) = delete;

		CMemory(CMemory&& other) = delete;

		CMemory& operator=(const CMemory& other) = delete;

		CMemory& operator=(CMemory&& other) = delete;

	public:
		static CMemory* create(const void* data, sl_size size, Referable* refer, sl_bool flagStatic) noexcept;

		static CMemory* create(sl_size size) noexcept;

		static CMemory* create(const void* data, sl_size size) noexcept;

		static CMemory* createResizable(sl_size size) noexcept;

		static CMemory* createResizable(const void* data, sl_size size) noexcept;

		static CMemory* createNoCopy(const void* data, sl_size size) noexcept;

		static CMemory* createStatic(const void* data, sl_size size, Referable* refer = sl_null) noexcept;
	
	public:
		void* getData() const noexcept;

		sl_size getSize() const noexcept;

		sl_bool setSize(sl_size size) noexcept;

		sl_bool isStatic() const noexcept;

		const Ref<Referable>& getRefer() const noexcept;

	public:
		CMemory* sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size read(sl_size offsetSource, sl_size size, void* dst) const noexcept;

		sl_size write(sl_size offsetTarget, sl_size size, const void* src) const noexcept;

		sl_size copy(sl_size offsetTarget, const CMemory* source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		CMemory* duplicate() const noexcept;

	public:
		String toString() override;

		sl_bool toJsonString(StringBuffer& buf) override;

		sl_bool toJsonBinary(MemoryBuffer& buf) override;

	protected:
		void* m_data;
		sl_size m_size;
		Ref<Referable> m_refer;
		sl_bool m_flagStatic;

	};
	
	class Memory;
	
	template <>
	class SLIB_EXPORT Atomic<Memory>
	{
	public:
		AtomicRef<CMemory> ref;
		SLIB_ATOMIC_REF_WRAPPER(CMemory)

	public:
		sl_size getSize() const noexcept;

		Memory sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size read(sl_size offsetSource, sl_size size, void* bufDst) const noexcept;

		sl_size write(sl_size offsetTarget, sl_size size, const void* bufSrc) const noexcept;

		sl_size copy(sl_size offsetTarget, const Memory& source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size copy(const Memory& source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		Memory duplicate() const noexcept;

		sl_bool getData(MemoryData& data) const noexcept;

		sl_compare_result compare(const Memory& other) const noexcept;
		
		sl_bool equals(const Memory& other) const noexcept;
		
		sl_size getHashCode() const noexcept;
		
	};
	
	class SLIB_EXPORT Memory
	{
	public:
		Ref<CMemory> ref;
		SLIB_REF_WRAPPER(Memory, CMemory)

	public:
		static Memory create(const void* buf, sl_size size, Referable* refer, sl_bool flagStatic) noexcept;

		static Memory create(sl_size count) noexcept;

		static Memory create(const void* buf, sl_size size) noexcept;

		static Memory createResizable(sl_size count) noexcept;

		static Memory createResizable(const void* buf, sl_size size) noexcept;

		static Memory createNoCopy(const void* buf, sl_size size) noexcept;

		static Memory createStatic(const void* buf, sl_size size, Referable* refer = sl_null) noexcept;

	public:
		void* getData() const noexcept;

		sl_size getSize() const noexcept;

		sl_bool setSize(sl_size size) noexcept;

		sl_bool isStatic() const noexcept;

		const Ref<Referable>& getRefer() const noexcept;

	public:
		Memory sub(sl_size offset, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size read(sl_size offsetSource, sl_size size, void* bufDst) const noexcept;

		sl_size write(sl_size offsetTarget, sl_size size, const void* bufSrc) const noexcept;

		sl_size copy(sl_size offsetTarget, const Memory& source, sl_size offsetSource = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		sl_size copy(const Memory& source, sl_size offset = 0, sl_size size = SLIB_SIZE_MAX) const noexcept;

		Memory duplicate() const noexcept;

		sl_bool getData(MemoryData& data) const noexcept;
		
		sl_compare_result compare(const Memory& other) const noexcept;

		sl_bool equals(const Memory& other) const noexcept;
		
		sl_size getHashCode() const noexcept;
		
	};
	
	typedef Atomic<Memory> AtomicMemory;
	
	
	sl_bool operator==(const Memory& a, const Memory& b) noexcept;
	
	sl_bool operator!=(const Memory& a, const Memory& b) noexcept;
	
	sl_bool operator>=(const Memory& a, const Memory& b) noexcept;
	
	sl_bool operator>(const Memory& a, const Memory& b) noexcept;
	
	sl_bool operator<=(const Memory& a, const Memory& b) noexcept;
	
	sl_bool operator<(const Memory& a, const Memory& b) noexcept;
	
	Memory operator+(const Memory& a, const Memory& b) noexcept;
	
	
	template <>
	class Compare<Memory>
	{
	public:
		sl_compare_result operator()(const Memory& a, const Memory& b) const noexcept;
	};
	
	template <>
	class Equals<Memory>
	{
	public:
		sl_bool operator()(const Memory& a, const Memory& b) const noexcept;
	};
	
	template <>
	class Hash<Memory>
	{
	public:
		sl_size operator()(const Memory& a) const noexcept;
	};
	
	
}

#endif
