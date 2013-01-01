#ifndef PREVIEW_TILESET_WIDGET_HPP_INCLUDED
#define PREVIEW_TILESET_WIDGET_HPP_INCLUDED

#include "level_object.hpp"
#include "widget.hpp"

class tile_map;

namespace gui {

class preview_tileset_widget : public widget
{
public:
	explicit preview_tileset_widget(const tile_map& tiles);
	explicit preview_tileset_widget(const variant& v, game_logic::formula_callable* e);
protected:
	void init();
	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;
private:
	void handle_draw() const;
	std::vector<level_tile> tiles_;
	int width_, height_;
};

typedef boost::intrusive_ptr<preview_tileset_widget> preview_tileset_widget_ptr;

}

#endif
