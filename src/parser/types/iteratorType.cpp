#include <ds/parser/types/iteratorType.hpp>
#include <ds/parser/types/builtinClassFunction.hpp>

using namespace ds;

ds::IteratorType::IteratorType(Type* baseType, TypeRegistry* registry)
{
	this->name = "iterator";
	this->baseType = baseType;
	this->isInterface = true;
	this->isGeneric = true;

	this->methods.insert({ "next",
		new BuiltinClassFunction({},
			registry->getEntry<BoolType>(), "system::iterator.next", "next", 1, this) });
	this->methods.insert({ "get",
		new BuiltinClassFunction({},
			baseType, "system::iterator.get", "get", 2, this) });
}

ds::IteratorType::~IteratorType()
{
	for (auto& i : this->methods)
	{
		delete i.second.function;
	}
}

BytecodeBuffer ds::IteratorType::compileNext(BytecodeBuffer thisValue, ErrorContext* errors, ParsedScope* with)
{
	ExpressionResult thisValueExpr;
	thisValueExpr.valid = true;
	thisValueExpr.type = this;
	thisValueExpr.code = thisValue;
	return compileMethodDirect(Token("next"), thisValueExpr, errors, with).code;
}

ExpressionResult ds::IteratorType::compileGet(BytecodeBuffer thisValue, ErrorContext* errors, ParsedScope* with)
{
	ExpressionResult thisValueExpr;
	thisValueExpr.valid = true;
	thisValueExpr.type = this;
	thisValueExpr.code = thisValue;
	return compileMethodDirect(Token("get"), thisValueExpr, errors, with);
}
