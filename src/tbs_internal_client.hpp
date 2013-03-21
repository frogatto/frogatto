/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef TBS_INTERNAL_CLIENT_HPP_INCLUDED
#define TBS_INTERNAL_CLIENT_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "variant.hpp"

namespace tbs
{
	class internal_client : public game_logic::formula_callable
	{
	public:
		internal_client(int session=-1);
		virtual ~internal_client();
		virtual void send_request(const variant& request, 
			int session_id,
			game_logic::map_formula_callable_ptr callable, 
			boost::function<void(const std::string&)> handler);
		void process();
		int session_id() const { return session_id_; }
	protected:
		variant get_value(const std::string& key) const;
	private:
		int session_id_;
		boost::function<void(variant)> handler_;
		game_logic::map_formula_callable_ptr callable_;
	};

	typedef boost::intrusive_ptr<internal_client> internal_client_ptr;
	typedef boost::intrusive_ptr<const internal_client> const_internal_client_ptr;
}

#endif