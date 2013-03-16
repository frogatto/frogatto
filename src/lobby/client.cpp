#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "client.hpp"

namespace http 
{
	namespace client
	{
		bool client(const std::string& addr, const std::string& port, const request& req, reply& reply)
		{
			using boost::asio::ip::tcp;
			boost::asio::io_service io_service;

			// Get a list of endpoints corresponding to the server name.
			tcp::resolver resolver(io_service);
			tcp::resolver::query query(addr, port);
			tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
			tcp::resolver::iterator end;

			// Try each endpoint until we successfully establish a connection.
			tcp::socket socket(io_service);
			boost::system::error_code error = boost::asio::error::host_not_found;
			while(error && endpoint_iterator != end) {
				socket.close();
				socket.connect(*endpoint_iterator++, error);
			}
			if(error) {
				throw boost::system::system_error(error);
			}

			// Form the request. We specify the "Connection: close" header so that the
			// server will close the socket after transmitting the response. This will
			// allow us to treat all data up until the EOF as the content.
			boost::asio::streambuf request;
			std::ostream request_stream(&request);
			request_stream << "POST /tbs" << " HTTP/1.0\r\n";
			request_stream << "Host: " << addr << "\r\n";
			request_stream << "Accept: */*\r\n";
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
			BOOST_FOREACH(auto it, req.headers) {
#else
			for(auto it : req.headers) {		
#endif
				request_stream << it.name << ": " << it.value << "\r\n";
			}
			if(!req.body.empty()) {
				request_stream << "Content-Length: " << req.body.length() << "\r\n";
			}
			request_stream << "Connection: close\r\n\r\n";
			if(!req.body.empty()) {
				request_stream << req.body;
			}

			// Send the request.
			boost::asio::write(socket, request);

			// Read the response status line.
			boost::asio::streambuf response;
			boost::asio::read_until(socket, response, "\r\n");

			// Check that response is OK.
			std::istream response_stream(&response);
			std::string http_version;
			response_stream >> http_version;
			response_stream >> reply.status_code;
			std::string status_message;
			std::getline(response_stream, status_message);
			if(!response_stream || http_version.substr(0, 5) != "HTTP/") {
				std::cerr << "Invalid response\n";
				return false;
			}
			if(reply.status_code != 200) {
				std::cerr << "Response returned with status code " << reply.status_code << "\n";
				return false; 
			}
			size_t pos = http_version.substr(5).find('.');
			reply.http_version_major = boost::lexical_cast<int>(http_version.substr(5,pos));
			reply.http_version_minor = boost::lexical_cast<int>(http_version.substr(pos+6));

			// Read the response headers, which are terminated by a blank line.
			boost::asio::read_until(socket, response, "\r\n\r\n");

			// Process the response headers.
			std::string header;
			while(std::getline(response_stream, header) && header != "\r") {
				http::server::header h;
				size_t n = header.find(':');
				if(n == std::string::npos) {
					std::cerr << "Bad header: " << header;
					return false;
				}
				h.name = header.substr(0, n);
				h.value = header.substr(n+1);
				boost::algorithm::trim(h.value);
				reply.headers.push_back(h);
			}

			reply.body.clear();
			reply.content_length = 0;

			if(response.size() > 0) {
				std::stringstream str;
				str << &response;
				reply.body += str.str();
				reply.content_length += reply.body.length();
			}

			// Read until EOF, writing data to output as we go.
			while(boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
				// Dump the rest into the reply body and set content length.
				std::stringstream str;
				str << &response;
				reply.body += str.str();
				reply.content_length += reply.body.length();
			}
			if (error != boost::asio::error::eof) {
				return false;
				//throw boost::system::system_error(error);
			}

			return true;
		}
	}
}