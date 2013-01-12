#pragma once
#ifndef HEX_OBJECT_FWD_HPP_INCLUDED
#define HEX_OBJECT_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

namespace hex {

	class hex_object;

	typedef boost::intrusive_ptr<hex_object> hex_object_ptr;
	typedef boost::intrusive_ptr<const hex_object> const_hex_object_ptr;

}

#endif // HEX_OBJECT_FWD_HPP_INCLUDED
