#include <parser/varArgs.hpp>
#include <languageTypes.hpp>

using namespace lang;

BytecodeBuffer lang::varArgs::writeVarArgs(std::vector<ExpressionResult> args)
{
	BytecodeBuffer result;

	for (auto i = args.rbegin(); i < args.rend(); i++)
	{
		result.addBuffer(i->code);
	}

	result.pushInt(Size(args.size()));

	return result;
}
