#ifndef COLLISION_UTILS_HPP_INCLUDED
#define COLLISION_UTILS_HPP_INCLUDED

#include "entity.hpp"
#include "level_solid_map.hpp"
#include "solid_map.hpp"

class level;

int get_num_solid_dimensions();
const std::string& get_solid_dimension_key(int id);
int get_solid_dimension_id(const std::string& key);

//struct which provides information about a surface we collide with.
struct collision_info {
	collision_info() : surf_info(0), friction(0), traction(0), damage(0), adjust_y(0), platform(false), area_id(0), collide_with_area_id(0)
	{}

	void read_surf_info();

	const surface_info* surf_info;
	int friction;
	int traction;
	int damage;

	//adjustment that should take place of the colliding object's position.
	//the reason for this is if the object is moving downwards, and at the
	//same time a platform is moving upwards. The platform will NOT check
	//for the downwards-moving object standing on it during its cycle, so
	//on the downwards-moving object's cycle it may already be below where
	//the platform is. This adjusts it so it is on top of the platform again.
	int adjust_y;

	//true iff the collided with area is a platform, rather than solid.
	bool platform;

	//the ID of the area of our body which collided.
	const std::string* area_id;

	//the object, if any, that we collided with. NULL if we collided with
	//a tile in the level.
	entity_ptr collide_with;

	//if collide_with is non-null this will contain the ID of the area
	//that we collided with.
	const std::string* collide_with_area_id;
};

//value of what kind of collision we are looking for. If we only want to
//know if there is a collision with solid space, or with platforms as well.
enum ALLOW_PLATFORM { SOLID_ONLY, SOLID_AND_PLATFORMS };

//function which finds it a given point can be stood on.
bool point_standable(const level& lvl, const entity& e, int x, int y, collision_info* info=NULL, ALLOW_PLATFORM allow_platform=SOLID_AND_PLATFORMS);

//function which finds if an entity's solid area collides with anything, when
//the object has just moved one pixel in the direction given by 'dir'. If
//'dir' is MOVE_NONE, then all pixels will be checked.
bool entity_collides(level& lvl, const entity& e, MOVE_DIRECTION dir, collision_info* info=NULL);

//function which finds if one entity collides with another given entity.
bool entity_collides_with_entity(const entity& e, const entity& other, collision_info* info=NULL);


//function which finds if an entity collides with a level tile.
bool entity_collides_with_level(const level& lvl, const entity& e, MOVE_DIRECTION dir, collision_info* info=NULL);

//function which finds how many pixels in an entity collide with the level.
//this is generally used for debug purposes.
int entity_collides_with_level_count(const level& lvl, const entity& e, MOVE_DIRECTION dir);

//function to try placing an entity in a level, without it colliding. The entity
//may be moved according to some heuristics to place it sensibly -- the object's
//location will be modified. Will return true iff it succeeds in placing it.
bool place_entity_in_level(level& lvl, entity& e);

//function to try to place an entity in a level, prioritizing finding a place
//to put it over keeping it near its starting point.
bool place_entity_in_level_with_large_displacement(level& lvl, entity& e);

//function which returns true iff an entity collides with the level in
//'non-solid' space. That is, if any of the entity's pixels collide with
//level solid space.
bool non_solid_entity_collides_with_level(const level& lvl, const entity& e);

//function which detects user collisions between two entities. All
//collision areas on the objects will be checked, and the results stored
//in areas_colliding. The function will return the number of collision
//combinations that were found.
typedef std::pair<const std::string*, const std::string*> collision_pair;
int entity_user_collision(const entity& a, const entity& b, collision_pair* areas_colliding, int buf_size);

//function which returns true iff area_a of 'a' collides with area_b of 'b'
bool entity_user_collision_specific_areas(const entity& a, const std::string& area_a, const entity& b, const std::string& area_b);

//function to detect all user collisions and fire appropriate events to
//the colliding objects.
void detect_user_collisions(level& lvl);

bool is_flightpath_clear(const level& lvl, const entity& e, const rect& area);

#endif
