#include <ds/jit/justInTimeCode_x64.hpp>
#include <ds/jit/justInTimeCompiler_x64.hpp>
#include <ds/jit/windowsJitDebugger.hpp>
#include <ds/jit/posixJitDebugger.hpp>

using namespace ds;

void ds::jit::JustInTimeCode::run(Pointer at, JustInTimeRuntime* runtime)
{
	if (this->debugger)
	{
		this->debugger->makeActive();
	}
	jmp_buf oldTarget;
	memcpy(oldTarget, returnBuffer, sizeof(returnBuffer));

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
	memcpy(returnBuffer, oldTarget, sizeof(returnBuffer));
}

void ds::jit::JustInTimeCode::resume(void* at, JustInTimeRuntime* runtime)
{
	if (this->debugger)
	{
		this->debugger->makeActive();
	}
	jmp_buf oldTarget;
	memcpy(oldTarget, returnBuffer, sizeof(returnBuffer));

	if (!unwinding && setjmp(returnBuffer))
	{
		return;
	}

	auto label = compiled.label_id_by_name("resume");

	auto target = compiled.label_offset(label);

	uint8_t* offset = (uint8_t*)entry + target;

	auto newEntry = (JitEntryFunction)offset;

	newEntry(at, runtime);
	memcpy(returnBuffer, oldTarget, sizeof(returnBuffer));
}

uint8_t ds::jit::JustInTimeCode::insertBreakpoint(void* at, uint8_t byte)
{
	if (!debugger)
	{
		return false;
	}

	asmjit::JitAllocator::Span breakpointSpan;
	auto result = jit.allocator().query(asmjit::Out<asmjit::JitAllocator::Span>(breakpointSpan), at);

	if (result != asmjit::Error::kOk)
	{
		throw result;
	}

	uint8_t interruptInstruction[] = { byte };

	Pointer diff = Pointer(at) - Pointer(breakpointSpan.rx());
	uint8_t oldByte = *(reinterpret_cast<uint8_t*>(breakpointSpan.rw()) + diff);

	result = jit.allocator().write(breakpointSpan, diff, interruptInstruction, sizeof(interruptInstruction));

	if (result != asmjit::Error::kOk)
	{
		throw result;
	}

	return oldByte;
}

void ds::jit::JustInTimeCode::insertBreakpoint(void* at)
{
	this->debugReplacedBytes.insert({ Pointer(at), insertBreakpoint(at, INSTRUCTION_INT3) });
}

void ds::jit::JustInTimeCode::restoreBreakpoint(void* at)
{
	auto old = debugReplacedBytes.find(Pointer(at));

	if (old != debugReplacedBytes.end())
	{
		insertBreakpoint(at, INSTRUCTION_INT3);
	}
}

bool ds::jit::JustInTimeCode::clearBreakpoint(void* at)
{
	auto old = debugReplacedBytes.find(Pointer(at));

	if (old != debugReplacedBytes.end())
	{
		insertBreakpoint(at, old->second);
		return true;
	}
	return false;
}

void ds::jit::JustInTimeCode::removeBreakpoint(void* at)
{
	auto old = debugReplacedBytes.find(Pointer(at));

	if (old != debugReplacedBytes.end())
	{
		insertBreakpoint(at, old->second);
		debugReplacedBytes.erase(old);
	}
}

void ds::jit::JustInTimeCode::getUnwindData(void* atPtr, std::vector<Pointer>& outPointers, bool skipFirst)
{
	// Very goofy stack shenanigans. atPtr is a previously saved value of rbp.
	uint64_t* functionPtr = reinterpret_cast<uint64_t*>(atPtr) - 9;

	do
	{
		if (!skipFirst)
		{
			outPointers.push_back(*functionPtr);
		}
		skipFirst = false;
		// Move 80 bytes down the stack, which is where the next call location will be.
		functionPtr += 10;
	} while (*(functionPtr + 9) != JustInTimeCompiler::MANAGED_STACK_BEGIN_MARKER);
}

void ds::jit::JustInTimeCode::unwindStack(void* atPtr, JustInTimeRuntime* rt)
{
	std::vector<Pointer> callAddresses;
	bool isSuspended = false;
	if (rt->suspendLocation)
	{
		callAddresses = { Pointer(rt->suspendLocation) };
		rt->suspendLocation = nullptr;
		isSuspended = true;
	}
	else
	{
		getUnwindData(atPtr, callAddresses);
	}

	doUnwind(callAddresses, rt, isSuspended);
}

void ds::jit::JustInTimeCode::doUnwind(std::vector<Pointer> callAddresses, JustInTimeRuntime* rt, bool isSuspended)
{
	unwinding = true;

	auto& buffer = rt->runtime->unwindBuffer;

	size_t callStackPos = callAddresses.size();

	jmp_buf returnTarget;
	memcpy(&returnTarget, &returnBuffer, sizeof(jmp_buf));
	rt->stackPos = 0;

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
			if (p.offset < codePos)
			{
				continue;
			}
			switch (p.op)
			{
			case UnwindOp::popClass: {
				if (p.start > codePos)
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
	unwinding = false;

	if (!isSuspended)
	{
#ifdef _MSC_VER
		// Disable MSVC's stack unwinding for longjmp since that doesn't work with the JIT code.
		((_JUMP_BUFFER*)&returnTarget[0])->Frame = 0;
#endif
		longjmp(returnTarget, 1);
	}
}

void ds::jit::JustInTimeCode::initializeBreakpointHandler(JustInTimeRuntime* rt)
{
	try
	{
#if _WIN32
		debugger = new WindowsJitDebugger(rt);
#elif _POSIX_VERSION
		//debugger = new PosixJitDebugger(rt);
#endif
	}
	catch (JitDebugNotSupportedException& e)
	{
	}
}
