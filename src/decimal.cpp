#include <stdio.h>
#include <sstream>

#include "decimal.hpp"
#include "unit_test.hpp"

#define DECIMAL(num) static_cast<int64_t>(num##LL)

decimal decimal::from_string(const std::string& s)
{
	bool negative = false;
	const char* ptr = s.c_str();
	if(*ptr == '-') {
		negative = true;
		++ptr;
	}
	char* endptr = NULL, *enddec = NULL;
	int64_t n = strtol(ptr, &endptr, 10);
	int64_t m = strtol(endptr+1, &enddec, 10);
	int dist = enddec - endptr;
	while(dist > (DECIMAL_PLACES+1)) {
		m /= 10;
		--dist;
	}
	while(dist < (DECIMAL_PLACES+1)) {
		m *= 10;
		++dist;
	}

	if(negative) {
		n = -n;
		m = -m;
	}

	int64_t result_value = n*DECIMAL_PRECISION + m;
	return decimal::from_raw_value(n*DECIMAL_PRECISION + m);
}

std::ostream& operator<<(std::ostream& s, decimal d)
{
	const char* minus = "";
	if(d.value() < 0 && d.value() > -DECIMAL_PRECISION) {
		//values between 0 and -1.0 won't have a minus sign, so correct that.
		minus = "-";
	}

	char buf[512];
	sprintf(buf, "%s%lld.%06lld", minus, static_cast<long long int>(d.value()/DECIMAL_PRECISION), static_cast<long long int>((d.value() > 0 ? d.value() : -d.value())%DECIMAL_PRECISION));
	char* ptr = buf + strlen(buf) - 1;
	while(*ptr == '0' && ptr[-1] != '.') {
		*ptr = 0;
		--ptr;
	}
	s << buf;
	return s;
}

decimal operator*(const decimal& a, const decimal& b)
{
	const int64_t va = a.value() > 0 ? a.value() : -a.value();
	const int64_t vb = b.value() > 0 ? b.value() : -b.value();

	const int64_t ia = va/DECIMAL_PRECISION;
	const int64_t ib = vb/DECIMAL_PRECISION;

	const int64_t fa = va%DECIMAL_PRECISION;
	const int64_t fb = vb%DECIMAL_PRECISION;

	const decimal result = decimal::from_raw_value(ia*ib*DECIMAL_PRECISION + fa*ib + fb*ia + (fa*fb)/DECIMAL_PRECISION);
	if((a.value() < 0 && b.value() > 0) || (b.value() < 0 && a.value() > 0)) {
		return -result;
	} else {
		return result;
	}
}

decimal operator/(const decimal& a, const decimal& b)
{
	int64_t va = a.value() > 0 ? a.value() : -a.value();
	int64_t vb = b.value() > 0 ? b.value() : -b.value();

	if(va == 0) {
		return a;
	}

	int64_t orders_of_magnitude_shift = 0;
	const int64_t target_value = DECIMAL(10000000000000);

	while(va < target_value) {
		va *= DECIMAL(10);
		++orders_of_magnitude_shift;
	}

	const int64_t target_value_b = DECIMAL(1000000);

	while(vb > target_value_b) {
		vb /= DECIMAL(10);
		++orders_of_magnitude_shift;
	}

	int64_t value = (va/vb);

	while(orders_of_magnitude_shift > 6) {
		value /= DECIMAL(10);
		--orders_of_magnitude_shift;
	}

	while(orders_of_magnitude_shift < 6) {
		value *= DECIMAL(10);
		++orders_of_magnitude_shift;
	}

	const decimal result(decimal::from_raw_value(value));
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

UNIT_TEST(decimal_from_string) {
	TestCase tests[] = {
		{ 5.5, "5.5" },
		{ -1.5, "-1.5" },
	};

	for(int n = 0; n != sizeof(tests)/sizeof(tests[0]); ++n) {
		CHECK_EQ(tests[n].value, decimal::from_string(tests[n].expected).as_float());
	}
}

UNIT_TEST(decimal_output) {
	TestCase tests[] = {
		{ 5.5, "5.5" },
		{ 4.0, "4.0" },
		{ -0.5, "-0.5" },
		{ -2.5, "-2.5" },
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
	CHECK_EQ(decimal::from_raw_value(DECIMAL(10934540000))*decimal::from_raw_value(DECIMAL(7649440000)), decimal::from_raw_value(DECIMAL(83643107657600)));
	CHECK_EQ(decimal::from_raw_value(-DECIMAL(10934540000))*decimal::from_raw_value(DECIMAL(7649440000)), -decimal::from_raw_value(DECIMAL(83643107657600)));

	CHECK_EQ(decimal::from_string("0.08")*decimal::from_string("0.5"), decimal::from_string("0.04"));
}

UNIT_TEST(decimal_div) {
	//10934.54 / 7649.44
	CHECK_EQ(decimal::from_raw_value(DECIMAL(10934540000))/decimal::from_raw_value(DECIMAL(7649440000)), decimal::from_raw_value(DECIMAL(1429456)));
}

BENCHMARK(decimal_div_bench) {
	BENCHMARK_LOOP {
		decimal res(decimal::from_raw_value(DECIMAL(0)));
		for(int n = 1; n < 1000000; ++n) {
			res += decimal::from_int(n)/decimal::from_int(1000100-n);
		}
	}
}
