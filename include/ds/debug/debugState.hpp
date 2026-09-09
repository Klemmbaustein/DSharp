#pragma once
#include <vector>
#include <ds/typeId.hpp>

namespace ds
{
	class DebugVariable
	{
	public:
		void* pointer = nullptr;
		const char* name = nullptr;
		ds::TypeId type = 0;
		bool isPrimitive = true;
	};

	class DebugFrame
	{
	public:

		virtual BytecodeOffset getOffset() = 0;
		virtual std::vector<DebugVariable> getVariables() = 0;
	};

	class DebugState
	{
	public:

		virtual ~DebugState() = default;

		virtual std::vector<DebugFrame*> getFrames() = 0;
	};
}