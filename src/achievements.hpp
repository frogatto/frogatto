#ifndef ACHIEVEMENTS_HPP_INCLUDED
#define ACHIEVEMENTS_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <string>

#include "variant.hpp"

class achievement;

typedef boost::shared_ptr<const achievement> achievement_ptr;

class achievement
{
public:
	static achievement_ptr get(const std::string& id);

	explicit achievement(variant node);

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	const std::string& description() const { return description_; }
	int points() const { return points_; }
	int of_id() const { return of_id_; }
private:
	std::string id_, name_, description_;
	int points_, of_id_;
};

bool attain_achievement(const std::string& id);

#endif
