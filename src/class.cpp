#include "class.hpp"
using namespace lang;

RuntimeClass* lang::RuntimeClass::allocateClass(size_t bodySize, bytecodeOffset destructor)
{
	void* classMemory = malloc(sizeof(RuntimeClass) + bodySize);

	if (!classMemory)
	{
		abort();
		return nullptr;
	}

	RuntimeClass* header = reinterpret_cast<RuntimeClass*>(classMemory);

	header->vtable = new bytecodeOffset[1]();
	header->vtable[0] = destructor;
	header->references = 1;

	return header;
}
