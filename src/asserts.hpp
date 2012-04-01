#ifndef ASSERTS_HPP_INCLUDED
#define ASSERTS_HPP_INCLUDED

#include <iostream>
#include <sstream>
#include <stdlib.h>

struct validation_failure_exception {};

bool throw_validation_failure_on_assert();

void output_backtrace();

class assert_recover_scope {
public:
	assert_recover_scope();
	~assert_recover_scope();
};

//various asserts of standard "equality" tests, such as "equals", "not equals", "greater than", etc.  Example usage:
//ASSERT_NE(x, y);
#define ASSERT_EQ(a,b) if((a) != (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT EQ FAILED: " << #a << " != " << #b << ": " << (a) << " != " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }

#define ASSERT_NE(a,b) if((a) == (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT NE FAILED: " << #a << " == " << #b << ": " << (a) << " == " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }

#define ASSERT_GE(a,b) if((a) < (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GE FAILED: " << #a << " < " << #b << ": " << (a) << " < " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }

#define ASSERT_LE(a,b) if((a) > (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LE FAILED: " << #a << " > " << #b << ": " << (a) << " > " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }

#define ASSERT_GT(a,b) if((a) <= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GT FAILED: " << #a << " <= " << #b << ": " << (a) << " <= " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }

#define ASSERT_LT(a,b) if((a) >= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LT FAILED: " << #a << " >= " << #b << ": " << (a) << " >= " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }

#define ASSERT_INDEX_INTO_VECTOR(a,b) if((a) < 0 || (a) >= (b).size()) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT INDEX INTO VECTOR FAILED: " << #a << " (" << (a) << " indexes " << #b << " (" << (b).size() << ")\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }

//for custom logging.  Example usage:
//ASSERT_LOG(x != y, "x not equal to y. Value of x: " << x << ", y: " << y);
#define ASSERT_LOG(a,b) if( !(a) ) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERTION FAILED: " << b << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(); } else { output_backtrace(); abort(); } }


#define VALIDATE_LOG(a,b) if( !(a) ) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " VALIDATION FAILED: " << b << "\n"; throw validation_failure_exception(); }

#endif
