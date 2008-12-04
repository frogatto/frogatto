#ifndef IMAGE_WIDGET_FWD_HPP_INCLUDED
#define IMAGE_WIDGET_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace gui {

class image_widget;
typedef boost::shared_ptr<image_widget> image_widget_ptr;
typedef boost::shared_ptr<const image_widget> const_image_widget_ptr;

}

#endif
