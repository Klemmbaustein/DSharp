#include <native/nativeModule.hpp>
#include <parser/bytecode/compileBytecode.hpp>
using namespace lang;

ExpressionResult NativeFunction::compileCall()
{
	ExpressionResult result;
	result.code.addNew<BytecodeCallNative>(this);
	result.type = this->returnType;
	result.valid = true;
	return result;
}

std::vector<FunctionArgument> NativeFunction::getArguments()
{
	return this->arguments;
}

Type* lang::NativeFunction::getReturnType()
{
	return this->returnType;
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

BytecodeBuffer lang::NativeFunction::compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const
{
	BytecodeBuffer result;
	result.addNew<BytecodeFunctionAddress>(this->getFullName(), true);
	result.addNew<BytecodeCallNative>("system::fn.new.native");
	return result;
}

NativeFunction* lang::NativeModule::addFunction(NativeFunction function)
{
	return this->functions.emplace_back(new NativeFunction(function));
}

EnumType* lang::NativeModule::createEnum(std::string name)
{
	auto newType = new EnumType();
	newType->name = name;
	this->enums.push_back(newType);
	return newType;
}

void lang::NativeModule::addEnumIntValue(EnumType* type, std::string name, int value)
{
	type->values.insert({ Token(name), value });
}

void lang::NativeModule::addClassConstructor(ClassType* type, NativeFunction constructor)
{
	type->constructors.push_back(addFunction(constructor));
}

void lang::NativeModule::addClassMethod(ClassType* type, NativeFunction function)
{
	auto fn = addFunction(function);
	type->methods.insert({ fn->name,
		fn });
	fn->name = type->name + "." + fn->name;
}

Module lang::NativeModule::create() const
{
	Module outModule;
	outModule.name = this->name;

	for (auto& i : this->functions)
	{
		i->moduleName = this->name;
		outModule.moduleFunctions.insert({ i->name, i });
	}
	for (auto& i : this->attributes)
	{
		outModule.moduleAttributes.insert({ i->name, i });
	}
	for (auto& i : this->types)
	{
		outModule.moduleTypes.insert({ i->name, i });
	}
	for (auto& i : this->enums)
	{
		outModule.moduleEnums.insert({ i->name, i });
	}

	return outModule;
}
