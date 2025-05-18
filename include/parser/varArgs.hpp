#pragma once
#include "compileBytecode.hpp"
#include "expression.hpp"
#include <vector>

namespace lang::varArgs
{
	BytecodeBuffer writeVarArgs(std::vector<ExpressionResult> args);
}