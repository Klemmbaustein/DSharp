#include <modules/system.hpp>
#include <parser/types/stringType.hpp>
#include <cstring>

using namespace lang;
using namespace lang::modules::system;

static void string_format(InterpretContext* context)
{
	uint32_t varArgsCount = context->getVarArgsCount();

	std::vector<RuntimeStr> strings;

	for (uint32_t i = 0; i < varArgsCount; i++)
	{
		strings.push_back(context->popRuntimeString());
	}

	auto string = context->popRuntimeString();

	std::string result;
	size_t stringsIt = 0;

	for (uint32_t i = 0; i < string.length(); i++)
	{
		char newChar = string.ptr()[i];

		if (newChar == '{')
		{
			auto& newString = strings[stringsIt++];
			result.append(std::string(newString.ptr(), newString.length()));
			i++;
		}
		else
		{
			result.push_back(newChar);
		}
	}

	context->pushRuntimeString(RuntimeStr(result.data(), result.size()));
}

static void string_compare(InterpretContext* context)
{
	auto second = context->popRuntimeString();
	auto first = context->popRuntimeString();

	auto cmp = std::strcmp(second.ptr(), first.ptr());

	context->pushValue(cmp);
}

static void int_toString(InterpretContext* context)
{
	auto str = std::to_string(context->popValue<uint32_t>());
	context->pushRuntimeString(RuntimeStr(str.data(), str.size()));
}

static void float_toString(InterpretContext* context)
{
	auto str = std::to_string(context->popValue<float>());
	context->pushRuntimeString(RuntimeStr(str.data(), str.size()));
}

static void array_delete(InterpretContext* context)
{
	ClassPtr<ArrayData> array = context->popValue<RuntimeClass*>();

	for (uint32_t i = 0; i < array->length; i++)
	{
		RuntimeClass** elem = reinterpret_cast<RuntimeClass**>(array->data);
		context->pushValue(elem[i]);
		context->virtualCall(RuntimeClass::unref(elem[i]));
	}
	free(array->data);
}

static void array_new(InterpretContext* context)
{
	uint32_t elementSize = context->popValue<uint32_t>();
	uint32_t length = context->popValue<uint32_t>();

	auto newClass = createArrayObject();

	ArrayData* data = (ArrayData*)newClass->getBody();
	data->length = length;
	if (data->length)
	{
		data->data = calloc(length, elementSize);
		for (uint32_t i = 0; i < length; i++)
		{
			context->popBytes((uint8_t*)data->data + (elementSize * (length - i - 1)), elementSize);
		}
	}
	context->pushValue(newClass);
}

static void array_push(InterpretContext* context)
{
	ArrayData* array = reinterpret_cast<ArrayData*>(context->popValue<RuntimeClass*>()->getBody());
	uint32_t elementSize = context->popValue<uint32_t>();

	array->length++;

	void* newData = realloc(array->data, array->length * elementSize);

	if (newData)
	{
		array->data = newData;
		context->popBytes((uint8_t*)array->data + elementSize * (array->length - 1), elementSize);
	}
}

static void array_at(InterpretContext* context)
{
	uint32_t elementSize = context->popValue<uint32_t>();
	uint32_t index = context->popValue<uint32_t>();
	ArrayData* array = reinterpret_cast<ArrayData*>(context->popValue<RuntimeClass*>()->getBody());

	if (array->length <= index)
	{
		abort();
	}

	context->pushBytes((uint8_t*)array->data + elementSize * index, elementSize);
}

lang::NativeModule lang::modules::system::createModule()
{
	NativeModule out;
	out.name = "system";

	out.addFunction(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("position")), FunctionArgument(IntType::getInstance(), Token("length")) }, StringType::getInstance(),
		"string.substr",
		[](InterpretContext* context) {
			std::puts(std::to_string(context->popValue<int32_t>()).c_str());
		}));

	out.addFunction(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("intValue")) }, StringType::getInstance(),
		"int.toString", &int_toString));

	out.addFunction(NativeFunction(
		{ FunctionArgument(IntType::getInstance(), Token("floatValue")) }, StringType::getInstance(),
		"float.toString", &float_toString));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"array.push", &array_push));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"array.new", &array_new));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"array.at", &array_at));

	out.addFunction(NativeFunction(
		{ FunctionArgument(StringType::getInstance(), Token("str")) }, StringType::getInstance(),
		"format", &string_format));

	out.addFunction(NativeFunction(
		{
			FunctionArgument(StringType::getInstance(), Token("str1")),
			FunctionArgument(StringType::getInstance(), Token("str2"))
		},
		IntType::getInstance(),
		"compareString", &string_compare));

	out.attributes.push_back(new EntryPointAttribute());
	out.attributes.push_back(new DiscardAttribute());

	return out;
}

RuntimeClass* lang::modules::system::createArrayObject()
{
	static VTableEntry arrayVTable = VTableEntry{
		.nativeFn = &array_delete
	};

	return RuntimeClass::allocateClass(sizeof(ArrayData), &arrayVTable);
}
