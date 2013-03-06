#pragma once

#include <boost/shared_ptr.hpp>

namespace http
{
	namespace server
	{
		class connection;
		typedef boost::shared_ptr<connection> connection_ptr;
	}
}
