#pragma once

namespace lang
{
	struct InterpretContext;
	using ExternalFunctionPointer = void (*)(InterpretContext* context);
}