#ifndef NO_MAIN
#include <ds/language.hpp>
#include <ds/modules/standardLibrary.hpp>
#include <cassert>
#include <condition_variable>
#include <print>
#include <ds/modules/system.async.hpp>

using namespace ds;
using namespace ds::modules::system::async;

static void awaitTask(LanguageRuntime* runtime)
{
	ClassPtr<Task> task = runtime->baseContext.popPtr<Task>();

	if (!task->completed)
	{
		bool completed = false;

		task->awaitNative = [](void* ptr) {
			*reinterpret_cast<bool*>(ptr) = true;
		};
		task->awaitNativeData = &completed;

		while (!completed)
		{
			std::this_thread::yield();
		}
	}
	std::println("task returned {}", getTaskResult<Int>(task.classPtr));
}

int main()
{
	LanguageContext language;
	modules::registerStandardLibrary(&language);

	ParseContext* compiler = language.createCompiler();
	compiler->addFile("../../../examples/test.ds");
	BytecodeStream compiled = compiler->compile();
	delete compiler;

	if (compiled.code.empty())
	{
		return 1;
	}

	LanguageRuntime* runtime = language.createRuntime();
	runtime->loadBytecode(&compiled);
	runtime->run();

	awaitTask(runtime);

	delete runtime;

	std::println("classes leaked: {}", RuntimeClass::classRefCount);
	assert(RuntimeClass::classRefCount == 0);
}
#endif