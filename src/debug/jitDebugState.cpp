#include <ds/debug/jitDebugState.hpp>
#include <ds/jit/justInTimeCode_x64.hpp>
#include <cassert>

using namespace ds;

ds::JitDebugState::JitDebugState(jit::JustInTimeRuntime* fromContext, Pointer codePos)
{
	this->fromContext = fromContext;


	callStack = { codePos };
	JitDebugFrame* lastFrame = nullptr;

	fromContext->code->getUnwindData(fromContext->lastStackPos, callStack, true);

	for (auto& itm : callStack)
	{
		lastFrame = new JitDebugFrame(this, itm, lastFrame ? lastFrame->baseVariablePosition : fromContext->variableStackPos);
		frames.push_back(lastFrame);
	}
}

ds::JitDebugState::~JitDebugState()
{
	for (auto& frame : frames)
	{
		delete frame;
	}
}

std::vector<DebugFrame*> ds::JitDebugState::getFrames()
{
	return frames;
}

ds::JitDebugFrame::JitDebugFrame(JitDebugState* fromState, Pointer callStackPosition, Pointer variableStackPosition)
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
				assert(p.start != callStackPosition);
				break;
			}

			auto& debugInfo = dbg->variables[p.debugId];

			ds::RuntimeClass* cls = *reinterpret_cast<ds::RuntimeClass**>(&fromContext->variableStack[baseVariablePosition - p.size]);

			variables.push_back(DebugVariable{
				.pointer = cls,
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

Pointer ds::JitDebugFrame::getOffset()
{
	return codePosition;
}

std::vector<DebugVariable> ds::JitDebugFrame::getVariables()
{
	return variables;
}
