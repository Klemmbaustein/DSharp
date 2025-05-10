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

		static RuntimeClass* allocateClass(size_t bodySize, bytecodeOffset destructor);
		
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

			if (header->references > 5)
			{
				abort();
			}

			if (header->references == 0)
			{
				delete[] header->vtable;
				free(header);
				return 0;
			}

			header->references--;

			if (header->references == 0)
			{
				if (!header->vtable[0])
				{
					delete[] header->vtable;
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
}