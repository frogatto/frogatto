#pragma once
#ifndef URI_HPP_INCLUDED
#define URI_HPP_INCLUDED

#include <string>
#include <algorithm>

namespace uri {

struct uri
{
public:
	std::string query_string() const { return query_string_; }
	std::string path()  const { return path_; }
	std::string protocol()  const { return protocol_; }
	std::string host() const { return host_; }
	std::string port() const { return port_; }

	static uri parse(const std::string &url)
	{
		uri result;

		typedef std::string::const_iterator iterator_t;

		if (url.length() == 0)
			return result;

		iterator_t uriEnd = url.end();

		// get query start
		iterator_t queryStart = std::find(url.begin(), uriEnd, '?');

		// protocol
		iterator_t protocolStart = url.begin();
		iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');            //"://");

		if (protocolEnd != uriEnd)
		{
			std::string prot = &*(protocolEnd);
			if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
			{
				result.protocol_ = std::string(protocolStart, protocolEnd);
				protocolEnd += 3;   //      ://
			}
			else
				protocolEnd = url.begin();  // no protocol
		}
		else
			protocolEnd = url.begin();  // no protocol

		// host
		iterator_t hostStart = protocolEnd;
		iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart

		iterator_t hostEnd = std::find(protocolEnd, 
			(pathStart != uriEnd) ? pathStart : queryStart,
			':');  // check for port

		result.host_ = std::string(hostStart, hostEnd);

		// port
		if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == ':'))  // we have a port
		{
			hostEnd++;
			iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
			result.port_ = std::string(hostEnd, portEnd);
		} else {
			result.port_ = "80";
		}

		// path
		if (pathStart != uriEnd)
			result.path_ = std::string(pathStart, queryStart);

		// query
		if (queryStart != uriEnd)
			result.query_string_ = std::string(queryStart, url.end());

		return result;
	}   // Parse

private:
	std::string query_string_, path_, protocol_, host_, port_;
};  // uri

}

#endif
