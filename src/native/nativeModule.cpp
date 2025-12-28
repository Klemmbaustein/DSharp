#include <ds/native/nativeModule.hpp>
#include <ds/parser/bytecode/compileBytecode.hpp>
#include <ds/parser/bytecode/compileBytecodeVirtual.hpp>
using namespace ds;

ExpressionResult NativeFunction::compileCall()
{
	ExpressionResult result;
	if (isVirtual())
	{
		result.code.addNew<BytecodeCallVirtual>(this);
	}
	else
	{
		result.code.addNew<BytecodeCallNative>(this);
	}
	result.type = this->returnType;
	result.valid = true;
	return result;
}

std::vector<FunctionArgument> NativeFunction::getArguments()
{
	return this->arguments;
}

Type* ds::NativeFunction::getReturnType()
{
	return this->returnType;
}

std::string ds::NativeFunction::getShortName() const
{
	return this->name;
}

Type* ds::NativeModule::getType(const std::string& name)
{
	for (auto& i : this->types)
	{
		if (i->name == name)
		{
			return i;
		}
	}
	return nullptr;
}

std::string NativeFunction::getFullName() const
{
	if (this->className.empty())
	{
		return this->moduleName + "::" + this->name;
	}
	return this->moduleName + "::" + this->className + "." + this->name;
}

bool ds::NativeFunction::discardable() const
{
	return true;
}

ds::TypeId ds::NativeModule::addAttribute(Attribute* attrib)
{
	attrib->moduleName = this->name;
	this->attributes.push_back(attrib);
	return attrib->getType();
}

BytecodeBuffer ds::NativeFunction::compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const
{
	BytecodeBuffer result;
	result.addNew<BytecodeFunctionAddress>(this->getFullName(), true);
	result.addNew<BytecodeCallNative>("system::fn.new.native");
	return result;
}

NativeFunction* ds::NativeModule::addFunction(NativeFunction function)
{
	return this->functions.emplace_back(new NativeFunction(function));
}

EnumType* ds::NativeModule::createEnum(std::string name)
{
	auto newType = new EnumType();
	newType->name = name;
	this->enums.push_back(newType);
	return newType;
}

void ds::NativeModule::addEnumIntValue(EnumType* type, std::string name, int value)
{
	type->values.insert({ Token(name), value });
}

void ds::NativeModule::addClassConstructor(ClassType* type, NativeFunction constructor)
{
	type->constructors.push_back(addFunction(constructor));
}

void ds::NativeModule::addClassMethod(ClassType* type, NativeFunction function)
{
	auto fn = addFunction(function);
	type->methods.insert({ fn->name,
		fn });
	fn->className = type->name;
}

void ds::NativeModule::addStructMethod(ClassType* type, NativeFunction function)
{
	auto fn = addFunction(function);
	type->methods.insert({ fn->name,
		fn });
	fn->className = type->name;
}

void ds::NativeModule::addClassVirtualMethod(ClassType* type, NativeFunction function, bytecodeOffset virtualId)
{
	auto fn = addFunction(function);
	type->methods.insert({ fn->name,
		fn });
	fn->className = type->name;
	fn->virtualId = virtualId;
}

void ds::NativeModule::initialize()
{
	for (auto& i : this->functions)
	{
		i->moduleName = this->name;
	}

	for (auto& i : this->types)
	{
		i->applyName();
	}
}

Module ds::NativeModule::create() const
{
	Module outModule;
	outModule.name = this->name;

	for (auto& i : this->functions)
	{
		if (i->className.empty())
		{
			outModule.moduleFunctions.insert({ i->name, i });
		}
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
