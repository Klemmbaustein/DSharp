#pragma once
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include "languageTypes.hpp"

namespace ds
{
	class InterpretContext;

	/**
	 * @brief
	 * Runtime class instance header
	 *
	 * Stores the reference count and a vtable for the class.
	 */
	struct RuntimeClass
	{
		/**
		 * @brief
		 * The class instance's virtual table.
		 *
		 * Stores function pointers to all the classes' virtual functions, which are then used
		 * to find the proper ones to call.
		 *
		 * The first function in the table always is the destructor.
		 */
		RuntimeFunction* vtable;
		TypeId type;
		Size references;
		// Debug variable to ensure no classes are leaked
		static inline size_t classRefCount = 0;

		static RuntimeClass* allocateClass(size_t bodySize, ds::TypeId id, RuntimeFunction* vTable);

		/**
		 * @brief
		 * Adds a reference to this class object, increasing it's reference count by 1
		 */
		void addRef()
		{
			references++;
		}

		/**
		 * @brief
		 * Un references the target object, decreasing it's reference count by 1 and deleting it if necessary.
		 *
		 * @param object
		 * The pointer to the Runtime class instance tha should be unreferenced.
		 * Can be null, in which case the function will do nothing.
		 * @return
		 * A function pointer to the runtime object's destructor if it has any and it needs to be called.
		 *
		 * You shouldn't call this function directly, use InterpretContext::destruct() to properly
		 * de reference and destruct an object, as that will automatically also call it's destructors.
		 */
		static RuntimeFunction unref(RuntimeClass* object)
		{
			if (!object)
			{
				return RuntimeFunction();
			}

#ifdef _DEBUG
			if (object->references > UINT16_MAX)
			{
				// If the refcount is this high we're probably reading invalid data
				abort();
			}
#endif

			if (object->references == 0)
			{
				classRefCount--;
				free(object);
				return RuntimeFunction();
			}

			object->references--;

			if (object->references == 0)
			{
				if (!object->vtable || !bool(object->vtable[0]))
				{
					classRefCount--;
					free(object);
				}
				else
					return object->vtable[0];
			}
			return RuntimeFunction();
		}

		/**
		 * @brief
		 * Gets a pointer to the body of this class object
		 *
		 * The RuntimeClass struct only stores the "header" of the object
		 * It's actual class data follows it right after in memory, and this function returns a pointer to that.
		 *
		 * @return
		 * The body
		 */
		inline uint8_t* getBody()
		{
			return (uint8_t*)(this) + sizeof(RuntimeClass);
		}
	};

	void FreePtr(InterpretContext* context, RuntimeClass* Ptr);

	template <typename T>
	struct ClassPtr
	{
		ClassPtr(RuntimeClass* classPtr, InterpretContext* context)
		{
			this->classPtr = classPtr;
			this->context = context;
		}

		ClassPtr()
		{
		}

		RuntimeClass* classPtr = nullptr;
		InterpretContext* context = nullptr;

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

		operator bool() const
		{
			return classPtr;
		}

		ClassPtr(const ClassPtr& other)
		{
			this->classPtr = other.classPtr;
			this->context = other.context;
			if (classPtr)
			{
				this->classPtr->addRef();
			}
		}

		~ClassPtr()
		{
			if (context)
			{
				FreePtr(context, classPtr);
			}
			else
			{
				RuntimeClass::unref(classPtr);
			}
		}
	};

	/**
	 * @brief
	 * A reference to a runtime object that wraps RuntimeClass into an interface that let's you easily access it's members.
	 * @tparam T
	 * The type this class reference is pointing to.
	 *
	 * This struct works like ClassPtr, but does not affect the target's reference count in any way.
	 *
	 * @see ClassPtr
	 */
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

		operator bool() const
		{
			return classPtr;
		}

		ClassRef(const ClassRef& other) = default;

		~ClassRef() = default;
	};


} // namespace ds