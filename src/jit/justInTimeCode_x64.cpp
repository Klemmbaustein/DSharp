#include <ds/jit/justInTimeCode_x64.hpp>
#include <ds/jit/justInTimeCompiler_x64.hpp>

using namespace ds;

void ds::jit::JustInTimeCode::run(Pointer at, JustInTimeRuntime* runtime)
{
	if (!unwinding && setjmp(returnBuffer))
	{
		return;
	}
	if (at == 0)
	{
		auto label = compiled.label_id_by_name("entry");

		auto target = compiled.label_offset(label);

		uint8_t* offset = (uint8_t*)entry + target;
		entry(offset, runtime);
	}
	else
	{
		entry((void*)at, runtime);
	}
}

void ds::jit::JustInTimeCode::resume(void* at, JustInTimeRuntime* runtime)
{
	if (!unwinding && setjmp(returnBuffer))
	{
		return;
	}

	auto label = compiled.label_id_by_name("resume");

	auto target = compiled.label_offset(label);

	uint8_t* offset = (uint8_t*)entry + target;

	auto newEntry = (JitEntryFunction)offset;

	newEntry(at, runtime);
}

void ds::jit::JustInTimeCode::getUnwindData(void* atPtr, std::vector<Pointer>& outPointers)
{
	// Very goofy stack shenanigans. atPtr is a previously saved value of rbp.
	uint64_t* functionPtr = reinterpret_cast<uint64_t*>(atPtr) - 9;

	do
	{
		outPointers.push_back(*functionPtr);
		// Move 80 bytes down the stack, which is where the next call location will be.
		functionPtr += 10;
#if _WIN32
	} while (*(functionPtr + 11) != JustInTimeCompiler::MANAGED_STACK_BEGIN_MARKER);
#else
	} while (*(functionPtr + 9) != JustInTimeCompiler::MANAGED_STACK_BEGIN_MARKER);
#endif
}

void ds::jit::JustInTimeCode::unwindStack(void* atPtr, JustInTimeRuntime* rt)
{
	std::vector<Pointer> callAddresses;
	getUnwindData(atPtr, callAddresses);
	unwinding = true;

	auto& buffer = rt->runtime->unwindBuffer;

	size_t callStackPos = callAddresses.size();

	for (int32_t i = 0; i < callStackPos; i++)
	{
		Pointer codePos = callAddresses[i];

		auto tbl = buffer.getSectionAt(codePos);

		if (!tbl)
		{
			std::cerr << "No unwind info for " << codePos << std::endl;
			continue;
		}

		for (auto& p : tbl->parts)
		{
			if (p.offset <= codePos)
			{
				continue;
			}
			switch (p.op)
			{
			case UnwindOp::popClass: {
				if (p.start >= codePos)
				{
					break;
				}
				ds::RuntimeClass* c;
				memcpy(&c, &rt->variableStack[rt->variableStackPos - p.size], sizeof(ds::RuntimeClass*));
				rt->destruct(c);
				break;
			}
			case UnwindOp::popBytes: {
				rt->variableStackPos -= p.size;
				break;
			}
			case UnwindOp::pushBytes: {
				rt->variableStackPos += p.size;
				break;
			}
			default:
				break;
			}
		}
	}
	rt->stackPos = 0;
	unwinding = false;

//	this->doUnwind(this->unwindBuffer);

#ifdef _MSC_VER
	// Disable MSVC's stack unwinding for longjmp since that doesn't work with the JIT code.
	((_JUMP_BUFFER*)returnBuffer)->Frame = 0;
#endif

	longjmp(returnBuffer, 1);
}
