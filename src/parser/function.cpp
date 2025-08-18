#include <parser/function.hpp>

using namespace lang;

BytecodeBuffer lang::Function::compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const
{
	return BytecodeBuffer();
}

std::string lang::Function::getSignatureText()
{
	std::string arguments;

	for (auto& i : getArguments())
	{
		arguments.append(Type::toString(i.type) + " " + i.name.string + ", ");
	}

	if (arguments.size())
	{
		arguments.pop_back();
		arguments.pop_back();
	}

	std::string parentSignature = "fn " + getFullName() + "(" + arguments + ")";

	auto returnType = getReturnType();
	if (returnType)
	{
		parentSignature.append(" -> " + Type::toString(returnType));
	}
	return parentSignature;
}