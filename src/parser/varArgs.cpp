#include <ds/parser/varArgs.hpp>
#include <ds/languageTypes.hpp>

using namespace ds;

BytecodeBuffer ds::varArgs::writeVarArgs(std::vector<ExpressionResult> args)
{
	BytecodeBuffer result;

	for (auto i = args.rbegin(); i < args.rend(); i++)
	{
		result.addBuffer(i->code);
	}

	result.pushInt(Size(args.size()));

	return result;
}
