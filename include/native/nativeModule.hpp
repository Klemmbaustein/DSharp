#pragma once
#include <interpreter.hpp>
#include <parser/function.hpp>
#include <parser/module.hpp>
#include <parser/types/classType.hpp>
#include "externalFunction.hpp"

namespace lang
{
	struct NativeFunction : public Function
	{
		NativeFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			std::string name, ExternalFunctionPointer function)
		{
			this->arguments = arguments;
			this->name = name;
			this->function = function;
			this->returnType = returnType;
		}

		std::vector<FunctionArgument> arguments;
		std::string name;
		ExternalFunctionPointer function = nullptr;
		Type* returnType = nullptr;
		std::string moduleName;

		ExpressionResult compileCall() override;
		std::vector<FunctionArgument> getArguments() override;
		Type* getReturnType() override;
		std::string getShortName() const override;
		std::string getFullName() const override;
		bool discardable() const override;
	};

	struct NativeModule
	{
		std::string name;

		std::vector<NativeFunction*> functions;

		NativeFunction* addFunction(NativeFunction function);

		std::vector<Attribute*> attributes;
		std::vector<Type*> types;

		template<typename TNative>
		ClassType* createClass(std::string name)
		{
			auto cls = new ClassType();
			cls->name = name;
			cls->classSize = sizeof(TNative);
			cls->vTableOffset = UINT32_MAX;
			this->types.push_back(cls);
			return cls;
		}

		void addClassConstructor(ClassType* type, NativeFunction constructor);
		void addClassMethod(ClassType* type, NativeFunction function);

		Module create() const;
	};
} // namespace lang