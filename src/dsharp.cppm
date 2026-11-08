module;

#include <ds/language.hpp>
#include <ds/languageTypes.hpp>
#include <ds/debug.hpp>
#include <ds/bytecode.hpp>
#include <ds/baseRuntime.hpp>
#include <ds/typeId.hpp>
#include <ds/parser/types/type.hpp>
#include <ds/parser/types/genericArgument.hpp>
#include <ds/parser/types/classType.hpp>
#include <ds/parser/types/enumType.hpp>
#include <ds/native/nativeModule.hpp>
#include <ds/native/nativeStructType.hpp>
#include <ds/parser/types/iteratorType.hpp>
#include <ds/parser/types/taskType.hpp>
#include <ds/parser/types/functionType.hpp>

export module dsharp;

export namespace ds
{
	export using ds::LanguageContext;
	export using ds::LanguageRuntime;
	export using ds::ParseContext;
	export using ds::ErrorCode;
	export using ds::ErrorContext;
	export using ds::Attribute;
	export using ds::DebugClass;
	export using ds::DebugFrame;
	export using ds::DebugInfo;
	export using ds::BytecodeBuffer;
	export using ds::BinaryBuffer;
	export using ds::ReflectInfo;
	export using ds::TypeInfo;
	export using ds::BinaryBufferRef;
	export using ds::ParserOptions;
	export using ds::TypeRegistry;

	export using ds::typeIdFromName;

	export using ds::ArrayType;
	export using ds::Type;
	export using ds::BoolType;
	export using ds::FloatType;
	export using ds::IntType;
	export using ds::StringType;
	export using ds::EnumType;
	export using ds::GenericArgument;
	export using ds::GenericArgumentType;
	export using ds::CharType;
	export using ds::IteratorType;
	export using ds::TaskType;
	export using ds::FunctionType;

	export using ds::ClassMember;
	export using ds::ClassMethod;
	export using ds::NativeModule;
	export using ds::NativeGenericClassType;
	export using ds::NativeFunction;
	export using ds::NativeStructType;

	export using ds::Int;
	export using ds::Float;
	export using ds::Size;
	export using ds::Bool;
	export using ds::Size;
	export using ds::Pointer;
	export using ds::RuntimeStr;
	export using ds::RuntimeStrRef;
	export using ds::RuntimeClass;
	export using ds::ClassRef;
	export using ds::ClassPtr;
}