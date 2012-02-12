#ifndef DECIMAL_HPP_INCLUDED
#define DECIMAL_HPP_INCLUDED

#include <iosfwd>
#include <inttypes.h>

static const int64_t DECIMAL_PRECISION = 1000000;
static const int64_t DECIMAL_PLACES = 6;

class decimal
{
public:
	static decimal from_int(int64_t v) { return decimal(v*DECIMAL_PRECISION); }
	decimal() : value_(0) {}
	explicit decimal(int64_t value) : value_(value) {}
	explicit decimal(double value) : value_(value*DECIMAL_PRECISION) {}

	int64_t value() const { return value_; }
	int as_int() const { return value_/DECIMAL_PRECISION; }
	double as_float() const { return value_/double(DECIMAL_PRECISION); }
	int64_t fractional() const { return value_%DECIMAL_PRECISION; }

	decimal operator-() const {
		return decimal(-value_);
	}

	friend decimal operator+(const decimal& a, const decimal& b);
	friend decimal operator-(const decimal& a, const decimal& b);
	friend decimal operator*(const decimal& a, const decimal& b);
	friend decimal operator/(const decimal& a, const decimal& b);

	void operator+=(decimal a) { *this = *this + a; } 
	void operator-=(decimal a) { *this = *this - a; } 
	void operator*=(decimal a) { *this = *this * a; } 
	void operator/=(decimal a) { *this = *this / a; }

	void operator+=(int a) { operator+=(decimal::from_int(a)); } 
	void operator-=(int a) { operator-=(decimal::from_int(a)); } 
	void operator*=(int a) { operator*=(decimal::from_int(a)); } 
	void operator/=(int a) { operator/=(decimal::from_int(a)); }

private:
	int64_t value_;
};

inline decimal operator+(const decimal& a, const decimal& b) {
	return decimal(a.value() + b.value());
}

inline decimal operator-(const decimal& a, const decimal& b) {
	return decimal(a.value() - b.value());
}

inline decimal operator*(const decimal& a, const decimal& b) {
	const int64_t va = a.value() > 0 ? a.value() : -a.value();
	const int64_t vb = b.value() > 0 ? b.value() : -b.value();

	const int64_t ia = va/DECIMAL_PRECISION;
	const int64_t ib = vb/DECIMAL_PRECISION;

	const int64_t fa = va%DECIMAL_PRECISION;
	const int64_t fb = vb%DECIMAL_PRECISION;

	const decimal result = decimal(ia*ib*DECIMAL_PRECISION + fa*ib + fb*ia + (fa*fb)/DECIMAL_PRECISION);
	if(a.value() < 0 && b.value() > 0 || b.value() < 0 && a.value() > 0) {
		return -result;
	} else {
		return result;
	}
}

decimal operator/(const decimal& a, const decimal& b);

inline bool operator==(const decimal& a, const decimal& b) {
	return a.value() == b.value();
}

inline bool operator!=(const decimal& a, const decimal& b) {
	return !operator==(a, b);
}

inline bool operator<=(const decimal& a, const decimal& b) {
	return a.value() <= b.value();
}

inline bool operator>=(const decimal& a, const decimal& b) {
	return b <= a;
}

inline bool operator<(const decimal& a, const decimal& b) {
	return !(b <= a);
}

inline bool operator>(const decimal& a, const decimal& b) {
	return !(a <= b);
}

inline decimal operator+(decimal a, int b) { return operator+(a, decimal::from_int(b)); }
inline decimal operator-(decimal a, int b) { return operator-(a, decimal::from_int(b)); }
inline decimal operator*(decimal a, int b) { return operator*(a, decimal::from_int(b)); }
inline decimal operator/(decimal a, int b) { return operator/(a, decimal::from_int(b)); }
inline bool operator<(decimal a, int b) { return operator<(a, decimal::from_int(b)); }
inline bool operator>(decimal a, int b) { return operator>(a, decimal::from_int(b)); }
inline bool operator<=(decimal a, int b) { return operator<=(a, decimal::from_int(b)); }
inline bool operator>=(decimal a, int b) { return operator>=(a, decimal::from_int(b)); }

inline decimal operator+(int a, decimal b) { return operator+(decimal::from_int(a), b); }
inline decimal operator-(int a, decimal b) { return operator-(decimal::from_int(a), b); }
inline decimal operator*(int a, decimal b) { return operator*(decimal::from_int(a), b); }
inline decimal operator/(int a, decimal b) { return operator/(decimal::from_int(a), b); }
inline bool operator<(int a, decimal b) { return operator<(decimal::from_int(a), b); }
inline bool operator>(int a, decimal b) { return operator>(decimal::from_int(a), b); }
inline bool operator<=(int a, decimal b) { return operator<=(decimal::from_int(a), b); }
inline bool operator>=(int a, decimal b) { return operator>=(decimal::from_int(a), b); }

std::ostream& operator<<(std::ostream& s, decimal d);

#endif
