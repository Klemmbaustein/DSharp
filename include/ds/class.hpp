#pragma once
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include "languageTypes.hpp"

namespace ds
{
	class InterpretContext;

	using TypeId = Size;

	struct RuntimeClass
	{
		VTableEntry* vtable;
		size_t references;
		static inline size_t classRefCount = 0;

		static RuntimeClass* allocateClass(size_t bodySize, VTableEntry* vTable);

		void addRef()
		{
			references++;
		}

		static VTableEntry unref(RuntimeClass* header)
		{
			if (!header)
			{
				return VTableEntry();
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
				classRefCount--;
				free(header);
				return VTableEntry();
			}

			header->references--;

			if (header->references == 0)
			{
				if (!header->vtable || !bool(header->vtable[0]))
				{
					classRefCount--;
					free(header);
				}
				else
					return header->vtable[0];
			}
			return VTableEntry();
		}

		inline uint8_t* getBody()
		{
			return (uint8_t*)(this) + sizeof(RuntimeClass);
		}
	};

	template <typename T>
	struct ClassPtr
	{
		ClassPtr(RuntimeClass* classPtr)
		{
			this->classPtr = classPtr;
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

	template <typename T>
	struct ClassRef
	{
		ClassRef(RuntimeClass* classPtr)
		{
			this->classPtr = classPtr;
		}

		RuntimeClass* classPtr = nullptr;

		T* get() const
		{
			return reinterpret_cast<T*>(this->classPtr->getBody());
		}

		T& getValue() const
		{
			return *reinterpret_cast<T*>(this->classPtr->getBody());
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

		ClassRef(const ClassRef& other) = default;

		~ClassRef() = default;
	};


} // namespace ds