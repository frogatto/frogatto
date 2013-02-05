#pragma once
#ifndef HTTP_CLIENT_CLIENT_HPP
#define HTTP_CLIENT_CLIENT_HPP

#include <string>
#include <vector>

#include "header.hpp"

namespace http 
{
	namespace client
	{
		struct reply
		{
			int status_code;
			int http_version_major;
			int http_version_minor;
			std::vector<http::server::header> headers;
			std::string body;
			long content_length;
		};

		struct request
		{
			std::vector<http::server::header> headers;
			std::string body;
		};

		bool client(const std::string& addr, const std::string& port, const request& req, reply& reply);
	}
}

#endif