#pragma once
#include "type.hpp"

namespace ds
{
	class GenericArgumentType : public Type
	{
	public:

		GenericArgumentType(size_t index, bool isFunctionIndex, bool isNullable = false);

		~GenericArgumentType();

		GenericArgumentType* nullable = nullptr;

		size_t index = 0;
		bool isFunctionIndex = false;
		bool isNullable = false;

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;

		static GenericArgumentType* getInstance(size_t index, bool isFunction)
		{
			if (isFunction)
			{
				GenericArgumentType*& instance = genericArgTypes[index];
				if (!instance)
				{
					instance = new GenericArgumentType(index, true);
				}
				return instance;
			}
			GenericArgumentType*& instance = genericClassTypes[index];
			if (!instance)
			{
				instance = new GenericArgumentType(index, false);
			}
			return instance;
		}

	private:
		static inline std::map<size_t, GenericArgumentType*> genericArgTypes;
		static inline std::map<size_t, GenericArgumentType*> genericClassTypes;
	};
}