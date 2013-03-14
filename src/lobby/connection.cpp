//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "request_handler.hpp"

namespace http {
namespace server {

connection::connection(boost::asio::io_service& io_service,
    request_handler& handler)
  : strand_(io_service),
    socket_(io_service),
    request_handler_(handler)
{
}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void connection::start()
{
  socket_.async_read_some(boost::asio::buffer(buffer_),
      strand_.wrap(
        boost::bind(&connection::handle_read, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred)));
}

void connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
  if (!e)
  {
    boost::tribool result;
	char* read_position;
    boost::tie(result, read_position) = request_parser_.parse(
        request_, buffer_.data(), buffer_.data() + bytes_transferred);

    if (result)
    {
	  auto it = std::find_if(request_.headers.begin(), request_.headers.end(), 
		  [](const header& v) { return v.name.compare("Content-Length") == 0; });

	  if(it != request_.headers.end()) {
#ifdef BOOST_NO_CXX11_NULLPTR
		  request_.content_length = strtol(it->value.c_str(), NULL, 10);
#else
		  request_.content_length = strtol(it->value.c_str(), nullptr, 10);
#endif
		  long bytes_remaining = buffer_.data() + bytes_transferred - read_position;
		  if(request_.content_length >= bytes_remaining) {
			  request_.body = std::string(read_position, read_position + request_.content_length);
		  } else {
			  request_.body = std::string(read_position, read_position + bytes_remaining);
			  socket_.async_read_some(boost::asio::buffer(buffer_),
				  strand_.wrap(
					boost::bind(&connection::handle_read_content, shared_from_this(),
					  boost::asio::placeholders::error,
					  boost::asio::placeholders::bytes_transferred)));
		  }
	  }

      if(request_handler_.handle_request(request_, reply_, shared_from_this())) {
		  std::cerr << "Reply: " << reply_.content << std::endl;
	      boost::asio::async_write(socket_, reply_.to_buffers(),
              strand_.wrap(
                boost::bind(&connection::handle_write, shared_from_this(),
                boost::asio::placeholders::error)));
	  } else {
		  std::cerr << "Reply deferred" << std::endl;
	  }
    }
    else if (!result)
    {
      reply_ = reply::stock_reply(reply::bad_request);
      boost::asio::async_write(socket_, reply_.to_buffers(),
          strand_.wrap(
            boost::bind(&connection::handle_write, shared_from_this(),
              boost::asio::placeholders::error)));
    }
    else
    {
      socket_.async_read_some(boost::asio::buffer(buffer_),
          strand_.wrap(
            boost::bind(&connection::handle_read, shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred)));
    }
  }

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the connection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The connection class's destructor closes the socket.
}

void connection::handle_read_content(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
	if(!e) {
		if(bytes_transferred >= request_.content_length + request_.body.length()) {
			request_.body += std::string(buffer_.data(), buffer_.data() + request_.content_length - request_.body.length());
		} else {
			request_.body += std::string(buffer_.data(), buffer_.data() + bytes_transferred);
			socket_.async_read_some(boost::asio::buffer(buffer_),
				strand_.wrap(
				boost::bind(&connection::handle_read_content, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
		}
	}
}

void connection::handle_write(const boost::system::error_code& e)
{
  if (!e)
  {
    // Initiate graceful connection closure.
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  // No new asynchronous operations are started. This means that all shared_ptr
  // references to the connection object will disappear and the object will be
  // destroyed automatically after this handler returns. The connection class's
  // destructor closes the socket.
}

void connection::handle_delayed_write()
{
	std::cerr << "Deferred Reply(" << this << "): " << reply_.content << std::endl;
	boost::asio::async_write(socket_, reply_.to_buffers(),
		strand_.wrap(
		boost::bind(&connection::handle_write, shared_from_this(),
		boost::asio::placeholders::error)));
}

} // namespace server
} // namespace http
