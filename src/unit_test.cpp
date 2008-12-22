#include <iostream>
#include <map>

#include "foreach.hpp"
#include "unit_test.hpp"

#include "SDL.h"

namespace test {

namespace {
typedef std::map<std::string, UnitTest> TestMap;
TestMap& get_test_map()
{
	static TestMap map;
	return map;
}

typedef std::map<std::string, BenchmarkTest> BenchmarkMap;
BenchmarkMap& get_benchmark_map()
{
	static BenchmarkMap map;
	return map;
}

}

int register_test(const std::string& name, UnitTest test)
{
	get_test_map()[name] = test;
	return 0;
}

bool run_tests(const std::vector<std::string>* tests)
{
	std::vector<std::string> all_tests;
	if(!tests) {
		for(TestMap::const_iterator i = get_test_map().begin(); i != get_test_map().end(); ++i) {
			all_tests.push_back(i->first);
		}

		tests = &all_tests;
	}

	int npass = 0, nfail = 0;
	foreach(const std::string& test, *tests) {
		try {
			get_test_map()[test]();
			std::cerr << "TEST " << test << " PASSED\n";
			++npass;
		} catch(failure_exception&) {
			std::cerr << "TEST " << test << " FAILED!!\n";
			++nfail;
		}
	}

	if(nfail) {
		std::cerr << npass << " TESTS PASSED, " << nfail << " TESTS FAILED\n";
		return false;
	} else {
		std::cerr << "ALL " << npass << " TESTS PASSED\n";
		return true;
	}
}

int register_benchmark(const std::string& name, BenchmarkTest test)
{
	get_benchmark_map()[name] = test;
	return 0;
}

namespace {
void run_benchmark(const std::string& name, BenchmarkTest fn)
{
	//run it once without counting it to let any initialization code be run.
	fn(1);

	std::cerr << "RUNNING BENCHMARK " << name << "...\n";
	const int MinTicks = 200;
	for(int nruns = 10; ; nruns *= 10) {
		const int start_time = SDL_GetTicks();
		fn(nruns);
		const int time_taken = SDL_GetTicks() - start_time;
		if(time_taken >= MinTicks) {
			const int ns = time_taken*1000000;
			const int ns_per_iter = ns/nruns;
			std::cerr << "BENCH " << name << ": " << nruns << " iterations, " << ns_per_iter << "ns/iteration; total, " << ns << "ns\n";
			return;
		}
	}
}
}

void run_benchmarks(const std::vector<std::string>* benchmarks)
{
	std::vector<std::string> all_benchmarks;
	if(!benchmarks) {
		for(BenchmarkMap::const_iterator i = get_benchmark_map().begin(); i != get_benchmark_map().end(); ++i) {
			all_benchmarks.push_back(i->first);
		}

		benchmarks = &all_benchmarks;
	}

	foreach(const std::string& benchmark, *benchmarks) {
		run_benchmark(benchmark, get_benchmark_map()[benchmark]);
	}
}

}
