#pragma once
#include <ds/bytecode.hpp>
#include <string>
#include <map>
#include <set>
#include <ds/parser/error.hpp>
#include <memory>
#include <iostream>

namespace ds
{
	struct BytecodeCompiler;
	struct NativeFunction;
	class ClassType;
	class Function;

	/**
	 * @brief
	 * Base bytecode instruction class.
	 *
	 * Holds logic on how to convert this instruction to into bytecode.
	 * For example, a function call might need to know it's target bytecode offset, which won't be known
	 * until all the bytecode has been generated and put together.
	 *
	 * @see BytecodeOp
	 */
	class BytecodeInstruction
	{
	public:
		/// The base size of this operation in bytes usually 2 bytes
		/// (1 byte instruction id, 1 byte arguments size) or 0 bytes (no size, this instruction is only really a marker, like a jump label etc.).
		size_t baseSize = sizeof(BytecodeOp) + sizeof(uint8_t);
		/// The operation of this instruction. Is ignored if the base size is 0.
		BytecodeOp operation = BytecodeOp::push;
		/// The offset (in bytes) this instruction has in the compiled program. This is only initialized when getArgs() is called
		BytecodeOffset offset = 0;
		/**
		 * @brief
		 * Gets the argument bytes of this instruction.
		 *
		 * @param stream
		 * The binary stream to write the bytes to.
		 *
		 * @param compiler
		 * The bytecode compiler calling this function.,
		 */
		virtual void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) = 0;
		virtual BytecodeOffset getArgsSize() = 0;
		virtual std::string toString() { return ""; }
		virtual void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section);
		std::string toStringDefault(const BinaryBuffer& arguments) const;

		virtual ~BytecodeInstruction() = default;
	};

	/**
	 * @brief
	 * Any raw bytecode operation with arguments.
	 */
	class BytecodeOperation : public BytecodeInstruction
	{
	public:
		BytecodeOperation(BytecodeOp operation, BinaryBuffer arguments);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		BinaryBuffer arguments;
	};

	class BytecodeCallFunction : public BytecodeInstruction
	{
	public:
		BytecodeCallFunction(std::string callName);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::string callName;
	};

	class BytecodeFunctionAddress : public BytecodeInstruction
	{
	public:
		BytecodeFunctionAddress(std::string callName, bool native = false);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		bool native = false;
		std::string callName;
	};

	class BytecodeCallNative : public BytecodeInstruction
	{
	public:
		BytecodeCallNative(NativeFunction* function);
		BytecodeCallNative(std::string functionName);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::string functionName;
	};
	class BytecodeAllocClass : public BytecodeInstruction
	{
	public:
		BytecodeAllocClass(ClassType* languageClass);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		ClassType* languageClass = nullptr;
	};

	class BytecodeJumpLabel : public BytecodeInstruction
	{
	public:
		BytecodeJumpLabel(std::string name);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::string name;
	};

	class BytecodeJump : public BytecodeInstruction
	{
	public:
		BytecodeJump(BytecodeOp operation, BytecodeJumpLabel* target);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		BytecodeJumpLabel* target = nullptr;
	};

	class BytecodeAwait : public BytecodeInstruction
	{
	public:
		BytecodeAwait(Size awaitSize, BytecodeJumpLabel* onFinished);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		Size awaitSize = 0;
		BytecodeJumpLabel* target = nullptr;
	};

	using InstructionPtr = std::shared_ptr<BytecodeInstruction>;

	struct BytecodeBuffer
	{
		std::vector<InstructionPtr> instructions;

		void add(InstructionPtr instruction);

		template<typename T, typename ...Args>
		void addNew(Args... args)
		{
			add(std::make_shared<T>(args...));
		}

		void pushInt(uint32_t data);
		void addOperation(BytecodeOp operation, const BinaryBuffer& arguments);
		void addOperation(BytecodeOp operation);

		void prependBuffer(const BytecodeBuffer& other);
		void addBuffer(const BytecodeBuffer& other);
	};

	struct BytecodeFunction : BytecodeBuffer
	{
		bool isEntryPoint = false;
		bool isPreCompiled = false;
		std::string name;
		BytecodeOffset offset = 0;
	};

	struct BytecodeCompiler
	{
		std::map<std::string, BytecodeFunction> functions;
		std::vector<std::string> externals;
		std::map<std::string, uint32_t> usedExternals;
		uint32_t variableStackPosition = 0;

		/// Prints an assembly-like debug listing of all functions, labels and instructions in this program.
		void printAssembly();

		/**
		 * @brief
		 * Compiles the functions of this program into the given stream.
		 *
		 * @param stream
		 * THe stream to compile to
		 * @param virtualTable
		 * The virtual table to reference when compiling virtual functions.
		 */
		void compileTo(BytecodeStream& stream, std::vector<Function*> virtualTable);
	};
} // namespace ds