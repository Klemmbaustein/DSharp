#pragma once
#include <cstdint>
#include "binaryBuffer.hpp"

namespace lang
{
	using bytecodeOffset = uint32_t;

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
		/// Calls an external function in the BytecodeStream::externalFunctions list,
		/// with the index for that call being the given argument.
		callExternal,
		/// Returns from a function call started with BytecodeOp::call
		ret,
		/// Adds the 2 topmost 32 bit ints on the stack and pushes the result.
		addInt,
		/// Subtracts the 2 topmost 32 bit ints on the stack and pushes the result.
		subInt,
		/// Multiplies the 2 topmost 32 bit ints on the stack and pushes the result.
		mulInt,
		/// Divides the 2 topmost 32 bit ints on the stack and pushes the result.
		divInt,
		/// Adds the 2 topmost 32 bit floats on the stack and pushes the result.
		addFloat,
		/// Multiplies the 2 topmost 32 bit floats on the stack and pushes the result.
		subFloat,
		/// Multiplies the 2 topmost 32 bit floats on the stack and pushes the result.
		mulFloat,
		/// Divides the 2 topmost 32 bit floats on the stack and pushes the result.
		divFloat,
		/// Takes the topmost int in the stack and pushes it's float equivalent.
		intToFloat,
		/// Takes the topmost float in the stack and pushes it's int equivalent.
		floatToInt,
		pushVariable,
		storeVariable,
		/// Reads a variable and pushes it's value on the stack.
		/// Arguments: 4 bytes - size, 4 bytes - stack offset (from top).
		readVariable,
		popVariable,
		/// Allocates a managed class object, pushes it's address on the stack.
		/// Arguments: 4 bytes - size, 4 bytes - type id
		allocClass,
		/// Adds a reference to the class object
		/// Arguments: pointer to the class object
		refClass,
		/// Removes a reference to the class object
		/// Arguments: pointer to the class object
		unrefClass,
		/// Pops a pointer of the class from the stack, pushes the given member of the class onto the stack
		/// Arguments: 4 bytes offset, 4 bytes size
		classMember,
		/// Pops a pointer of the class and a value from the stack and sets the value in the class
		/// Arguments: 4 bytes offset, 4 bytes size
		setClassMember
	};

	struct BytecodeStream
	{
		void addOperation(BytecodeOp operationCode, const BinaryBuffer& argument);
		void addOperation(BytecodeOp operationCode);
		BinaryBuffer code;

		std::vector<std::string> externalFunctions;
	};
} // namespace lang