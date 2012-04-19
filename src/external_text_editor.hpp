#ifndef EXTERNAL_TEXT_EDITOR_HPP_INCLUDED
#define EXTERNAL_TEXT_EDITOR_HPP_INCLUDED

#include <string>

#include <boost/shared_ptr.hpp>

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
};

#endif
