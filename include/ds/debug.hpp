#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <ds/languageTypes.hpp>
#include <ds/reflection.hpp>

namespace ds
{
	struct DebugVariableInfo
	{
		ds::TypeId type = 0;
		std::string name;
	};

	struct DebugLine
	{
		uint32_t lineNumber = 0;
		Pointer offset = 0;
	};

	struct DebugSection
	{
		Pointer offset = 0;
		std::string name;
		std::string file;

		std::vector<DebugVariableInfo> variables;
		std::vector<DebugLine> lines;
		DebugLine* getLineAt(uint32_t lineNumber);
	};

	class DebugMember
	{
	public:
		std::string name;
		BytecodeOffset offset = 0;
		TypeId type = 0;
		bool isPointerMember = false;
		bool isPrimitive = false;
	};

	class DebugClass
	{
	public:
		std::vector<DebugMember> members;
	};

	class DebugInfo
	{
	public:
		DebugInfo();
		~DebugInfo();

		std::vector<DebugSection> sections;

		std::map<ds::TypeId, DebugClass> classInfo;

		DebugSection* getSectionAt(Pointer offset);
		DebugLine* getLineAt(std::string file, uint32_t lineNumber);
		std::pair<DebugSection*, DebugLine*> getLineAt(Pointer offset);

	private:
	};
} // namespace ds