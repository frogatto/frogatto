#ifndef UNIT_TEST_HPP_INCLUDED
#define UNIT_TEST_HPP_INCLUDED

#include <boost/function.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace test {

struct failure_exception {
};

typedef boost::function<void ()> UnitTest;
typedef boost::function<void (int)> BenchmarkTest;

int register_test(const std::string& name, UnitTest test);
int register_benchmark(const std::string& name, BenchmarkTest test);
bool run_tests(const std::vector<std::string>* tests=NULL);
void run_benchmarks(const std::vector<std::string>* benchmarks=NULL);

}

#define CHECK(cond, msg) if(!(cond)) { std::cerr << __FILE__ << ":" << __LINE__ << ": TEST CHECK FAILED: " << #cond << ": " << msg << "\n"; throw test::failure_exception(); }

#define CHECK_CMP(a, b, cmp) CHECK((a) cmp (b), #a << ": " << (a) << "; " << #b << ": " << #b)

#define CHECK_EQ(a, b) CHECK_CMP(a, b, ==)
#define CHECK_NE(a, b) CHECK_CMP(a, b, !=)
#define CHECK_LE(a, b) CHECK_CMP(a, b, <=)
#define CHECK_GE(a, b) CHECK_CMP(a, b, >=)
#define CHECK_LT(a, b) CHECK_CMP(a, b, <)
#define CHECK_GT(a, b) CHECK_CMP(a, b, >)

#define UNIT_TEST(name) \
	void TEST_##name(); \
	static int TEST_VAR_##name = test::register_test(#name, TEST_##name); \
	void TEST_##name()

#define BENCHMARK(name) \
	void BENCHMARK_##name(int benchmark_iterations); \
	static int BENCHMARK_VAR_##name = test::register_benchmark(#name, BENCHMARK_##name); \
	void BENCHMARK_##name(int benchmark_iterations)

#define BENCHMARK_LOOP while(benchmark_iterations--)

#define BENCHMARK_ARG(name, arg) \
	void BENCHMARK_ARG_##name(int benchmark_iterations, arg)

#define BENCHMARK_ARG_CALL(name, id, arg) \
	void BENCHMARK_ARG_CALL_##name_##id(int benchmark_iterations) { \
		BENCHMARK_ARG_##name(benchmark_iterations, arg); \
	} \
	static int BENCHMARK_ARG_VAR_##name_##id = test::register_benchmark(#name " " #id, BENCHMARK_ARG_CALL_##name_##id);

#endif
