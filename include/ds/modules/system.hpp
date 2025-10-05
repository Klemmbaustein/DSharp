#pragma once
#include <ds/native/nativeModule.hpp>
#include <ds/parser/attribute.hpp>
#include <cstring>

namespace ds::modules
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
		 * @ingroup stdlib
		 *
		 * @brief
		 * Reflect member.
		 *
		 * Marks a class member as reflected, making it visible to the reflection system.
		 *
		 * @see Attribute
		 */
		class ReflectAttribute : public Attribute
		{
		public:
			ReflectAttribute()
			{
				this->name = "Reflect";
			}
		};

		/**
		 * @brief
		 * Creates the system module.
		 *
		 * @see ds::modules::createStandardLibrary
		 */
		NativeModule createModule();

		class ArrayData
		{
		public:
			uint32_t length = 0;
			void* data = nullptr;
			bool isType = false;
		};

		RuntimeClass* createArrayObject();

		template<typename T>
		RuntimeClass* createArray(T* items, Size length, bool isType)
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
			data->isType = isType;
			return array;
		}

	} // namespace system
} // namespace ds::modules