#include <queue>

#include "math.h"
#include "level.hpp"
#include "pathfinding.hpp"
#include "tile_map.hpp"
#include "unit_test.hpp"

namespace pathfinding {

template<> double manhattan_distance(const point& p1, const point& p2) {
	return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

template<> decimal manhattan_distance(const variant& p1, const variant& p2) {
	const std::vector<decimal>& v1 = p1.as_list_decimal();
	const std::vector<decimal>& v2 = p2.as_list_decimal();
	decimal x1 = v1[0] - v2[0];
	decimal x2 = v1[1] - v2[1];
	return (x1 < 0 ? -x1 : x1) + (x2 < 0 ? -x2 : x2);
}

variant directed_graph::get_value(const std::string& key) const {
	if(key == "vertices") {
		std::vector<variant> v(vertices_);
		return variant(&v);
	} else if(key == "edges") {
		std::vector<variant> edges;
		std::pair<variant, std::vector<variant> > edge;
		foreach(edge, edges_) {
			std::vector<variant> from_to;
			foreach(const variant& e1, edge.second) {
				from_to.push_back(edge.first);
				from_to.push_back(e1);
				edges.push_back(variant(&from_to));
			}
		}
		return variant(&edges);
	} else if(key == "edge_map") {
		std::map<variant, variant> edgemap;
		std::pair<variant, std::vector<variant> > edge;
		foreach(edge, edges_) {
			std::vector<variant> v(edge.second);
			edgemap[edge.first] = variant(&v);
		}
		return variant(&edgemap);
	}
	return variant();
}

variant weighted_directed_graph::get_value(const std::string& key) const {
	if(key == "weights") {
		std::map<variant, variant> w;
		std::pair<graph_edge, decimal> wit;
		foreach(wit, weights_) {
			std::vector<variant> from_to;
			from_to.push_back(wit.first.first);
			from_to.push_back(wit.first.second);
			w[variant(&from_to)] = variant(wit.second);
		}
		return variant(&w);
	} else if(key == "vertices"){
		return dg_->get_value(key);
	} else if(key == "edges") {
		return dg_->get_value(key);
	} else if(key == "edge_map") {
		return dg_->get_value(key);
	}
	return variant();
}

variant a_star_search(weighted_directed_graph* wg, 
	const variant src_node, 
	const variant dst_node, 
	game_logic::expression_ptr heuristic, 
	game_logic::map_formula_callable_ptr callable)
{
	//std::priority_queue<graph_node_ptr> open_list;
	std::vector<variant> path;
	std::deque<graph_node<variant, decimal>::graph_node_ptr> open_list;
	variant& a = callable->add_direct_access("a");
	variant& b = callable->add_direct_access("b");
	b = dst_node;

	if(src_node == dst_node) {
		return variant(&path);
	}

	bool searching = true;
	try {
		a = src_node;
		graph_node<variant, decimal>::graph_node_ptr current = wg->get_graph_node(src_node);
		current->set_cost(decimal::from_int(0), heuristic->evaluate(*callable).as_decimal());
		current->set_on_open_list(true);
		open_list.push_back(current);

		while(searching) {
			//std::cerr << "OPEN_LIST:\n"; 
			//foreach(graph_node_ptr g, open_list) {
			//	std::cerr << *g; 
			//}

			if(open_list.empty()) {
				// open list is empty node not found.
				PathfindingException<variant> path_error = {
					"Open list was empty -- no path found. ", 
					src_node, 
					dst_node
				};
				throw path_error;
			}
			current = open_list.front(); open_list.pop_front();
			current->set_on_open_list(false);

			if(current->get_node_value() == dst_node) {
				// Found the path to our node.
				searching = false;
				graph_node<variant, decimal>::graph_node_ptr p = current->get_parent();
				path.push_back(dst_node);
				while(p != 0) {
					path.insert(path.begin(),p->get_node_value());
					p = p->get_parent();
				}
				searching = false;
			} else {
				// Push lowest f node to the closed list so we don't consider it anymore.
				current->set_on_closed_list(true);
				foreach(const variant& e, wg->get_edges_from_node(current->get_node_value())) {
					graph_node<variant, decimal>::graph_node_ptr neighbour_node = wg->get_graph_node(e);
					if(neighbour_node->on_closed_list()) {
						if(current->G() < neighbour_node->G()) {
							neighbour_node->G(current->G() + wg->get_weight(current->get_node_value(), e));
							neighbour_node->set_parent(current);
						}
					} else {
						// not on closed list.
						if(neighbour_node->on_open_list()) {
							// on open list already.
							if(current->G() < neighbour_node->G()) {
								neighbour_node->G(current->G());
								neighbour_node->set_parent(current);
							}
						} else {
							// not on open or closed lists.
							a = e;
							neighbour_node->set_parent(current);
							neighbour_node->set_cost(wg->get_weight(current->get_node_value(), e) + current->G(), 
								heuristic->evaluate(*callable).as_decimal());
							neighbour_node->set_on_open_list(true);
							open_list.push_back(neighbour_node);
						}
					}
					std::sort(open_list.begin(), open_list.end(), graph_node_cmp<variant, decimal>);
				}
			}
		}
	} catch (PathfindingException<variant>& e) {
		std::cerr << e.msg << " " << e.src.to_debug_string() << ", " << e.dest.to_debug_string() << std::endl;
	}
	wg->reset_graph();
	return variant(&path);
}

point get_midpoint(const point& src_pt, const int tile_size_x, const int tile_size_y) {
	return point(int(src_pt.x/tile_size_x)*tile_size_x + tile_size_x/2,
		int(src_pt.y/tile_size_y)*tile_size_y + tile_size_y/2);
}

variant point_as_variant_list(const point& pt) {
	std::vector<variant> v;
	v.push_back(variant(pt.x));
	v.push_back(variant(pt.y));
	return variant(&v);
}

// Calculate the neighbour set of rectangles from a point.
std::vector<point> get_neighbours_from_rect(const int mid_x, 
	const int mid_y, 
	const int tile_size_x, 
	const int tile_size_y, 
	const bool allow_diagonals) {
	// Use some outside knowledge to grab the bounding rect for the level
	const rect& b = level::current().boundaries();
	std::vector<point> res;
	if(mid_x - tile_size_x >= b.x()) {
		// west
		res.push_back(point(mid_x - tile_size_x, mid_y));
	} 
	if(mid_x + tile_size_x < b.x2()) {
		// east
		res.push_back(point(mid_x + tile_size_y, mid_y));
	} 
	if(mid_y - tile_size_y >= b.y()) {
		// north
		res.push_back(point(mid_x, mid_y - tile_size_y));
	}
	if(mid_y + tile_size_y < b.y2()) {
		// south
		res.push_back(point(mid_x, mid_y + tile_size_y));
	}
	if(allow_diagonals) {
		if(mid_x - tile_size_x >= b.x() && mid_y - tile_size_y >= b.y()) {
			// north-west
			res.push_back(point(mid_x - tile_size_x, mid_y - tile_size_y));
		} 
		if(mid_x + tile_size_x < b.x2() && mid_y - tile_size_y >= b.y()) {
			// north-east
			res.push_back(point(mid_x + tile_size_x, mid_y - tile_size_y));
		} 
		if(mid_x - tile_size_x >= b.x() && mid_y + tile_size_y < b.y2()) {
			// south-west
			res.push_back(point(mid_x - tile_size_x, mid_y + tile_size_y));
		}
		if(mid_x + tile_size_x < b.x2() && mid_y + tile_size_y < b.y2()) {
			// south-east
			res.push_back(point(mid_x + tile_size_x, mid_y + tile_size_y));
		}
	}
	return res;
}

double calc_weight(const point& p1, const point& p2) {
	return sqrt(double((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y)));
	//return abs(p1.x - p2.x) + abs(p1.y - p2.y);
	//int diags = abs(p1.x - p2.x) - abs(p1.y - p2.y);
}

void clip_pt_to_rect(point& pt, const rect& r) {
	if(pt.x < r.x())  {pt.x = r.x();}
	if(pt.x > r.x2()) {pt.x = r.x2();}
	if(pt.y < r.y())  {pt.y = r.y();}
	if(pt.y > r.y2()) {pt.y = r.y2();}
}

template<typename N, typename T>
bool graph_node_cmp(const typename graph_node<N,T>::graph_node_ptr& lhs, 
	const typename graph_node<N,T>::graph_node_ptr& rhs) {
	return lhs->F() < rhs->F();
}

variant a_star_find_path(const point& src_pt1, 
	const point& dst_pt1, 
	game_logic::expression_ptr heuristic, 
	game_logic::expression_ptr weight_expr, 
	game_logic::map_formula_callable_ptr callable, 
	const int tile_size_x, 
	const int tile_size_y) 
{
	std::vector<variant> path;
	std::deque<graph_node<point, double>::graph_node_ptr> open_list;
	typedef std::map<point, graph_node<point, double>::graph_node_ptr> graph_node_list;
	graph_node_list node_list;
	level& lvl = level::current();
	point src_pt(src_pt1), dst_pt(dst_pt1);
	clip_pt_to_rect(src_pt, level::current().boundaries());
	clip_pt_to_rect(dst_pt, level::current().boundaries());
	point src(get_midpoint(src_pt, tile_size_x, tile_size_y));
	point dst(get_midpoint(dst_pt, tile_size_x, tile_size_y));
	variant& a = callable->add_direct_access("a");
	variant& b = callable->add_direct_access("b");

	if(src == dst) {
		return variant(&path);
	}

	if(lvl.solid(src.x, src.y, tile_size_x, tile_size_y) || lvl.solid(dst.x, dst.y, tile_size_x, tile_size_y)) {
		return variant(&path);
	}

	bool searching = true;
	try {
		a = point_as_variant_list(src);
		b = point_as_variant_list(dst);
		graph_node<point, double>::graph_node_ptr current = boost::shared_ptr<graph_node<point, double> >(new graph_node<point, double>(src));
		current->set_cost(0.0, heuristic->evaluate(*callable).as_decimal().as_float());
		current->set_on_open_list(true);
		open_list.push_back(current);
		node_list[src] = current;

		while(searching) {
			if(open_list.empty()) {
				// open list is empty node not found.
				PathfindingException<point> path_error = {
					"Open list was empty -- no path found. ", 
					src, 
					dst
				};
				throw path_error;
			}
			current = open_list.front(); open_list.pop_front();
			current->set_on_open_list(false);

			//std::cerr << std::endl << "CURRENT: " << *current;
			//std::cerr << "OPEN_LIST:\n";
			//graph_node<point, double>::graph_node_ptr g;
			//foreach(g, open_list) {
			//	std::cerr << *g; 
			//}

			if(current->get_node_value() == dst) {
				// Found the path to our node.
				searching = false;
				graph_node<point, double>::graph_node_ptr p = current->get_parent();
				path.push_back(point_as_variant_list(dst_pt));
				while(p != 0) {
					point pt_to_add = p->get_node_value();
					if(pt_to_add == src) {
						pt_to_add = src_pt;
					}
					path.insert(path.begin(), point_as_variant_list(pt_to_add));
					p = p->get_parent();
				}
			} else {
				// Push lowest f node to the closed list so we don't consider it anymore.
				current->set_on_closed_list(true);
				// Search through all the neighbour nodes connected to this one.
				// XXX get_neighbours_from_rect should(?) implement a cache of the point to edges
				foreach(const point& p, get_neighbours_from_rect(current->get_node_value().x, current->get_node_value().y, tile_size_x, tile_size_y)) {
					if(!lvl.solid(p.x, p.y, tile_size_x, tile_size_y)) {
						graph_node_list::const_iterator neighbour_node = node_list.find(p);
						double g_cost = current->G();
						if(weight_expr) {
							a = point_as_variant_list(current->get_node_value());
							b = point_as_variant_list(p);
							g_cost += weight_expr->evaluate(*callable).as_decimal().as_float();
						} else {
							g_cost += calc_weight(p, current->get_node_value());
						}
						if(neighbour_node == node_list.end()) {
							// not on open or closed list (i.e. no mapping for it yet.
							a = point_as_variant_list(p);
							b = point_as_variant_list(dst);
							graph_node<point, double>::graph_node_ptr new_node = boost::shared_ptr<graph_node<point, double> >(new graph_node<point, double>(p));
							new_node->set_parent(current);
							new_node->set_cost(g_cost, heuristic->evaluate(*callable).as_decimal().as_float());
							new_node->set_on_open_list(true);
							node_list[p] = new_node;
							open_list.push_back(new_node);
						} else if(neighbour_node->second->on_closed_list() || neighbour_node->second->on_open_list()) {
							// on closed list.
							if(g_cost < neighbour_node->second->G()) {
								neighbour_node->second->G(g_cost);
								neighbour_node->second->set_parent(current);
							}
						} else {
							PathfindingException<point> path_error = {
								"graph node on list, but not on open or closed lists. ", 
								p, 
								dst_pt
							};
							throw path_error;
						}
					}
				}
				std::sort(open_list.begin(), open_list.end(), graph_node_cmp<point, double>);
			}
		}
	} catch (PathfindingException<point>& e) {
		std::cerr << e.msg << " (" << e.src.x << "," << e.src.y << ") : (" << e.dest.x << "," << e.dest.y << ")" << std::endl;
	}
	return variant(&path);
}

}

UNIT_TEST(directed_graph_function) {
	CHECK_EQ(game_logic::formula(variant("directed_graph(map(range(4), [value/2,value%2]), null).vertices")).execute(), game_logic::formula(variant("[[0,0],[0,1],[1,0],[1,1]]")).execute());
	CHECK_EQ(game_logic::formula(variant("directed_graph(map(range(4), [value/2,value%2]), filter(links(v), inside_bounds(value))).edges where links = def(v) [[v[0]-1,v[1]], [v[0]+1,v[1]], [v[0],v[1]-1], [v[0],v[1]+1]], inside_bounds = def(v) v[0]>=0 and v[1]>=0 and v[0]<2 and v[1]<2")).execute(), 
		game_logic::formula(variant("[[[0, 0], [1, 0]], [[0, 0], [0, 1]], [[0, 1], [1, 1]], [[0, 1], [0, 0]], [[1, 0], [0, 0]], [[1, 0], [1, 1]], [[1, 1], [0, 1]], [[1, 1], [1, 0]]]")).execute());
}

UNIT_TEST(weighted_graph_function) {
	CHECK_EQ(game_logic::formula(variant("weighted_graph(directed_graph(map(range(4), [value/2,value%2]), null), 10).vertices")).execute(), game_logic::formula(variant("[[0,0],[0,1],[1,0],[1,1]]")).execute());
}
