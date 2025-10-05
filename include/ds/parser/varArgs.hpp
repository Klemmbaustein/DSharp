#pragma once
#include "bytecode/compileBytecode.hpp"
#include "expression.hpp"
#include <vector>

namespace ds::varArgs
{
	BytecodeBuffer writeVarArgs(std::vector<ExpressionResult> args);
}