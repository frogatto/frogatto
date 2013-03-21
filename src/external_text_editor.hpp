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
#ifndef EXTERNAL_TEXT_EDITOR_HPP_INCLUDED
#define EXTERNAL_TEXT_EDITOR_HPP_INCLUDED
#ifndef NO_EDITOR

#include <string>

#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "variant.hpp"

class external_text_editor;

typedef boost::shared_ptr<external_text_editor> external_text_editor_ptr;

class external_text_editor
{
public:
	struct manager {
		manager();
		~manager();
	};

	static external_text_editor_ptr create(variant key);

	external_text_editor();
	virtual ~external_text_editor();

	void process();

	bool replace_in_game_editor() const { return replace_in_game_editor_; }
	
	virtual void load_file(const std::string& fname) = 0;
	virtual void shutdown() = 0;
protected:
	struct editor_error {};
private:
	external_text_editor(const external_text_editor&);
	virtual std::string get_file_contents(const std::string& fname) = 0;
	virtual int get_line(const std::string& fname) const = 0;
	virtual std::vector<std::string> loaded_files() const = 0;

	bool replace_in_game_editor_;

	//As long as there's one of these things active, we're dynamically loading
	//in code, and so want to recover from asserts.
	assert_recover_scope assert_recovery_;
};

#endif // NO_EDITOR
#endif
