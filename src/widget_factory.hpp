#pragma once
#ifndef WIDGET_FACTORY_HPP_INCLUDED
#define WIDGET_FACTORY_HPP_INCLUDED

#include <boost/function.hpp>

#include "formula_callable.hpp"
#include "variant.hpp"
#include "widget.hpp"

namespace widget_factory {

gui::widget_ptr create(const variant& v, const game_logic::formula_callable_ptr& e);

}

#endif // WIDGET_FACTORY_HPP_INCLUDED
