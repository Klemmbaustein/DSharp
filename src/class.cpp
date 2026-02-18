#include <ds/class.hpp>
#include <ds/interpreter.hpp>
using namespace ds;

RuntimeClass* ds::RuntimeClass::allocateClass(size_t bodySize, ds::TypeId id, RuntimeFunction* vTable)
{
	classRefCount++;
	void* classMemory = calloc(sizeof(RuntimeClass) + bodySize, 1);

	if (!classMemory)
	{
		abort();
		return nullptr;
	}

	RuntimeClass* header = reinterpret_cast<RuntimeClass*>(classMemory);

	header->vtable = vTable;
	header->type = id;
	header->references = 1;
	header->referencesAreOffset = false;

	return header;
}

void ds::FreePtr(InterpretContext* context, RuntimeClass* Ptr)
{
	context->destruct(Ptr);
}
