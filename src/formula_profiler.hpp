#ifndef FORMULA_PROFILER_HPP_INCLUDED
#define FORMULA_PROFILER_HPP_INCLUDED

#ifdef DISABLE_FORMULA_PROFILER

namespace formula_profiler
{

class manager
{
public:
	explicit manager(const char* output_file) {}
	~manager() {}
};

class suspend_scope
{
};

}

#else

#include <vector>

class custom_object_type;

namespace formula_profiler
{

struct custom_object_event_frame {
	const custom_object_type* type;
	int event_id;
	bool executing_commands;

	bool operator<(const custom_object_event_frame& f) const;
};

typedef std::vector<custom_object_event_frame> event_call_stack_type;
extern event_call_stack_type event_call_stack;

class manager
{
public:
	explicit manager(const char* output_file);
	~manager();
};

class suspend_scope
{
public:
	suspend_scope() { event_call_stack.swap(backup_); }
	~suspend_scope() { event_call_stack.swap(backup_); }
private:
	event_call_stack_type backup_;
};

}

#endif

#endif
