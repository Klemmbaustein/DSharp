#pragma once
#include <ds/debug/debugState.hpp>
#include <ds/jit/justInTime.hpp>

namespace ds
{

	class JitDebugState : public DebugState
	{
	public:
		JitDebugState(jit::JustInTimeRuntime* fromContext, Pointer codePos);
		~JitDebugState() override;

		// Inherited via DebugState
		std::vector<DebugFrame*> getFrames() override;

		std::vector<DebugFrame*> frames;
		std::vector<Pointer> callStack;

		jit::JustInTimeRuntime* fromContext = nullptr;
	};

	class JitDebugFrame : public DebugFrame
	{
	public:
		JitDebugFrame(JitDebugState* fromState, Pointer callStackPosition, Pointer variableStackPosition);

		// Inherited via DebugFrame
		Pointer getOffset() override;
		std::vector<DebugVariable> getVariables() override;

		JitDebugState* fromState = nullptr;

		std::vector<DebugVariable> variables;

		Pointer codePosition = 0;
		Pointer baseVariablePosition = 0;
	};
} // namespace ds