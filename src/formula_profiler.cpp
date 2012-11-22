#ifndef DISABLE_FORMULA_PROFILER

#include <SDL_thread.h>

#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <signal.h>
#include <stdio.h>
#include <string.h>
#if !defined( _WINDOWS )
#include <sys/time.h>
#else
#include <time.h>
#endif // defined( _WINDOWS )

#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula_profiler.hpp"
#include "object_events.hpp"
#include "variant.hpp"

namespace formula_profiler
{

namespace {
bool profiler_on = false;

struct InstrumentationRecord {
	InstrumentationRecord() : time_us(0), nsamples(0)
	{}
	int time_us, nsamples;
};

std::map<const char*, InstrumentationRecord> g_instrumentation;
}

instrument::instrument(const char* id) : id_(id)
{
	if(profiler_on) {
		gettimeofday(&tv_, NULL);
	}
}

instrument::~instrument()
{
	if(profiler_on) {
		struct timeval end_tv;
		gettimeofday(&end_tv, NULL);
		const int time_us = (end_tv.tv_sec - tv_.tv_sec)*1000000 + (end_tv.tv_usec - tv_.tv_usec);
		InstrumentationRecord& r = g_instrumentation[id_];
		r.time_us += (end_tv.tv_sec - tv_.tv_sec)*1000000 + (end_tv.tv_usec - tv_.tv_usec);
		r.nsamples++;
	}
}

void dump_instrumentation()
{
	static struct timeval prev_call;
	static bool first_call = true;

	struct timeval tv;
	gettimeofday(&tv, NULL);

	if(!first_call && g_instrumentation.empty() == false) {
		const int time_us = (tv.tv_sec - prev_call.tv_sec)*1000000 + (tv.tv_usec - prev_call.tv_usec);
		if(time_us) {
			fprintf(stderr, "FRAME INSTRUMENTATION TOTAL TIME: %dus. INSTRUMENTS: ", time_us);
			for(std::map<const char*,InstrumentationRecord>::const_iterator i = g_instrumentation.begin(); i != g_instrumentation.end(); ++i) {
				const int percent = (i->second.time_us*100)/time_us;
				fprintf(stderr, "%s: %dus (%d%%) in %d calls; ", i->first, i->second.time_us, percent, i->second.nsamples);
			}

			fprintf(stderr, "\n");
		}

		g_instrumentation.clear();
	}

	first_call = false;
	prev_call = tv;
}

event_call_stack_type event_call_stack;

namespace {
bool handler_disabled = false;
std::string output_fname;
int main_thread;

int empty_samples = 0;

std::map<std::vector<const game_logic::formula_expression*>, int> expression_call_stack_samples;
std::vector<const game_logic::formula_expression*> current_expression_call_stack;

std::vector<custom_object_event_frame> event_call_stack_samples;
int num_samples = 0;
const size_t max_samples = 10000;

int nframes_profiled = 0;

#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
SDL_TimerID sdl_profile_timer;
#endif

#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
Uint32 sdl_timer_callback(Uint32 interval, void *param)
#else
void sigprof_handler(int sig)
#endif
{
	//NOTE: Nothing in this function should allocate memory, since
	//we might be called while allocating memory.
#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
	if(handler_disabled) {
		return interval;
	}
#else
	if(handler_disabled || main_thread != SDL_GetThreadID(NULL)) {
		return;
	}
#endif

	if(current_expression_call_stack.empty() && current_expression_call_stack.capacity() >= get_expression_call_stack().size()) {
		//Very important that this doesnot allocate memory.
		current_expression_call_stack = get_expression_call_stack();
	}

	if(num_samples == max_samples) {
#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
		return interval;
#else
		return;
#endif
	}

	if(event_call_stack.empty()) {
		++empty_samples;
	} else {
		event_call_stack_samples[num_samples++] = event_call_stack.back();
	}
#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
	return interval;
#endif
}

}

manager::manager(const char* output_file)
{
	if(output_file) {
		current_expression_call_stack.reserve(10000);
		event_call_stack_samples.resize(max_samples);

		main_thread = SDL_GetThreadID(NULL);

		fprintf(stderr, "SETTING UP PROFILING...\n");
		profiler_on = true;
		output_fname = output_file;

#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
		// Crappy windows approximation.
		sdl_profile_timer = SDL_AddTimer(10, sdl_timer_callback, 0);
		if(sdl_profile_timer == NULL) {
			std::cerr << "Couldn't create a profiling timer!" << std::endl;
		}
#else
		signal(SIGPROF, sigprof_handler);

		struct itimerval timer;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 10000;
		timer.it_value = timer.it_interval;
		setitimer(ITIMER_PROF, &timer, 0);
#endif
	}
}

manager::~manager()
{
	end_profiling();
}

void end_profiling()
{
	if(profiler_on){
#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
		SDL_RemoveTimer(sdl_profile_timer);
#else
		struct itimerval timer;
		memset(&timer, 0, sizeof(timer));
		setitimer(ITIMER_PROF, &timer, 0);
#endif

		std::map<std::string, int> samples_map;


		for(int n = 0; n != num_samples; ++n) {
			const custom_object_event_frame& frame = event_call_stack_samples[n];

			std::string str = formatter() << frame.type->id() << ":" << get_object_event_str(frame.event_id) << ":" << (frame.executing_commands ? "CMD" : "FFL");

			samples_map[str]++;
		}

		std::vector<std::pair<int, std::string> > sorted_samples, cum_sorted_samples;
		for(std::map<std::string, int>::const_iterator i = samples_map.begin(); i != samples_map.end(); ++i) {
			sorted_samples.push_back(std::pair<int, std::string>(i->second, i->first));
		}

		std::sort(sorted_samples.begin(), sorted_samples.end());
		std::reverse(sorted_samples.begin(), sorted_samples.end());

		const int total_samples = empty_samples + num_samples;
		if(!total_samples) {
			return;
		}

		std::ostringstream s;
		s << "TOTAL SAMPLES: " << total_samples << "\n";
		s << (100*empty_samples)/total_samples << "% (" << empty_samples << ") CORE ENGINE (non-FFL processing)\n";

		for(int n = 0; n != sorted_samples.size(); ++n) {
			s << (100*sorted_samples[n].first)/total_samples << "% (" << sorted_samples[n].first << ") " << sorted_samples[n].second << "\n";
		}

		sorted_samples.clear();

		std::map<const game_logic::formula_expression*, int> expr_samples, cum_expr_samples;

		int total_expr_samples = 0;

		for(std::map<std::vector<const game_logic::formula_expression*>, int>::const_iterator i = expression_call_stack_samples.begin(); i != expression_call_stack_samples.end(); ++i) {
			const std::vector<const game_logic::formula_expression*>& sample = i->first;
			const int nsamples = i->second;
			if(sample.empty()) {
				continue;
			}

			foreach(const game_logic::formula_expression* fe, sample) {
				cum_expr_samples[fe] += nsamples;
			}

			expr_samples[sample.back()] += nsamples;

			total_expr_samples += nsamples;
		}

		for(std::map<const game_logic::formula_expression*, int>::const_iterator i = expr_samples.begin(); i != expr_samples.end(); ++i) {
			sorted_samples.push_back(std::pair<int, std::string>(i->second, formatter() << i->first->debug_pinpoint_location() << " (called " << double(i->first->ntimes_called())/double(nframes_profiled) << " times per frame)"));
		}

		for(std::map<const game_logic::formula_expression*, int>::const_iterator i = cum_expr_samples.begin(); i != cum_expr_samples.end(); ++i) {
			cum_sorted_samples.push_back(std::pair<int, std::string>(i->second, formatter() << i->first->debug_pinpoint_location() << " (called " << double(i->first->ntimes_called())/double(nframes_profiled) << " times per frame)"));
		}

		std::sort(sorted_samples.begin(), sorted_samples.end());
		std::reverse(sorted_samples.begin(), sorted_samples.end());

		std::sort(cum_sorted_samples.begin(), cum_sorted_samples.end());
		std::reverse(cum_sorted_samples.begin(), cum_sorted_samples.end());

		s << "\n\nPROFILE BROKEN DOWN INTO FFL EXPRESSIONS:\n\nTOTAL SAMPLES: " << total_expr_samples << "\n OVER " << nframes_profiled << " FRAMES\nSELF TIME:\n";

		for(int n = 0; n != sorted_samples.size(); ++n) {
			s << (100*sorted_samples[n].first)/total_expr_samples << "% (" << sorted_samples[n].first << ") " << sorted_samples[n].second << "\n";
		}

		s << "\n\nCUMULATIVE TIME:\n";
		for(int n = 0; n != cum_sorted_samples.size(); ++n) {
			s << (100*cum_sorted_samples[n].first)/total_expr_samples << "% (" << cum_sorted_samples[n].first << ") " << cum_sorted_samples[n].second << "\n";
		}

		if(!output_fname.empty()) {
			sys::write_file(output_fname, s.str());
		} else {
			std::cerr << "===\n=== PROFILE REPORT ===\n";
			std::cerr << s.str();
			std::cerr << "=== END PROFILE REPORT ===\n";
		}

		profiler_on = false;
	}
}

void pump()
{
	static int instr_count = 0;
	if(++instr_count%50 == 0) {
		dump_instrumentation();
	}

	if(current_expression_call_stack.empty() == false) {
		expression_call_stack_samples[current_expression_call_stack]++;
		current_expression_call_stack.clear();
	}

	++nframes_profiled;
}

bool custom_object_event_frame::operator<(const custom_object_event_frame& f) const
{
	return type < f.type || type == f.type && event_id < f.event_id ||
	       type == f.type && event_id == f.event_id && executing_commands < f.executing_commands;
}

std::string get_profile_summary()
{
	if(!profiler_on) {
		return "";
	}

	handler_disabled = true;

	static int last_empty_samples = 0;
	static int last_num_samples = 0;

	const int nsamples = num_samples - last_num_samples;
	const int nempty = empty_samples - last_empty_samples;

	std::sort(event_call_stack_samples.end() - nsamples, event_call_stack_samples.end());

	std::ostringstream s;

	s << "PROFILE: " << (nsamples + nempty) << " CPU. " << nsamples << " IN FFL ";


	std::vector<std::pair<int, std::string> > samples;
	int count = 0;
	for(int n = last_num_samples; n < num_samples; ++n) {
		if(n+1 == num_samples || event_call_stack_samples[n].type != event_call_stack_samples[n+1].type) {
			samples.push_back(std::pair<int, std::string>(count + 1, event_call_stack_samples[n].type->id()));
			count = 0;
		} else {
			++count;
		}
	}

	std::sort(samples.begin(), samples.end());
	std::reverse(samples.begin(), samples.end());
	for(int n = 0; n != samples.size(); ++n) {
		s << samples[n].second << " " << samples[n].first << " ";
	}

	last_empty_samples = empty_samples;
	last_num_samples = num_samples;

	handler_disabled = false;

	return s.str();
}

}

#endif
