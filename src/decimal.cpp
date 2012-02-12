#include <stdio.h>
#include <sstream>
#include <iostream>

#include "decimal.hpp"

std::ostream& operator<<(std::ostream& s, decimal d)
{
	char buf[512];
	sprintf(buf, "%lld.%06llu", d.value()/DECIMAL_PRECISION, (d.value() > 0 ? d.value() : -d.value())%DECIMAL_PRECISION);
	std::cerr << "OUTPUT DECIMAL: " << d.value() << " -> " << buf << "\n";
	s << buf;
	return s;
}
