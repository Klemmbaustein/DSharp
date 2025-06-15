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

	template<typename T>
	struct ClassPtr
	{
		ClassPtr(RuntimeClass* classPtr)
		{
			this->classPtr = classPtr;
		}
		ClassPtr(const char* stringPtr, size_t stringLength)
		{
			uint32_t contentSize = stringLength + 1;

			this->classPtr = RuntimeClass::allocateClass(contentSize + sizeof(uint32_t), 0);

			(*(uint32_t*)this->classPtr->getBody()) = stringLength;
			char* strBegin = (char*)(this->classPtr->getBody() + sizeof(uint32_t));
			memcpy(strBegin, stringPtr, stringLength);
		}

		RuntimeClass* classPtr = nullptr;

		T* get() const
		{
			return reinterpret_cast<T*>(this->classPtr->getBody());
		}

		T* operator->()
		{
			return get();
		}
		const T* operator->() const
		{
			return get();
		}
		T& operator*()
		{
			return *get();
		}
		const T& operator*() const
		{
			return *get();
		}

		ClassPtr(const ClassPtr& other)
		{
			this->classPtr = other.classPtr;
			this->classPtr->addRef();
		}

		~ClassPtr()
		{
			RuntimeClass::unref(classPtr);
		}
	};

} // namespace lang