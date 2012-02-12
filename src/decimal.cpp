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
