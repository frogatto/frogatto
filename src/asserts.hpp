#ifndef ASSERTS_HPP_INCLUDED
#define ASSERTS_HPP_INCLUDED

#include <iostream>
#include <stdlib.h>

//various asserts of standard "equality" tests, such as "equals", "not equals", "greater than", etc.  Example usage:
//ASSERT_NE(x, y);
#define ASSERT_EQ(a,b) if((a) != (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT EQ FAILED: " << #a << " != " << #b << ": " << (a) << " != " << (b) << "\n"; abort(); }

#define ASSERT_NE(a,b) if((a) == (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT NE FAILED: " << #a << " == " << #b << ": " << (a) << " == " << (b) << "\n"; abort(); }

#define ASSERT_GE(a,b) if((a) < (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GE FAILED: " << #a << " < " << #b << ": " << (a) << " < " << (b) << "\n"; abort(); }

#define ASSERT_LE(a,b) if((a) > (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LE FAILED: " << #a << " > " << #b << ": " << (a) << " > " << (b) << "\n"; abort(); }

#define ASSERT_GT(a,b) if((a) <= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GT FAILED: " << #a << " <= " << #b << ": " << (a) << " <= " << (b) << "\n"; abort(); }

#define ASSERT_LT(a,b) if((a) >= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LT FAILED: " << #a << " >= " << #b << ": " << (a) << " >= " << (b) << "\n"; abort(); }

#define ASSERT_INDEX_INTO_VECTOR(a,b) if((a) < 0 || (a) >= (b).size()) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT INDEX INTO VECTOR FAILED: " << #a << " indexes " << #b << ": " << (a) << " indexes " << (b).size(); abort(); }

//for custom logging.  Example usage:
//ASSERT_LOG(x != y, "x not equal to y. Value of x: " << x << ", y: " << y);
#define ASSERT_LOG(a,b) if( !(a) ) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERTION FAILED: " << b << "\n"; abort(); }


#endif
