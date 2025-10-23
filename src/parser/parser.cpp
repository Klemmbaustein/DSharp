#include <ds/parser/parser.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/types/stringType.hpp>
#include <ds/parser/types/listType.hpp>
#include <ds/parser/types/functionType.hpp>
#include <ds/parser/types/lambdaType.hpp>
#include <ds/language.hpp>
#include <ds/service/languageService.hpp>
#include <ds/modules/system.hpp>
#include <ds/parser/parseFunction.hpp>
using namespace ds;

ds::ParseContext::ParseContext(LanguageContext* context)
{
	for (auto& [name, module] : context->languageModules)
	{
		this->programModules.insert({ name, module->create() });
	}

	for (auto& i : this->programModules)
	{
		std::string moduleName = i.first + "::";

		for (auto& m : this->programModules)
		{
			if (m.first.substr(0, moduleName.size()) == moduleName)
			{
				i.second.submodules.insert({ m.first, &m.second });
			}
		}
	}
}

ds::ParseContext::~ParseContext()
{
}

void ds::ParseContext::addFile(std::string filePath)
{
	this->errors.currentFile = filePath;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromFile(filePath, &errors);
	newFile.name = filePath;
	newFile.context = this;
	newFile.scan(&errors);
}

void ds::ParseContext::addString(const std::string& str, std::string fileName)
{
	this->errors.currentFile = fileName;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromString(str, fileName, &errors);
	newFile.name = fileName;
	newFile.context = this;
	newFile.scan(&errors);
}

void ds::ParseContext::updateFile(const std::string& str, std::string fileName)
{
	for (auto& i : this->files)
	{
		if (i.name == fileName)
		{
			i.stream = TokenStream();
			i.stream.fromString(str, fileName, &errors);
			i.context = this;
			i.scan(&errors);
			break;
		}
	}
}

BytecodeStream ds::ParseContext::compile()
{
	this->defaultTypes.clear();
	this->defaultTypes.push_back(IntType::getInstance());
	this->defaultTypes.push_back(FloatType::getInstance());
	this->defaultTypes.push_back(BoolType::getInstance());
	this->defaultTypes.push_back(StringType::getInstance());
	this->defaultTypes.push_back(CharType::getInstance());
	this->defaultTypes.push_back(ListType::getInstance());
	this->defaultTypes.push_back(NullType::getInstance());
	this->defaultTypes.push_back(FunctionType::getInstance(nullptr, {}));
	this->defaultTypes.push_back(LambdaType::getInstance());
	this->defaultTypes.push_back(TaskType::getInstance(nullptr));

#ifdef WITH_LANGUAGE_SERVICE
	if (this->service)
	{
		this->service->files.clear();
	}
#endif

	scanModules();

	if (!errors.isOk())
	{
		return BytecodeStream();
	}

	for (auto& i : this->files)
	{
		this->errors.currentFile = i.name;
		i.compile(this);
	}

#ifdef WITH_LANGUAGE_SERVICE
	if (!service)
#endif
	{
		if (!errors.isOk())
		{
			return BytecodeStream();
		}
		this->compiler.compileTo(initialCode, virtualTable, &errors);
	}
#ifdef WITH_LANGUAGE_SERVICE
	else
	{
		emitServiceTypes();
	}
#endif

	//if (!this->service)
	//{
	//	this->compiler.printAssembly();
	//}

	if (!errors.isOk())
	{
		return BytecodeStream();
	}

	if (!service)
	{
		generateReflectionMetadata(initialCode);
	}
	return initialCode;
}

void ds::ParseContext::generateReflectionMetadata(BytecodeStream& toStream)
{
	for (auto& i : this->files)
	{
		for (auto& cls : i.classes)
		{
			std::vector<TypeMember> members;

			for (auto& m : cls.members)
			{
				auto reflectAttribute = m.second.getAttribute<modules::system::ReflectAttribute>();

				if (reflectAttribute)
				{
					members.push_back(TypeMember{
						.type = 0,
						.name = m.second.name.string,
						.offset = m.second.offset,
					});
				}
			}

			toStream.reflect.types[0] = TypeInfo{
				.name = cls.classModule->name + "::" + cls.name.string,
				.vTableOffset = cls.thisType->vTableOffset,
				.constructor = this->compiler.functions[cls.getDefaultConstructor()->getFullName()].offset,
				.bodySize = cls.thisType->classSize,
				.members = members,
			};
		}
	}
}

#ifdef WITH_LANGUAGE_SERVICE
void ds::ParseContext::emitServiceTypes()
{
	for (auto& [name, module] : this->programModules)
	{
		emitServiceTypesForModule(&module);
	}
}

void ds::ParseContext::emitServiceTypesForModule(Module* mod)
{
	for (auto& i : mod->moduleTypes)
	{
		auto t = i.second->toScanned();
		this->service->types[t.id] = t;
	}

	for (auto& i : mod->submodules)
	{
		emitServiceTypesForModule(i.second);
	}
}
#endif

void ds::ParseContext::scanModules()
{
	// Create global module
	// TODO: Somehow clear out the functions for repeated scans
	auto& globalModule = this->programModules.insert({ "", Module() }).first->second;

	// Register all modules
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		Module& mod = this->programModules[file.scopeName];
		mod.name = file.scopeName;
		file.fileModule = &mod;

		for (auto& function : file.functions)
		{
			mod.moduleFunctions.insert({ function.name.string, &function });
			function.functionModule = &mod;
		}

		for (Type* i : this->defaultTypes)
		{
			mod.moduleTypes.insert({ i->name, i });
		}
	}

	for (auto& i : this->programModules)
	{
		if (i.second.name != globalModule.name)
		{
			globalModule.submodules.insert({ i.first, &i.second });
		}
	}

	// Resolve module dependencies
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& [name, module] : file.usings)
		{
			auto foundModule = this->programModules.find(name.string);
			if (foundModule == this->programModules.end())
			{
				errors.error(ErrorCode::parseUnknownModule, name, "Unknown module: " + name.string);
				continue;
			}
			module = &foundModule->second;
		}
		file.usings.insert({ Token(""), &globalModule });
	}

	// Register types
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& i : file.classes)
		{
			i.registerType(this, &file);
		}
		for (auto& i : file.enums)
		{
			i.registerType(this, &file);
		}
	}

	// Parse classes in modules
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& i : file.classes)
		{
			i.scan(&errors, &file);
		}
	}

	// Scan classes
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& i : file.classes)
		{
			i.scanClass(this, &file);
		}
		for (auto& function : file.functions)
		{
			function.resolveTypes(this, &errors);
		}
	}
}
