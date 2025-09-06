#include <modules/system.io.hpp>
#include <parser/types/stringType.hpp>
#include <cstring>
#include <cstdio>

using namespace lang::modules::system;
using namespace lang;

static void io_writeLine(InterpretContext* context)
{
	auto rtString = context->popRuntimeString();

	std::fwrite(rtString.ptr(), rtString.length(), 1, stdout);
	std::fputc('\n', stdout);
}

static void io_write(InterpretContext* context)
{
	auto rtString = context->popRuntimeString();
	std::fwrite(rtString.ptr(), rtString.length(), 1, stdout);
}

static void io_writeInt(InterpretContext* context)
{
	std::puts(std::to_string(context->popValue<int32_t>()).c_str());
}

static void io_readLine(InterpretContext* context)
{
	char lineBuffer[2048];
	std::cin.getline(lineBuffer, sizeof(lineBuffer) - 1);

	size_t len = strnlen(lineBuffer, sizeof(lineBuffer) - 1);

	context->pushRuntimeString(RuntimeStr(lineBuffer, len));
}

static void io_file_destruct(InterpretContext* context)
{
	ClassPtr<io::File> thisPtr = context->popValue<RuntimeClass*>();
	std::fclose(thisPtr->handle);
}

#if HAS_POPEN
static void io_procFile_destruct(InterpretContext* context)
{
	ClassPtr<io::File> thisPtr = context->popValue<RuntimeClass*>();
#if WIN32
	_pclose(thisPtr->handle);
	#else
	pclose(thisPtr->handle);
#endif
}

static VTableEntry io_procFile_vTable = VTableEntry{
	.nativeFn = &io_procFile_destruct,
};

#endif

static VTableEntry io_file_vTable = VTableEntry{
	.nativeFn = &io_file_destruct,
};

static void io_file_construct(InterpretContext* context)
{
	ClassRef<io::File> thisPtr = context->popValue<RuntimeClass*>();
	thisPtr.classPtr->vtable = &io_file_vTable;
	RuntimeStr filePath = context->popRuntimeString();

	thisPtr->handle = std::fopen(filePath.ptr(), "r");

	if (!thisPtr->handle)
	{
		context->runtimePanic(RuntimeStr(std::strerror(errno)));
	}

	context->pushValue(thisPtr);
}

static void io_file_readLine(InterpretContext* context)
{
	ClassRef<io::File> thisPtr = context->popValue<RuntimeClass*>();

	std::string result;

	char c = 0;
	while (true)
	{
		c = std::fgetc(thisPtr->handle);

		if (c == '\n' || c == EOF)
		{
			break;
		}
		if (c == '\r')
		{
			continue;
		}
		result.push_back(c);
	}

	context->pushRuntimeString(RuntimeStr(result.data(), result.size()));
}

static void io_file_isEmpty(InterpretContext* context)
{
	ClassRef<io::File> thisPtr = context->popValue<RuntimeClass*>();

	context->pushValue<Bool>(std::feof(thisPtr->handle));
}
#if HAS_POPEN

static void io_popen(InterpretContext* context)
{
	RuntimeStr command = context->popRuntimeString();
	ClassRef<io::File> newFile = RuntimeClass::allocateClass(sizeof(io::File), &io_procFile_vTable);

#if _WIN32
	newFile->handle = _popen(command.ptr(), "r");
#else
	newFile->handle = popen(command.ptr(), "r");

#endif

	if (!newFile->handle)
	{
		context->runtimePanic(RuntimeStr(std::strerror(errno)));
	}

	context->pushValue(newFile);
}
#endif

lang::NativeModule lang::modules::system::io::createModule()
{
	NativeModule out;
	out.name = "system::io";

	out.addFunction(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), "toWrite") }, nullptr,
		"writeln", &io_writeLine));

	out.addFunction(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), "toWrite") }, nullptr,
		"write", &io_write));

	out.addFunction(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), "toWrite") }, nullptr,
		"writeInt", &io_writeInt));

	out.addFunction(NativeFunction(
		{}, StringType::getInstance(),
		"readln", &io_readLine));

	auto fileClass = out.createClass<io::File>("File");

	out.addClassConstructor(fileClass,
		NativeFunction(
			{ FunctionArgument(StringType::getInstance(), "path") }, nullptr,
			"File.new", &io_file_construct));

	out.addClassMethod(fileClass,
		NativeFunction(
			{}, StringType::getInstance(),
			"readln", &io_file_readLine));

	out.addClassMethod(fileClass,
		NativeFunction(
			{}, BoolType::getInstance(),
			"isEmpty", &io_file_isEmpty));

#if HAS_POPEN
	out.addFunction(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), "command") }, fileClass,
		"popen", &io_popen));
#endif

	return out;
}