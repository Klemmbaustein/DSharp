#pragma once
#include "class.hpp"
#include "languageTypes.hpp"
#include <cstring>

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
			Size contentSize = Size(stringLength + 1);

			this->classPtr = RuntimeClass::allocateClass(contentSize + sizeof(Size), 0);

			(*(Size*)this->classPtr->getBody()) = Size(stringLength);
			char* strBegin = (char*)(this->classPtr->getBody() + sizeof(Size));
			memcpy(strBegin, stringPtr, stringLength);
		}

		explicit RuntimeStr(const char* stringPtr)
			: RuntimeStr(stringPtr, std::strlen(stringPtr))
		{

		}

		RuntimeClass* classPtr = nullptr;

		const char* ptr()
		{
			return (const char*)(classPtr->getBody() + sizeof(Size));
		}

		Size length() const
		{
			return *(Size*)classPtr->getBody();
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
			Size contentSize = Size(stringLength + 1);

			this->classPtr = RuntimeClass::allocateClass(contentSize + sizeof(Size), 0);

			(*(Size*)this->classPtr->getBody()) = Size(stringLength);
			char* strBegin = (char*)(this->classPtr->getBody() + sizeof(Size));
			memcpy(strBegin, stringPtr, stringLength);
		}


		RuntimeClass* classPtr = nullptr;

		const char* ptr()
		{
			return (const char*)(classPtr->getBody() + sizeof(uint32_t));
		}

		Size length() const
		{
			return *(Size*)classPtr->getBody();
		}

		RuntimeStrRef(const RuntimeStrRef& other)
		{
			this->classPtr = other.classPtr;
		}
	};

}