#ifndef FORMULA_CALLABLE_DEFINITION_FWD_HPP_INCLUDED
#define FORMULA_CALLABLE_DEFINITION_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace game_logic
{

class formula_callable_definition;

typedef boost::shared_ptr<formula_callable_definition> formula_callable_definition_ptr;
typedef boost::shared_ptr<const formula_callable_definition> const_formula_callable_definition_ptr;

}

#endif

