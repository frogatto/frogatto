#ifndef DISABLE_FORMULA_PROFILER

#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formula_profiler.hpp"
#include "object_events.hpp"

namespace formula_profiler
{

event_call_stack_type event_call_stack;

namespace {
bool profiler_on = false;
std::string output_fname;
pthread_t main_thread;

int empty_samples = 0;

std::vector<custom_object_event_frame> event_call_stack_samples;
int num_samples = 0;
const size_t max_samples = 10000;

void sigprof_handler(int sig)
{
	if(!pthread_equal(main_thread, pthread_self())) {
		return;
	}

	if(num_samples == max_samples) {
		return;
	}

	if(event_call_stack.empty()) {
		++empty_samples;
	} else {
		event_call_stack_samples[num_samples++] = event_call_stack.back();
	}
}

}

manager::manager(const char* output_file)
{
	if(output_file) {
		event_call_stack_samples.resize(max_samples);

		main_thread = pthread_self();

		fprintf(stderr, "SETTING UP PROFILING...\n");
		profiler_on = true;
		output_fname = output_file;

		const sighandler_t sigresult = signal(SIGPROF, sigprof_handler);
		assert(sigresult != SIG_ERR);

		struct itimerval timer;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 10000;
		timer.it_value = timer.it_interval;
		setitimer(ITIMER_PROF, &timer, 0);
	}
}

manager::~manager()
{
	if(profiler_on){
		struct itimerval timer;
		memset(&timer, 0, sizeof(timer));
		setitimer(ITIMER_PROF, &timer, 0);

		std::ostringstream s;
		s << "SAMPLES EMPTY: " << empty_samples << "\n";
		for(int n = 0; n != num_samples; ++n) {
			const custom_object_event_frame& frame = event_call_stack_samples[n];
			s << frame.type->id() << ":" << get_object_event_str(frame.event_id) << ":" << (frame.executing_commands ? "CMD" : "FFL") << "\n";

		}

		if(!output_fname.empty()) {
			sys::write_file(output_fname, s.str());
		} else {
			std::cerr << "===\n=== PROFILE REPORT ===\n";
			std::cerr << s.str();
			std::cerr << "=== END PROFILE REPORT ===\n";
		}
	}
}

bool custom_object_event_frame::operator<(const custom_object_event_frame& f) const
{
	return type < f.type || type == f.type && event_id < f.event_id ||
	       type == f.type && event_id == f.event_id && executing_commands < f.executing_commands;
}


}

#endif
