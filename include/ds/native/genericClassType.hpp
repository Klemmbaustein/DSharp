#pragma once
#include <ds/parser/types/classType.hpp>
#include <ds/parser/types/genericArgument.hpp>

namespace ds
{
	class NativeGenericClassType : public ClassType
	{
	public:
		NativeGenericClassType(std::vector<GenericArgument> arguments);

		NativeGenericClassType(const NativeGenericClassType&) = default;
		~NativeGenericClassType();

		virtual std::vector<GenericArgument> getGenericArguments() override
		{
			return arguments;
		}

		std::vector<Type*> getGenericTypes() override
		{
			return types;
		}

		Type* instantiateGeneric(std::vector<Type*> types, Token at, ErrorContext* with,
			TypeRegistry* registry) override;

	private:
		std::vector<GenericArgument> arguments;
		std::vector<Type*> types;

		class GenericMethod : public Function
		{
		public:
			GenericMethod(NativeGenericClassType* classInstance, Function* base,
				Token at, ErrorContext* errors, TypeRegistry* registry);

			ExpressionResult compileCall() override;
			std::vector<FunctionArgument> getArguments() override;
			Type* getReturnType() override;
			std::string getShortName() const override;
			std::string getFullName() const override;
			bool discardable() const override;

			bool isVirtual() const override;

			virtual BytecodeOffset getVirtualOffset() const;

			BytecodeBuffer compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const override;

		private:
			NativeGenericClassType* classInstance = nullptr;
			Function* base = nullptr;

			std::vector<FunctionArgument> convertedArgs;
			Type* convertedReturnType = nullptr;
		};
	};
} // namespace ds