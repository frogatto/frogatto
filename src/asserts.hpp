#ifndef ASSERTS_HPP_INCLUDED
#define ASSERTS_HPP_INCLUDED

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

#if defined(__ANDROID__)
#include <android/log.h>
#include <sstream>
#define LOG(str_data) \
    do{ std::stringstream oss; \
	    oss << str_data; \
	    __android_log_print(ANDROID_LOG_INFO, "Frogatto", oss.str().c_str()); }while(0)
#else
#define LOG(fmt,...) do {}while(0)
#endif // ANDROID

#if defined(_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef _DEBUG
//#define abort()		do{exit(1);}while(0)
#endif
#endif

void report_assert_msg(const std::string& m );

struct validation_failure_exception {
	explicit validation_failure_exception(const std::string& m);
	std::string msg;
};

bool throw_validation_failure_on_assert();

void output_backtrace();

class assert_recover_scope {
public:
	assert_recover_scope();
	~assert_recover_scope();
};

//various asserts of standard "equality" tests, such as "equals", "not equals", "greater than", etc.  Example usage:
//ASSERT_NE(x, y);
#define ASSERT_EQ(a,b) if((a) != (b)) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERT EQ FAILED: " << #a << " != " << #b << ": " << (a) << " != " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }

#define ASSERT_NE(a,b) if((a) == (b)) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERT NE FAILED: " << #a << " == " << #b << ": " << (a) << " == " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }

#define ASSERT_GE(a,b) if((a) < (b)) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERT GE FAILED: " << #a << " < " << #b << ": " << (a) << " < " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }

#define ASSERT_LE(a,b) if((a) > (b)) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERT LE FAILED: " << #a << " > " << #b << ": " << (a) << " > " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }

#define ASSERT_GT(a,b) if((a) <= (b)) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERT GT FAILED: " << #a << " <= " << #b << ": " << (a) << " <= " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }

#define ASSERT_LT(a,b) if((a) >= (b)) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERT LT FAILED: " << #a << " >= " << #b << ": " << (a) << " >= " << (b) << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }

#define ASSERT_INDEX_INTO_VECTOR(a,b) if((a) < 0 || (a) >= (b).size()) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERT INDEX INTO VECTOR FAILED: " << #a << " (" << (a) << " indexes " << #b << " (" << (b).size() << ")\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }

//for custom logging.  Example usage:
//ASSERT_LOG(x != y, "x not equal to y. Value of x: " << x << ", y: " << y);
#define ASSERT_LOG(a,b) if( !(a) ) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " ASSERTION FAILED: " << b << "\n"; if(throw_validation_failure_on_assert()) { throw validation_failure_exception(s.str()); } else { std::cerr << s.str(); output_backtrace(); report_assert_msg(s.str()); abort(); } }


#define VALIDATE_LOG(a,b) if( !(a) ) { std::ostringstream s; s << __FILE__ << ":" << __LINE__ << " VALIDATION FAILED: " << b << "\n"; throw validation_failure_exception(s.str()); }


#endif
