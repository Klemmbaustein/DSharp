#include <ds/parser/bytecode/constantEvaluate.hpp>

bool ds::ConstantEvaluate::run(const BytecodeBuffer& buffer)
{
	for (auto& inst : buffer.instructions)
	{
		if (!inst->constantEvaluate(this))
		{
			return false;
		}
	}

	return true;
}
