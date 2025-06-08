#pragma once
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <bytecode.hpp>

namespace lang
{
	struct RuntimeClass
	{
		bytecodeOffset* vtable;
		size_t references;

		static RuntimeClass* allocateClass(size_t bodySize, bytecodeOffset* vTable);

		void addRef()
		{
			references++;
		}

		static bytecodeOffset unref(RuntimeClass* header)
		{
			if (!header)
			{
				return 0;
			}

#ifdef _DEBUG
			if (header->references > UINT16_MAX)
			{
				// If the refcount is this high we're probably reading invalid data
				abort();
			}
#endif

			if (header->references == 0)
			{
				free(header);
				return 0;
			}

			header->references--;

			if (header->references == 0)
			{
				if (!header->vtable || !header->vtable[0])
				{
					free(header);
				}
				else
					return header->vtable[0];
			}
			return 0;
		}

		inline uint8_t* getBody()
		{
			return (uint8_t*)(this) + sizeof(RuntimeClass);
		}
	};
} // namespace lang