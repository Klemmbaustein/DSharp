#include <ds/class.hpp>
using namespace ds;

RuntimeClass* ds::RuntimeClass::allocateClass(size_t bodySize, VTableEntry* vTable)
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
	header->references = 1;

	return header;
}
