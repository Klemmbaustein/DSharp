#pragma once
#include "bytecode.hpp"
#include "baseRuntime.hpp"

namespace ds
{
	struct LanguageContext;

	class RuntimeInterpretContext : public InterpretContext
	{
	public:

		RuntimeInterpretContext(LanguageRuntime* runtime);

		// Inherited via InterpretContext
		void loadBytecode(BytecodeStream* code) override;
		InterpretContext* createSuspendedCopy(BytecodeOffset stackOffset, size_t streamPosition);
		void run(Pointer position = 0) override;
		bool resumeSuspend() override;
		std::vector<DebugSection*> getStackTrace() const override;
		void doUnwind() override;
		InterpretContext* createCopy() override;

		BinaryBufferRef code;
		bool suspended = false;
		bool canAwait = false;
		BytecodeOffset suspendStackPos = 0;
		std::array<BytecodeOffset, CALL_STACK_SIZE> callStack = {};
		uint32_t callStackPos = 0;


	private:
		void runLoop(BytecodeOffset& baseCallStackPos);
	};

} // namespace ds