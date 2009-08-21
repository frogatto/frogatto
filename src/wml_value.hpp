#ifndef WML_VALUE_HPP_INCLUDED
#define WML_VALUE_HPP_INCLUDED

#include <sstream>
#include <string>

namespace wml
{

class value
{
public:
	value() : line_(-1)
	{}

	value(const char* str, const std::string* fname=NULL, int line=-1)
	  : str_(str), fname_(fname), line_(line)
	{
	}

	value(const std::string& str, const std::string* fname=NULL, int line=-1)
	  : str_(str), fname_(fname), line_(line)
	{
	}

	operator const std::string&() const { return str_; }

	const std::string& str() const { return str_; }
	const std::string& val() const { return str_; }
	const std::string* filename() const { return fname_; }
	int line() const { return line_; }

	bool empty() const { return str_.empty(); }

	const char* c_str() const { return str_.c_str(); }
private:
	std::string str_;
	const std::string* fname_;
	int line_;
};

inline std::ostream& operator<<(std::ostream& s, const value& v) {
	s << v.val();
	return s;
}
}

#endif
