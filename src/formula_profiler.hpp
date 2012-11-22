#ifndef FORMULA_PROFILER_HPP_INCLUDED
#define FORMULA_PROFILER_HPP_INCLUDED

#include <string>

#ifdef DISABLE_FORMULA_PROFILER

namespace formula_profiler
{

void dump_instrumentation() {}

class instrument
{
public:
	explicit instrument(const char* id) {}
	~instrument() {}
}

//should be called every cycle while the profiler is running.
void pump();

class manager
{
public:
	explicit manager(const char* output_file) {}
	~manager() {}
};

class suspend_scope
{
};

inline std::string get_profile_summary() { return ""; }

}

#else

#include <vector>

#if defined(_WINDOWS)
#include "utils.hpp"
#else
#include <sys/time.h>
#endif

class custom_object_type;

namespace formula_profiler
{

//instruments inside a given scope.
class instrument
{
public:
	explicit instrument(const char* id);
	~instrument();
private:
	const char* id_;
	struct timeval tv_;
};

void dump_instrumentation();

//should be called every cycle while the profiler is running.
void pump();

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

void end_profiling();

class suspend_scope
{
public:
	suspend_scope() { event_call_stack.swap(backup_); }
	~suspend_scope() { event_call_stack.swap(backup_); }
private:
	event_call_stack_type backup_;
};

std::string get_profile_summary();

}

#endif

#endif
