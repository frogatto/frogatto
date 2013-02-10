#ifndef DRAW_PRIMITIVES_HPP_INCLUDED
#define DRAW_PRIMITIVES_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"

namespace graphics
{

class draw_primitive : public game_logic::formula_callable
{
public:
	static boost::intrusive_ptr<draw_primitive> create(const variant& v);

	void draw() const;
private:

	virtual void handle_draw() const = 0;
};

typedef boost::intrusive_ptr<draw_primitive> draw_primitive_ptr;
typedef boost::intrusive_ptr<const draw_primitive> const_draw_primitive_ptr;

}

#endif
