#ifdef WITH_LANGUAGE_SERVICE
#include <ds/service/scannedSymbols.hpp>
#include <ds/parser/function.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/parseClass.hpp>

ds::ScannedFunction::ScannedFunction(Function* from, Token atToken, Kind kind, TokenPos argEnd)
{
	this->at = atToken;

	this->name = from->getFullName();
	this->shortName = from->getShortName();
	this->argEnd = argEnd;
	this->kind = kind;
	auto functionArgs = from->getArguments();
	this->arguments.reserve(functionArgs.size());
	this->isVirtual = from->isVirtual();

	for (auto& i : functionArgs)
	{
		this->arguments.push_back({ Type::toString(i.type), i.name.string });
	}

	auto functionReturnType = from->getReturnType();

	definition = from->getDefinition();

	if (functionReturnType)
	{
		returnTypeId = functionReturnType->id;
		returnType = Type::toString(functionReturnType);
	}
}

ds::ScannedFunction::ScannedFunction(Function* from, const GenericParseData* generic, Token atToken, Kind kind, TokenPos argEnd)
	: ScannedFunction(from, atToken, kind, argEnd)
{
	arguments.clear();
	for (auto& i : generic->args)
	{
		this->arguments.push_back({ Type::toString(i.type), i.name.string });
	}

	auto functionReturnType = generic->returnType;
	if (functionReturnType)
	{
		returnTypeId = functionReturnType->id;
		returnType = Type::toString(functionReturnType);
	}
}

ds::ScannedVariable::ScannedVariable(ScopeVariable* from, Token atToken)
{
	this->at = atToken;
	this->name = from->name.string;
	this->isThis = from->isThis;

	definition = SymbolDefinition{
		.file = from->ownedBy->scopeFile->displayName,
		.at = from->name,
	};

	kind = Kind::localVariable;
	type = Type::toString(from->type);
	if (from->type)
	{
		typeId = from->type->id;
	}
}

ds::ScannedVariable::ScannedVariable(ParsedClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken)
{
	this->at = atToken;
	this->name = from->name.string;

	definition = SymbolDefinition{
		.file = file ? file->displayName : "",
		.at = from->name,
	};

	this->inClass = Type::toFullString(inClass);
	kind = Kind::classMember;
	type = Type::toString(from->type);
	if (from->type)
	{
		typeId = from->type->id;
	}
}

ds::ScannedVariable::ScannedVariable(ClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken)
{
	this->at = atToken;
	this->name = from->name.string;

	definition = SymbolDefinition{
		.file = file ? file->displayName : "",
		.at = from->name,
	};

	auto source = from->source ? from->source : inClass;

	this->inClass = Type::toFullString(source);
	kind = Kind::classMember;
	type = Type::toString(from->type);
	if (from->type)
	{
		typeId = from->type->id;
	}
}
#endif