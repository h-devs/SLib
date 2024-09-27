#include <slib.h>

using namespace slib;

static void test_decimal128_to_string__infinity(void)
{
	SLIB_ASSERT(Decimal128(0x7800000000000000, 0).toString() == "Infinity");
	SLIB_ASSERT(Decimal128(0xf800000000000000, 0).toString() == "-Infinity");
}

static void test_decimal128_to_string__nan(void)
{
	SLIB_ASSERT(Decimal128(0x7c00000000000000, 0).toString() == "NaN");
	SLIB_ASSERT(Decimal128(0xfc00000000000000, 0).toString() == "NaN");
	SLIB_ASSERT(Decimal128(0x7e00000000000000, 0).toString() == "NaN");
	SLIB_ASSERT(Decimal128(0xfe00000000000000, 0).toString() == "NaN");
	SLIB_ASSERT(Decimal128(0x7e00000000000000, 12).toString() == "NaN");
}

static void test_decimal128_to_string__regular(void)
{
	SLIB_ASSERT(Decimal128(0x3040000000000000, 0x0000000000000001).toString() == "1");
	SLIB_ASSERT(Decimal128(0x3040000000000000, 0x0000000000000000).toString() == "0");
	SLIB_ASSERT(Decimal128(0x3040000000000000, 0x0000000000000002).toString() == "2");
	SLIB_ASSERT(Decimal128(0xb040000000000000, 0x0000000000000001).toString() == "-1");
	SLIB_ASSERT(Decimal128(0xb040000000000000, 0x0000000000000000).toString() == "-0");
	SLIB_ASSERT(Decimal128(0x303e000000000000, 0x0000000000000001).toString() == "0.1");
	SLIB_ASSERT(Decimal128(0x3034000000000000, 0x00000000000004d2).toString() == "0.001234");
	SLIB_ASSERT(Decimal128(0x3040000000000000, 0x0000001cbe991a14).toString() == "123456789012");
	SLIB_ASSERT(Decimal128(0x302a000000000000, 0x00000000075aef40).toString() == "0.00123400000");
	SLIB_ASSERT(Decimal128(0x2ffc3cde6fff9732, 0xde825cd07e96aff2).toString() == "0.1234567890123456789012345678901234");
	SLIB_ASSERT(Decimal128(0x3040ffffffffffff, 0xffffffffffffffff).toString() == "5192296858534827628530496329220095");
}

static void test_decimal128_to_string__scientific(void)
{
	SLIB_ASSERT(Decimal128(0x5ffe314dc6448d93, 0x38c15b0a00000000).toString() == "1.000000000000000000000000000000000E+6144");
	SLIB_ASSERT(Decimal128(0x0000000000000000, 0x0000000000000001).toString() == "1E-6176");
	SLIB_ASSERT(Decimal128(0x8000000000000000, 0x0000000000000001).toString() == "-1E-6176");
	SLIB_ASSERT(Decimal128(0x3108000000000000, 0x000009184db63eb1).toString() == "9.999987654321E+112");
	SLIB_ASSERT(Decimal128(0x5fffed09bead87c0, 0x378d8e63ffffffff).toString() == "9.999999999999999999999999999999999E+6144");
	SLIB_ASSERT(Decimal128(0x0001ed09bead87c0, 0x378d8e63ffffffff).toString() == "9.999999999999999999999999999999999E-6143");
	SLIB_ASSERT(Decimal128(0x304c000000000000, 0x000000000000041a).toString() == "1.050E+9");
	SLIB_ASSERT(Decimal128(0x3042000000000000, 0x000000000000041a).toString() == "1.050E+4");
	SLIB_ASSERT(Decimal128(0x3040000000000000, 0x0000000000000069).toString() == "105");
	SLIB_ASSERT(Decimal128(0x3042000000000000, 0x0000000000000069).toString() == "1.05E+3");
	SLIB_ASSERT(Decimal128(0x3046000000000000, 0x0000000000000001).toString() == "1E+3");
}

static void test_decimal128_to_string__zeros(void)
{
	SLIB_ASSERT(Decimal128(0x3040000000000000, 0x0000000000000000).toString() == "0");
	SLIB_ASSERT(Decimal128(0x3298000000000000, 0x0000000000000000).toString() == "0E+300");
	SLIB_ASSERT(Decimal128(0x2b90000000000000, 0x0000000000000000).toString() == "0E-600");
}

static void test_decimal128_from_string__invalid_inputs(void)
{
	SLIB_ASSERT(Decimal128::fromString(".").isNaN());
	SLIB_ASSERT(Decimal128::fromString(".e").isNaN());
	SLIB_ASSERT(Decimal128::fromString("").isNaN());
	SLIB_ASSERT(Decimal128::fromString("invalid").isNaN());
	SLIB_ASSERT(Decimal128::fromString("in").isNaN());
	SLIB_ASSERT(Decimal128::fromString("i").isNaN());
	SLIB_ASSERT(Decimal128::fromString("E02").isNaN());
	SLIB_ASSERT(Decimal128::fromString("..1").isNaN());
	SLIB_ASSERT(Decimal128::fromString("1abcede").isNaN());
	SLIB_ASSERT(Decimal128::fromString("1.24abc").isNaN());
	SLIB_ASSERT(Decimal128::fromString("1.24abcE+02").isNaN());
	SLIB_ASSERT(Decimal128::fromString("1.24E+02abc2d").isNaN());
	SLIB_ASSERT(Decimal128::fromString("E+02").isNaN());
	SLIB_ASSERT(Decimal128::fromString("e+02").isNaN());
}

static void test_decimal128_from_string__nan(void)
{
	SLIB_ASSERT(Decimal128::fromString("NaN").isNaN());
	SLIB_ASSERT(Decimal128::fromString("+NaN").isNaN());
	SLIB_ASSERT(Decimal128::fromString("-NaN").isNaN());
	SLIB_ASSERT(Decimal128::fromString("-nan").isNaN());
	SLIB_ASSERT(Decimal128::fromString("1e").isNaN());
	SLIB_ASSERT(Decimal128::fromString("+nan").isNaN());
	SLIB_ASSERT(Decimal128::fromString("nan").isNaN());
	SLIB_ASSERT(Decimal128::fromString("Nan").isNaN());
	SLIB_ASSERT(Decimal128::fromString("+Nan").isNaN());
	SLIB_ASSERT(Decimal128::fromString("-Nan").isNaN());
}

static void test_decimal128_from_string__infinity(void)
{
	SLIB_ASSERT(Decimal128::fromString("Infinity").isPositiveInfinity());
	SLIB_ASSERT(Decimal128::fromString("+Infinity").isPositiveInfinity());
	SLIB_ASSERT(Decimal128::fromString("+Inf").isPositiveInfinity());
	SLIB_ASSERT(Decimal128::fromString("-Inf").isNegativeInfinity());
	SLIB_ASSERT(Decimal128::fromString("-Infinity").isNegativeInfinity());
}

static void test_decimal128_from_string__simple(void)
{
	SLIB_ASSERT(Decimal128::fromString("1") == Decimal128(0x3040000000000000, 0x0000000000000001));
	SLIB_ASSERT(Decimal128::fromString("-1") == Decimal128(0xb040000000000000, 0x0000000000000001));
	SLIB_ASSERT(Decimal128::fromString("0") == Decimal128(0x3040000000000000, 0x0000000000000000));
	SLIB_ASSERT(Decimal128::fromString("-0") == Decimal128(0xb040000000000000, 0x0000000000000000));
	SLIB_ASSERT(Decimal128::fromString("12345678901234567") == Decimal128(0x3040000000000000, 0x002bdc545d6b4b87));
	SLIB_ASSERT(Decimal128::fromString("989898983458") == Decimal128(0x3040000000000000, 0x000000e67a93c822));
	SLIB_ASSERT(Decimal128::fromString("-12345678901234567") == Decimal128(0xb040000000000000, 0x002bdc545d6b4b87));
	SLIB_ASSERT(Decimal128::fromString("0.12345") == Decimal128(0x3036000000000000, 0x0000000000003039));
	SLIB_ASSERT(Decimal128::fromString("0.0012345") == Decimal128(0x3032000000000000, 0x0000000000003039));
	SLIB_ASSERT(Decimal128::fromString("00012345678901234567") == Decimal128(0x3040000000000000, 0x002bdc545d6b4b87));
}

static void test_decimal128_from_string__scientific(void)
{
	SLIB_ASSERT(Decimal128::fromString("10e0") == Decimal128(0x3040000000000000, 0x000000000000000a));
	SLIB_ASSERT(Decimal128::fromString("1e1") == Decimal128(0x3042000000000000, 0x0000000000000001));
	SLIB_ASSERT(Decimal128::fromString("10e-1") == Decimal128(0x303e000000000000, 0x000000000000000a));
	SLIB_ASSERT(Decimal128::fromString("12345678901234567e6111") == Decimal128(0x5ffe000000000000, 0x002bdc545d6b4b87));
	SLIB_ASSERT(Decimal128::fromString("1e-6176") == Decimal128(0x0000000000000000, 0x0000000000000001));
	SLIB_ASSERT(Decimal128::fromString("-100E-10") == Decimal128(0xb02c000000000000, 0x0000000000000064));
	SLIB_ASSERT(Decimal128::fromString("10.50E8") == Decimal128(0x304c000000000000, 0x000000000000041a));
}

static void test_decimal128_from_string__large(void)
{
	SLIB_ASSERT(Decimal128::fromString("12345689012345789012345") == Decimal128(0x304000000000029d, 0x42da3a76f9e0d979));
	SLIB_ASSERT(Decimal128::fromString("1234567890123456789012345678901234") == Decimal128(0x30403cde6fff9732, 0xde825cd07e96aff2));
	SLIB_ASSERT(Decimal128::fromString("9.999999999999999999999999999999999E+6144") == Decimal128(0x5fffed09bead87c0, 0x378d8e63ffffffff));
	SLIB_ASSERT(Decimal128::fromString("9.999999999999999999999999999999999E-6143") == Decimal128(0x0001ed09bead87c0, 0x378d8e63ffffffff));
	SLIB_ASSERT(Decimal128::fromString("5.192296858534827628530496329220095E+33") == Decimal128(0x3040ffffffffffff, 0xffffffffffffffff));
}

static void test_decimal128_from_string__exponent_normalization(void)
{
	SLIB_ASSERT(Decimal128::fromString("1000000000000000000000000000000000000000") == Decimal128(0x304c314dc6448d93, 0x38c15b0a00000000));
	SLIB_ASSERT(Decimal128::fromString("10000000000000000000000000000000000") == Decimal128(0x3042314dc6448d93, 0x38c15b0a00000000));
	SLIB_ASSERT(Decimal128::fromString("1000000000000000000000000000000000") == Decimal128(0x3040314dc6448d93, 0x38c15b0a00000000));
	SLIB_ASSERT(Decimal128::fromString(
		"100000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"000000000000000000000000000000000000000000000000000000000000000000000"
		"0000000000000000000000000000000000")
		== Decimal128(0x37cc314dc6448d93, 0x38c15b0a00000000));
}

static void test_decimal128_from_string__zeros(void)
{
	SLIB_ASSERT(Decimal128::fromString("0") == Decimal128(0x3040000000000000, 0x0000000000000000));
	SLIB_ASSERT(Decimal128::fromString("0e-611") == Decimal128(0x2b7a000000000000, 0x0000000000000000));
	SLIB_ASSERT(Decimal128::fromString("0e+6000") == Decimal128(0x5f20000000000000, 0x0000000000000000));
	SLIB_ASSERT(Decimal128::fromString("-0e-1") == Decimal128(0xb03e000000000000, 0x0000000000000000));
}

static void test_decimal128_from_string__special(void)
{
	SLIB_ASSERT(Decimal128::fromString(StringView("12345678901234567abcd", 17)) == Decimal128(0x3040000000000000, 0x002bdc545d6b4b87));
	SLIB_ASSERT(Decimal128::fromString(StringView("989898983458abcd", 12)) == Decimal128(0x3040000000000000, 0x000000e67a93c822));
	SLIB_ASSERT(Decimal128::fromString(StringView("-12345678901234567abcd", 18)) == Decimal128(0xb040000000000000, 0x002bdc545d6b4b87));
}

int main(int argc, const char * argv[])
{
	test_decimal128_to_string__infinity();
	test_decimal128_to_string__nan();
	test_decimal128_to_string__regular();
	test_decimal128_to_string__scientific();
	test_decimal128_to_string__zeros();
	test_decimal128_from_string__invalid_inputs();
	test_decimal128_from_string__nan();
	test_decimal128_from_string__infinity();
	test_decimal128_from_string__simple();
	test_decimal128_from_string__scientific();
	test_decimal128_from_string__large();
	test_decimal128_from_string__exponent_normalization();
	test_decimal128_from_string__zeros();
	test_decimal128_from_string__special();
	return 0;
}
