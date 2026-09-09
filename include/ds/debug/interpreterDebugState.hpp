#pragma once
#include <ds/debug/debugState.hpp>
#include <ds/interpreter.hpp>

namespace ds
{

	class InterpreterDebugState : public DebugState
	{
	public:

		InterpreterDebugState(RuntimeInterpretContext* fromContext, Pointer codePos);
		~InterpreterDebugState() override;

		// Inherited via DebugState
		std::vector<DebugFrame*> getFrames() override;

		std::vector<DebugFrame*> frames;

		RuntimeInterpretContext* fromContext = nullptr;
	};

	class InterpreterDebugFrame : public DebugFrame
	{
	public:
		InterpreterDebugFrame(InterpreterDebugState* fromState, Pointer callStackPosition, Pointer variableStackPosition);

		// Inherited via DebugFrame
		BytecodeOffset getOffset() override;
		std::vector<DebugVariable> getVariables() override;

		InterpreterDebugState* fromState = nullptr;

		std::vector<DebugVariable> variables;

		Pointer codePosition = 0;
		Pointer baseVariablePosition = 0;
	};
}