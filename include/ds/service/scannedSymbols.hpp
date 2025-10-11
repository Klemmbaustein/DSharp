#pragma once
#include <ds/parser/parseScope.hpp>
#include <ds/parser/parseClass.hpp>

namespace ds
{
	struct ScannedFunction
	{
		Token at;
		TokenPos argEnd;
		std::string name;
		std::optional<SymbolDefinition> definition;
		std::string returnType;
		TypeId returnTypeId = 0;
		std::vector<std::pair<std::string, std::string>> arguments;

		enum class Kind
		{
			functionCall,
			functionDefinition,
			functionReference,
		};

		Kind kind = Kind::functionCall;

		ScannedFunction(Function* from, Token atToken, Kind kind, TokenPos argEnd = TokenPos())
		{
			this->at = atToken;
			this->name = from->getFullName();
			this->argEnd = argEnd;
			auto functionArgs = from->getArguments();
			this->arguments.reserve(functionArgs.size());

			for (auto& i : functionArgs)
			{
				this->arguments.push_back({ Type::toString(i.type), i.name.string });
			}

			auto functionReturnType = from->getReturnType();

			definition = from->getDefinition();

			if (functionReturnType)
			{
				returnTypeId = functionReturnType->id;
				this->returnType = functionReturnType->getName();
			}
		}
	};

	struct ScannedVariable
	{
		Token at;
		std::string name;
		std::string defaultValue;
		std::optional<SymbolDefinition> definition;
		std::string type;
		std::string inClass;
		TypeId typeId = 0;

		enum class Kind
		{
			localVariable,
			classMember,
		};

		Kind kind = Kind::localVariable;

		ScannedVariable(ScopeVariable* from, Token atToken)
		{
			this->at = atToken;
			this->name = from->name.string;

			definition = SymbolDefinition{
				.file = from->ownedBy->scopeFile,
				.at = from->name,
			};

			kind = Kind::localVariable;
			type = Type::toString(from->type);
			if (from->type)
			{
				typeId = from->type->id;
			}
		}

		ScannedVariable(ParsedClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken)
		{
			this->at = atToken;
			this->name = from->name.string;

			if (this->defaultValue.size())
			{
				this->defaultValue.pop_back();
			}

			definition = SymbolDefinition{
				.file = file,
				.at = from->name,
			};

			this->inClass = inClass->languageClass->classModule->name + "::" + inClass->languageClass->name.string;
			kind = Kind::classMember;
			type = Type::toString(from->type);
			if (from->type)
			{
				typeId = from->type->id;
			}
		}

		ScannedVariable(ClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken)
		{
			this->at = atToken;
			this->name = from->name.string;

			definition = SymbolDefinition{
				.file = file,
				.at = from->name,
			};

			this->inClass = Type::toString(inClass);
			kind = Kind::classMember;
			type = Type::toString(from->type);
			if (from->type)
			{
				typeId = from->type->id;
			}
		}
	};
}