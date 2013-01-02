#ifndef GRID_WIDGET_FWD_HPP_INCLUDED
#define GRID_WIDGET_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

namespace gui {

class grid;
typedef boost::intrusive_ptr<grid> grid_ptr;
typedef boost::intrusive_ptr<const grid> const_grid_ptr;

}

#endif
