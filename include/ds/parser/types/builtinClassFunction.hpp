#pragma once
#include <ds/parser/function.hpp>
#include <ds/parser/bytecode/compileBytecodeVirtual.hpp>

namespace ds
{
	class BuiltinClassFunction : public Function
	{
	public:
		BuiltinClassFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			std::string name, std::string shortName)
		{
			this->arguments = arguments;
			this->returnType = returnType;
			this->name = name;
			this->shortName = shortName;
		}
		BuiltinClassFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			std::string name, std::string shortName, BytecodeOffset virtualId, ClassType* thisClass)
		{
			this->arguments = arguments;
			this->returnType = returnType;
			this->name = name;
			this->virtualId = virtualId;
			this->thisClass = thisClass;
			this->shortName = shortName;
		}

		std::vector<FunctionArgument> arguments;
		Type* returnType;
		std::string name;
		std::string shortName;
		BytecodeOffset virtualId = UINT32_MAX;
		ClassType* thisClass = nullptr;
		ExpressionResult compileCall() override
		{
			ExpressionResult result;
			if (isVirtual())
			{
				result.code.addNew<BytecodeCallVirtual>(this);
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

		virtual BytecodeOffset getVirtualOffset() const
		{
			return virtualId;
		}

		Type* getReturnType() override
		{
			return returnType;
		}
		std::string getFullName() const override
		{
			return thisClass ? (Type::toString(thisClass) + "." + name) : name;
		}
		std::string getShortName() const override
		{
			return shortName;
		}

		bool discardable() const override
		{
			return false;
		}
	};

}