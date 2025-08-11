#pragma once
#include <native/nativeModule.hpp>
#include <parser/attribute.hpp>
#include <cstring>

namespace lang::modules
{
	/**
	 * @ingroup stdlib
	 * @brief
	 * Base standard library module.
	 *
	 * It contains the basic standard library functionalities.
	 */
	namespace system
	{
		/**
		 * @ingroup stdlib
		 *
		 * @brief
		 * Entry point attribute.
		 *
		 * Marks the function it's applied to as the program entry point.
		 * The program entry point is the default function that's called when calling InterpretContext::run()
		 * Only one EntryPoint attribute can be used in a program.
		 *
		 * @see Attribute
		 * @see InterpretContext
		 */
		class EntryPointAttribute : public Attribute
		{
		public:
			EntryPointAttribute()
			{
				this->name = "EntryPoint";
			}
		};

		/**
		 * @ingroup stdlib
		 *
		 * @brief
		 * No discard.
		 *
		 * Marks the return value of the function as Discard. If this attribute isn't applied to a function,
		 * the compiler will emit an error if the return value of this function is ignored.
		 *
		 * @see Attribute
		 * @see Function
		 */
		class DiscardAttribute : public Attribute
		{
		public:
			DiscardAttribute()
			{
				this->name = "Discard";
			}
		};

		/**
		 * @brief
		 * Creates the system module.
		 *
		 * @see lang::modules::createStandardLibrary
		 */
		NativeModule createModule();

		class ArrayData
		{
		public:
			uint32_t length = 0;
			void* data = nullptr;
		};

		RuntimeClass* createArrayObject();

		template<typename T>
		RuntimeClass* createArray(T* items, size_t length)
		{
			size_t sizeInBytes = length * sizeof(T);
			void* buffer = malloc(sizeInBytes);

			if (!buffer)
			{
				abort();
			}

			memcpy(buffer, items, sizeInBytes);

			RuntimeClass* array = createArrayObject();
			ArrayData* data = reinterpret_cast<ArrayData*>(array->getBody());
			data->data = buffer;
			data->length = length;
			return array;
		}

	} // namespace system
} // namespace lang::modules