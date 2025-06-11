#include <native/nativeModule.hpp>
#include <parser/bytecode/compileBytecode.hpp>
using namespace lang;

ExpressionResult NativeFunction::compileCall()
{
	ExpressionResult result;
	result.code.add(new BytecodeCallNative(this));
	result.type = nullptr;
	result.valid = true;
	return result;
}

std::vector<FunctionArgument> NativeFunction::getArguments()
{
	return this->arguments;
}

Type* lang::NativeFunction::getReturnType()
{
	return nullptr;
}

std::string lang::NativeFunction::getShortName() const
{
	return this->name;
}

std::string NativeFunction::getFullName() const
{
	return this->moduleName + "::" + this->name;
}
bool lang::NativeFunction::discardable() const
{
	return true;
}

Module lang::NativeModule::create() const
{
	Module outModule;
	outModule.name = this->name;

	for (auto& i : this->functions)
	{
		auto function = new NativeFunction(i);
		function->moduleName = this->name;
		outModule.moduleFunctions.insert({ i.name, function });
	}
	for (auto& i : this->attributes)
	{
		outModule.moduleAttributes.insert({ i->name, i });
	}

	return outModule;
}
