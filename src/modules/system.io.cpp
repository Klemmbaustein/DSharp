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

static void readLine(InterpretContext* context)
{
	char lineBuffer[2048];
	std::cin.getline(lineBuffer, sizeof(lineBuffer) - 1);

	context->pushRuntimeString(RuntimeStr(lineBuffer, strnlen(lineBuffer, sizeof(lineBuffer - 1))));
}

lang::NativeModule lang::modules::system::io::createModule()
{
	NativeModule out;
	out.name = "system::io";

	out.addFunction(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), Token("toWrite")) }, nullptr,
		"writeln", &writeLine));

	out.addFunction(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), Token("toWrite")) }, nullptr,
		"write", &write));

	out.addFunction(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("toWrite")) }, nullptr,
		"writeInt",	&writeInt));

	out.addFunction(NativeFunction(
		{}, StringType::getInstance(),
		"readln", &readLine));

	return out;
}