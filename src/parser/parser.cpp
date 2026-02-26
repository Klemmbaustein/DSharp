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
	resetModules(context);
	this->registry = new TypeRegistry(*context->registry);
}

ds::ParseContext::~ParseContext()
{
	delete this->registry;
}

void ds::ParseContext::addFile(std::string filePath)
{
	this->errors.currentFile = filePath;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromFile(filePath, &errors);
	newFile.displayName = filePath;
	newFile.name = filePath;
	newFile.context = this;
	newFile.scan(&errors);
}

void ds::ParseContext::addString(const std::string& str, std::string fileName)
{
	this->errors.currentFile = fileName;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromString(str, fileName, &errors);
	newFile.displayName = fileName;
	newFile.name = fileName;
	newFile.context = this;
	newFile.scan(&errors);
}

ds::ParsedClass* ds::ParseContext::addClass(Token className, std::string moduleName,
	const std::string& body, std::string fileName, std::vector<std::vector<Token>> derived)
{
	this->errors.currentFile = fileName;
	auto& newFile = this->files.emplace_back();
	newFile.name = fileName;
	newFile.displayName = fileName;
	newFile.context = this;
	newFile.usings[Token(moduleName)] = nullptr;
	newFile.moduleName = moduleName;

	ParsedClass& newClass = newFile.classes.emplace_back(&newFile);
	newClass.name = className;
	newClass.classStream.fromString(body, fileName, &errors);
	newClass.derivedFrom = derived;
	return &newClass;
}

ds::ParsedClass* ds::ParseContext::addClass(Token className, std::string moduleName,
	ds::TokenStream& stream, std::string fileName, std::vector<std::vector<Token>> derived)
{
	this->errors.currentFile = fileName;
	auto& newFile = this->files.emplace_back();
	newFile.displayName = fileName;
	newFile.name = fileName;
	newFile.context = this;
	newFile.usings[Token(moduleName)] = nullptr;
	newFile.moduleName = moduleName;

	ParsedClass& newClass = newFile.classes.emplace_back(&newFile);
	newClass.name = className;
	newClass.classStream = stream;
	newClass.derivedFrom = derived;
	newClass.isFileClass = true;
	return &newClass;
}

void ds::ParseContext::updateFile(const std::string& str, std::string fileName)
{
	for (auto& i : this->files)
	{
		if (i.name == fileName)
		{
			this->errors.currentFile = fileName;
			i.classes.clear();
			i.functions.clear();
			i.enums.clear();
			i.usings.clear();

			i.stream = TokenStream();
			i.stream.fromString(str, fileName, &errors);
			i.context = this;
			i.scan(&errors);
			break;
		}
	}
}

ds::ParsedClass* ds::ParseContext::updateClass(Token className, std::string moduleName, ds::TokenStream& stream,
	std::string fileName, std::vector<std::vector<Token>> derived)
{
	for (auto& i : this->files)
	{
		if (i.name == fileName)
		{
			this->errors.currentFile = fileName;
			i.classes.clear();
			i.functions.clear();
			i.enums.clear();
			i.usings.clear();

			i.name = fileName;
			i.context = this;
			i.usings[Token(moduleName)] = nullptr;
			i.moduleName = moduleName;

			ParsedClass& newClass = i.classes.emplace_back(&i);
			newClass.name = className;
			newClass.classStream = stream;
			newClass.derivedFrom = derived;
			newClass.isFileClass = true;
			return &newClass;
		}
	}
	return addClass(className, moduleName, stream, fileName, derived);
}

BytecodeStream ds::ParseContext::compile()
{
	initializeModules();

	if (!errors.isOk())
	{
#ifdef WITH_LANGUAGE_SERVICE
		if (this->service)
		{
			emitServiceTypes();
		}
#endif
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
		this->compiler.compileTo(initialCode, virtualTable);
	}
#ifdef WITH_LANGUAGE_SERVICE
	else
	{
		emitServiceTypes();
	}
#endif

	 if (!this->service)
	 {
		this->compiler.printAssembly();
	 }

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

void ds::ParseContext::initializeModules()
{
	this->defaultTypes.clear();
	this->defaultTypes.push_back(registry->getEntry<IntType>());
	this->defaultTypes.push_back(registry->getEntry<FloatType>());
	this->defaultTypes.push_back(registry->getEntry<BoolType>());
	this->defaultTypes.push_back(registry->getEntry<StringType>());
	this->defaultTypes.push_back(registry->getEntry<CharType>());
	this->defaultTypes.push_back(registry->getEntry<ListType>());
	this->defaultTypes.push_back(NullType::getInstance());
	this->defaultTypes.push_back(FunctionType::getInstance(nullptr, {}, registry));
	this->defaultTypes.push_back(registry->getEntry<LambdaType>());
	this->defaultTypes.push_back(TaskType::getInstance(nullptr, registry));

	scanModules();
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
						.type = m.second.type->id,
						.attributeType = reflectAttribute->attribute->getType(),
						.name = m.second.name.string,
						.parameterData = reflectAttribute->parametersToString(),
						.offset = m.second.offset,
					});
				}
			}

			std::map<TypeId, BytecodeOffset> superClasses;

			for (auto& [offset, super] : cls.thisType->interfaces)
			{
				superClasses.insert({ super->id, offset });
			}

			toStream.reflect.types[cls.thisType->id] = TypeInfo{
				.hash = cls.thisType->id,
				.name = cls.classModule->name + "::" + cls.name.string,
				.vTableOffset = cls.thisType->vTableOffset,
				.constructor = this->compiler.functions[cls.getDefaultConstructor()->getFullName()].offset,
				.bodySize = cls.thisType->classSize,
				.members = members,
				.superClass = cls.thisType->parent ? cls.thisType->parent->id : 0,
				.interfaces = superClasses,
			};
		}
	}
}

#ifdef WITH_LANGUAGE_SERVICE
void ds::ParseContext::emitServiceTypes()
{
	auto types = this->registry->getAllTypes();

	for (auto& i : types)
	{
		auto t = i->toScanned();
		this->service->types.insert({ t.id, t });
	}

	for (auto& [name, module] : this->programModules)
	{
		emitServiceTypesForModule(&module);
	}
	std::function<void(ds::Module*, ds::ParsedFile&, bool)> processModule;

	processModule = [this, &processModule](ds::Module* module, ds::ParsedFile& file, bool isRecursing) {
		auto& f = service->files[file.name];

		for (auto& [_, type] : module->moduleTypes)
		{
			if (type->id && type->name.find_first_of(".()[]<>") == std::string::npos)
			{
				auto found = f.accessibleTypes.find(type->id);
				if (found == f.accessibleTypes.end() || (!isRecursing && !found->second.empty()))
				{
					f.accessibleTypes[type->id] = isRecursing ? module->name : "";
				}
			}
		}
		for (auto& [_, fn] : module->moduleFunctions)
		{
			auto name = fn->getShortName();
			if (name.find('.') == std::string::npos)
			{
				auto found = f.accessibleFunctions.find(name);
				if (found == f.accessibleFunctions.end() || (!isRecursing && !found->second.empty()))
				{
					f.accessibleFunctions[name] = isRecursing ? module->name : "";
				}
			}
		}
		for (auto& [_, enumEntry] : module->moduleEnums)
		{
			for (auto& [name, _1] : enumEntry->values)
			{
				std::string nameString = enumEntry->name + "::" + name.string;
				auto found = f.accessibleEnums.find(nameString);
				if (found == f.accessibleEnums.end() || (!isRecursing && !found->second.empty()))
				{
					f.accessibleEnums[nameString] = isRecursing ? module->name : "";
				}
			}
		}

		for (auto& [_, submodule] : module->submodules)
		{
			processModule(submodule.module, file, true);
		}
	};

	for (auto& file : this->files)
	{
		processModule(file.fileModule, file, false);

		for (auto& [_, module] : file.usings)
		{
			if (module)
			{
				processModule(module, file, false);
			}
		}
	}
}

void ds::ParseContext::emitServiceTypesForModule(Module* mod)
{
	for (auto& i : mod->moduleTypes)
	{
		if (this->service->types.find(i.second->id) != this->service->types.end())
		{
			continue;
		}

		auto t = i.second->toScanned();
		this->service->types.insert({ t.id, t });
	}

	for (auto& i : mod->submodules)
	{
		emitServiceTypesForModule(i.second.module);
	}
}
#endif

void ds::ParseContext::resetModules(LanguageContext* context)
{
	this->virtualTable.clear();
	this->compiler.functions.clear();

	for (ParsedFile& file : this->files)
	{
		for (auto& i : file.classes)
		{
			i.scanned = false;
		}
	}

	this->programModules.clear();
	for (auto& i : this->files)
	{
		for (auto& [name, ptr] : i.usings)
		{
			ptr = nullptr;
		}
	}

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
				i.second.addModule(m.first, &m.second);
			}
		}
	}
}

void ds::ParseContext::scanModules()
{
	// Create global module
	auto& globalModule = this->programModules[""];
	globalModule = {};

	// Register all modules
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		Module& mod = this->programModules[file.moduleName];
		mod.name = file.moduleName;
		file.fileModule = &mod;

		for (auto& function : file.functions)
		{
			mod.moduleFunctions.insert({ function.name.string, &function });
			function.functionModule = &mod;
		}

#ifdef WITH_LANGUAGE_SERVICE
		// Add the modules to the service.
#endif

		for (Type* i : this->defaultTypes)
		{
			mod.moduleTypes.insert({ i->name, i });
		}
	}

	for (Type* i : this->defaultTypes)
	{
		globalModule.moduleTypes.insert({ i->name, i });
	}

	for (auto& i : this->programModules)
	{
		if (i.second.name != globalModule.name)
		{
			globalModule.addModule(i.first, &i.second);
		}
	}

	// Resolve module dependencies
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		file.updateUsings();
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
