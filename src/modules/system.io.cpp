#include <modules/system.io.hpp>
#include <parser/types/stringType.hpp>

using namespace lang;

static void writeLine(InterpretContext* context)
{
	auto rtString = context->popRuntimeString();
	std::fwrite(rtString.ptr(), rtString.length(), 1, stdout);
	std::fputc('\n', stdout);
}

static void write(InterpretContext* context)
{
	auto rtString = context->popRuntimeString();
	std::fwrite(rtString.ptr(), rtString.length(), 1, stdout);
}

static void writeInt(InterpretContext* context)
{
	std::puts(std::to_string(context->popValue<int32_t>()).c_str());
}

lang::NativeModule lang::modules::system::io::createModule()
{
	NativeModule out;
	out.name = "system::io";

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), Token("toWrite")) }, nullptr,
		"writeln", &writeLine));

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), Token("toWrite")) }, nullptr,
		"write", &write));

	out.functions.push_back(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("toWrite")) }, nullptr,
		"writeInt",	&writeInt));

	return out;
}