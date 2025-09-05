#pragma once

namespace lang
{
	class InterpretContext;
	using ExternalFunctionPointer = void (*)(InterpretContext* context);
}