#include <ds/native/genericClassType.hpp>

using namespace ds;

ds::NativeGenericClassType::NativeGenericClassType(std::vector<GenericArgument> arguments)
{
	this->isGeneric = true;
	this->arguments = arguments;
}

Type* ds::NativeGenericClassType::instantiateGeneric(std::vector<Type*> types, Token at,
	ErrorContext* with, TypeRegistry* registry)
{
	return registry->getGenericEntry<NativeGenericClassType>(this->id, types,
		[this, types, at, with, registry] {
			auto t = new NativeGenericClassType(*this);
			t->nullable = new NullableClassType(t);
			t->types = types;

			for (auto& i : t->members)
			{
				i.type = convertGenericType(i.type, types, false, at, with, registry);
			}

			for (auto& i : t->methods)
			{
				i.second = new GenericMethod(t, i.second, at, with, registry);
			}

			return t;
		});
}

ds::NativeGenericClassType::~NativeGenericClassType()
{
	for (auto& i : this->methods)
	{
		delete i.second;
	}
}

ds::NativeGenericClassType::GenericMethod::GenericMethod(NativeGenericClassType* classInstance, Function* base,
	Token at, ErrorContext* errors, TypeRegistry* registry)
{
	this->base = base;
	this->classInstance = classInstance;

	this->convertedReturnType = convertGenericType(base->getReturnType(), classInstance->types, false, at,
		errors, registry);

	convertedArgs = base->getArguments();

	for (auto& i : convertedArgs)
	{
		i.type = convertGenericType(i.type, classInstance->types, false, at, errors, registry);
	}
}

std::string ds::NativeGenericClassType::GenericMethod::getShortName() const
{
	return base->getShortName();
}

std::string ds::NativeGenericClassType::GenericMethod::getFullName() const
{
	return base->getFullName();
}

bool ds::NativeGenericClassType::GenericMethod::discardable() const
{
	return base->discardable();
}

bool ds::NativeGenericClassType::GenericMethod::isVirtual() const
{
	return base->isVirtual();
}

bytecodeOffset ds::NativeGenericClassType::GenericMethod::getVirtualOffset() const
{
	return base->getVirtualOffset();
}

BytecodeBuffer ds::NativeGenericClassType::GenericMethod::compileCallable(ErrorContext* errors,
	ParsedScope* with, Type* hintType) const
{
	return base->compileCallable(errors, with, hintType);
}

ExpressionResult ds::NativeGenericClassType::GenericMethod::compileCall()
{
	auto call = base->compileCall();
	call.type = this->getReturnType();
	return call;
}

std::vector<FunctionArgument> ds::NativeGenericClassType::GenericMethod::getArguments()
{
	return convertedArgs;
}

Type* ds::NativeGenericClassType::GenericMethod::getReturnType()
{
	return convertedReturnType;
}