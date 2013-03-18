//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_REQUEST_HANDLER_HPP
#define HTTP_SERVER_REQUEST_HANDLER_HPP

#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <string>
#include <boost/noncopyable.hpp>
#include <json_spirit.h>

#include "connection_fwd.hpp"
#include "shared_data.hpp"

namespace http {
namespace server {

struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler
  : private boost::noncopyable
{
public:
  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root, 
	  game_server::shared_data& data);

  /// Handle a request and produce a reply.
  bool handle_request(const request& req, reply& rep, http::server::connection_ptr conn);

  // Handle post's.
  bool handle_post_tbs(const request& req, reply& rep, http::server::connection_ptr conn);
  bool handle_post_user(const request& req, reply& rep, http::server::connection_ptr conn);
  // Handle get
  bool handle_get(const request& req, reply& rep, http::server::connection_ptr conn);

  // Check message queue and reply based on that.
  bool check_messages(const std::string& user, reply& rep, http::server::connection_ptr conn);
private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);

  // Data (game/client lists) shared with the worker thread.
  game_server::shared_data& data_;
};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_REQUEST_HANDLER_HPP
