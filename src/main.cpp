#ifdef WITH_MAIN
#include <ds/language.hpp>
#include <ds/modules/standardLibrary.hpp>
#include <print>
#include <ds/jit/justInTime.hpp>

using namespace ds;

int main()
{
	LanguageContext language;
	modules::registerStandardLibrary(&language);

	ParseContext* compiler = language.createCompiler({
		.printAssembly = true,
		});
	compiler->addFile("../../../examples/async.ds");
	BytecodeStream compiled = compiler->compile();
	delete compiler;

	if (compiled.code.empty())
	{
		return 1;
	}

	LanguageRuntime* runtime = language.createRuntime({
		.useJustInTimeCompiler = false,
		});
	runtime->loadBytecode(&compiled);
	runtime->run();

	delete runtime;

	std::println("classes leaked: {}", RuntimeClass::classRefCount);
}
#endif