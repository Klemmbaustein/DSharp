#include <modules/system.hpp>

lang::NativeModule lang::modules::system::createModule()
{
	NativeModule out;
	out.name = "system";

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(IntType::instance, Token("intArg")) },
		"println",
		[](InterpretContext* context) {
			std::puts(std::to_string(context->popValue<int32_t>()).c_str());
		}));

	out.attributes.push_back(new EntryPointAttribute());
	out.attributes.push_back(new DiscardAttribute());

	return out;
}
