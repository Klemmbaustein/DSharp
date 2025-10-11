#pragma once
#include "attribute.hpp"
#include "bytecode/compileBytecode.hpp"
#include "expression.hpp"
#include "parseClass.hpp"
#include "module.hpp"
#include "types/type.hpp"
#include <ds/bytecode.hpp>
#include <map>
#include <string>
#include <vector>
#include "error.hpp"
#include "parseFile.hpp"

namespace ds
{
	struct LanguageContext;
	struct ScopeVariable;
	class LanguageService;

	struct ParseContext
	{
		ParseContext(LanguageContext* context);
		~ParseContext();

		void addFile(std::string filePath);
		void addString(const std::string& str, std::string fileName);
		void updateFile(const std::string& str, std::string fileName);

		BytecodeStream compile();

		struct ParsedVariable
		{
			Type* type;
			Token name;
			std::vector<Token> value;
		};

		friend struct ParsedFunction;
		friend struct ParsedFile;

		void generateReflectionMetadata(BytecodeStream& toStream);

		ErrorContext errors;
		BytecodeCompiler compiler;
		std::vector<Function*> virtualTable;
#ifdef WITH_LANGUAGE_SERVICE
		LanguageService* service = nullptr;

		void emitServiceTypes();
		void emitServiceTypesForModule(Module* mod);
#endif
		std::vector<Type*> defaultTypes;

	private:
		void scanModules();

		std::map<std::string, Module> programModules;
		std::list<ParsedFile> files;
	};
} // namespace ds