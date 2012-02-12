#include <stdio.h>
#include <sstream>

#include "decimal.hpp"

std::ostream& operator<<(std::ostream& s, decimal d)
{
	char buf[512];
	sprintf(buf, "%d.%06d", d.value()/DECIMAL_PRECISION, d.value()%DECIMAL_PRECISION);
	s << buf;
	return s;
}
