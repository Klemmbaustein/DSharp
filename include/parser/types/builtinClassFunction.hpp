#pragma once
#include <parser/function.hpp>
#include <parser/bytecode/compileBytecodeVirtual.hpp>

namespace lang
{
	class BuiltinClassFunction : public Function
	{
	public:
		BuiltinClassFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			std::string name)
		{
			this->arguments = arguments;
			this->returnType = returnType;
			this->name = name;
		}
		BuiltinClassFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			std::string name, bytecodeOffset virtualId, ClassType* thisClass)
		{
			this->arguments = arguments;
			this->returnType = returnType;
			this->name = name;
			this->virtualId = virtualId;
			this->thisClass = thisClass;
		}

		std::vector<FunctionArgument> arguments;
		Type* returnType;
		std::string name;
		bytecodeOffset virtualId = UINT32_MAX;
		ClassType* thisClass = nullptr;
		ExpressionResult compileCall() override
		{
			ExpressionResult result;
			if (isVirtual())
			{
				result.code.addNew<BytecodeCallVirtual>(this, thisClass);
			}
			else
			{
				result.code.addNew<BytecodeCallNative>(name);
			}
			result.valid = true;
			result.type = returnType;
			return result;
		}
		std::vector<FunctionArgument> getArguments() override
		{
			return this->arguments;
		}

		bool isVirtual() const override
		{
			return virtualId != UINT32_MAX;
		}

		virtual bytecodeOffset getVirtualOffset() const
		{
			return virtualId;
		}

		Type* getReturnType() override
		{
			return returnType;
		}
		std::string getFullName() const override
		{
			return name;
		}
		std::string getShortName() const override
		{
			return name;
		}

		bool discardable() const override
		{
			return false;
		}
	};

}