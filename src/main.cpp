#include <language.hpp>
#include <modules/standardLibrary.hpp>

int main()
{
	using namespace lang;

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
}
