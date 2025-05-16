#pragma once
#include <bytecode.hpp>
#include <string>
#include <map>
#include <set>
#include "error.hpp"

namespace lang
{
	struct BytecodeCompiler;
	struct NativeFunction;
	class ClassType;

	class BytecodeInstruction
	{
	public:
		size_t baseSize = 3;
		BytecodeOp operation = BytecodeOp::push;
		size_t offset = 0;
		virtual void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) = 0;
		virtual bytecodeOffset getArgsSize() = 0;
		virtual std::string toString() { return ""; }
		std::string toStringDefault(const BinaryBuffer& arguments);
	};

	class BytecodeOperation : public BytecodeInstruction
	{
	public:
		BytecodeOperation(BytecodeOp operation, BinaryBuffer arguments);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		BinaryBuffer arguments;
	};

	class BytecodeCallFunction : public BytecodeInstruction
	{
	public:
		BytecodeCallFunction(std::string callName);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::string callName;
	};

	class BytecodeCallNative : public BytecodeInstruction
	{
	public:
		BytecodeCallNative(NativeFunction* function);
		BytecodeCallNative(std::string functionName);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::string functionName;
	};
	class BytecodeAllocClass : public BytecodeInstruction
	{
	public:
		BytecodeAllocClass(ClassType* languageClass);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		ClassType* languageClass = nullptr;
	};

	class BytecodeJumpLabel : public BytecodeInstruction
	{
	public:
		BytecodeJumpLabel(std::string name);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::string name;
	};

	class BytecodeJump : public BytecodeInstruction
	{
	public:
		BytecodeJump(BytecodeOp operation, BytecodeJumpLabel* target);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		BytecodeJumpLabel* target = nullptr;
	};

	struct BytecodeBuffer
	{
		std::vector<BytecodeInstruction*> instructions;

		void add(BytecodeInstruction* instruction);
		void pushInt(uint32_t data);
		void addOperation(BytecodeOp operation, const BinaryBuffer& arguments);
		void addOperation(BytecodeOp operation);

		void prependBuffer(const BytecodeBuffer& other);
		void addBuffer(const BytecodeBuffer& other);
	};

	struct BytecodeFunction : BytecodeBuffer
	{
		bool isEntryPoint = false;
		bytecodeOffset offset = 0;
	};

	struct BytecodeCompiler
	{
		std::map<std::string, BytecodeFunction> functions;
		std::vector<std::string> externals;
		std::map<std::string, uint32_t> usedExternals;
		uint32_t variableStackPosition = 0;

		void printAssembly();

		void compileTo(BytecodeStream& stream, ErrorContext* errors);
	};
} // namespace lang