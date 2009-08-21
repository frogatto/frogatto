#include "unit_test.hpp"
#include "wml_parser.hpp"

UNIT_TEST(wml_parser)
{
	using namespace wml;

	const std::string test =
"[template:unit goblin(x,y)]\n"
"x={x}\n"
"y={y}\n"
"[/unit]\n"
"[goblin(10,4)]\n";
	wml::node_ptr node = wml::parse_wml(test);
	CHECK_EQ((*node)["x"].str(), "10");
	CHECK_EQ((*node)["y"].str(), "4");
}
