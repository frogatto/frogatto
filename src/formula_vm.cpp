#include <vector>

#include "formula_vm.hpp"

namespace formula
{

namespace vm
{

variant execute(const Instruction* i1, const Instruction* i2,
                const formula_callable& callable)
{
	static variant stack_buf[65536];
	variant* stack_ptr = stack_buf;

	while(i1 != i2) {
		const INSTRUCTION_CODE code = *i1;
		switch(code) {
		case PUSH_NULL:
			*stack_ptr++ = variant();
			break;
		case PUSH_INT_0:
			*stack_ptr++ = variant(0);
			break;
		case PUSH_INT_1:
			*stack_ptr++ = variant(1);
			break;
		case PUSH_INT_2:
			*stack_ptr++ = variant(2);
			break;
		case PUSH_INT_3:
			*stack_ptr++ = variant(3);
			break;
		case PUSH_INT_4:
			*stack_ptr++ = variant(4);
			break;
		case PUSH_INT_5:
			*stack_ptr++ = variant(5);
			break;
		case PUSH_INT_100:
			*stack_ptr++ = variant(100);
			break;
		case PUSH_INT_1000:
			*stack_ptr++ = variant(1000);
			break;
		case PUSH_INT_1B:
			*stack_ptr++ = variant(*++i1);
			break;
		case PUSH_INT_NEGATIVE_1B:
			*stack_ptr++ = variant(- (*++i1));
			break;
		case PUSH_INT_3B: {
			int value = *++i1;
			value = (value << 8) + *++i1;
			value = (value << 8) + *++i1;
			*stack_ptr++ = variant(value);
			break;
		}

		case OP_UNARY_NEGATIVE:
			stack_ptr[-1] = -stack_ptr[-1];
			break;
		}

		++i1;
	}

	return stack_buf[0];
}

}

}
