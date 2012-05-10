#pragma once
#ifndef PATHFINDING_HPP_INCLUDED
#define PATHFINDING_HPP_INCLUDED

#include <iostream>
#include <map>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "decimal.hpp"
#include "foreach.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "geometry.hpp"
#include "variant.hpp"

namespace pathfinding {

template<typename N>
struct PathfindingException {
	const char* msg;
	const N src;
	const N dest;
};

typedef variant_pair graph_edge;
typedef std::map<variant, std::vector<variant> > graph_edge_list;
typedef std::map<graph_edge, decimal> edge_weights;

class directed_graph;
class weighted_directed_graph;
typedef boost::intrusive_ptr<directed_graph> directed_graph_ptr;
typedef boost::intrusive_ptr<weighted_directed_graph> weighted_directed_graph_ptr;

template<typename N, typename T>
class graph_node {
public:
	typedef boost::shared_ptr<graph_node<N, T> > graph_node_ptr;
	graph_node(const N& src) 
		: src_(src), f_(0.0), g_(0.0), 
		h_(0.0), on_open_list_(false), on_closed_list_(false)
	{}
	graph_node(const N& src, T g, T h, graph_node_ptr parent) 
		: f_(g+h), g_(g), h_(h), src_(src), parent_(parent), 
		on_open_list_(false), on_closed_list_(false)
	{}
	const bool operator< (const graph_node& rhs) const { return f_ < rhs.f_;}
	N get_node_value() const {return src_;}
	T F() const {return f_;}
	T G() const {return g_;}
	T H() const {return h_;}
	void G(T g) {f_ += g - g_; g = g_;}
	void H(T h) {f_ += h - h_; h = h_;}
	void set_cost(T g, T h) {
		g_ = g;
		h_ = h;
		f_ = g+h;
	}
	void set_parent(graph_node_ptr parent) {parent_ = parent;}
	graph_node_ptr get_parent() const {return parent_;}
	void set_on_open_list(const bool val) {on_open_list_ = val;}
	bool on_open_list() const {return on_open_list_;}
	void set_on_closed_list(const bool val) {on_closed_list_ = val;}
	bool on_closed_list() const {return on_closed_list_;}
	void reset_node() {
		on_open_list_ = on_closed_list_ = false;
		f_ = T(0.0);
		g_ = T(0.0);
		h_ = T(0.0);
		parent_ = graph_node_ptr();
	}
	friend std::ostream& operator<<(std::ostream& out, const graph_node<N,T>& n) {
		out << "GNODE: " << n.src_.to_string() << " : cost( " << n.f_ << "," << n.g_ << "," << n.h_ 
			<< ") : parent(" << (n.parent_ == boost::shared_ptr<graph_node<N,T> >() ? "NULL" : n.parent_->get_node_value().to_string())
			<< ") : (" << n.on_open_list_ << "," << n.on_closed_list_ << ")" << std::endl;
		return out;
	}
private:
	T f_, g_, h_;
	N src_;
	graph_node_ptr parent_;
	bool on_open_list_;
	bool on_closed_list_;
};

template<typename N, typename T>
bool graph_node_cmp(const typename graph_node<N,T>::graph_node_ptr& lhs, 
	const typename graph_node<N,T>::graph_node_ptr& rhs);
template<typename N, typename T> T manhattan_distance(const N& p1, const N& p2);

typedef std::map<variant, graph_node<variant, decimal>::graph_node_ptr > vertex_list;

class directed_graph : public game_logic::formula_callable {
	std::vector<variant> vertices_;
	graph_edge_list edges_;
public:
	directed_graph(std::vector<variant>* vertices, 
		graph_edge_list* edges )
	{
		// Here we pilfer the contents of vertices and the edges.
		vertices_.swap(*vertices);
		edges_.swap(*edges);
	}
	variant get_value(const std::string& key) const;
	const graph_edge_list* get_edges() const {return &edges_;}
	std::vector<variant>& get_vertices() {return vertices_;}
	std::vector<variant> get_edges_from_node(const variant node) const {
		graph_edge_list::const_iterator e = edges_.find(node);
		if(e != edges_.end()) {
			return e->second;
		}
		return std::vector<variant>();
	}
};

class weighted_directed_graph : public game_logic::formula_callable {
	edge_weights weights_;
	directed_graph_ptr dg_;
	vertex_list graph_node_list_;
public:
	weighted_directed_graph(directed_graph_ptr dg, edge_weights* weights) 
		: dg_(dg)
	{
		weights_.swap(*weights);
		foreach(const variant& v, dg->get_vertices()) {
			graph_node_list_[v] = boost::shared_ptr<graph_node<variant, decimal> >(new graph_node<variant, decimal>(v));
		}
	}
	variant get_value(const std::string& key) const;
	std::vector<variant> get_edges_from_node(const variant node) const {
		return dg_->get_edges_from_node(node);
	}
	decimal get_weight(const variant& src, const variant& dest) const {
		edge_weights::const_iterator w = weights_.find(graph_edge(src,dest));
		if(w != weights_.end()) {
			return w->second;
		}
		PathfindingException<variant> weighted_graph_error = {"Couldn't find edge weight for nodes.", src, dest};
		throw weighted_graph_error;
	}
	graph_node<variant, decimal>::graph_node_ptr get_graph_node(const variant& src) {
		vertex_list::const_iterator it = graph_node_list_.find(src);
		if(it != graph_node_list_.end()) {
			return it->second;
		}
		PathfindingException<variant> src_not_found = {
			"weighted_directed_graph::get_graph_node() No node found having a value of ",
			src,
			variant()
		};
		throw src_not_found;
	}
	void reset_graph() {
		std::pair<variant, graph_node<variant, decimal>::graph_node_ptr> p;
		foreach(p, graph_node_list_) {
			p.second->reset_node();
		}
	}
};

std::vector<point> get_neighbours_from_rect(const int mid_x, 
	const int mid_y, 
	const int tile_size_x, 
	const int tile_size_y,
	const bool allow_diagonals = true);
variant point_as_variant_list(const point& pt);

variant a_star_search(weighted_directed_graph* wg, 
	const variant src_node, 
	const variant dst_node, 
	game_logic::expression_ptr heuristic, 
	game_logic::map_formula_callable_ptr callable);

variant a_star_find_path(const point& src, 
	const point& dst, 
	game_logic::expression_ptr heuristic, 
	game_logic::expression_ptr weight_expr, 
	game_logic::map_formula_callable_ptr callable, 
	const int tile_size_x, 
	const int tile_size_y);

}


#endif // PATHFINDING_HPP_INCLUDED
