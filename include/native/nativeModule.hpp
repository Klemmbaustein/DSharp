#pragma once
#include <interpreter.hpp>
#include <parser/function.hpp>
#include <parser/module.hpp>
#include "externalFunction.hpp"

namespace lang
{
	struct NativeFunction : public Function
	{
		NativeFunction(std::vector<FunctionArgument> arguments,
			std::string name,
			ExternalFunctionPointer function)
		{
			this->arguments = arguments;
			this->name = name;
			this->function = function;
		}

		std::vector<FunctionArgument> arguments;
		std::string name;
		ExternalFunctionPointer function = nullptr;

		ExpressionResult compileCall() override;
		std::vector<FunctionArgument> getArguments() override;
		std::string getFullName() const override;
		bool discardable() const override;

		friend struct NativeModule;

	private:
		std::string moduleName;
	};

	struct NativeModule
	{
		std::string name;

		std::vector<NativeFunction> functions;
		std::vector<Attribute*> attributes;
		std::vector<Type*> types;

		Module create() const;
	};
} // namespace lang