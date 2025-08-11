#pragma once
#include "class.hpp"

namespace lang
{
	struct RuntimeStr
	{
		RuntimeStr(RuntimeClass* classPtr)
		{
			this->classPtr = classPtr;
		}
		RuntimeStr(const char* stringPtr, size_t stringLength)
		{
			uint32_t contentSize = stringLength + 1;

			this->classPtr = RuntimeClass::allocateClass(contentSize + sizeof(uint32_t), 0);

			(*(uint32_t*)this->classPtr->getBody()) = stringLength;
			char* strBegin = (char*)(this->classPtr->getBody() + sizeof(uint32_t));
			memcpy(strBegin, stringPtr, stringLength);
		}

		RuntimeClass* classPtr = nullptr;

		const char* ptr()
		{
			return (const char*)(classPtr->getBody() + sizeof(uint32_t));
		}

		uint32_t length() const
		{
			return *(uint32_t*)classPtr->getBody();
		}

		RuntimeStr(const RuntimeStr& other)
		{
			this->classPtr = other.classPtr;
			this->classPtr->addRef();
		}

		~RuntimeStr()
		{
			RuntimeClass::unref(classPtr);
		}
	};

	struct RuntimeStrRef
	{
		RuntimeStrRef(RuntimeClass* classPtr)
		{
			this->classPtr = classPtr;
		}
		RuntimeStrRef(const char* stringPtr, size_t stringLength)
		{
			uint32_t contentSize = stringLength + 1;

			this->classPtr = RuntimeClass::allocateClass(contentSize + sizeof(uint32_t), 0);

			(*(uint32_t*)this->classPtr->getBody()) = stringLength;
			char* strBegin = (char*)(this->classPtr->getBody() + sizeof(uint32_t));
			memcpy(strBegin, stringPtr, stringLength);
		}


		RuntimeClass* classPtr = nullptr;

		const char* ptr()
		{
			return (const char*)(classPtr->getBody() + sizeof(uint32_t));
		}

		uint32_t length() const
		{
			return *(uint32_t*)classPtr->getBody();
		}

		RuntimeStrRef(const RuntimeStrRef& other)
		{
			this->classPtr = other.classPtr;
		}
	};

}