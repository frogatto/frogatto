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
#include "tbs_internal_client.hpp"
#include "tbs_internal_server.hpp"

namespace tbs
{
	internal_client::internal_client(int session)
		: session_id_(session)
	{
	}

	internal_client::~internal_client()
	{
	}

	void internal_client::send_request(const variant& request, 
		int session_id,
		game_logic::map_formula_callable_ptr callable, 
		boost::function<void(const std::string&)> handler)
	{
		internal_server::send_request(request, session_id, callable, handler);
	}

	void internal_client::process()
	{
		// do nothing
	}

	variant internal_client::get_value(const std::string& key) const
	{
		return variant();
	}

}