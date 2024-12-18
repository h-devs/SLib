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
	class MemoryView;

/*
	Notice:

		CBigInt, BigInt is not thread-safe on modification operations
*/
	class SLIB_EXPORT CBigInt : public CRef
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

		sl_int32 makeNegative() noexcept;

		void makeBitwiseNot() noexcept;

		sl_bool getBit(sl_size pos) const noexcept;

		sl_bool setBit(sl_size pos, sl_bool bit) noexcept;

		sl_bool isEven() const noexcept;

		sl_bool isOdd() const noexcept;

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

		void moveFrom(CBigInt& other) noexcept;

		sl_bool compact() noexcept;

		sl_bool growLength(sl_size newLength) noexcept;

		sl_bool setLength(sl_size newLength) noexcept;

		sl_bool setValueFromElements(const sl_uint32* data, sl_size n) noexcept;

		// set/get data from/to bytes buffer, the sign is not changed
		sl_bool setBytesLE(const void* bytes, sl_size nBytes, sl_bool flagSigned = sl_false) noexcept;
		sl_bool setBytesBE(const void* bytes, sl_size nBytes, sl_bool flagSigned = sl_false) noexcept;

		void setBytesLE(const MemoryView& mem, sl_bool flagSigned = sl_false) noexcept;
		void setBytesBE(const MemoryView& mem, sl_bool flagSigned = sl_false) noexcept;

		static CBigInt* fromBytesLE(const void* bytes, sl_size nBytes, sl_bool flagSigned = sl_false) noexcept;
		static CBigInt* fromBytesBE(const void* bytes, sl_size nBytes, sl_bool flagSigned = sl_false) noexcept;

		static CBigInt* fromBytesLE(const MemoryView& mem, sl_bool flagSigned = sl_false) noexcept;
		static CBigInt* fromBytesBE(const MemoryView& mem, sl_bool flagSigned = sl_false) noexcept;

		// fill zeros in remaining spaces
		void getBytesLE(void* buf, sl_size n, sl_bool flagSigned = sl_false) const noexcept;
		void getBytesBE(void* buf, sl_size n, sl_bool flagSigned = sl_false) const noexcept;

		Memory getBytesLE(sl_bool flagSigned = sl_false) const noexcept;
		Memory getBytesBE(sl_bool flagSigned = sl_false) const noexcept;


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

		float getFloat() const noexcept;

		double getDouble() const noexcept;


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


		sl_bool shiftLeft(const CBigInt& other, sl_size n, const CBigInt* pM = sl_null) noexcept;

		sl_bool shiftLeftOneBit(const CBigInt& other, const CBigInt* pM = sl_null) noexcept;

		sl_bool shiftRight(const CBigInt& other, sl_size n) noexcept;

		sl_bool shiftLeft(sl_size n) noexcept;

		sl_bool shiftRight(sl_size n) noexcept;


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

		sl_bool mulMod(const CBigInt& a, const CBigInt& b, const CBigInt& M) noexcept;


		static sl_bool divAbs(const CBigInt& a, const CBigInt& b, CBigInt* quotient = sl_null, CBigInt* remainder = sl_null) noexcept;

		static sl_bool divAbs(const CBigInt& a, sl_uint32 b, CBigInt* quotient = sl_null, sl_uint32* remainder = sl_null) noexcept;

		static sl_bool div(const CBigInt& a, const CBigInt& b, CBigInt* quotient = sl_null, CBigInt* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		static sl_bool divInt32(const CBigInt& a, sl_int32 b, CBigInt* quotient = sl_null, sl_int32* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// always non-negative remainder
		static sl_bool divUint32(const CBigInt& a, sl_uint32 b, CBigInt* quotient = sl_null, sl_uint32* remainder = sl_null) noexcept;

		static sl_bool divInt64(const CBigInt& a, sl_int64 b, CBigInt* quotient = sl_null, sl_int64* remainder = sl_null, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// always non-negative remainder
		static sl_bool divUint64(const CBigInt& a, sl_uint64 b, CBigInt* quotient = sl_null, sl_uint64* remainder = sl_null) noexcept;

		sl_bool mod(const CBigInt& a, const CBigInt& M, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		sl_bool mod(const CBigInt& M, sl_bool flagNonNegativeRemainder = sl_false) noexcept;


		sl_bool bitwiseAnd(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool bitwiseAnd(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool bitwiseAnd(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool bitwiseAnd(const CBigInt& o) noexcept;

		sl_bool bitwiseAnd(sl_uint32 v) noexcept;

		sl_bool bitwiseAnd(sl_uint64 v) noexcept;


		sl_bool bitwiseXor(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool bitwiseXor(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool bitwiseXor(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool bitwiseXor(const CBigInt& o) noexcept;

		sl_bool bitwiseXor(sl_uint32 v) noexcept;

		sl_bool bitwiseXor(sl_uint64 v) noexcept;


		sl_bool bitwiseOr(const CBigInt& a, const CBigInt& b) noexcept;

		sl_bool bitwiseOr(const CBigInt& a, sl_uint32 v) noexcept;

		sl_bool bitwiseOr(const CBigInt& a, sl_uint64 v) noexcept;

		sl_bool bitwiseOr(const CBigInt& o) noexcept;

		sl_bool bitwiseOr(sl_uint32 v) noexcept;

		sl_bool bitwiseOr(sl_uint64 v) noexcept;

		/*
			E > 0
			M is not null => C = A^E mod M
			M is null => C = A^E
		*/
		sl_bool pow(const CBigInt& A, const CBigInt& E, const CBigInt* pM = sl_null) noexcept;

		sl_bool pow(const CBigInt& E, const CBigInt* pM = sl_null) noexcept;

		sl_bool powMod(const CBigInt& A, const CBigInt& E, const CBigInt& M) noexcept;

		sl_bool powMod(const CBigInt& E, const CBigInt& M) noexcept;

		sl_bool pow(const CBigInt& A, sl_uint32 E, const CBigInt* pM = sl_null) noexcept;

		sl_bool pow(sl_uint32 E, const CBigInt* pM = sl_null) noexcept;

		sl_bool powMod(const CBigInt& A, sl_uint32 E, const CBigInt& M) noexcept;

		sl_bool powMod(sl_uint32 E, const CBigInt& M) noexcept;


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
			Calculate R:
				R ^ 2 = A (mod M)
			Input:
				M is prime number.
		*/
		sl_bool sqrtMod(const CBigInt& A, const CBigInt& M) noexcept;

		sl_bool sqrtMod(const CBigInt& M) noexcept;

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


		sl_bool runOperator(sl_uint32 op, Variant& result, const Variant& secondOperand, sl_bool flagThisOnLeft) override;


		String toString(sl_uint32 radix, sl_bool flagUpperCase = sl_true) const noexcept;

		String toString() override;

		String toHexString(sl_bool flagUpperCase = sl_true) const noexcept;
	
	private:
		void _free() noexcept;

	};

	class BigInt;
	template <> class Atomic<BigInt>;

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

		static BigInt fromBytesLE(const void* bytes, sl_size nBytes, sl_bool flagSigned = sl_false) noexcept;

		static BigInt fromBytesLE(const MemoryView& mem, sl_bool flagSigned = sl_false) noexcept;

		static BigInt fromBytesBE(const void* bytes, sl_size nBytes, sl_bool flagSigned = sl_false) noexcept;

		static BigInt fromBytesBE(const MemoryView& mem, sl_bool flagSigned = sl_false) noexcept;

		static BigInt fromString(const StringParam& str, sl_uint32 radix = 10) noexcept;

		static BigInt fromHexString(const StringParam& str) noexcept;

		CBigInt& instance() const noexcept;

		BigInt duplicate() const noexcept;

		BigInt compact() const noexcept;

		sl_size getElementCount() const noexcept;

		sl_uint32* getElements() const noexcept;

		sl_int32 getSign() const noexcept;

		sl_bool getBit(sl_uint32 pos) const noexcept;

		sl_bool isEven() const noexcept;

		sl_bool isOdd() const noexcept;

		sl_size getMostSignificantElements() const noexcept;

		sl_size getLeastSignificantElements() const noexcept;

		sl_size getMostSignificantBytes() const noexcept;

		sl_size getLeastSignificantBytes() const noexcept;

		sl_size getMostSignificantBits() const noexcept;

		sl_size getLeastSignificantBits() const noexcept;

		sl_bool isZero() const noexcept;

		sl_bool isNotZero() const noexcept;

		// fill zeros in remaining spaces
		void getBytesLE(void* buf, sl_size n, sl_bool flagSigned = sl_false) const noexcept;

		Memory getBytesLE(sl_bool flagSigned = sl_false) const noexcept;

		// fill zeros in remaining spaces
		void getBytesBE(void* buf, sl_size n, sl_bool flagSigned = sl_false) const noexcept;

		Memory getBytesBE(sl_bool flagSigned = sl_false) const noexcept;

		sl_int32 getInt32() const noexcept;

		sl_uint32 getUint32() const noexcept;

		sl_int64 getInt64() const noexcept;

		sl_uint64 getUint64() const noexcept;

		float getFloat() const noexcept;

		double getDouble() const noexcept;

		String toString(sl_uint32 radix = 10, sl_bool flagUpperCase = sl_true) const noexcept;

		String toHexString(sl_bool flagUpperCase = sl_true) const noexcept;


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


		void makeBitwiseNot() const noexcept;

		BigInt bitwiseNot() const noexcept;


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


		static BigInt mod(const BigInt& A, const BigInt& B, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		sl_bool mod(const BigInt& other, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		static sl_int32 modInt32(const BigInt& A, sl_int32 v, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// non-negative remainder
		static sl_uint32 modUint32(const BigInt& A, sl_uint32 v) noexcept;

		static sl_int64 modInt64(const BigInt& A, sl_int64 v, sl_bool flagNonNegativeRemainder = sl_false) noexcept;

		// non-negative remainder
		static sl_uint64 modUint64(const BigInt& A, sl_uint64 v) noexcept;


		static BigInt bitwiseAnd(const BigInt& A, const BigInt& B) noexcept;

		sl_bool bitwiseAnd(const BigInt& other) noexcept;

		static BigInt bitwiseAnd(const BigInt& A, sl_uint32 v) noexcept;

		sl_bool bitwiseAnd(sl_uint32 v) noexcept;

		static BigInt bitwiseAnd(const BigInt& A, sl_uint64 v) noexcept;

		sl_bool bitwiseAnd(sl_uint64 v) noexcept;


		static BigInt bitwiseXor(const BigInt& A, const BigInt& B) noexcept;

		sl_bool bitwiseXor(const BigInt& other) noexcept;

		static BigInt bitwiseXor(const BigInt& A, sl_uint32 v) noexcept;

		sl_bool bitwiseXor(sl_uint32 v) noexcept;

		static BigInt bitwiseXor(const BigInt& A, sl_uint64 v) noexcept;

		sl_bool bitwiseXor(sl_uint64 v) noexcept;


		static BigInt bitwiseOr(const BigInt& A, const BigInt& B) noexcept;

		sl_bool bitwiseOr(const BigInt& other) noexcept;

		static BigInt bitwiseOr(const BigInt& A, sl_uint32 v) noexcept;

		sl_bool bitwiseOr(sl_uint32 v) noexcept;

		static BigInt bitwiseOr(const BigInt& A, sl_uint64 v) noexcept;

		sl_bool bitwiseOr(sl_uint64 v) noexcept;


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

		static BigInt powMod(const BigInt& A, const BigInt& E, const BigInt& M) noexcept;

		sl_bool powMod(const BigInt& E, const BigInt& M) noexcept;

		static BigInt pow(const BigInt& A, sl_uint32 E, const BigInt* pM = sl_null) noexcept;

		sl_bool pow(sl_uint32 E, const BigInt* pM = sl_null) noexcept;

		static BigInt powMod(const BigInt& A, sl_uint32 E, const BigInt& M) noexcept;

		sl_bool powMod(sl_uint32 E, const BigInt& M) noexcept;

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
			M: Prime
		*/
		static BigInt sqrtMod(const BigInt& A, const BigInt& M) noexcept;

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

#define PRIV_SLIB_DECLARE_BIGINT_OPERATORS(OP, RET, SUFFIX) \
		RET OP(const BigInt& other) SUFFIX noexcept; \
		RET OP(sl_int32 v) SUFFIX noexcept; \
		RET OP(sl_uint32 v) SUFFIX noexcept; \
		RET OP(sl_int64 v) SUFFIX noexcept; \
		RET OP(sl_uint64 v) SUFFIX noexcept;

#define PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(OP, RET, SUFFIX) \
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS(OP, RET, SUFFIX) \
		friend RET OP(sl_int32 v, const BigInt& n) noexcept; \
		friend RET OP(sl_uint32 v, const BigInt& n) noexcept; \
		friend RET OP(sl_int64 v, const BigInt& n) noexcept; \
		friend RET OP(sl_uint64 v, const BigInt& n) noexcept;

#define PRIV_SLIB_DECLARE_BIGINT_BITWISE_OPERATORS(OP) \
		BigInt OP(const BigInt& other) const noexcept; \
		BigInt OP(sl_uint32 v) const noexcept; \
		BigInt OP(sl_uint64 v) const noexcept; \
		friend BigInt OP(sl_uint32 v, const BigInt& n) noexcept; \
		friend BigInt OP(sl_uint64 v, const BigInt& n) noexcept;

		BigInt& operator=(sl_int32 n) noexcept;
		BigInt& operator=(sl_uint32 n) noexcept;
		BigInt& operator=(sl_int64 n) noexcept;
		BigInt& operator=(sl_uint64 n) noexcept;

		BigInt& operator++() noexcept;
		BigInt operator++(int) noexcept;
		BigInt& operator--() noexcept;
		BigInt operator--(int) noexcept;

		PRIV_SLIB_DECLARE_BIGINT_OPERATORS(operator+=, BigInt&,)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS(operator-=, BigInt&,)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS(operator*=, BigInt&,)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS(operator/=, BigInt&, )

		BigInt& operator%=(const BigInt& other) noexcept;
		BigInt& operator%=(sl_int32 v) noexcept;

		BigInt& operator<<=(sl_uint32 n) noexcept;
		BigInt& operator>>=(sl_uint32 n) noexcept;

		BigInt operator-() const noexcept;
		BigInt operator~() const noexcept;
		sl_bool operator!() const noexcept;
		explicit operator sl_bool() const noexcept;

		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator==, sl_bool, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator!= , sl_bool, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator> , sl_bool, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator<, sl_bool, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator>=, sl_bool, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator<=, sl_bool, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator+ , BigInt, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator-, BigInt, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator*, BigInt, const)
		PRIV_SLIB_DECLARE_BIGINT_OPERATORS_WITH_FRIENDS(operator/, BigInt, const)
		PRIV_SLIB_DECLARE_BIGINT_BITWISE_OPERATORS(operator&)
		PRIV_SLIB_DECLARE_BIGINT_BITWISE_OPERATORS(operator^)
		PRIV_SLIB_DECLARE_BIGINT_BITWISE_OPERATORS(operator|)

		BigInt operator%(const BigInt& other) const noexcept;
		sl_int32 operator%(sl_int32 v) const noexcept;
		sl_int64 operator%(sl_int64 v) const noexcept;
		friend BigInt operator%(sl_int32 v, const BigInt& n) noexcept;
		friend BigInt operator%(sl_uint32 v, const BigInt& n) noexcept;
		friend BigInt operator%(sl_int64 v, const BigInt& n) noexcept;
		friend BigInt operator%(sl_uint64 v, const BigInt& n) noexcept;

		BigInt operator<<(sl_size n) const noexcept;
		BigInt operator>>(sl_size n) const noexcept;

	};

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
		Atomic& operator=(sl_int32 n) noexcept;

		Atomic& operator=(sl_uint32 n) noexcept;

		Atomic& operator=(sl_int64 n) noexcept;

		Atomic& operator=(sl_uint64 n) noexcept;

	};

	typedef Atomic<BigInt> AtomicBigInt;


}

#endif

