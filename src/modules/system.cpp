#include <ds/modules/system.hpp>
#include <ds/parser/types/stringType.hpp>
#include <cstring>
#include <ds/language.hpp>
#include <ds/parser/types/functionType.hpp>
#include <ds/parser/types/genericArgument.hpp>
#include <ds/native/nativeGeneric.hpp>

using namespace ds;
using namespace ds::modules::system;

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

	Int cmp = std::strcmp(second.ptr(), first.ptr());

	context->pushValue(cmp);
}

static void string_concat(InterpretContext* context)
{
	auto second = context->popRuntimeStringRef();
	auto first = context->popRuntimeStringRef();

	Size newSize = first.length() + second.length();
	// Space for null terminator
	Size contentSize = newSize + 1;

	RuntimeClass* newClass = RuntimeClass::allocateClass(contentSize + sizeof(uint32_t),
		first.classPtr->type, 0);

	(*(Size*)newClass->getBody()) = newSize;
	char* strBegin = (char*)(newClass->getBody() + sizeof(uint32_t));
	memcpy(strBegin, first.ptr(), first.length());
	memcpy(strBegin + first.length(), second.ptr(), second.length());
	strBegin[first.length() + second.length()] = 0;
	context->pushValue<Pointer>(Pointer(newClass));
}

static void string_indexString(InterpretContext* context)
{
	auto index = context->popValue<int32_t>();
	auto first = context->popRuntimeString();
	first.classPtr->addRef();
	context->pushValue<Char>(Char(first.ptr()[index]));
}

static void string_setIndexCopy(InterpretContext* context)
{
	auto index = context->popValue<int32_t>();
	auto str = context->popRuntimeString();
	str.classPtr->addRef();
	Char newChar = context->popValue<Char>();
	str.classPtr->addRef();

	// Space for null terminator
	Size strLength = str.length();
	Size contentSize = str.length() + 1;

	RuntimeClass* newClass = RuntimeClass::allocateClass(contentSize + sizeof(Size),
		str.classPtr->type, 0);

	(*(Size*)newClass->getBody()) = strLength;
	char* strBegin = (char*)(newClass->getBody() + sizeof(Size));
	memcpy(strBegin, str.ptr(), strLength);
	strBegin[index] = newChar;
	context->pushValue<Pointer>(Pointer(newClass));
}

static void string_substr(InterpretContext* context)
{
	RuntimeStrRef baseStr = context->popRuntimeStringRef();
	Int count = context->popValue<Int>();
	Int start = context->popValue<Int>();

	if (start < 0 || count < 0)
	{
		context->runtimePanic("Invalid negative string.substr() argument");
		return;
	}

	Int length = baseStr.length();

	if (start == 0 && length <= count)
	{
		baseStr.classPtr->addRef();
		context->pushValue(baseStr);
		return;
	}

	if (start >= length)
	{
		context->pushValue(RuntimeStrRef(nullptr, 0));
		return;
	}

	RuntimeStr str = RuntimeStr(baseStr.ptr() + start, std::min(count, length - start));

	str.classPtr->addRef();

	context->pushValue(str);
}

static void int_toString(InterpretContext* context)
{
	auto str = std::to_string(context->popValue<Int>());
	context->pushRuntimeString(RuntimeStr(str.data(), str.size()));
}

static void float_toString(InterpretContext* context)
{
	auto str = std::to_string(context->popValue<float>());
	context->pushRuntimeString(RuntimeStr(str.data(), str.size()));
}

static void array_delete(InterpretContext* context)
{
	ClassPtr<ArrayData> array = context->popPtr<ArrayData>();

	if (array->data)
	{
		if (array->isType)
		{
			for (uint32_t i = 0; i < array->length; i++)
			{
				RuntimeClass** elem = reinterpret_cast<RuntimeClass**>(array->data);
				context->destruct(elem[i]);
			}
		}
		free(array->data);
	}
}

static void map_delete(InterpretContext* context)
{
	ClassPtr<MapData> map = context->popPtr<MapData>();

	map->deleteNode(map->rootNode, context);
}

static RuntimeFunction mapVTable = RuntimeFunction{
	.nativeFn = &map_delete
};

static void map_new(InterpretContext* context)
{
	auto value = GenericData(context);
	auto key = GenericData(context);

	ClassRef<MapData> map = context->popValue<RuntimeClass*>();
	map.classPtr->vtable = &mapVTable;
	map->comparator = nullptr;
	map->rootNode = nullptr;
	map->keyIsClassType = key.isClassType;
	map->valueIsClassType = value.isClassType;
	context->pushValue(map);
}

static void map_insert(InterpretContext* context)
{
	auto value = GenericData(context);
	auto key = GenericData(context);
	ClassRef<MapData> map = context->popValue<RuntimeClass*>();

	std::vector<uint8_t> buffer;
	buffer.resize(value.typeSize + key.typeSize);

	uint8_t* keyPtr = buffer.data() + value.typeSize;
	uint8_t* valuePtr = buffer.data();

	context->popBytes(valuePtr, value.typeSize);
	context->popBytes(keyPtr, key.typeSize);

	map->insert(keyPtr, key, valuePtr, value, context);
	if (map->keyIsClassType)
	{
		context->destruct(MapData::getClass(keyPtr));
	}
	if (map->valueIsClassType)
	{
		context->destruct(MapData::getClass(valuePtr));
	}

	return;
}

static void map_at(InterpretContext* context)
{
	auto value = GenericData(context);
	auto key = GenericData(context);
	ClassRef<MapData> map = context->popValue<RuntimeClass*>();

	std::vector<uint8_t> buffer;
	buffer.resize(key.typeSize);

	context->popBytes(buffer.data(), key.typeSize);

	MapData::Node*& node = map->getNode(buffer.data(), key, context);
	if (map->keyIsClassType)
	{
		context->destruct(MapData::getClass(buffer.data()));
	}

	if (node)
	{
		if (map->valueIsClassType)
		{
			MapData::getClass(node->value)->addRef();
		}
		context->pushBytes(node->value, value.typeSize);
	}
	else
	{
		context->runtimePanic("Map.at failed. No such key.");
	}

	return;
}

static void map_remove(InterpretContext* context)
{
	auto value = GenericData(context);
	auto key = GenericData(context);
	ClassRef<MapData> map = context->popValue<RuntimeClass*>();

	std::vector<uint8_t> buffer;
	buffer.resize(key.typeSize);

	context->popBytes(buffer.data(), key.typeSize);

	map->remove(buffer.data(), key, context);
	if (map->keyIsClassType)
	{
		context->destruct(MapData::getClass(buffer.data()));
	}

	return;
}

MapData::Node*& ds::modules::system::MapData::getMinimumNode(Node*& at)
{
	Node** found = &at;
	while ((*found)->a)
	{
		found = &(*found)->a;
	}

	return *found;
}

MapData::Node*& ds::modules::system::MapData::getNode(uint8_t* key, GenericData keyType, InterpretContext* context)
{
	Node** currentNode = &rootNode;

	while (*currentNode)
	{
		Node* n = *currentNode;
		int compareResult = compare(key, n->key, keyType, context, comparator);
		if (compareResult > 0)
		{
			currentNode = &n->a;
		}
		else if (compareResult < 0)
		{
			currentNode = &n->b;
		}
		else
		{
			break;
		}
	}

	return *currentNode;
}

void ds::modules::system::MapData::remove(uint8_t* key, GenericData keyType, InterpretContext* context)
{
	Node*& node = getNode(key, keyType, context);

	if (node)
	{
		Node* oldNode = node;
		if (keyIsClassType)
		{
			context->destruct(MapData::getClass(node->key));
		}
		if (valueIsClassType)
		{
			context->destruct(MapData::getClass(node->value));
		}
		if (!node->a)
		{
			node = node->b;
			delete oldNode;
			return;
		}
		if (!node->b)
		{
			node = node->a;
			delete oldNode;
			return;
		}

		Node*& minimumNode = getMinimumNode(node->b);

		node->key = minimumNode->key;
		node->value = minimumNode->value;

		oldNode = minimumNode;

		minimumNode = minimumNode->b;
		delete oldNode;
	}
	else
	{
		context->runtimePanic("Map.remove failed. No such key.");
	}
}

void ds::modules::system::MapData::insert(uint8_t* key, GenericData keyType, uint8_t* value,
	GenericData valueSize, InterpretContext* context)
{
	Node*& found = getNode(key, keyType, context);

	if (found)
	{
		if (keyType.isClassType)
		{
			context->destruct(getClass(found->value));
		}
	}
	else
	{
		found = new Node();

		found->key = new uint8_t[keyType.typeSize];
		memcpy(found->key, key, keyType.typeSize);

		if (keyIsClassType)
		{
			getClass(found->key)->addRef();
		}
	}

	found->value = new uint8_t[valueSize.typeSize];
	memcpy(found->value, value, valueSize.typeSize);

	if (valueIsClassType)
	{
		getClass(found->value)->addRef();
	}

	return;
}

RuntimeClass* ds::modules::system::MapData::getClass(uint8_t* atPtr)
{
	return *reinterpret_cast<RuntimeClass**>(atPtr);
}

void ds::modules::system::MapData::deleteNode(Node* target, InterpretContext* context)
{
	if (!target)
	{
		return;
	}

	deleteNode(target->a, context);
	deleteNode(target->b, context);

	if (keyIsClassType)
	{
		context->destruct(MapData::getClass(target->key));
	}
	delete[] target->key;
	if (valueIsClassType)
	{
		context->destruct(MapData::getClass(target->value));
	}
	delete[] target->value;

	delete target;
}

int ds::modules::system::MapData::compare(uint8_t* a, uint8_t* b, GenericData type, InterpretContext* context,
	RuntimeClass* comparator)
{
	if (type.id == StringType::STRING_ID)
	{
		context->pushBytes(b, type.typeSize);
		getClass(b)->addRef();
		context->pushBytes(a, type.typeSize);
		getClass(a)->addRef();

		string_compare(context);

		return context->popValue<Int>();
	}

	if (lessThan(a, b, type, context, comparator))
	{
		return -1;
	}
	else if (lessThan(b, a, type, context, comparator))
	{
		return 1;
	}
	return 0;
}

bool ds::modules::system::MapData::lessThan(uint8_t* a, uint8_t* b, GenericData type, InterpretContext* context,
	RuntimeClass* comparator)
{
	if (comparator)
	{
		context->pushBytes(b, type.typeSize);
		context->pushBytes(a, type.typeSize);
		context->virtualCall(comparator->vtable[1]);
		return context->popValue<Bool>();
	}

	switch (type.id)
	{
	case IntType::INT_ID:
		return *(Int*)(a) < *(Int*)(b);
	case FloatType::FLOAT_ID:
		return *(Float*)(a) < *(Float*)(b);
	}

	if (type.isClassType)
	{
		return *(Pointer*)(a) < *(Pointer*)(b);
	}

	return 0;
}

static void array_new(InterpretContext* context)
{
	Bool isType = context->popValue<Bool>();
	uint32_t elementSize = context->popValue<uint32_t>();
	uint32_t length = context->popValue<uint32_t>();

	auto newClass = createArrayObject();

	ArrayData* data = (ArrayData*)newClass->getBody();
	data->isType = isType;
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
	auto generic = GenericData(context);
	ArrayData* array = reinterpret_cast<ArrayData*>(context->popValue<RuntimeClass*>()->getBody());

	array->length++;

	void* newData = realloc(array->data, array->length * generic.typeSize);

	if (newData)
	{
		array->data = newData;
		context->popBytes((uint8_t*)array->data + generic.typeSize * (array->length - 1), generic.typeSize);
	}
}

static void array_pop(InterpretContext* context)
{
	auto generic = GenericData(context);
	ArrayData* array = reinterpret_cast<ArrayData*>(context->popValue<RuntimeClass*>()->getBody());

	if (array->length == 0)
	{
		return;
	}

	array->length--;

	RuntimeClass** elem = reinterpret_cast<RuntimeClass**>(array->data);

	context->destruct(elem[array->length]);

	void* newData = realloc(array->data, array->length * generic.typeSize);

	if (newData || array->length == 0)
	{
		array->data = newData;
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

static void fn_delete(InterpretContext* context)
{
	RuntimeClass* cls = context->popValue<RuntimeClass*>();
	delete[] cls->vtable;
	RuntimeClass::unref(cls);
}

static void fn_call(InterpretContext* context)
{
	RuntimeClass* cls = context->popValue<RuntimeClass*>();
	context->virtualCall(cls->vtable[2]);
}

static void fn_new_bytecode(InterpretContext* context)
{
	auto offset = context->popValue<Pointer>();

	RuntimeFunction* entries = new RuntimeFunction[3]();
	entries[0].nativeFn = &fn_delete;
	entries[1].nativeFn = &fn_call;
	entries[2].codeOffset = offset;

	context->pushValue(RuntimeClass::allocateClass(0, 0, entries));
}

static void fn_delete_lambda(InterpretContext* context)
{
	RuntimeClass* cls = context->popValue<RuntimeClass*>();
	int32_t size = *(int32_t*)cls->getBody();
	context->pushBytes(cls->getBody() + 4, size);
	context->virtualCall(cls->vtable[2]);
	delete[] cls->vtable;
	RuntimeClass::unref(cls);
}

static void fn_new_lambda(InterpretContext* context)
{
	auto offset = context->popValue<Pointer>();

	RuntimeFunction* entries = new RuntimeFunction[3]();
	entries[0].nativeFn = &fn_delete_lambda;
	entries[1].codeOffset = offset;

	int32_t size = context->popValue<BytecodeOffset>();

	auto deref = context->popValue<Pointer>();
	entries[2].codeOffset = deref;

	auto cls = RuntimeClass::allocateClass(size + 4, 0, entries);

	*(int32_t*)cls->getBody() = size;

	context->popBytes(cls->getBody() + 4, size);

	context->pushValue(cls);
}

static void fn_new_native(InterpretContext* context)
{
	auto offset = context->popValue<BytecodeOffset>();

	RuntimeFunction* entries = new RuntimeFunction[3]();
	entries[0].nativeFn = &fn_delete;
	entries[1].nativeFn = &fn_call;
	entries[2].nativeFn = context->runtime->externals[offset];

	context->pushValue(RuntimeClass::allocateClass(0, 0, entries));
}

ds::NativeModule ds::modules::system::createModule(LanguageContext* to)
{
	NativeModule out;
	out.name = "system";

	auto intType = to->registry->getEntry<IntType>();
	auto stringType = to->registry->getEntry<StringType>();

	out.addFunction(NativeFunction(
		{ FunctionArgument(intType, Token("intValue")) }, stringType,
		"int.toString", &int_toString));

	out.addFunction(NativeFunction(
		{ FunctionArgument(intType, Token("floatValue")) }, stringType,
		"float.toString", &float_toString));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"array.push", &array_push));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"array.pop", &array_pop));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"array.new", &array_new));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"array.at", &array_at));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"fn.new.bytecode", &fn_new_bytecode));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"fn.new.native", &fn_new_native));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"fn.new.lambda", &fn_new_lambda));

	out.addFunction(NativeFunction(
		{ FunctionArgument(stringType, "str") }, stringType,
		"string.format", &string_format));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"string.concat", &string_concat));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"string.substr", &string_substr));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"string.index", &string_indexString));

	out.addFunction(NativeFunction(
		{}, nullptr,
		"string.setIndexCopy", &string_setIndexCopy));

	out.addFunction(NativeFunction(
		{ FunctionArgument(stringType, Token("str1")),
			FunctionArgument(stringType, Token("str2")) },
		intType,
		"compareString", &string_compare));

	out.addAttribute(new EntryPointAttribute());
	out.addAttribute(new DiscardAttribute());
	out.addAttribute(new ReflectAttribute());

	out.addConstant("INT_MAX", intType, std::numeric_limits<Int>::max());
	out.addConstant("INT_MIN", intType, std::numeric_limits<Int>::min());

	auto mapType = out.createGenericClass<MapData>("Map", { GenericArgument("K"), GenericArgument("V") });

	out.addClassConstructor(mapType, NativeFunction({}, nullptr, "Map.new", map_new));

	auto firstGeneric = GenericArgumentType::getInstance(0, false);
	auto secondGeneric = GenericArgumentType::getInstance(1, false);

	mapType->members.push_back(ClassMember{
		.name = "comparator",
		.offset = offsetof(MapData, comparator),
		.type = FunctionType::getInstance(to->registry->getEntry<BoolType>(),
			{ firstGeneric, secondGeneric }, to->registry)
	        ->nullable });

	out.addClassMethod(mapType,
		NativeFunction(
			{ FunctionArgument(firstGeneric, "key"), FunctionArgument(secondGeneric, "value") },
			nullptr, "insert", &map_insert));

	out.addClassMethod(mapType,
		NativeFunction(
			{ FunctionArgument(firstGeneric, "key") },
			secondGeneric, "at", &map_at));

	out.addClassMethod(mapType,
		NativeFunction(
			{ FunctionArgument(firstGeneric, "key") },
			nullptr, "remove", &map_remove));

	return out;
}

RuntimeClass* ds::modules::system::createArrayObject()
{
	static RuntimeFunction arrayVTable = RuntimeFunction{
		.nativeFn = &array_delete
	};

	return RuntimeClass::allocateClass(sizeof(ArrayData), 0, &arrayVTable);
}
RuntimeClass* ds::modules::system::createMapObject()
{
	return RuntimeClass::allocateClass(sizeof(MapData), 0, &mapVTable);
}