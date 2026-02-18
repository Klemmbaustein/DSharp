#pragma once
#include "bytecode.hpp"
#include "native/externalFunction.hpp"
#include "runtimeString.hpp"
#include "unwindInfo.hpp"
#include <array>
#include <functional>
#include <list>
#include <map>
#include <thread>

namespace ds
{
	struct LanguageContext;
	class LanguageRuntime;

	class InterpretContext
	{
	public:

		void run(BytecodeOffset position = 0);

		template <typename T>
		T popValue()
		{
			stackPos -= sizeof(T);
			return *(T*)&this->stack[stackPos];
		}

		std::string popString();
		RuntimeStr popRuntimeString();
		void pushRuntimeString(RuntimeStr str);

		template<typename T>
		ClassPtr<T> popPtr()
		{
			return ClassPtr<T>(popValue<RuntimeClass*>(), this);
		}

		template <typename T>
		void pushValue(const T& value)
		{
			*((T*)&this->stack[stackPos]) = value;
			stackPos += sizeof(value);
		}

		void pushBytes(const uint8_t* data, Size size)
		{
			if (size == 0)
				return;

			memcpy(&this->stack[stackPos], data, size);
			stackPos += size;
		}
		void popBytes(uint8_t* to, Size size)
		{
			if (size == 0)
				return;

			stackPos -= size;
			memcpy(to, &this->stack[stackPos], size);
		}
		void copyBytes(Size size)
		{
			if (size == 0)
				return;

			memcpy(&this->stack[stackPos], &this->stack[stackPos - size], size);
			stackPos += size;
		}

		template<typename T>
		[[nodiscard]]
		T callVirtualMethod(RuntimeClass* targetObject, BytecodeOffset vTableIndex)
		{
			auto entry = targetObject->vtable[vTableIndex];
			if (!entry)
			{
				return T();
			}
			pushValue(targetObject);
			virtualCall(entry);

			return popValue<T>();
		}

		void callVirtualMethodVoid(RuntimeClass* targetObject, BytecodeOffset vTableIndex)
		{
			auto entry = targetObject->vtable[vTableIndex];
			if (!entry)
			{
				return;
			}
			pushValue(targetObject);
			virtualCall(entry);
		}

		uint32_t getVarArgsCount()
		{
			return popValue<uint32_t>();
		}

		std::vector<DebugSection*> getStackTrace() const;

		void virtualCall(RuntimeFunction target);
		bool resumeSuspend();

		void runtimePanic(RuntimeStr message);

		void destruct(RuntimeClass* classObject);

		void copyFrom(InterpretContext* other);

		LanguageRuntime* runtime = nullptr;
		BinaryBufferRef code;
		bool suspended = false;
		bool canAwait = false;
		BytecodeOffset suspendStackPos = 0;

		void doUnwind();

	private:

		void runLoop(BytecodeOffset& baseCallStackPos);

		constexpr static size_t STACK_SIZE = 4096;
		constexpr static size_t VAR_STACK_SIZE = 4096;
		constexpr static size_t CALL_STACK_SIZE = 512;
		RuntimeStrRef popRuntimeStringRef();

		std::array<uint8_t, STACK_SIZE> stack = {};
		std::array<uint8_t, VAR_STACK_SIZE> variableStack = {};
		std::array<BytecodeOffset, CALL_STACK_SIZE> callStack = {};
		uint32_t stackPos = 0;
		uint32_t variableStackPos = 0;
		uint32_t callStackPos = 0;
	};

	class LanguageRuntime
	{
	public:
		DebugInfo* debug = nullptr;
		BinaryBuffer* bytecodeBuffer = nullptr;
		UnwindInfo unwindBuffer;
		ReflectInfo* reflect = nullptr;
		std::vector<ExternalFunctionPointer> externals;
		std::vector<RuntimeFunction>* vTable = nullptr;

		LanguageRuntime(LanguageContext* from);
		LanguageContext* language = nullptr;
		LanguageRuntime(const LanguageRuntime& other) = delete;
		~LanguageRuntime();

		void defaultCreateBackgroundThread(std::function<void()> f);

		// TODO: replace with some kind of thread pool interface
		std::map<size_t, std::thread*> backgroundThreads;
		std::function<void(std::function<void()>)> createBackgroundThread;
		std::function<void(const char*)> writeError;

		void loadBytecode(BytecodeStream* code);

		InterpretContext baseContext;

		std::list<InterpretContext> asyncContexts;

		void run(BytecodeOffset position = 0);
	};

} // namespace ds