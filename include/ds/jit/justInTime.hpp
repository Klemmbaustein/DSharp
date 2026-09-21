#pragma once
#include <ds/bytecode.hpp>
#include <ds/native/externalFunction.hpp>
#include <ds/language.hpp>

namespace ds::jit
{
	class JustInTimeCode;

	class JustInTimeRuntime : public InterpretContext
	{
	public:
		JustInTimeRuntime(LanguageContext* from);
		~JustInTimeRuntime();

		// Inherited via InterpretContext
		void loadBytecode(BytecodeStream* code) override;
		RunResult run(Pointer position) override;
		void doUnwind() override;
		bool resumeSuspend() override;
		std::vector<DebugSection*> getStackTrace() const override;
		InterpretContext* createCopy() override;
		InterpretContext* createSuspendedCopy(void* streamPosition);

		bool setDebugBreakpoint(Pointer instructionOffset) override;
		void removeDebugBreakpoint(Pointer instructionOffset) override;

		void* lastStackPos = nullptr;
		void* suspendLocation = nullptr;
		bool canAwait = false;
		std::shared_ptr<JustInTimeCode> code = nullptr;

	private:
		LanguageContext* language = nullptr;
	};
} // namespace ds::jit