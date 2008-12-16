#ifndef ASSERTS_HPP_INCLUDED
#define ASSERTS_HPP_INCLUDED

#include <iostream>

//various asserts of standard "equality" tests, such as "equals", "not equals", "greater than", etc.  Example usage:
//assert_ne(x, y);
#define assert_eq(a,b) if((a) != (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT EQ FAILED: " << #a << " != " << #b << ": " << (a) << " != " << (b) << "\n"; abort(); }

#define assert_ne(a,b) if((a) == (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT NE FAILED: " << #a << " == " << #b << ": " << (a) << " == " << (b) << "\n"; abort(); }

#define assert_ge(a,b) if((a) < (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GE FAILED: " << #a << " < " << #b << ": " << (a) << " < " << (b) << "\n"; abort(); }

#define assert_le(a,b) if((a) > (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LE FAILED: " << #a << " > " << #b << ": " << (a) << " > " << (b) << "\n"; abort(); }

#define assert_gt(a,b) if((a) <= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GT FAILED: " << #a << " <= " << #b << ": " << (a) << " <= " << (b) << "\n"; abort(); }

#define assert_lt(a,b) if((a) >= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LT FAILED: " << #a << " >= " << #b << ": " << (a) << " >= " << (b) << "\n"; abort(); }

//for custom logging.  Example usage:
//assert_log(x != y, "x not equal to y. Value of x: " << x << ", y: " << y);
#define assert_log(a,b) if( !(a) ) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSSERTION FAILED: " << b << "\n"; abort(); }


#endif