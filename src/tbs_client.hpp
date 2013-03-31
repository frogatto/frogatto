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
#ifndef TBS_CLIENT_HPP_INCLUDED
#define TBS_CLIENT_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "formula_callable.hpp"
#include "http_client.hpp"

namespace tbs {
using boost::asio::ip::tcp;

class game;

class client : public http_client
{
public:
	client(const std::string& host, const std::string& port, int session=-1, boost::asio::io_service* service=NULL);

	void send_request(variant request, 
		game_logic::map_formula_callable_ptr callable, 
		boost::function<void(std::string)> handler);

	virtual void process();

	void set_use_local_cache(bool value) { use_local_cache_ = value; }
private:
	boost::function<void(std::string)> handler_;
	game_logic::map_formula_callable_ptr callable_;

	void recv_handler(const std::string& msg);
	void error_handler(const std::string& err);
	variant get_value(const std::string& key) const;

	bool use_local_cache_;
	tbs::game* local_game_cache_;
	boost::intrusive_ptr<game_logic::formula_callable> local_game_cache_holder_;
	int local_nplayer_;

	std::vector<std::string> local_responses_;
};

}

#endif
