#pragma once
#include <ds/native/nativeModule.hpp>
#include <ds/native/nativeGeneric.hpp>
#include <ds/parser/attribute.hpp>
#include <cstring>
#include <cassert>

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
		NativeModule createModule(LanguageContext* to);

		class ArrayData
		{
		public:
			uint32_t length = 0;
			void* data = nullptr;
			template<typename T>
			T& at(uint32_t index)
			{
				assert(index < length);
				return reinterpret_cast<T*>(data)[index];
			}
			bool isType = false;
		};

		class MapData
		{
		public:
			class Node
			{
			public:
				Node* a = nullptr;
				Node* b = nullptr;
				uint8_t* key = nullptr;
				uint8_t* value = nullptr;
			};

			void insert(uint8_t* key, GenericData keyType, uint8_t* value, GenericData valueSize,
				InterpretContext* context);

			void remove(uint8_t* key, GenericData keyType, InterpretContext* context);

			Node*& getNode(uint8_t* key, GenericData keyType, InterpretContext* context);
			Node*& getMinimumNode(Node*& at);

			static int compare(uint8_t* a, uint8_t* b, GenericData type, InterpretContext* context,
				RuntimeClass* comparator);

			static bool lessThan(uint8_t* a, uint8_t* b, GenericData type, InterpretContext* context,
				RuntimeClass* comparator);

			void deleteNode(Node* target, InterpretContext* context);

			static RuntimeClass* getClass(uint8_t* atPtr);

			RuntimeClass* comparator;
			Node* rootNode = nullptr;
			bool keyIsClassType = false;
			bool valueIsClassType = false;
		};

		RuntimeClass* createArrayObject();

		template <typename T>
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
		RuntimeClass* createMapObject();

		template <typename T>
		RuntimeClass* createMap(T* items, Size length, bool isType)
		{
			size_t sizeInBytes = length * sizeof(T);
			void* buffer = malloc(sizeInBytes);

			if (!buffer)
			{
				abort();
			}

			memcpy(buffer, items, sizeInBytes);

			RuntimeClass* array = createMapObject();
			ArrayData* data = reinterpret_cast<ArrayData*>(array->getBody());
			data->data = buffer;
			data->length = length;
			data->isType = isType;
			return array;
		}

	} // namespace system
} // namespace ds::modules