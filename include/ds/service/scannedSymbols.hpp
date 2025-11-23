#pragma once
#include <ds/parser/tokens.hpp>
#include <ds/parser/symbolDefinition.hpp>
#include <optional>
#include <ds/languageTypes.hpp>

namespace ds
{
	class Function;
	struct ScopeVariable;
	struct ParsedClassMember;
	struct ClassMember;
	class ClassType;

	struct ScannedFunction
	{
		Token at;
		TokenPos argEnd;
		std::string name;
		std::string shortName;
		std::optional<SymbolDefinition> definition;
		std::string returnType;
		TypeId returnTypeId = 0;
		std::vector<std::pair<std::string, std::string>> arguments;

		enum class Kind
		{
			functionCall,
			functionDefinition,
			functionReference,
			classMember,
		};

		Kind kind = Kind::functionCall;

		ScannedFunction(Function* from, Token atToken, Kind kind, TokenPos argEnd = TokenPos());
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

		ScannedVariable(ScopeVariable* from, Token atToken);

		ScannedVariable(ParsedClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken);

		ScannedVariable(ClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken);
	};

	class ScannedMember
	{
	public:
		TypeId memberTypeId;
		std::string memberTypeName;
		std::string name;
	};

	class ScannedType
	{
	public:
		std::string name;
		TypeId id = 0;

		std::vector<ScannedMember> members;
		std::vector<ScannedFunction> methods;
	};

}