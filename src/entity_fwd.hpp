#ifndef ENTITY_FWD_HPP_INCLUDED
#define ENTITY_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

class entity;

typedef boost::intrusive_ptr<entity> entity_ptr;
typedef boost::intrusive_ptr<const entity> const_entity_ptr;

#endif
