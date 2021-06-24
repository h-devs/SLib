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

#ifndef CHECKHEADER_SLIB_MATH_BIGINT
#define CHECKHEADER_SLIB_MATH_BIGINT

#include "definition.h"

#include "../core/ref.h"

namespace slib
{

	class Memory;

/*
	Notice:
		
		CBigInt, BigInt is not thread-safe on modification operations
*/
	class SLIB_EXPORT CBigInt : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		sl_uint32* elements;

		// count of elements
		sl_size length;

		// 1 : positive,  -1 : negative
		sl_int32 sign;

	private:
		sl_bool m_flagUserData;

	public:
		CBigInt() noexcept;

		~CBigInt() noexcept;
	
	public:
		void setUserDataElements(sl_uint32* data, sl_size n) noexcept;

		sl_int32 makeNagative() noexcept;
	
		sl_bool getBit(sl_size pos) const noexcept;

		sl_bool setBit(sl_size pos, sl_bool bit) noexcept;
	
		// get size in elements
		sl_size getMostSignificantElements() const noexcept;

		sl_size getLeastSignificantElements() const noexcept;

		// get size in bytes
		sl_size getMostSignificantBytes() const noexcept;

		sl_size getLeastSignificantBytes() const noexcept;

		// get size in bits
		sl_size getMostSignificantBits() const noexcept;

		sl_size getLeastSignificantBits() const noexcept;
	
		sl_bool isZero() const noexcept;

		sl_bool isNotZero() const noexcept;

		void setZero() noexcept;


		static CBigInt* allocate(sl_size length) noexcept;

		CBigInt* duplicate(sl_size newLength) const noexcept;

		CBigInt* duplicate() const noexcept;

		CBigInt* duplicateCompact() const noexcept;

		sl_bool copyAbsFrom(const CBigInt& other) noexcept;

		sl_bool copyFrom(const CBigInt& other) noexcept;

		sl_bool compact() noexcept;

		sl_bool growLength(sl_size newLength) noexcept;

		sl_bool setLength(sl_size newLength) noexcept;

		sl_bool setValueFromElements(const sl_uint32* data, sl_size n) noexcept;

		// set/get data from/to bytes buffer (Little Endian), the sign is not changed
		sl_bool setBytesLE(const void* bytes, sl_size nBytes) noexcept;

		void setBytesLE(const Memory& mem) noexcept;

		static CBigInt* fromBytesLE(const void* bytes, sl_size nBytes) noexcept;

		static CBigInt* fromBytesLE(const Memory& mem) noexcept;

		sl_bool getBytesLE(void* buf, sl_size n) const noexcept;

		Memory getBytesLE() const noexcept;

		// set/get data from/to bytes buffer (Big Endian), the sign is not changed
		sl_bool setBytesBE(const void* bytes, sl_size nBytes) noexcept;

		void setBytesBE(const Memory& mem) noexcept;

		static CBigInt* fromBytesBE(const void* bytes, sl_size nBytes) noexcept;

		static CBigInt* fromBytesBE(const Memory& mem) noexcept;

		sl_bool getBytesBE(void* buf, sl_size n) const noexcept;

		Memory getBytesBE() const noexcept;


		sl_bool setValue(sl_int32 v) noexcept;

		static CBigInt* fromInt32(sl_int32 v) noexcept;

		sl_bool setValue(sl_uint32 v) noexcept;

		static CBigInt* fromUint32(sl_uint32 v) noexcept;

		sl_bool setValue(sl_int64 v) noexcept;

		static CBigInt* fromInt64(sl_int64 v) noexcept;

		sl_bool setValue(sl_uint64 v) noexcept;
		
		static CBigInt* fromUint64(sl_uint64 v) noexcept;

		sl_int32 getInt32() const noexcept;

		sl_uint32 getUint32() const noexcept;
		
		sl_int64 getInt64() const noexcept;

		sl_uint64 getUint64() const noexcept;

		String toString(sl_uint32 radix = 10) const noexcept;

		String toHexString() const noexcept;


		sl_bool equals(const CBigInt& other) const noexcept;
		
		sl_bool equals(sl_int32 v) const noexcept;
		
		sl_bool equals(sl_uint32 v) const noexcept;
		
		sl_bool equals(sl_int64 v) const noexcept;
		
		sl_bool equals(sl_uint64 v) const noexcept;
		
		
		// compare returns
		//  0: equal,  negative: less than, positive: greater than
		sl_compare_result compareAbs(const CBigInt& other) const noexcept;

		sl_compare_result compare(const CBigInt& other) const noexcept;

		sl_compare_result compare(sl_int32 v) const noexcept;

		sl_compare_result compare(sl_uint32 v) const noexcept;

		sl_compare_result compare(sl_int64 v) const noexcept;

		sl_compare_result compare(sl_uint64 v) const noexcept;
	

		sl_bool addAbs(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool addAbs(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool addAbs(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool add(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool add(const CBigInt& a, sl_int32 v) noexcept;

		sl_bool add(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool add(const CBigInt& a, sl_int64 v) noexcept;

		sl_bool add(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool add(const CBigInt& o) noexcept;

		sl_bool add(sl_int32 v) noexcept;

		sl_bool add(sl_uint32 v) noexcept;

		sl_bool add(sl_int64 v) noexcept;

		sl_bool add(sl_uint64 v) noexcept;
	

		sl_bool subAbs(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool subAbs(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool subAbs(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool sub(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool sub(const CBigInt& a, sl_int32 v) noexcept;

		sl_bool sub(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool sub(const CBigInt& a, sl_int64 v) noexcept;

		sl_bool sub(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool sub(const CBigInt& o) noexcept;

		sl_bool sub(sl_int32 v) noexcept;

		sl_bool sub(sl_uint32 v) noexcept;

		sl_bool sub(sl_int64 v) noexcept;

		sl_bool sub(sl_uint64 v) noexcept;

	
		sl_bool mulAbs(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool mulAbs(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool mul(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool mul(const CBigInt& a, sl_int32 v) noexcept;

		sl_bool mul(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool mul(const CBigInt& a, sl_int64 v) noexcept;

		sl_bool mul(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool mul(const CBigInt& o) noexcept;

		sl_bool mul(sl_int32 v) noexcept;

		sl_bool mul(sl_uint32 v) noexcept;

		sl_bool mul(sl_int64 v) noexcept;

		sl_bool mul(sl_uint64 v) noexcept;
	

		static sl_bool divAbs(const CBigInt& a, const CBigInt& b, CBigInt* quotient = sl_null, CBigInt* remainder = sl_null) noexcept;

		static sl_bool divAbs(const CBigInt& a, sl_uint32 b, CBigInt* quotient = sl_null, sl_uint32* remainder = sl_null) noexcept;

		static sl_bool div(const CBigInt& a, const CBigInt& b, CBigInt* quotient = sl_null, CBigInt* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		static sl_bool divInt32(const CBigInt& a, sl_int32 b, CBigInt* quotient = sl_null, sl_int32* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// always non-negative remainder
		static sl_bool divUint32(const CBigInt& a, sl_uint32 b, CBigInt* quotient = sl_null, sl_uint32* remainder = sl_null) noexcept;

		static sl_bool divInt64(const CBigInt& a, sl_int64 b, CBigInt* quotient = sl_null, sl_int64* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// always non-negative remainder
		static sl_bool divUint64(const CBigInt& a, sl_uint64 b, CBigInt* quotient = sl_null, sl_uint64* remainder = sl_null) noexcept;
	

		sl_bool shiftLeft(const CBigInt& other, sl_size n) noexcept;

		sl_bool shiftRight(const CBigInt& other, sl_size n) noexcept;

		sl_bool shiftLeft(sl_size n) noexcept;

		sl_bool shiftRight(sl_size n) noexcept;
	
		/*
			E > 0
			M is not null => C = A^E mod M
			M is null => C = A^E
		*/
		sl_bool pow(const CBigInt& A, const CBigInt& E, const CBigInt* pM = sl_null) noexcept;

		sl_bool pow_mod(const CBigInt& A, const CBigInt& E, const CBigInt& M) noexcept;

		sl_bool pow_mod(const CBigInt& E, const CBigInt& M) noexcept;

		sl_bool pow(const CBigInt& E) noexcept;

		sl_bool pow(const CBigInt& A, sl_uint32 E, const CBigInt* pM = sl_null) noexcept;
	
		sl_bool pow_mod(const CBigInt& A, sl_uint32 E, const CBigInt& M) noexcept;

		sl_bool pow_mod(sl_uint32 E, const CBigInt& M) noexcept;

		sl_bool pow(sl_uint32 E) noexcept;
	

		/*
			Exponentiation based on Montgomery Reduction
				C = A^E mod M
			Available Input:
				M - an odd value (M%2=1), M>0
				E > 0
		*/
		sl_bool pow_montgomery(const CBigInt& A, const CBigInt& E, const CBigInt& M) noexcept;

		sl_bool pow_montgomery(const CBigInt& E, const CBigInt& M) noexcept;
	
		/*
			C = A^-1 mod M
	
			Available Input:
				A != 0
				M > 0
				gcd(A, M) = 1
		*/
		sl_bool inverseMod(const CBigInt& A, const CBigInt& M) noexcept;

		sl_bool inverseMod(const CBigInt& M) noexcept;
	
		/*
			gcd - greatest common divisor
		*/
		sl_bool gcd(const CBigInt& A, const CBigInt& B) noexcept;

		sl_bool gcd(const CBigInt& B) noexcept;
		
		/*
		 	lcm - least common multiple
		*/
		sl_bool lcm(const CBigInt& A, const CBigInt& B) noexcept;
		
		sl_bool lcm(const CBigInt& B) noexcept;
		
		/*
		 	prime check and generation
		*/
		sl_bool isProbablePrime(sl_uint32 nChecks = 0, sl_bool* pFlagError = sl_null) const noexcept;
		
		sl_bool generatePrime(sl_size nBits) noexcept;
		
		sl_bool random(sl_size nBits) noexcept;
		
		
		sl_size getHashCode() const noexcept;
	
	private:
		void _free() noexcept;

	};
	
	
	class BigInt;
	
	template <>
	class SLIB_EXPORT Atomic<BigInt>
	{
	public:
		AtomicRef<CBigInt> ref;
		SLIB_ATOMIC_REF_WRAPPER_NO_OP(CBigInt)

	public:
		Atomic(sl_int32 n) noexcept;

		Atomic(sl_uint32 n) noexcept;

		Atomic(sl_int64 n) noexcept;

		Atomic(sl_uint64 n) noexcept;

		~Atomic() noexcept;

	public:
		BigInt duplicate() const noexcept;

		BigInt compact() const noexcept;

		sl_bool isZero() const noexcept;

		sl_bool isNotZero() const noexcept;

		sl_bool getBytesLE(void* buf, sl_size n) const noexcept;

		Memory getBytesLE() const noexcept;

		sl_bool getBytesBE(void* buf, sl_size n) const noexcept;

		Memory getBytesBE() const noexcept;

		String toString(sl_uint32 radix = 10) const noexcept;

		String toHexString() const noexcept;

		sl_bool equals(const BigInt& other) const noexcept;
		
		sl_bool equals(sl_int32 v) const noexcept;
		
		sl_bool equals(sl_uint32 v) const noexcept;
		
		sl_bool equals(sl_int64 v) const noexcept;
		
		sl_bool equals(sl_uint64 v) const noexcept;
		
		// compare returns
		//  0: equal,  negative: less than, positive: greater than
		sl_compare_result compare(const BigInt& other) const noexcept;

		sl_compare_result compare(sl_int32 v) const noexcept;

		sl_compare_result compare(sl_uint32 v) const noexcept;

		sl_compare_result compare(sl_int64 v) const noexcept;

		sl_compare_result compare(sl_uint64 v) const noexcept;

		BigInt negative() const noexcept;

		sl_size getHashCode() const noexcept;

	public:
		Atomic& operator=(sl_int32 n) noexcept;

		Atomic& operator=(sl_uint32 n) noexcept;

		Atomic& operator=(sl_int64 n) noexcept;

		Atomic& operator=(sl_uint64 n) noexcept;

	};
	
	typedef Atomic<BigInt> AtomicBigInt;
	
	
	class SLIB_EXPORT BigInt
	{
	public:
		Ref<CBigInt> ref;
		SLIB_REF_WRAPPER_NO_OP(BigInt, CBigInt)
	
	public:
		BigInt(sl_int32 n) noexcept;

		BigInt(sl_uint32 n) noexcept;

		BigInt(sl_int64 n) noexcept;

		BigInt(sl_uint64 n) noexcept;

		~BigInt() noexcept;

	public:
		static BigInt fromInt32(sl_int32 v) noexcept;

		static BigInt fromUint32(sl_uint32 v) noexcept;

		static BigInt fromInt64(sl_int64 v) noexcept;

		static BigInt fromUint64(sl_uint64 v) noexcept;

		static BigInt fromBytesLE(const void* bytes, sl_size nBytes) noexcept;

		static BigInt fromBytesLE(const Memory& mem) noexcept;

		static BigInt fromBytesBE(const void* bytes, sl_size nBytes) noexcept;

		static BigInt fromBytesBE(const Memory& mem) noexcept;

		static BigInt fromString(const StringParam& str, sl_uint32 radix = 10) noexcept;

		static BigInt fromHexString(const StringParam& str) noexcept;

		CBigInt& instance() const noexcept;

		BigInt duplicate() const noexcept;

		BigInt compact() const noexcept;

		sl_size getElementsCount() const noexcept;

		sl_uint32* getElements() const noexcept;

		sl_int32 getSign() const noexcept;

		sl_bool getBit(sl_uint32 pos) const noexcept;

		sl_size getMostSignificantElements() const noexcept;

		sl_size getLeastSignificantElements() const noexcept;

		sl_size getMostSignificantBytes() const noexcept;

		sl_size getLeastSignificantBytes() const noexcept;

		sl_size getMostSignificantBits() const noexcept;

		sl_size getLeastSignificantBits() const noexcept;

		sl_bool isZero() const noexcept;

		sl_bool isNotZero() const noexcept;

		sl_bool getBytesLE(void* buf, sl_size n) const noexcept;

		Memory getBytesLE() const noexcept;

		sl_bool getBytesBE(void* buf, sl_size n) const noexcept;

		Memory getBytesBE() const noexcept;

		String toString(sl_uint32 radix = 10) const noexcept;

		String toHexString() const noexcept;

		
		sl_bool equals(const BigInt& other) const noexcept;
		
		sl_bool equals(sl_int32 v) const noexcept;
		
		sl_bool equals(sl_uint32 v) const noexcept;
		
		sl_bool equals(sl_int64 v) const noexcept;
		
		sl_bool equals(sl_uint64 v) const noexcept;
		
		
		// compare returns
		//  0: equal,  negative: less than, positive: greater than
		sl_compare_result compare(const BigInt& other) const noexcept;

		sl_compare_result compare(sl_int32 v) const noexcept;

		sl_compare_result compare(sl_uint32 v) const noexcept;

		sl_compare_result compare(sl_int64 v) const noexcept;

		sl_compare_result compare(sl_uint64 v) const noexcept;

	
		static BigInt add(const BigInt& A, const BigInt& B) noexcept;

		sl_bool add(const BigInt& other) noexcept;

		static BigInt add(const BigInt& A, sl_int32 v) noexcept;

		sl_bool add(sl_int32 v) noexcept;

		static BigInt add(const BigInt& A, sl_uint32 v) noexcept;

		sl_bool add(sl_uint32 v) noexcept;

		static BigInt add(const BigInt& A, sl_int64 v) noexcept;
	
		sl_bool add(sl_int64 v) noexcept;

		static BigInt add(const BigInt& A, sl_uint64 v) noexcept;
	
		sl_bool add(sl_uint64 v) noexcept;

		sl_bool increase() noexcept;


		static BigInt sub(const BigInt& A, const BigInt& B) noexcept;
	
		sl_bool sub(const BigInt& other) noexcept;

		static BigInt sub(const BigInt& A, sl_int32 v) noexcept;

		sl_bool sub(sl_int32 v) noexcept;

		static BigInt sub(const BigInt& A, sl_uint32 v) noexcept;

		sl_bool sub(sl_uint32 v) noexcept;

		static BigInt sub(const BigInt& A, sl_int64 v) noexcept;
	
		sl_bool sub(sl_int64 v) noexcept;

		static BigInt sub(const BigInt& A, sl_uint64 v) noexcept;
	
		sl_bool sub(sl_uint64 v) noexcept;

		sl_bool decrease() noexcept;


		void makeNegative() const noexcept;

		BigInt negative() const noexcept;

	
		static BigInt mul(const BigInt& A, const BigInt& B) noexcept;

		sl_bool mul(const BigInt& other) noexcept;
	
		static BigInt mul(const BigInt& A, sl_int32 v) noexcept;

		sl_bool mul(sl_int32 v) noexcept;

		static BigInt mul(const BigInt& A, sl_uint32 v) noexcept;

		sl_bool mul(sl_uint32 v) noexcept;

		static BigInt mul(const BigInt& A, sl_int64 v) noexcept;

		sl_bool mul(sl_int64 v) noexcept;

		static BigInt mul(const BigInt& A, sl_uint64 v) noexcept;

		sl_bool mul(sl_uint64 v) noexcept;


		static BigInt div(const BigInt& A, const BigInt& B, BigInt* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		sl_bool div(const BigInt& other, BigInt* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		static BigInt divInt32(const BigInt& A, sl_int32 v, sl_int32* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		sl_bool divInt32(sl_int32 v, sl_int32* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// always non-negative remainder
		static BigInt divUint32(const BigInt& A, sl_uint32 v, sl_uint32* remainder = sl_null) noexcept;

		// always non-negative remainder
		sl_bool divUint32(sl_uint32 v, sl_uint32* remainder = sl_null) noexcept;

		static BigInt divInt64(const BigInt& A, sl_int64 v, sl_int64* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		sl_bool divInt64(sl_int64 v, sl_int64* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// always non-negative remainder
		static BigInt divUint64(const BigInt& A, sl_uint64 v, sl_uint64* remainder = sl_null) noexcept;

		// always non-negative remainder
		sl_bool divUint64(sl_uint64 v, sl_uint64* remainder = sl_null) noexcept;


		static BigInt mod(const BigInt& A, const BigInt& B) noexcept;
	
		static BigInt mod_NonNegativeRemainder(const BigInt& A, const BigInt& B) noexcept;
		
		sl_bool mod(const BigInt& other) noexcept;

		sl_bool mod_NonNegativeRemainder(const BigInt& other) noexcept;

		static sl_int32 modInt32(const BigInt& A, sl_int32 v) noexcept;

		static sl_int32 modInt32_NonNegativeRemainder(const BigInt& A, sl_int32 v) noexcept;

		// non-negative remainder
		static sl_uint32 modUint32(const BigInt& A, sl_uint32 v) noexcept;

		static sl_int64 modInt64(const BigInt& A, sl_int64 v) noexcept;

		static sl_int64 modInt64_NonNegativeRemainder(const BigInt& A, sl_int64 v) noexcept;

		// non-negative remainder
		static sl_uint64 modUint64(const BigInt& A, sl_uint64 v) noexcept;
	

		static BigInt shiftLeft(const BigInt& A, sl_size n) noexcept;

		sl_bool shiftLeft(sl_size n) noexcept;

		static BigInt shiftRight(const BigInt& A, sl_size n) noexcept;

		sl_bool shiftRight(sl_size n) noexcept;
		
		
		BigInt abs() const noexcept;
	

		/*
			E > 0
			M is not null => C = A^E mod M
			M is null => C = A^E
		*/
		static BigInt pow(const BigInt& A, const BigInt& E, const BigInt* pM = sl_null) noexcept;

		sl_bool pow(const BigInt& E, const BigInt* pM = sl_null) noexcept;

		static BigInt pow_mod(const BigInt& A, const BigInt& E, const BigInt& M) noexcept;
	
		sl_bool pow_mod(const BigInt& E, const BigInt& M) noexcept;

		static BigInt pow(const BigInt& A, sl_uint32 E, const BigInt* pM = sl_null) noexcept;

		sl_bool pow(sl_uint32 E, const BigInt* pM = sl_null) noexcept;

		BigInt pow_mod(const BigInt& A, sl_uint32 E, const BigInt& M) noexcept;

		sl_bool pow_mod(sl_uint32 E, const BigInt& M) noexcept;

		/*
			Exponentiation based on Montgomery Reduction
				C = A^E mod M
			Available Input:
				M - an odd value (M%2=1), M>0
				E > 0
		*/
		static BigInt pow_montgomery(const BigInt& A, const BigInt& E, const BigInt& M) noexcept;

		sl_bool pow_montgomery(const BigInt& E, const BigInt& M) noexcept;
	

		/*
			C = A^-1 mod M
	
			Available Input:
				A != 0
				M > 0
				gcd(A, M) = 1
		*/
		static BigInt inverseMod(const BigInt& A, const BigInt& M) noexcept;
	
		/*
			gcd - greatest common divisor
		*/
		static BigInt gcd(const BigInt& A, const BigInt& B) noexcept;
		
		/*
		 	lcm - least common multiple
		 */
		static BigInt lcm(const BigInt& A, const BigInt& B) noexcept;
		
		/*
		 	prime check and generation
		*/
		sl_bool isProbablePrime(sl_uint32 nChecks = 0, sl_bool* pFlagError = sl_null) const noexcept;
		
		static BigInt generatePrime(sl_size nBits) noexcept;
		
		static BigInt random(sl_size nBits) noexcept;
		

		sl_size getHashCode() const noexcept;


		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS
		SLIB_DECLARE_CLASS_PARSE_INT_MEMBERS(BigInt)

	public:
		BigInt& operator=(sl_int32 n) noexcept;

		BigInt& operator=(sl_uint32 n) noexcept;

		BigInt& operator=(sl_int64 n) noexcept;

		BigInt& operator=(sl_uint64 n) noexcept;

		BigInt& operator+=(const BigInt& other) noexcept;

		BigInt& operator+=(sl_int32 v) noexcept;

		BigInt& operator+=(sl_uint32 v) noexcept;

		BigInt& operator+=(sl_int64 v) noexcept;

		BigInt& operator+=(sl_uint64 v) noexcept;

		BigInt& operator++() noexcept;

		BigInt operator++(int) noexcept;

		BigInt& operator-=(const BigInt& other) noexcept;

		BigInt& operator-=(sl_int32 v) noexcept;

		BigInt& operator-=(sl_uint32 v) noexcept;

		BigInt& operator-=(sl_int64 v) noexcept;

		BigInt& operator-=(sl_uint64 v) noexcept;

		BigInt& operator--() noexcept;

		BigInt operator--(int) noexcept;

		BigInt& operator*=(const BigInt& other) noexcept;

		BigInt& operator*=(sl_int32 v) noexcept;

		BigInt& operator*=(sl_uint32 v) noexcept;

		BigInt& operator*=(sl_int64 v) noexcept;

		BigInt& operator*=(sl_uint64 v) noexcept;

		BigInt& operator/=(const BigInt& other) noexcept;

		BigInt& operator/=(sl_int32 v) noexcept;

		BigInt& operator/=(sl_uint32 v) noexcept;

		BigInt& operator/=(sl_int64 v) noexcept;

		BigInt& operator/=(sl_uint64 v) noexcept;

		BigInt& operator%=(const BigInt& other) noexcept;

		BigInt& operator%=(sl_int32 v) noexcept;

		BigInt& operator<<=(sl_uint32 n) noexcept;

		BigInt& operator>>=(sl_uint32 n) noexcept;
	
	};
	
	
	sl_bool operator==(const BigInt& a, const BigInt& b) noexcept;
	
	sl_bool operator==(const BigInt& a, sl_int32 v) noexcept;
	
	sl_bool operator==(sl_int32 v, const BigInt& b) noexcept;
	
	sl_bool operator==(const BigInt& a, sl_uint32 v) noexcept;
	
	sl_bool operator==(sl_uint32 v, const BigInt& b) noexcept;
	
	sl_bool operator==(const BigInt& a, sl_int64 v) noexcept;
	
	sl_bool operator==(sl_int64 v, const BigInt& b) noexcept;
	
	sl_bool operator==(const BigInt& a, sl_uint64 v) noexcept;
	
	sl_bool operator==(sl_uint64 v, const BigInt& b) noexcept;
	
	
	sl_bool operator!=(const BigInt& a, const BigInt& b) noexcept;
	
	sl_bool operator!=(const BigInt& a, sl_int32 v) noexcept;
	
	sl_bool operator!=(sl_int32 v, const BigInt& b) noexcept;
	
	sl_bool operator!=(const BigInt& a, sl_uint32 v) noexcept;
	
	sl_bool operator!=(sl_uint32 v, const BigInt& b) noexcept;
	
	sl_bool operator!=(const BigInt& a, sl_int64 v) noexcept;
	
	sl_bool operator!=(sl_int64 v, const BigInt& b) noexcept;
	
	sl_bool operator!=(const BigInt& a, sl_uint64 v) noexcept;
	
	sl_bool operator!=(sl_uint64 v, const BigInt& b) noexcept;
	
	
	sl_bool operator>=(const BigInt& a, const BigInt& b) noexcept;
	
	sl_bool operator>=(const BigInt& a, sl_int32 v) noexcept;
	
	sl_bool operator>=(sl_int32 v, const BigInt& b) noexcept;
	
	sl_bool operator>=(const BigInt& a, sl_uint32 v) noexcept;
	
	sl_bool operator>=(sl_uint32 v, const BigInt& b) noexcept;
	
	sl_bool operator>=(const BigInt& a, sl_int64 v) noexcept;
	
	sl_bool operator>=(sl_int64 v, const BigInt& b) noexcept;
	
	sl_bool operator>=(const BigInt& a, sl_uint64 v) noexcept;
	
	sl_bool operator>=(sl_uint64 v, const BigInt& b) noexcept;
	
	
	sl_bool operator>(const BigInt& a, const BigInt& b) noexcept;
	
	sl_bool operator>(const BigInt& a, sl_int32 v) noexcept;
	
	sl_bool operator>(sl_int32 v, const BigInt& b) noexcept;
	
	sl_bool operator>(const BigInt& a, sl_uint32 v) noexcept;
	
	sl_bool operator>(sl_uint32 v, const BigInt& b) noexcept;
	
	sl_bool operator>(const BigInt& a, sl_int64 v) noexcept;
	
	sl_bool operator>(sl_int64 v, const BigInt& b) noexcept;
	
	sl_bool operator>(const BigInt& a, sl_uint64 v) noexcept;
	
	sl_bool operator>(sl_uint64 v, const BigInt& b) noexcept;
	
	
	sl_bool operator<=(const BigInt& a, const BigInt& b) noexcept;
	
	sl_bool operator<=(const BigInt& a, sl_int32 v) noexcept;
	
	sl_bool operator<=(sl_int32 v, const BigInt& b) noexcept;
	
	sl_bool operator<=(const BigInt& a, sl_uint32 v) noexcept;
	
	sl_bool operator<=(sl_uint32 v, const BigInt& b) noexcept;
	
	sl_bool operator<=(const BigInt& a, sl_int64 v) noexcept;
	
	sl_bool operator<=(sl_int64 v, const BigInt& b) noexcept;
	
	sl_bool operator<=(const BigInt& a, sl_uint64 v) noexcept;
	
	sl_bool operator<=(sl_uint64 v, const BigInt& b) noexcept;
	
	
	sl_bool operator<(const BigInt& a, const BigInt& b) noexcept;
	
	sl_bool operator<(const BigInt& a, sl_int32 v) noexcept;
	
	sl_bool operator<(sl_int32 v, const BigInt& b) noexcept;
	
	sl_bool operator<(const BigInt& a, sl_uint32 v) noexcept;
	
	sl_bool operator<(sl_uint32 v, const BigInt& b) noexcept;
	
	sl_bool operator<(const BigInt& a, sl_int64 v) noexcept;
	
	sl_bool operator<(sl_int64 v, const BigInt& b) noexcept;
	
	sl_bool operator<(const BigInt& a, sl_uint64 v) noexcept;
	
	sl_bool operator<(sl_uint64 v, const BigInt& b) noexcept;
	
	
	BigInt operator+(const BigInt& a, const BigInt& b) noexcept;
	
	BigInt operator+(const BigInt& a, sl_int32 v) noexcept;
	
	BigInt operator+(sl_int32 v, const BigInt& b) noexcept;
	
	BigInt operator+(const BigInt& a, sl_uint32 v) noexcept;
	
	BigInt operator+(sl_uint32 v, const BigInt& b) noexcept;
	
	BigInt operator+(const BigInt& a, sl_int64 v) noexcept;
	
	BigInt operator+(sl_int64 v, const BigInt& b) noexcept;
	
	BigInt operator+(const BigInt& a, sl_uint64 v) noexcept;
	
	BigInt operator+(sl_uint64 v, const BigInt& b) noexcept;
	
	
	BigInt operator-(const BigInt& a, const BigInt& b) noexcept;
	
	BigInt operator-(const BigInt& a, sl_int32 v) noexcept;
	
	BigInt operator-(sl_int32 v, const BigInt& b) noexcept;
	
	BigInt operator-(const BigInt& a, sl_uint32 v) noexcept;
	
	BigInt operator-(sl_uint32 v, const BigInt& b) noexcept;
	
	BigInt operator-(const BigInt& a, sl_int64 v) noexcept;
	
	BigInt operator-(sl_int64 v, const BigInt& b) noexcept;
	
	BigInt operator-(const BigInt& a, sl_uint64 v) noexcept;
	
	BigInt operator-(sl_uint64 v, const BigInt& b) noexcept;
	
	BigInt operator-(const BigInt& a) noexcept;
	
	
	BigInt operator*(const BigInt& a, const BigInt& b) noexcept;
	
	BigInt operator*(const BigInt& a, sl_int32 v) noexcept;
	
	BigInt operator*(sl_int32 v, const BigInt& b) noexcept;
	
	BigInt operator*(const BigInt& a, sl_uint32 v) noexcept;
	
	BigInt operator*(sl_uint32 v, const BigInt& b) noexcept;
	
	BigInt operator*(const BigInt& a, sl_int64 v) noexcept;
	
	BigInt operator*(sl_int64 v, const BigInt& b) noexcept;
	
	BigInt operator*(const BigInt& a, sl_uint64 v) noexcept;
	
	BigInt operator*(sl_uint64 v, const BigInt& b) noexcept;
	
	
	BigInt operator/(const BigInt& a, const BigInt& b) noexcept;
	
	BigInt operator/(const BigInt& a, sl_int32 v) noexcept;
	
	BigInt operator/(sl_int32 v, const BigInt& b) noexcept;
	
	BigInt operator/(const BigInt& a, sl_uint32 v) noexcept;
	
	BigInt operator/(sl_uint32 v, const BigInt& b) noexcept;
	
	BigInt operator/(const BigInt& a, sl_int64 v) noexcept;
	
	BigInt operator/(sl_int64 v, const BigInt& b) noexcept;
	
	BigInt operator/(const BigInt& a, sl_uint64 v) noexcept;
	
	BigInt operator/(sl_uint64 v, const BigInt& b) noexcept;
	
	
	BigInt operator%(const BigInt& a, const BigInt& b) noexcept;
	
	sl_int32 operator%(const BigInt& a, sl_int32 v) noexcept;
	
	BigInt operator%(sl_int32 v, const BigInt& b) noexcept;
	
	BigInt operator%(sl_uint32 v, const BigInt& b) noexcept;
	
	sl_int64 operator%(const BigInt& a, sl_int64 v) noexcept;
	
	BigInt operator%(sl_int64 v, const BigInt& b) noexcept;
	
	BigInt operator%(sl_uint64 v, const BigInt& b) noexcept;
	
	
	BigInt operator<<(const BigInt& a, sl_size n) noexcept;
	
	BigInt operator>>(const BigInt& a, sl_size n) noexcept;
	
}

#endif

