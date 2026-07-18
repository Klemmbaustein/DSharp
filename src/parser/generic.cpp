#include <ds/parser/generic.hpp>
#include <ds/parser/types/classType.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/types/genericArgument.hpp>

using namespace ds;

std::vector<Type*> ds::parseGenericArguments(std::vector<GenericArgument> args,
	TokenLine& line, ErrorContext* errors, ParsedFile* with)
{
	std::vector<Type*> foundTypes;

	if (line.expect("<", errors))
	{
		return foundTypes;
	}

	if (line.peek() == ">")
	{
		line.get();
		return {};
	}

	while (true)
	{
		auto newType = with->getType(line, errors);

		if (newType)
		{
			foundTypes.push_back(newType);
		}
		else
		{
			auto typeName = line.get();
			errors->error(ErrorCode::parseInvalidType, typeName, "Unknown type: '" + typeName.string + "'");
		}
		auto& next = line.get();

		if (next == ">")
		{
			break;
		}
		else if (next != ",")
		{
			errors->error(ErrorCode::parseUnexpectedToken, next, "Unexpected '" + next.string + "'");
			break;
		}
	}

	return foundTypes;
}
bool ds::checkGenericArguments(std::vector<GenericArgument> args, std::vector<Type*> types,
	Token at, ErrorContext* errors)
{
	if (types.size() != args.size())
	{
		errors->error(ErrorCode::parseInvalidType, at, "Wrong number of arguments");
		return false;
	}

	for (size_t i = 0; i < args.size(); i++)
	{
		if (types[i] && args[i].baseClassType)
		{
			auto foundClass = dynamic_cast<ClassType*>(types[i]);

			if (!foundClass)
			{
				errors->error(ErrorCode::parseInvalidType, at, "Invalid type");
			}

			else if (!foundClass->isSubclassOf(args[i].baseClassType) && !foundClass->sameAs(args[i].baseClassType))
			{
				errors->error(ErrorCode::parseInvalidType, at, "Invalid type " + Type::toString(foundClass) +
					" is not a subclass of " + Type::toString(args[i].baseClassType));
			}
		}
	}

	return true;
}

BytecodeBuffer ds::compileGenericArguments(std::vector<Type*> types)
{
	BytecodeBuffer result;

	for (size_t i = 0; i < types.size(); i++)
	{
		BinaryBuffer genericData;
		genericData.addValue(types[i]->id);
		genericData.addValue(types[i]->size);

		auto classType = types[i]->asClass();

		genericData.addValue<Bool>(classType && !classType->isByValueType);
		result.addOperation(BytecodeOp::push, genericData);
	}

	return result;
}

GenericParseData ds::getGenericFunctionData(ds::Function* fn, TokenLine& line, ErrorContext* errors, ParsedScope* with)
{
	if (!fn->isGeneric())
	{
		return GenericParseData{
			.args = fn->getArguments(),
			.returnType = fn->getReturnType(),
		};
	}

	auto functionTypes = fn->getGenericTypes();
	auto args = parseGenericArguments(functionTypes, line, errors, with->scopeFile);

	checkGenericArguments(functionTypes, args, line.previous(), errors);

	GenericParseData outData;

	outData.code = compileGenericArguments(args);
	outData.returnType = convertGenericType(fn->getReturnType(), args, true,
		line.previous(), errors, with->context->registry);
	outData.args = fn->getArguments();

	for (auto& i : outData.args)
	{
		i.type = convertGenericType(i.type, args, true, line.previous(), errors, with->context->registry);
	}

	return outData;
}

Type* ds::convertGenericType(Type* inType, std::vector<Type*> args, bool isFunction, Token at,
	ErrorContext* errors, TypeRegistry* registry)
{
	if (!inType)
	{
		return inType;
	}
	std::vector<Type*> genericArguments = inType->getGenericTypes();
	if (!genericArguments.empty())
	{
		bool changed = false;
		std::vector<Type*> newArguments;

		for (auto& i : genericArguments)
		{
			Type* newType = convertGenericType(i, args, isFunction, at, errors, registry);
			if (newType != i)
			{
				changed = true;
			}
			newArguments.push_back(newType);
		}

		return changed ? inType->instantiateGeneric(newArguments, at, errors, registry) : inType;
	}

	auto genericType = dynamic_cast<GenericArgumentType*>(inType);

	if (genericType && genericType->isFunctionIndex == isFunction)
	{
		if (genericType->index >= args.size())
		{
			return nullptr;
		}

		auto type = args[genericType->index];

		if (genericType->isNullable)
		{
			auto classType = type->asClass();

			if (classType && !classType->isByValueType)
			{
				return classType->nullable;
			}
		}

		return type;
	}

	return inType;
}