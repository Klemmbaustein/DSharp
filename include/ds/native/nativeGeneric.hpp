#pragma once
#include "nativeModule.hpp"
#include <ds/parser/types/genericArgument.hpp>

namespace ds
{
	struct NativeGenericFunction : public NativeFunction
	{
		NativeGenericFunction(std::vector<FunctionArgument> arguments, std::vector<GenericArgument> genericArgs,
			Type* returnType, std::string name, ExternalFunctionPointer function)
			: NativeFunction(arguments, returnType, name, function)
		{
			this->genericArgs = genericArgs;
		}

		NativeGenericFunction(std::vector<FunctionArgument> arguments, std::vector<GenericArgument> genericArgs,
			Type* returnType, const char* name, ExternalFunctionPointer function)
			: NativeFunction(arguments, returnType, name, function)
		{
			this->genericArgs = genericArgs;
		}

		NativeGenericFunction(const NativeGenericFunction&) = default;

		~NativeGenericFunction() = default;

		bool isGeneric() const override
		{
			return true;
		}

		virtual std::vector<GenericArgument> getGenericTypes()
		{
			return genericArgs;
		}

		virtual NativeFunction* create() const
		{
			return new NativeGenericFunction(*this);
		}

	private:
		std::vector<GenericArgument> genericArgs;
	};

	struct GenericData
	{
		GenericData(InterpretContext* context);

		TypeId id = 0;
		Size typeSize = 0;
	};

} // namespace ds