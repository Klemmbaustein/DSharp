#include <ds/baseRuntime.hpp>
#include <ds/interpreter.hpp>
#include <ds/jit/justInTime.hpp>
#include <mutex>

using namespace ds;

static std::mutex threadsMutex;

ds::LanguageRuntime::~LanguageRuntime()
{
	do
	{
		std::map<size_t, std::thread*> threadsCopy;
		{
			std::lock_guard l{ threadsMutex };
			threadsCopy = this->backgroundThreads;
		}
		for (auto& [_, i] : threadsCopy)
		{
			i->join();
		}
	} while (backgroundThreads.size());

	delete this->baseContext;
}

void ds::LanguageRuntime::defaultCreateBackgroundThread(std::function<void()> f)
{
	static size_t id = 0;

	size_t thisThreadId = id++;

	std::lock_guard l{ threadsMutex };
	auto t = new std::thread([this, f, thisThreadId] {
		f();

		std::lock_guard l{ threadsMutex };
		this->backgroundThreads.erase(thisThreadId);
	});

	backgroundThreads.insert({ thisThreadId, t });
}

ds::LanguageRuntime::LanguageRuntime(LanguageContext* from, RuntimeOptions settings)
{
	createBackgroundThread = std::bind(&LanguageRuntime::defaultCreateBackgroundThread, this, std::placeholders::_1);
	this->language = from;

	if (settings.useJustInTimeCompiler)
	{
		createContext = [](LanguageRuntime* rt) -> InterpretContext* {
			auto context = new jit::JustInTimeRuntime(rt->language);
			context->runtime = rt;
			return context;
		};
	}
	else
	{
		createContext = [](LanguageRuntime* rt) -> InterpretContext* {
			auto context = new RuntimeInterpretContext(rt);
			return context;
		};
	}

	baseContext = createContext(this);
}

void ds::LanguageRuntime::loadBytecode(BytecodeStream* code)
{
	this->baseContext->loadBytecode(code);
}

void ds::LanguageRuntime::run(BytecodeOffset position)
{
	baseContext->run(position);
}

void ds::InterpretContext::pushRuntimeString(RuntimeStr str)
{
	str.classPtr->addRef();
	pushValue<RuntimeClass*>(str.classPtr);
}

ds::RuntimeStr ds::InterpretContext::popRuntimeString()
{
	return RuntimeStr(popValue<RuntimeClass*>());
}

std::vector<DebugSection*> ds::InterpretContext::getStackTrace() const
{
	return std::vector<DebugSection*>();
}

void ds::InterpretContext::runtimePanic(const char* message)
{
	std::vector<DebugSection*> stack = getStackTrace();

	std::string errorString = message;
	errorString.push_back('\n');

	for (DebugSection* i : stack)
	{
		if (i)
		{
			errorString += "\t" + i->name + "()\n";
		}
		else
		{
			errorString += "\t<unknown stack frame>\n";
		}
	}

	if (runtime->writeError)
	{
		runtime->writeError(errorString.c_str());
	}
	else
	{
		std::printf("%s", errorString.c_str());
	}

	doUnwind();
}

void ds::InterpretContext::virtualCall(RuntimeFunction target)
{
	if (!target)
	{
		return;
	}

	if (target.nativeFn)
	{
		target.nativeFn(this);
	}
	else
	{
		run(target.codeOffset);
	}
}

void ds::InterpretContext::destruct(RuntimeClass* classObject)
{
	auto ptr = RuntimeClass::unref(classObject);

	if (ptr)
	{
		pushValue(classObject);
		virtualCall(ptr);
	}
}

std::string ds::InterpretContext::popString()
{
	RuntimeClass* ptr = popValue<RuntimeClass*>();

	std::string out = { (const char*)(ptr->getBody() + sizeof(Size)),
		*(Size*)ptr->getBody() };
	RuntimeClass::unref(ptr);
	return out;
}

ds::RuntimeStrRef ds::InterpretContext::popRuntimeStringRef()
{
	return RuntimeStrRef(popValue<RuntimeClass*>());
}
