#ifndef COLLISION_UTILS_HPP_INCLUDED
#define COLLISION_UTILS_HPP_INCLUDED

#include "solid_map.hpp"

class entity;
class level;

bool entity_collides_with_level(const level& lvl, const entity& e, MOVE_DIRECTION dir, int* friction=NULL, int* traction=NULL, int* damage=NULL);
int entity_collides_with_level_count(const level& lvl, const entity& e, MOVE_DIRECTION dir);

#endif
