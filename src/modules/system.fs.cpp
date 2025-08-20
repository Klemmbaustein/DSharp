#if MODULE_FS
#include <modules/system.fs.hpp>
#include <modules/system.hpp>
#include <parser/types/stringType.hpp>
#include <parser/types/arrayType.hpp>
#include <filesystem>
#include <print>

using namespace lang;
using namespace lang::modules::system;
using std::filesystem::directory_iterator;
using std::filesystem::canonical;
using std::filesystem::current_path;

static void fs_path_delete(InterpretContext* context)
{
	ClassPtr<fs::FilePath> path = context->popValue<RuntimeClass*>();
	RuntimeClass::unref(path->pathString.classPtr);
}

static VTableEntry pathVTable = VTableEntry{
	.nativeFn = &fs_path_delete
};

static void fs_path_construct(InterpretContext* context)
{
	ClassRef<fs::FilePath> path = context->popValue<RuntimeClass*>();
	path.classPtr->vtable = &pathVTable;
	auto str = context->popValue<RuntimeClass*>();
	str->addRef();
	path->pathString = str;
	context->pushValue(path);
}

static void fs_path_getFiles(InterpretContext* context)
{
	ClassRef<fs::FilePath> path = context->popValue<RuntimeClass*>();

	std::vector<RuntimeClass*> paths;

	for (auto& i : directory_iterator(path->pathString.ptr()))
	{
		auto str = i.path().string();

		ClassRef<fs::FilePath> newPath = fs::createPath();
		auto newString = RuntimeStrRef(str.data(), str.size());
		newString.classPtr->addRef();
		newPath->pathString = newString.classPtr;
		paths.push_back(newPath.classPtr);
	}

	auto f = createArray(paths.data(), paths.size(), true);
	context->pushValue(f);
}

static void fs_path_getFull(InterpretContext* context)
{
	ClassRef<fs::FilePath> path = context->popValue<RuntimeClass*>();

	auto fullPath = canonical(path->pathString.ptr()).string();

	context->pushRuntimeString(RuntimeStr(fullPath.data(), fullPath.size()));
}

static void fs_path_getExtension(InterpretContext* context)
{
	ClassRef<fs::FilePath> path = context->popValue<RuntimeClass*>();

	auto str = std::string(path->pathString.ptr(), path->pathString.length());
	str = str.substr(str.find_last_of("\\/"));

	auto dot = str.find_last_of(".");

	if (dot == std::string::npos)
	{
		context->pushValue(nullptr);
		return;
	}
	auto extension = str.substr(dot + 1);
	context->pushRuntimeString(RuntimeStr(extension.data(), extension.size()));
}

static void fs_path_getName(InterpretContext* context)
{
	ClassRef<fs::FilePath> path = context->popValue<RuntimeClass*>();

	auto str = std::string(path->pathString.ptr(), path->pathString.length());
	str = str.substr(str.find_last_of("\\/") + 1);
	context->pushRuntimeString(RuntimeStr(str.data(), str.size()));
}

#if HAS_FS_CURRENT_PATH
static void fs_getCurrentPath(InterpretContext* context)
{
	std::string str = current_path().string();

	ClassRef<fs::FilePath> newPath = fs::createPath();
	auto newString = RuntimeStrRef(str.data(), str.size());
	newString.classPtr->addRef();
	newPath->pathString = newString.classPtr;
	context->pushValue(newPath);
}
#endif

lang::NativeModule fs::createModule()
{
	NativeModule out;
	out.name = "system::fs";

	StringType* strType = StringType::getInstance();

	ClassType* pathType = out.createClass<fs::FilePath>("FilePath");

	out.addClassConstructor(pathType,
		NativeFunction(
			{ FunctionArgument(strType, Token("pathString")) }, nullptr,
			"Path.new", &fs_path_construct));

	out.addClassMethod(pathType,
		NativeFunction(
			{}, ArrayType::getInstance(pathType),
			"getFiles", &fs_path_getFiles));

	out.addClassMethod(pathType,
		NativeFunction(
			{}, strType,
			"full", &fs_path_getFull));

	out.addClassMethod(pathType,
		NativeFunction(
			{}, strType->nullable,
			"extension", &fs_path_getExtension));

	out.addClassMethod(pathType,
		NativeFunction(
			{}, strType,
			"name", &fs_path_getName));

	pathType->members.push_back(ClassMember{
		.name = "pathString",
		.offset = offsetof(fs::FilePath, pathString),
		.type = strType,
	});

#if HAS_FS_CURRENT_PATH
	out.addFunction(NativeFunction(
		{}, pathType,
		"getCurrentPath", fs_getCurrentPath));
#endif

	return out;
}

RuntimeClass* lang::modules::system::fs::createPath()
{
	return RuntimeClass::allocateClass(sizeof(FilePath), &pathVTable);
}
#endif