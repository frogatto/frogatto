#ifndef HTTP_CLIENT_HPP_INCLUDED
#define HTTP_CLIENT_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "formula_callable.hpp"

using boost::asio::ip::tcp;

class http_client : public game_logic::formula_callable
{
public:
	http_client(const std::string& host, const std::string& port, int session=-1);
	void send_request(const std::string& method_path,
	                  const std::string& request,
					  boost::function<void(std::string)> handler,
					  boost::function<void(std::string)> error_handler,
					  boost::function<void(int,int,bool)> progress_handler);
	void process();

protected:
	variant get_value(const std::string& key) const;

private:
	
	int session_id_;

	boost::asio::io_service io_service_;

	struct Connection {
		explicit Connection(boost::asio::io_service& serv) : socket(serv), nbytes_sent(0), expected_len(-1)
		{}
		tcp::socket socket;
		std::string method_path;
		std::string request, response;
		int nbytes_sent;
		boost::function<void(int,int,bool)> progress_handler;
		boost::function<void(std::string)> handler, error_handler;
		game_logic::map_formula_callable_ptr callable;

		boost::array<char, 1024> buf;
		
		int expected_len;
	};

	typedef boost::shared_ptr<Connection> connection_ptr;

	void handle_resolve(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator, connection_ptr conn);
	void handle_connect(const boost::system::error_code& error, connection_ptr conn, tcp::resolver::iterator resolve_itor);
	void write_connection_data(connection_ptr conn);
	void handle_send(connection_ptr conn, const boost::system::error_code& e, size_t nbytes);
	void handle_receive(connection_ptr conn, const boost::system::error_code& e, size_t nbytes);

	tcp::resolver resolver_;
	tcp::resolver::query resolver_query_;
	tcp::resolver::iterator endpoint_iterator_;
	std::string host_;

	int in_flight_;
};


#endif
