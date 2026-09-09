#include <ds/debug/interpreterDebugState.hpp>

using namespace ds;

ds::InterpreterDebugState::InterpreterDebugState(RuntimeInterpretContext* fromContext, Pointer codePos)
{
	this->fromContext = fromContext;

	InterpreterDebugFrame* lastFrame = new InterpreterDebugFrame(this, codePos, fromContext->variableStackPos);

	frames.push_back(lastFrame);

	for (uint32_t i = fromContext->callStackPos - 1; i > 0; i--)
	{
		lastFrame = new InterpreterDebugFrame(this, fromContext->callStack[i], lastFrame->baseVariablePosition);
		frames.push_back(lastFrame);
	}
}

ds::InterpreterDebugState::~InterpreterDebugState()
{
	for (auto& i : frames)
	{
		delete i;
	}
}

std::vector<DebugFrame*> ds::InterpreterDebugState::getFrames()
{
	return frames;
}

ds::InterpreterDebugFrame::InterpreterDebugFrame(InterpreterDebugState* fromState, Pointer callStackPosition,
	Pointer variableStackPosition)
{
	this->fromState = fromState;
	codePosition = callStackPosition;

	auto fromContext = fromState->fromContext;

	auto& buffer = fromContext->runtime->unwindBuffer;
	auto tbl = buffer.getSectionAt(callStackPosition);
	auto dbg = fromContext->runtime->debug->getSectionAt(callStackPosition);

	if (!dbg)
	{
		return;
	}

	baseVariablePosition = variableStackPosition;

	for (auto& p : tbl->parts)
	{
		if (p.offset < callStackPosition)
		{
			continue;
		}
		switch (p.op)
		{
		case UnwindOp::popClass: {
			if (p.start > callStackPosition || p.debugId == UINT32_MAX)
			{
				break;
			}

			auto& debugInfo = dbg->variables[p.debugId];

			variables.push_back(DebugVariable{
				.pointer = *reinterpret_cast<ds::RuntimeClass**>(&fromContext->variableStack[baseVariablePosition - p.size]),
				.name = debugInfo.name.c_str(),
				.type = debugInfo.type,
				.isPrimitive = false,
			});
			break;
		}
		case UnwindOp::debugPopPrimitive: {
			if (p.start > callStackPosition || p.debugId == UINT32_MAX)
			{
				break;
			}

			auto& debugInfo = dbg->variables[p.debugId];

			variables.push_back(DebugVariable{
				.pointer = &fromContext->variableStack[baseVariablePosition - p.size],
				.name = debugInfo.name.c_str(),
				.type = debugInfo.type,
				.isPrimitive = true,
				});

			break;
		}
		case UnwindOp::popBytes: {
			baseVariablePosition -= p.size;
			break;
		}
		case UnwindOp::pushBytes: {
			baseVariablePosition += p.size;
			break;
		}
		default:
			break;
		}
	}
}

BytecodeOffset ds::InterpreterDebugFrame::getOffset()
{
	return codePosition;
}

std::vector<DebugVariable> ds::InterpreterDebugFrame::getVariables()
{
	return variables;
}
