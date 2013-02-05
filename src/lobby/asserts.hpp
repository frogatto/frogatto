#pragma once
#ifndef ASSERTS_HPP_INCLUDED
#define ASSERTS_HPP_INCLUDED

#include <iostream>
#include <sstream>

#if defined(NDEBUG) || defined(__linux__)
#define ASSERT_LOG(_a,_b)																\
	do {																				\
		if(!(_a)) {																		\
			std::ostringstream _s;														\
			_s << __FILE__ << ":" << __LINE__ << " ASSERTION FAILED: " << _b << "\n";	\
			std::cerr << _s.str();														\
			exit(1);																	\
		}																				\
	} while(0)
#else
#define ASSERT_LOG(_a,_b)																\
	do {																				\
		if(!(_a)) {																		\
			std::ostringstream _s;														\
			_s << __FILE__ << ":" << __LINE__ << " ASSERTION FAILED: " << _b << "\n";	\
			OutputDebugStringA(_s.str().c_str());										\
			exit(1);																	\
		}																				\
	} while(0)
#endif

#endif // ASSERTS_HPP_INCLUDED
