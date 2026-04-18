#pragma once
#include <cstdint>
#include "binaryBuffer.hpp"
#include "debug.hpp"
#include "reflection.hpp"
#include "unwindInfo.hpp"

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
	 * @see BytecodeInstruction
	 */
	enum class BytecodeOp : uint8_t
	{
		/// Pushes the argument on the stack.
		push,
		/// Pushes an address of a function on the stack.
		pushAddr,
		/// Pops n bytes off of the stack, where n is the argument.
		pop,
		/// Same as BytecodeOp::jump, but pushes the current instruction position
		/// on the call stack before jumping, to later return back to it when BytecodeOp::ret is called.
		call,
		/// Sets the current instruction position to the given argument.
		jump,
		/// Sets the current instruction position to the given argument if the topmost bool on the stack is true.
		jumpIf,
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
		/// Takes the modulo value of the two topmost ints
		modInt,
		/// Takes the 2 topmost 32 bit ints on the stack and pushes a bool value that's true
		/// if the first pushed (meaning lowest) int is bigger than the last pushed (meaning highest)
		greaterInt,
		/// Takes the topmost 32 bit int on the stack and pushes it's inverse value.
		negativeInt,
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
		/// Takes the topmost 32 bit float on the stack and pushes it's inverse value.
		negativeFloat,
		greaterFloat,
		boolAnd,
		boolOr,
		/// Takes the topmost bool on the stack and pushes it's inverse.
		boolNot,
		/// Takes the topmost int on the stack and pushes it's float equivalent.
		intToFloat,
		/// Takes the topmost float on the stack and pushes it's int equivalent.
		floatToInt,
		/// Pushes the variable stack by the given value
		/// Arguments: 4 bytes push amount
		pushVariable,
		/// Stores a variable value to the variable stack.
		storeVariable,
		/// Reads a variable and pushes it's value on the stack.
		/// Arguments: 4 bytes - size, 4 bytes - stack offset (from top).
		readVariable,
		/// Pops n bytes off of the variable stack.
		popVariable,
		/// Allocates a managed class object, pushes it's address on the stack.
		/// Arguments: 4 bytes - size, 4 bytes - type id, 4 bytes - vtable offset
		allocClass,
		/// Adds an interface class header into the class body.
		/// Arguments: 4 bytes - offset 4 bytes - vtable offset
		implInterface,
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
		/// Pops an offset, size and a pointer to the class from the stack,
		/// pushes the given member of the class onto the stack
		classMemberPtr,
		/// Pops a pointer of the class and a value from the stack and sets the value in the class
		/// Arguments: 4 bytes offset, 4 bytes size
		setClassMemberPtr,
		/// Pops a pointer of the class and a value from the stack and sets the value in the class.
		/// Will push the pointer of the class again after the set operation
		/// Arguments: 4 bytes offset, 4 bytes size
		setClassMemberPushAgain,
		/// Calls a virtual function.
		virtualCall,
		/// Verifies that the current pointer on the stack is not null.
		nullCheck,
		getStructMember,
		setStructMember,
		/// Suspends the current context, to be resumed later.
		suspend,
		/// Suspends the current context, resumed when a given task finishes.
		awaitTask,
		/// Returns from an async function.
		returnAsync,
		/// Unwinds the stack, destructing all objects on the way.
		unwind,
		/// Checks if a class has the given type.
		classIs,
		/// Converts a class to the given type if possible.
		classAs,
		/// Marks the end of a function with a missing return statement.
		noReturn,
		/// Casts a pointer from the stack to an interface pointer,
		/// adding the given offset to it and pushing it back on the stack.
		/// Arguments: 4 bytes offset, 1 byte uncast (reverse direction of cast)
		castInterface,
	};

	struct VTableFunction
	{
		Pointer codeOffset = UINTPTR_MAX;
		uint32_t nativeFunction = 0;
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
		UnwindInfo unwind;

		std::vector<std::string> externalFunctions;
		std::vector<VTableFunction> virtualTable;
	};
} // namespace ds