/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef B2D_FFL_HPP_INCLUDED
#define B2D_FFL_HPP_INCLUDED

#ifdef USE_BOX2D

#include <Box2D/Box2D.h>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>

#include <vector>
#include <map>

#include "entity.hpp"
#include "formula_callable.hpp"
#include "geometry.hpp"
#include "variant.hpp"

namespace box2d
{
	class manager
	{
	public:
		manager();
		~manager();
	};

	class world;
	typedef boost::intrusive_ptr<world> world_ptr;
	typedef boost::intrusive_ptr<const world> const_world_ptr;
	class body;
	typedef boost::intrusive_ptr<body> body_ptr;
	typedef boost::intrusive_ptr<const body> const_body_ptr;
	class joint;
	typedef boost::intrusive_ptr<joint> joint_ptr;
	typedef boost::intrusive_ptr<const joint> const_joint_ptr;

	class destruction_listener : public b2DestructionListener
	{
	public:
		destruction_listener();
		void SayGoodbye(b2Joint* joint);
		void SayGoodbye(b2Fixture* fix);
	};

	class debug_draw : public b2Draw
	{
	public:
		debug_draw();
		void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
		void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
		void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
		void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
		void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
		void DrawTransform(const b2Transform& xf);
		void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color);
		void DrawString(int x, int y, const char* string, ...); 
		void DrawAABB(b2AABB* aabb, const b2Color& color);
	};

	class body : public game_logic::formula_callable
	{
	public:
		explicit body(const variant& b);
		virtual ~body();
		const b2Body& get_body() const { return *body_; }
		b2Body& get_body() { return *body_; }
		const boost::shared_ptr<const b2Body> get_body_ptr() const { return body_; }
		boost::shared_ptr<b2Body> get_body_ptr() { return body_; }
		b2Body* get_raw_body_ptr() { return body_.get(); }
		const b2BodyDef* get_body_definition() const { return &body_def_; }

		virtual variant get_value(const std::string&) const;
		virtual void set_value(const std::string& key, const variant& value);

		bool active() const;
		void set_active(bool actv=true);

		void finish_loading(entity_ptr e=NULL);
		boost::shared_ptr<b2FixtureDef> create_fixture(const variant& fix);

		variant write();
		variant fix_write();
		variant shape_write(const b2Shape* shape);
	protected:
	private:
		b2BodyDef body_def_;
		std::vector<boost::shared_ptr<b2FixtureDef> > fix_defs_;
		std::vector<boost::shared_ptr<b2Shape> > shape_list_;
		boost::shared_ptr<b2Body> body_;
	};

	class joint : public game_logic::formula_callable
	{
	public:
		explicit joint(b2Joint* j);
		virtual variant get_value(const std::string& key) const;
		virtual void set_value(const std::string& key, const variant& value);
		
		b2Joint* get_b2Joint() { return joint_; }
	private:
		b2Joint* joint_;
	};

	class world : public game_logic::formula_callable
	{
	public:
		world(const variant& w);
		virtual ~world();
		const b2World& get_world() const { return world_; }
		b2World& get_world() { return world_; }

		virtual variant get_value(const std::string&) const;
		virtual void set_value(const std::string& key, const variant& value);

		void finish_loading();
		void step(float time_step);

		joint_ptr find_joint_by_id(const std::string& key) const;

		float x1() const { return world_x1_; }
		float x2() const { return world_x2_; }
		float y1() const { return world_y1_; }
		float y2() const { return world_y2_; }

		float last_dt() const { return last_dt_; }
		float last_inv_dt() const { return last_inv_dt_; }
		void set_dt(float time_step);

		variant write();

		static b2World& current();
		static b2World* current_ptr();
		static const world& our_world();
		static world_ptr our_world_ptr();

		void set_as_current_world();
		static void clear_current_world();

		b2Body* create_body(body*);

		int scale() const { return pixel_scale_; }
		void set_scale(int scale) { pixel_scale_ = scale; }

		bool draw_debug_data() const { return draw_debug_data_; }
		void enable_draw_debug_data(bool draw=true) { draw_debug_data_ = draw; }
	protected:
	private:
		int velocity_iterations_;
		int position_iterations_;
		b2World world_;

		float world_x1_, world_y1_;
		float world_x2_, world_y2_;

		float last_dt_;
		float last_inv_dt_;

		int pixel_scale_;

		bool draw_debug_data_;
		debug_draw debug_draw_;

		destruction_listener destruction_listener_;
	};
}

#endif
#endif
