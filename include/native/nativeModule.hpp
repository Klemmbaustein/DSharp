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
		NativeFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			const char* name, ExternalFunctionPointer function)
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
		std::string className;
		ClassType* virtualType = nullptr;
		bytecodeOffset virtualId = 0;

		ExpressionResult compileCall() override;
		std::vector<FunctionArgument> getArguments() override;
		Type* getReturnType() override;
		std::string getShortName() const override;
		std::string getFullName() const override;
		bool discardable() const override;

		bool isVirtual() const override
		{
			return virtualId != 0;
		}

		virtual bytecodeOffset getVirtualOffset() const
		{
			return virtualId;
		}

		BytecodeBuffer compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const override;
	};

	struct NativeModule
	{
		std::string name;

		std::vector<NativeFunction*> functions;

		NativeFunction* addFunction(NativeFunction function);

		std::vector<Attribute*> attributes;
		std::vector<Type*> types;
		std::vector<EnumType*> enums;

		template<typename TNative>
		ClassType* createClass(std::string name, ClassType* derived = nullptr)
		{
			auto cls = new ClassType();
			cls->name = name;
			cls->classSize = sizeof(TNative);
			cls->vTableOffset = UINT32_MAX;
			cls->parents = { derived };
			this->types.push_back(cls);
			return cls;
		}

		EnumType* createEnum(std::string name);

		void addEnumIntValue(EnumType* type, std::string name, int value);

		template<typename T>
		void addEnumValue(EnumType* type, std::string name, T value)
		{
			addEnumIntValue(type, name, int(value));
		}

		void addClassConstructor(ClassType* type, NativeFunction constructor);
		void addClassMethod(ClassType* type, NativeFunction function);
		void addClassVirtualMethod(ClassType* type, NativeFunction function, bytecodeOffset virtualId);

		Module create() const;
	};
} // namespace lang