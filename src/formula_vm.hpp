#ifndef FORMULA_VM_HPP_INCLUDED
#define FORMULA_VM_HPP_INCLUDED

#include <inttypes.h>

#include "formula_callable.hpp"
#include "variant.hpp"

namespace formula
{

namespace vm
{

enum INSTRUCTION_CODE {
	PUSH_NULL,
	PUSH_INT_0,
	PUSH_INT_1,
	PUSH_INT_2,
	PUSH_INT_3,
	PUSH_INT_4,
	PUSH_INT_5,
	PUSH_INT_100,
	PUSH_INT_1000,
	PUSH_INT_1B,
	PUSH_INT_NEGATIVE_1B,
	PUSH_INT_3B,

	OP_UNARY_NEGATIVE,
};

typedef uint8_t Instruction;

variant execute(const Instruction* i1, const Instruction* i2,
                const formula_callable& callable);

}

}

#endif
