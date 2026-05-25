#pragma once
#include <ds/binaryBuffer.hpp>
#include <asmjit/x86.h>
#include <ds/native/externalFunction.hpp>
#include <map>
#include <optional>
#include <utility>
#include <ds/languageTypes.hpp>
#include <ds/jit/justInTime.hpp>
#include <ds/jit/justInTimeCode_x64.hpp>

namespace ds::jit
{
	class JustInTimeRuntime;

	struct StackValue
	{
		bool isNumber = false;
		asmjit::x86::Gp gpRegister;
		size_t number = 0;
		size_t size = 0;
		size_t stackDiff = 0;
	};

	class JustInTimeCompiler
	{
	public:
		JustInTimeCode* compileBytecode(BinaryBuffer& code,
			const std::vector<ds::ExternalFunctionPointer>& pointers,
			std::vector<ds::RuntimeFunction>& vTable,
			ReflectInfo& reflect,
			UnwindInfo& unwind);

		static constexpr Pointer MANAGED_STACK_BEGIN_MARKER = 12345;

		~JustInTimeCompiler();

	private:
		/**
		 * @brief
		 * Maps bytecode offsets to their AsmJit label IDs
		 */
		std::map<BytecodeOffset, asmjit::Label> functionMappings;
		std::map<BytecodeOffset, asmjit::Label> jumpTargetMappings;
		std::map<BytecodeOffset, asmjit::Label> unwindMappings;
		std::vector<std::pair<std::vector<uint8_t>, asmjit::Label>> embeddedStrings;

		const asmjit::x86::Gp runtimeRegister = asmjit::x86::rdx;
		const asmjit::x86::Gp stackRegister = asmjit::x86::rcx;
		const asmjit::x86::Mem stackPos = ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, stackPos));
		const asmjit::x86::Mem varStackPos = ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, variableStackPos));

		JustInTimeCode* result = new JustInTimeCode();
		asmjit::x86::Assembler* assembler = nullptr;
		asmjit::x86::Mem runtime = asmjit::x86::ptr_64(asmjit::x86::rsp, 32);

#ifdef _WIN32
		const std::array<asmjit::x86::Gp, 4> argumentRegisters = {
			asmjit::x86::rcx,
			asmjit::x86::rdx,
			asmjit::x86::r8,
			asmjit::x86::r9,
		};
		const std::array<asmjit::x86::Gp, 4> halfArgumentRegisters = {
			asmjit::x86::ecx,
			asmjit::x86::edx,
			asmjit::x86::r8d,
			asmjit::x86::r9d,
		};
#elif __linux__
		const std::array<asmjit::x86::Gp, 4> argumentRegisters = {
			asmjit::x86::rdi,
			asmjit::x86::rsi,
			asmjit::x86::rdx,
			asmjit::x86::rcx,
		};
		const std::array<asmjit::x86::Gp, 4> halfArgumentRegisters = {
			asmjit::x86::edi,
			asmjit::x86::esi,
			asmjit::x86::edx,
			asmjit::x86::ecx,
		};
#endif

		const asmjit::x86::Gp returnValueRegister = asmjit::x86::rax;

		void scanForFunctions(BinaryBuffer& code, std::vector<ds::RuntimeFunction>& vTable,
			ReflectInfo& reflect, UnwindInfo& unwind);

		void compileToAssembly(BinaryBuffer& code,
			const std::vector<ds::ExternalFunctionPointer>& pointers,
			std::vector<ds::RuntimeFunction>& vTable);

		void buildVTable(std::vector<ds::RuntimeFunction>& vTable);
		void updateReflectionOffsets(ReflectInfo& reflect);
		void updateUnwindOffsets(UnwindInfo& unwind);

		void buildProlog();

		/**
		 * @brief
		 * Compiles various instructions pushing data to the stack.
		 *
		 * If the next instruction that uses the stack will use that value,
		 * it will never be pushed to the stack but instead stored in registers.
		 */
		void compilePushValue(asmjit::x86::Gp gpRegister);

		/**
		 * @brief
		 * Compiles various instructions pushing data to the stack.
		 *
		 * If the next instruction that uses the stack will use that value,
		 * it will never be pushed to the stack but instead stored in registers.
		 */
		void compilePushValue(size_t value, size_t size);

		/**
		 * @brief
		 * Compiles a memory copy from the rsi register to the rdi register.
		 *
		 */
		void compileMemoryCopy(size_t size);

		/**
		 * @brief
		 * Compiles a memory copy from the rsi register to the rdi register.
		 *
		 */
		void compileMemoryCopy(asmjit::x86::Gp size);
		void restoreRegisters();
		void getStack();

		void compileAbort(const char* msg);

		/**
		 * @brief
		 * Compiles popping a value from the stack.
		 *
		 * If the size is not 1, 4 or 8, the result will be a register containing a pointer to the data.
		 */
		[[nodiscard]]
		StackValue compilePopValue(size_t size, bool applyStackPos);

		/**
		 * @brief
		 * Compiles popping a value from the stack
		 *
		 * @return
		 * The size on the stack the item took up
		 */
		int32_t compilePopValueToRegister(asmjit::x86::Gp target, bool applyStackPos);

		void changeStackBy(int32_t amount);

		void flushStack();

		std::optional<StackValue> currentStackValue;

		void generateEmbeddedStrings();
	};
} // namespace ds::jit