#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "tbs_client.hpp"
#include "wml_formula_callable.hpp"

#if defined(_MSC_VER)
#define strtoll _strtoi64
#endif

namespace tbs {

void client::send_request(const std::string& request, game_logic::map_formula_callable_ptr callable, boost::function<void(std::string)> handler)
{
	handler_ = handler;
	callable_ = callable;

	http_client::send_request("POST /tbs", 
		request, 
		boost::bind(&client::recv_handler, this, _1), 
		boost::bind(&client::error_handler, this, _1), 
		0);
}

void client::recv_handler(const std::string& msg)
{
	if(handler_) {
		variant v;
		{
			const game_logic::wml_formula_callable_read_scope read_scope;
			v = json::parse(msg);
			if(v.has_key(variant("serialized_objects"))) {
				foreach(variant obj_node, v["serialized_objects"]["character"].as_list()) {
					game_logic::wml_serializable_formula_callable_ptr obj = obj_node.try_convert<game_logic::wml_serializable_formula_callable>();
					ASSERT_LOG(obj.get() != NULL, "ILLEGAL OBJECT FOUND IN SERIALIZATION");
					std::string addr_str = obj->addr();
					const intptr_t addr_id = strtoll(addr_str.c_str(), NULL, 16);

					game_logic::wml_formula_callable_read_scope::register_serialized_object(addr_id, obj);
				}
			}
		}
		//fprintf(stderr, "RECV: (((%s)))\n", msg.c_str());
		//fprintf(stderr, "SERIALIZE: (((%s)))\n", v.write_json().c_str());
		callable_->add("message", v);

//		try {
			handler_("message_received");
//		} catch(...) {
//			std::cerr << "ERROR PROCESSING TBS MESSAGE\n";
//			throw;
//		}
	}
}

void client::error_handler(const std::string& err)
{
	if(handler_) {
		callable_->add("error", json::parse(err, json::JSON_NO_PREPROCESSOR));
		handler_("connection_error");
	}
}

variant client::get_value(const std::string& key) const
{
	return http_client::get_value(key);
}

}
