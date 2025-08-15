#include <language.hpp>
#include <modules/standardLibrary.hpp>
#include <cassert>
#include <print>

using namespace lang;

int main()
{
	LanguageContext language;
	modules::registerStandardLibrary(&language);

	ParseContext* compiler = language.createCompiler();
	compiler->addFile("test.lang");
	BytecodeStream compiled = compiler->compile();
	delete compiler;

	InterpretContext* interpreter = language.createInterpreter();
	interpreter->loadBytecode(&compiled);
	interpreter->run();
	delete interpreter;

	std::println("classes leaked: {}", RuntimeClass::classRefCount);
	assert(RuntimeClass::classRefCount == 0);
}
