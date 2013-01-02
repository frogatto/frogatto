#ifndef TBS_BOT_HPP_INCLUDED
#define TBS_BOT_HPP_INCLUDED

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "formula_callable.hpp"
#include "tbs_client.hpp"
#include "variant.hpp"

namespace tbs {

class bot : public game_logic::formula_callable
{
public:
	bot(boost::asio::io_service& io_service, const std::string& host, const std::string& port, variant v);
	~bot();

	void process();

private:
	void handle_response(const std::string& type, game_logic::formula_callable_ptr callable);
	variant get_value(const std::string& key) const;

	variant generate_report() const;

	std::string host_, port_;
	std::vector<variant> script_;
	std::vector<variant> response_;
	boost::shared_ptr<client> client_;

	boost::asio::io_service& service_;
	boost::asio::deadline_timer timer_;
};

}

#endif
