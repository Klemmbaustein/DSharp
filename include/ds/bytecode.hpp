#pragma once
#include <cstdint>
#include "binaryBuffer.hpp"
#include "debug.hpp"
#include "reflection.hpp"

namespace ds
{
	class InterpretContext;

	/**
	 * @brief
	 * A bytecode instruction.
	 *
	 * Each instruction executes some logic in an interpreter context.
	 *
	 * @see InterpretContext
	 */
	enum class BytecodeOp : uint8_t
	{
		/// Pushes the argument on the stack.
		push,
		/// Pops n bytes off of the stack, where n is the argument.
		pop,
		/// Same as BytecodeOp::jump, but pushes the current instruction position
		/// on the call stack before jumping, to later return back to it when BytecodeOp::ret is called.
		call,
		/// Sets the current instruction position to the given argument.
		jump,
		/// Sets the current instruction position to the given argument if the topmost bool on the stack is not true.
		jumpIfNot,
		/// Calls an external function in the BytecodeStream::externalFunctions list
		/// with the index for that call being the given argument.
		callExternal,
		/// Returns from a function call started with BytecodeOp::call
		ret,
		/// copies the top of the stack and pushes it on the stack
		copy,
		/// Adds the 2 topmost 32 bit ints on the stack and pushes the result.
		addInt,
		/// Subtracts the 2 topmost 32 bit ints on the stack and pushes the result.
		subInt,
		/// Multiplies the 2 topmost 32 bit ints on the stack and pushes the result.
		mulInt,
		/// Divides the 2 topmost 32 bit ints on the stack and pushes the result.
		divInt,
		greaterInt,
		/// Checks if the 2 topmost values on the stack are equal.
		/// Arguments: 4 bytes value size
		equals,
		/// Adds the 2 topmost 32 bit floats on the stack and pushes the result.
		addFloat,
		/// Multiplies the 2 topmost 32 bit floats on the stack and pushes the result.
		subFloat,
		/// Multiplies the 2 topmost 32 bit floats on the stack and pushes the result.
		mulFloat,
		/// Divides the 2 topmost 32 bit floats on the stack and pushes the result.
		divFloat,
		greaterFloat,
		boolAnd,
		boolOr,
		/// Takes the topmost bool on the stack and pushes it's inverse.
		boolNot,
		/// Takes the topmost int on the stack and pushes it's float equivalent.
		intToFloat,
		/// Takes the topmost float on the stack and pushes it's int equivalent.
		floatToInt,
		pushVariable,
		storeVariable,
		/// Reads a variable and pushes it's value on the stack.
		/// Arguments: 4 bytes - size, 4 bytes - stack offset (from top).
		readVariable,
		/// Pops n bytes off of the variable stack.
		popVariable,
		/// Allocates a managed class object, pushes it's address on the stack.
		/// Arguments: 4 bytes - size, 4 bytes - type id
		allocClass,
		/// Adds a reference to the class object on the stack, will push the reference again
		refClass,
		/// Removes a reference to the class object on the stack
		unrefClass,
		/// Pops an offset, size and a pointer to the class from the stack,
		/// pushes the given member of the class onto the stack
		classMember,
		/// Pops a pointer of the class and a value from the stack and sets the value in the class
		/// Arguments: 4 bytes offset, 4 bytes size
		setClassMember,
		/// Pops a pointer of the class and a value from the stack and sets the value in the class.
		/// Will push the pointer of the class again after the set operation
		/// Arguments: 4 bytes offset, 4 bytes size
		setClassMemberPushAgain,
		concatString,
		indexString,
		/// Creates a new string in which a single character is different. Used for:
		/// string x = "hello world"
		/// x[0] = 'h'
		setStringIndexCopy,
		virtualCall,
		/// Verifies that the current pointer on the stack is not null.
		nullCheck,
		getStructMember,
		setStructMember,
	};

	/**
	 * @brief
	 * Contains executable bytecode.
	 */
	struct BytecodeStream
	{
		void addOperation(BytecodeOp operationCode, const BinaryBuffer& argument);
		void addOperation(BytecodeOp operationCode);
		BinaryBuffer code;

		DebugInfo debug;
		ReflectInfo reflect;

		std::vector<std::string> externalFunctions;
		std::vector<RuntimeFunction> virtualTable;
	};
} // namespace ds