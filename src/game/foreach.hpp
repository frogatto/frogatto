#pragma once
#ifndef FOREACH_HPP_INCLUDED
#define FOREACH_HPP_INCLUDED

#include <boost/foreach.hpp>

#ifndef foreach
#define foreach BOOST_FOREACH
#endif

#ifndef reverse_foreach
#define reverse_foreach	BOOST_REVERSE_FOREACH
#endif

#endif // FOREACH_HPP_INCLUDED
