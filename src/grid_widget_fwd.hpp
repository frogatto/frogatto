#ifndef GRID_WIDGET_FWD_HPP_INCLUDED
#define GRID_WIDGET_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace gui {

class grid;
typedef boost::shared_ptr<grid> grid_ptr;
typedef boost::shared_ptr<const grid> const_grid_ptr;

}

#endif
