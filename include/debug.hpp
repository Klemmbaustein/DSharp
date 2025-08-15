#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace lang
{
	struct DebugSection
	{
		uint32_t offset = 0;
		std::string name;
	};

	class DebugInfo
	{
	public:
		DebugInfo();
		~DebugInfo();

		std::vector<DebugSection> sections;

		DebugSection* getSectionAt(uint32_t offset);

	private:
	};
}