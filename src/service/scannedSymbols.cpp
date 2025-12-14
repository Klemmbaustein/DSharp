#ifdef WITH_LANGUAGE_SERVICE
#include <ds/service/scannedSymbols.hpp>
#include <ds/parser/function.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/parseClass.hpp>

ds::ScannedFunction::ScannedFunction(Function* from, Token atToken, Kind kind, TokenPos argEnd)
{
	this->at = atToken;
	this->name = from->getFullName();
	this->name = from->getShortName();
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

ds::ScannedVariable::ScannedVariable(ScopeVariable* from, Token atToken)
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

ds::ScannedVariable::ScannedVariable(ParsedClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken)
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

ds::ScannedVariable::ScannedVariable(ClassMember* from, ClassType* inClass, ParsedFile* file, Token atToken)
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
#endif