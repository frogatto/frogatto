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