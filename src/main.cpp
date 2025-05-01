#include <language.hpp>
#include <modules/standardLibrary.hpp>

int main()
{
	lang::LanguageContext language;
	lang::modules::registerStandardLibrary(&language);

	lang::ParseContext* compiler = language.createCompiler();
	compiler->addFile("test.lang");
	lang::BytecodeStream compiled = compiler->compile();
	delete compiler;

	lang::InterpretContext* interpreter = language.createInterpreter();
	interpreter->loadBytecode(&compiled);
	interpreter->run();
	delete interpreter;
}
