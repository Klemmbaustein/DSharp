#include <ds/jit/justInTime.hpp>
#include <ds/jit/justInTimeCompiler_x64.hpp>

using namespace ds;
using namespace ds::jit;

ds::jit::JustInTimeRuntime::JustInTimeRuntime(LanguageContext* from)
{
	this->language = from;
}

ds::jit::JustInTimeRuntime::~JustInTimeRuntime()
{
}

void ds::jit::JustInTimeRuntime::loadBytecode(BytecodeStream* code)
{
	runtime->debug = &code->debug;
	runtime->unwindBuffer = code->unwind;
	runtime->reflect = &code->reflect;
	runtime->externals.clear();
	runtime->externals.reserve(code->externalFunctions.size());

	for (auto& i : code->externalFunctions)
	{
		size_t lastColon = i.find_last_of(':');

		std::string first = i.substr(0, lastColon - 1);
		std::string second = i.substr(lastColon + 1);

		bool found = false;
		for (auto& fn : this->language->languageModules[first]->getFunctions())
		{
			if (fn->getFullName() == i)
			{
				runtime->externals.push_back(fn->function);
				found = true;
				break;
			}
		}

		if (!found)
		{
			std::printf("Could not find function: %s\n", i.c_str());
		}
	}

	runtime->vTable.clear();
	runtime->vTable.reserve(code->virtualTable.size());
	this->usedVTable = &runtime->vTable;
	for (auto& [offset, native] : code->virtualTable)
	{
		if (native)
		{
			runtime->vTable.push_back(RuntimeFunction{
				.nativeFn = runtime->externals[native] });
		}
		else
		{
			runtime->vTable.push_back(RuntimeFunction{
				.codeOffset = offset });
		}
	}

	JustInTimeCompiler compiler;
	this->code = compiler.compileBytecode(code->code, runtime->externals, runtime->vTable, code->reflect,
		runtime->unwindBuffer, runtime->debug);
}

void ds::jit::JustInTimeRuntime::run(Pointer atOffset)
{
	code->run(atOffset, this);
}

void ds::jit::JustInTimeRuntime::doUnwind()
{
	code->unwindStack(this->lastStackPos, this);
}

bool ds::jit::JustInTimeRuntime::resumeSuspend()
{
	if (suspendLocation)
	{
		auto oldLocation = suspendLocation;
		suspendLocation = nullptr;
		code->resume(oldLocation, this);
		return true;
	}
	return false;
}

std::vector<DebugSection*> ds::jit::JustInTimeRuntime::getStackTrace() const
{
	std::vector<Pointer> trace;
	code->getUnwindData(this->lastStackPos, trace);

	std::vector<DebugSection*> sections;

	for (auto& i : trace)
	{
		sections.push_back(runtime->debug->getSectionAt(i));
	}

	return sections;
}
InterpretContext* ds::jit::JustInTimeRuntime::createCopy()
{
	JustInTimeRuntime* copy = new JustInTimeRuntime(language);

	if (stackPos)
	{
		memcpy(copy->stack.data(), stack.data(), stackPos);
	}
	copy->stackPos = stackPos;
	if (variableStackPos)
	{
		memcpy(copy->variableStack.data(), variableStack.data(), variableStackPos);
	}
	copy->variableStackPos = variableStackPos;
	// if (callStackPos)
	//{
	//	memcpy(other->callStack.data(), callStack.data(), callStackPos);
	// }
	copy->code = code;
	copy->runtime = runtime;
	copy->usedVTable = usedVTable;

	return copy;
}

InterpretContext* ds::jit::JustInTimeRuntime::createSuspendedCopy(void* streamPosition)
{
	auto copy = reinterpret_cast<JustInTimeRuntime*>(createCopy());
	copy->canAwait = true;
	copy->suspendLocation = streamPosition;

	return copy;
}
