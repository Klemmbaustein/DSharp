#pragma once

namespace ds
{
	class InterpretContext;
	using ExternalFunctionPointer = void (*)(InterpretContext* context);
}