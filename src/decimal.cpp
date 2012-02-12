#include <stdio.h>
#include <sstream>

#include "decimal.hpp"
#include "unit_test.hpp"


std::ostream& operator<<(std::ostream& s, decimal d)
{
	const char* minus = "";
	if(d.value() < 0 && d.value() > -DECIMAL_PRECISION) {
		//values between 0 and -1.0 won't have a minus sign, so correct that.
		minus = "-";
	}

	char buf[512];
	sprintf(buf, "%s%lld.%06llu", minus, d.value()/DECIMAL_PRECISION, (d.value() > 0 ? d.value() : -d.value())%DECIMAL_PRECISION);
	s << buf;
	return s;
}

decimal operator/(const decimal& a, const decimal& b)
{
	int64_t va = a.value() > 0 ? a.value() : -a.value();
	int64_t vb = b.value() > 0 ? b.value() : -b.value();

	int64_t orders_of_magnitude_shift = 0;
	const int64_t target_value = 10000000000000LL;

	while(va < target_value) {
		va *= 10LL;
		++orders_of_magnitude_shift;
	}

	while(vb&10 == 0) {
		vb /= 10LL;
		++orders_of_magnitude_shift;
	}

	const int64_t target_value_b = 1000000LL;

	while(vb > target_value_b) {
		vb /= 10LL;
		++orders_of_magnitude_shift;
	}

	int64_t value = (va/vb);

	while(orders_of_magnitude_shift > 6) {
		value /= 10LL;
		--orders_of_magnitude_shift;
	}

	while(orders_of_magnitude_shift < 6) {
		value *= 10LL;
		++orders_of_magnitude_shift;
	}

	const decimal result(value);
	if(a.value() < 0 && b.value() > 0 || b.value() < 0 && a.value() > 0) {
		return -result;
	} else {
		return result;
	}
}

namespace {
struct TestCase {
	double value;
	std::string expected;
};
}

UNIT_TEST(decimal_output) {
	TestCase tests[] = {
		{ 5.5, "5.500000" },
		{ 4.0, "4.000000" },
		{ -0.5, "-0.500000" },
		{ -2.5, "-2.500000" },
	};

	for(int n = 0; n != sizeof(tests)/sizeof(tests[0]); ++n) {
		std::ostringstream s;
		s << decimal(tests[n].value);
		CHECK_EQ(s.str(), tests[n].expected);
	}
}

UNIT_TEST(decimal_mul) {
	for(int64_t n = 0; n < 45000; n += 1000) {
		CHECK_EQ(n*(n > 0 ? n : -n), (decimal::from_int(n)*decimal::from_int(n > 0 ? n : -n)).as_int());
	}


	//10934.54 * 7649.44
	CHECK_EQ(decimal(10934540000LL)*decimal(7649440000LL), decimal(83643107657600LL));
	CHECK_EQ(decimal(-10934540000LL)*decimal(7649440000LL), -decimal(83643107657600LL));
}

UNIT_TEST(decimal_div) {
	//10934.54 / 7649.44
	CHECK_EQ(decimal(10934540000LL)/decimal(7649440000LL), decimal(1429456LL));
}
