#include <modules/system.hpp>
#include <parser/stringType.hpp>

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
		{ FunctionArgument(StringType::getInstance(), Token("str")) },
		"format",
		[](InterpretContext* context) {
			auto string = context->popRuntimeString();

			context->pushRuntimeString(string);
		}));

	out.attributes.push_back(new EntryPointAttribute());
	out.attributes.push_back(new DiscardAttribute());

	return out;
}
