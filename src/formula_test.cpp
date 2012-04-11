#include <boost/intrusive_ptr.hpp>

#include "formula.hpp"
#include "formula_callable.hpp"
#include "unit_test.hpp"

namespace {
using namespace game_logic;
class mock_char : public formula_callable {
	variant get_value(const std::string& key) const {
		if(key == "strength") {
			return variant(15);
		} else if(key == "agility") {
			return variant(12);
		}

		return variant(10);
	}
};
class mock_party : public formula_callable {
	variant get_value(const std::string& key) const {
		c_.add_ref();
		i_[0].add_ref();
		i_[1].add_ref();
		i_[2].add_ref();
		if(key == "members") {
			i_[0].add("strength",variant(12));
			i_[1].add("strength",variant(16));
			i_[2].add("strength",variant(14));
			std::vector<variant> members;
			for(int n = 0; n != 3; ++n) {
				members.push_back(variant(&i_[n]));
			}

			return variant(&members);
		} else if(key == "char") {
			return variant(&c_);
		} else {
			return variant(0);
		}
	}

	mock_char c_;
	mutable map_formula_callable i_[3];

};

}

UNIT_TEST(formula)
{
	boost::intrusive_ptr<mock_char> cp(new mock_char);
	boost::intrusive_ptr<mock_party> pp(new mock_party);
#define FML(a) formula(variant(a))
	mock_char& c = *cp;
	mock_party& p = *pp;
	CHECK_EQ(FML("strength").execute(c).as_int(), 15);
	CHECK_EQ(FML("17").execute(c).as_int(), 17);
	CHECK_EQ(FML("strength/2 + agility").execute(c).as_int(), 19);
	CHECK_EQ(FML("(strength+agility)/2").execute(c).as_int(), 13);
	CHECK_EQ(FML("strength > 12").execute(c).as_int(), 1);
	CHECK_EQ(FML("strength > 18").execute(c).as_int(), 0);
	CHECK_EQ(FML("if(strength > 12, 7, 2)").execute(c).as_int(), 7);
	CHECK_EQ(FML("if(strength > 18, 7, 2)").execute(c).as_int(), 2);
	CHECK_EQ(FML("2 and 1").execute(c).as_int(), 1);
	CHECK_EQ(FML("2 and 0").execute(c).as_int(), 0);
	CHECK_EQ(FML("2 or 0").execute(c).as_int(), 2);
	CHECK_EQ(FML("-5").execute(c).as_int(),-5);
	CHECK_EQ(FML("not 5").execute(c).as_int(), 0);
	CHECK_EQ(FML("not 0").execute(c).as_int(), 1);
	CHECK_EQ(FML("abs(5)").execute(c).as_int(), 5);
	CHECK_EQ(FML("abs(-5)").execute(c).as_int(), 5);
	CHECK_EQ(FML("sign(5)").execute(c).as_int(), 1);
	CHECK_EQ(FML("sign(-5)").execute(c).as_int(), -1);
	CHECK_EQ(FML("min(3,5)").execute(c).as_int(), 3);
	CHECK_EQ(FML("min(5,2)").execute(c).as_int(), 2);
	CHECK_EQ(FML("max(3,5)").execute(c).as_int(), 5);
	CHECK_EQ(FML("max(5,2)").execute(c).as_int(), 5);
	CHECK_EQ(FML("char.strength").execute(p).as_int(), 15);
	CHECK_EQ(FML("choose(members,strength).strength").execute(p).as_int(), 16);
	CHECK_EQ(FML("4^2").execute().as_int(), 16);
	CHECK_EQ(FML("2+3^3").execute().as_int(), 29);
	CHECK_EQ(FML("2*3^3+2").execute().as_int(), 56);
	CHECK_EQ(FML("9^3").execute().as_int(), 729);
	CHECK_EQ(FML("x*5 where x=1").execute().as_int(), 5);
	CHECK_EQ(FML("x*(a*b where a=2,b=1) where x=5").execute().as_int(), 10);
	CHECK_EQ(FML("char.strength * ability where ability=3").execute(p).as_int(), 45);
	CHECK_EQ(FML("'abcd' = 'abcd'").execute(p).as_bool(), true);
	CHECK_EQ(FML("'abcd' = 'acd'").execute(p).as_bool(), false);
	CHECK_EQ(FML("'strength, agility: {strength}, {agility}'").execute(c).as_string(),
	               "strength, agility: 15, 12");
	for(int n = 0; n != 128; ++n) {
		const int dice_roll = FML("3d6").execute().as_int();
		CHECK_GE(dice_roll, 3);
   		CHECK_LE(dice_roll, 18);
	}

	CHECK_EQ(formula::create_string_formula("Your strength is {strength}")->execute(c).as_string(),
					"Your strength is 15");
	variant myarray = FML("[1,2,3]").execute();
	CHECK_EQ(myarray.num_elements(), 3);
	CHECK_EQ(myarray[0].as_int(), 1);
	CHECK_EQ(myarray[1].as_int(), 2);
	CHECK_EQ(myarray[2].as_int(), 3);

}

BENCHMARK(construct_int_variant)
{
	BENCHMARK_LOOP {
		variant v(0);
	}
}

BENCHMARK_ARG(formula, const std::string& fm)
{
	static mock_party p;
	formula f = formula(variant(fm));
	BENCHMARK_LOOP {
		f.execute(p);
	}
}

BENCHMARK_ARG_CALL(formula, integer, "0");
BENCHMARK_ARG_CALL(formula, where, "x where x = 5");
BENCHMARK_ARG_CALL(formula, add, "5 + 4");
BENCHMARK_ARG_CALL(formula, arithmetic, "(5 + 4)*17 + 12*9 - 5/2");
BENCHMARK_ARG_CALL(formula, read_input, "char");
BENCHMARK_ARG_CALL(formula, read_input_sub, "char.strength");
BENCHMARK_ARG_CALL(formula, array, "[4, 5, 8, 12, 17, 0, 19]");
BENCHMARK_ARG_CALL(formula, array_str, "['stand', 'walk', 'run', 'jump']");
BENCHMARK_ARG_CALL(formula, string, "'blah'");
BENCHMARK_ARG_CALL(formula, null_function, "null()");
BENCHMARK_ARG_CALL(formula, if_function, "if(4 > 5, 7, 8)");
