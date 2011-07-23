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
	mock_char c;
	mock_party p;
	try {
		CHECK_EQ(formula("strength").execute(c).as_int(), 15);
		CHECK_EQ(formula("17").execute(c).as_int(), 17);
		CHECK_EQ(formula("strength/2 + agility").execute(c).as_int(), 19);
		CHECK_EQ(formula("(strength+agility)/2").execute(c).as_int(), 13);
		CHECK_EQ(formula("strength > 12").execute(c).as_int(), 1);
		CHECK_EQ(formula("strength > 18").execute(c).as_int(), 0);
		CHECK_EQ(formula("if(strength > 12, 7, 2)").execute(c).as_int(), 7);
		CHECK_EQ(formula("if(strength > 18, 7, 2)").execute(c).as_int(), 2);
		CHECK_EQ(formula("2 and 1").execute(c).as_int(), 1);
		CHECK_EQ(formula("2 and 0").execute(c).as_int(), 0);
		CHECK_EQ(formula("2 or 0").execute(c).as_int(), 2);
		CHECK_EQ(formula("-5").execute(c).as_int(),-5);
		CHECK_EQ(formula("not 5").execute(c).as_int(), 0);
		CHECK_EQ(formula("not 0").execute(c).as_int(), 1);
		CHECK_EQ(formula("abs(5)").execute(c).as_int(), 5);
		CHECK_EQ(formula("abs(-5)").execute(c).as_int(), 5);
		CHECK_EQ(formula("sign(5)").execute(c).as_int(), 1);
		CHECK_EQ(formula("sign(-5)").execute(c).as_int(), -1);
		CHECK_EQ(formula("min(3,5)").execute(c).as_int(), 3);
		CHECK_EQ(formula("min(5,2)").execute(c).as_int(), 2);
		CHECK_EQ(formula("max(3,5)").execute(c).as_int(), 5);
		CHECK_EQ(formula("max(5,2)").execute(c).as_int(), 5);
		CHECK_EQ(formula("max(4,5,[2,18,7])").execute(c).as_int(), 18);
		CHECK_EQ(formula("char.strength").execute(p).as_int(), 15);
		CHECK_EQ(formula("choose(members,strength).strength").execute(p).as_int(), 16);
		CHECK_EQ(formula("4^2").execute().as_int(), 16);
		CHECK_EQ(formula("2+3^3").execute().as_int(), 29);
		CHECK_EQ(formula("2*3^3+2").execute().as_int(), 56);
		CHECK_EQ(formula("9^3").execute().as_int(), 729);
		CHECK_EQ(formula("x*5 where x=1").execute().as_int(), 5);
		CHECK_EQ(formula("x*(a*b where a=2,b=1) where x=5").execute().as_int(), 10);
		CHECK_EQ(formula("char.strength * ability where ability=3").execute(p).as_int(), 45);
		CHECK_EQ(formula("'abcd' = 'abcd'").execute(p).as_bool(), true);
		CHECK_EQ(formula("'abcd' = 'acd'").execute(p).as_bool(), false);
		CHECK_EQ(formula("'strength, agility: {strength}, {agility}'").execute(c).as_string(),
		               "strength, agility: 15, 12");
		for(int n = 0; n != 128; ++n) {
			const int dice_roll = formula("3d6").execute().as_int();
			CHECK_GE(dice_roll, 3);
	   		CHECK_LE(dice_roll, 18);
		}

		CHECK_EQ(formula::create_string_formula("Your strength is {strength}")->execute(c).as_string(),
						"Your strength is 15");
		variant myarray = formula("[1,2,3]").execute();
		CHECK_EQ(myarray.num_elements(), 3);
		CHECK_EQ(myarray[0].as_int(), 1);
		CHECK_EQ(myarray[1].as_int(), 2);
		CHECK_EQ(myarray[2].as_int(), 3);

	} catch(formula_error& e) {
		std::cerr << "parse error\n";
	}
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
	formula f(fm);
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
