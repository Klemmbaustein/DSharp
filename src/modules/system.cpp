#include <modules/system.hpp>
#include <parser/stringType.hpp>
#include <print>

using namespace lang;

static void format(InterpretContext* context)
{
	uint32_t varArgsCount = context->getVarArgsCount();

	std::vector<RuntimeStr> strings;

	for (uint32_t i = 0; i < varArgsCount; i++)
	{
		strings.push_back(context->popRuntimeString());
	}

	auto string = context->popRuntimeString();

	std::string result;
	size_t stringsIt = 0;

	for (uint32_t i = 0; i < string.length(); i++)
	{
		char newChar = string.ptr()[i];

		if (newChar == '{')
		{
			auto& newString = strings[stringsIt++];
			result.append(std::string(newString.ptr(), newString.length()));
			i++;
		}
		else
		{
			result.push_back(newChar);
		}
	}

	context->pushRuntimeString(RuntimeStr(result.data(), result.size()));
}

static void int_toString(InterpretContext* context)
{
	auto str = std::to_string(context->popValue<uint32_t>());
	context->pushRuntimeString(RuntimeStr(str.data(), str.size()));
}

lang::NativeModule lang::modules::system::createModule()
{
	NativeModule out;
	out.name = "system";

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), Token("toPrint")) },
		"println",
		[](InterpretContext* context) {
			auto popped = context->popString();
			std::puts(popped.c_str());
		}));
	out.functions.push_back(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("toPrint")) },
		"printInt",
		[](InterpretContext* context) {
			std::puts(std::to_string(context->popValue<int32_t>()).c_str());
		}));

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("position")), FunctionArgument(IntType::getInstance(), Token("length")) },
		"string.substr",
		[](InterpretContext* context) {
			std::puts(std::to_string(context->popValue<int32_t>()).c_str());
		}));

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("intValue")) },
		"int.toString", &int_toString));

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), Token("str")) },
		"format", &format));

	out.attributes.push_back(new EntryPointAttribute());
	out.attributes.push_back(new DiscardAttribute());

	return out;
}
