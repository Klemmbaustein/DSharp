#include <ds/language.hpp>
#include <ds/modules/standardLibrary.hpp>
#include <chrono>

using namespace ds;

static bool runTestFile(const char* file, LanguageContext& context)
{
	auto compiler = context.createCompiler({ .printAssembly = true });

	compiler->addFile(file);

	auto source = compiler->compile();

	if (source.code.empty())
	{
		return false;
	}

	delete compiler;

	std::cout << "=== Compiled ===" << std::endl;

	bool okay = true;

	for (int useJIT = 0; useJIT < 2; useJIT++)
	{
		std::cout << "=== Run (Use JIT: " << useJIT << ") ===" << std::endl;
		auto runtime = context.createRuntime({ .useJustInTimeCompiler = bool(useJIT) });

		runtime->writeError = [&okay, useJIT](const char* message) {
			std::cerr << "Runtime error: " << message << " (using JIT: " << useJIT << ")" << std::endl;
			okay = false;
		};

		runtime->loadBytecode(&source);
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		runtime->run();
		delete runtime;
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "=== Took " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << " microseconds ===" << std::endl;
	}

	std::cout << "Classes leaked: " << RuntimeClass::classRefCount << std::endl;

	return okay && !RuntimeClass::classRefCount;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		return 1;
	}

	LanguageContext context;

	modules::registerStandardLibrary(&context);

	try
	{
		return runTestFile(argv[1], context) ? 0 : 1;
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}