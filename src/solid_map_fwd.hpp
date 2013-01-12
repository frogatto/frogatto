#ifndef SOLID_MAP_FWD_HPP_INCLUDED
#define SOLID_MAP_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

class solid_map;

typedef boost::shared_ptr<solid_map> solid_map_ptr;
typedef boost::shared_ptr<const solid_map> const_solid_map_ptr;

class solid_info;

typedef boost::shared_ptr<const solid_info> const_solid_info_ptr;

#endif
