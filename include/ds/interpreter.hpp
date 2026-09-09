#pragma once
#include "bytecode.hpp"
#include "baseRuntime.hpp"

namespace ds
{
	struct LanguageContext;

	struct BreakpointData
	{
		BytecodeOp oldOp;
		uint8_t oldArgLength;
	};

	class RuntimeInterpretContext : public InterpretContext
	{
	public:

		RuntimeInterpretContext(LanguageRuntime* runtime);

		// Inherited via InterpretContext
		void loadBytecode(BytecodeStream* code) override;
		InterpretContext* createSuspendedCopy(BytecodeOffset stackOffset, size_t streamPosition);
		RunResult run(Pointer position = 0) override;
		bool resumeSuspend() override;
		std::vector<DebugSection*> getStackTrace() const override;
		void doUnwind() override;
		InterpretContext* createCopy() override;
		bool setDebugBreakpoint(size_t instructionOffset) override;
		void removeDebugBreakpoint(size_t instructionOffset) override;
		void continueFromBreakpoint(size_t instructionOffset, BytecodeOffset& baseCallStackPos);

		BinaryBufferRef code;
		bool suspended = false;
		bool canAwait = false;
		BytecodeOffset suspendStackPos = 0;
		std::array<BytecodeOffset, CALL_STACK_SIZE> callStack = {};
		uint32_t callStackPos = 0;

		std::map<size_t, BreakpointData> breakpointInstructions;

		void debugBreak(BytecodeOffset& baseCallStackPos);

	private:
		RunResult lastResult = RunResult::ok;

		[[msvc::forceinline]]
		bool runInstruction(BytecodeOp op, uint8_t argsSize, BytecodeOffset& baseCallStackPos);
		void runLoop(BytecodeOffset& baseCallStackPos);
	};

} // namespace ds