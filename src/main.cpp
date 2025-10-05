#ifndef NO_MAIN
#include <ds/language.hpp>
#include <ds/modules/standardLibrary.hpp>
#include <cassert>
#include <print>

using namespace ds;

int main()
{
	LanguageContext language;
	modules::registerStandardLibrary(&language);

	ParseContext* compiler = language.createCompiler();
	compiler->addFile("test.ds");
	BytecodeStream compiled = compiler->compile();
	delete compiler;

	InterpretContext* interpreter = language.createInterpreter();
	interpreter->loadBytecode(&compiled);
	interpreter->run();
	delete interpreter;

	std::println("classes leaked: {}", RuntimeClass::classRefCount);
	assert(RuntimeClass::classRefCount == 0);
}
#endif