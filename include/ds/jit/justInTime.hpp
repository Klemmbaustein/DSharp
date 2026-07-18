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
		void run(Pointer position) override;
		void doUnwind() override;
		bool resumeSuspend() override;
		std::vector<DebugSection*> getStackTrace() const override;
		InterpretContext* createCopy() override;
		InterpretContext* createSuspendedCopy(void* streamPosition);

		size_t* lastStackPos = nullptr;
		void* suspendLocation = nullptr;
		bool canAwait = false;

	private:
		JustInTimeCode* code = nullptr;
		LanguageContext* language = nullptr;
		std::vector<ExternalFunctionPointer> externals;
	};
} // namespace ds::jit