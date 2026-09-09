#pragma once
#include "class.hpp"
#include "debug/debugState.hpp"
#include "native/externalFunction.hpp"
#include "runtimeString.hpp"
#include "unwindInfo.hpp"
#include <array>
#include <functional>
#include <list>
#include <map>
#include <thread>
#include <vector>

namespace ds
{
	class LanguageRuntime;
	class LanguageContext;

	enum class RunResult
	{
		ok,
		error
	};

	class InterpretContext
	{
	public:
		virtual ~InterpretContext() = default;
		[[nodiscard]]
		virtual RunResult run(Pointer position = 0) = 0;
		virtual void doUnwind() = 0;
		virtual bool resumeSuspend() = 0;
		virtual std::vector<DebugSection*> getStackTrace() const = 0;
		virtual void loadBytecode(BytecodeStream* code) = 0;
		virtual InterpretContext* createCopy() = 0;
		virtual bool setDebugBreakpoint(size_t instructionOffset)
		{
			return false;
		}
		virtual void removeDebugBreakpoint(size_t instructionOffset)
		{
		}

		void destruct(RuntimeClass* classObject);

		template <typename T>
		T popValue()
		{
			T result;
			stackPos -= sizeof(T);
			memcpy(&result, &stack[stackPos], sizeof(T));
			return result;
		}

		std::string popString();
		RuntimeStr popRuntimeString();
		void pushRuntimeString(RuntimeStr str);

		template <typename T>
		ClassPtr<T> popPtr()
		{
			return ClassPtr<T>(popValue<RuntimeClass*>(), this);
		}

		template <typename T>
		void pushValue(const T& value)
		{
			memcpy(&this->stack[stackPos], &value, sizeof(T));
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

		template <typename T>
		[[nodiscard]]
		T callVirtualMethod(RuntimeClass* targetObject, BytecodeOffset vTableIndex)
		{
			auto entry = targetObject->vtable[vTableIndex];
			if (!entry)
			{
				return T();
			}
			pushValue(targetObject);
			auto result = virtualCall(entry);

			return popValue<T>();
		}

		[[nodiscard]]
		RunResult callVirtualMethodVoid(RuntimeClass* targetObject, BytecodeOffset vTableIndex)
		{
			auto entry = targetObject->vtable[vTableIndex];
			if (!entry)
			{
				return RunResult::ok;
			}
			pushValue(targetObject);
			return virtualCall(entry);
		}

		uint32_t getVarArgsCount()
		{
			return popValue<uint32_t>();
		}

		RunResult virtualCall(RuntimeFunction target);

		void runtimePanic(const char* message);

		constexpr static size_t STACK_SIZE = 4096;
		constexpr static size_t VAR_STACK_SIZE = 4096;
		constexpr static size_t CALL_STACK_SIZE = 512;

		std::array<uint8_t, STACK_SIZE> stack = {};
		size_t stackPos = 0;
		std::array<uint8_t, VAR_STACK_SIZE> variableStack = {};
		size_t variableStackPos = 0;
		std::vector<RuntimeFunction>* usedVTable = nullptr;

		LanguageRuntime* runtime = nullptr;

		RuntimeStrRef popRuntimeStringRef();
	};

	struct RuntimeOptions
	{
		bool useJustInTimeCompiler = false;
	};

	class LanguageRuntime
	{
	public:
		DebugInfo* debug = nullptr;
		BinaryBuffer* bytecodeBuffer = nullptr;
		UnwindInfo unwindBuffer;
		ReflectInfo* reflect = nullptr;
		std::vector<ExternalFunctionPointer> externals;
		std::vector<RuntimeFunction> vTable;

		LanguageRuntime(LanguageContext* from, RuntimeOptions settings);
		LanguageContext* language = nullptr;
		LanguageRuntime(const LanguageRuntime&) = delete;
		~LanguageRuntime();

		void defaultCreateBackgroundThread(std::function<void()> f);

		// TODO: replace with some kind of thread pool interface
		std::map<size_t, std::thread*> backgroundThreads;
		std::function<void(std::function<void()>)> createBackgroundThread;
		std::function<void(const char*)> writeError;
		std::function<bool(InterpretContext*, Pointer bytecodePosition, DebugState* state)> onDebugBreak;

		void loadBytecode(BytecodeStream* code);

		InterpretContext* (*createContext)(LanguageRuntime* fromRuntime) = nullptr;

		InterpretContext* baseContext = nullptr;

		std::list<InterpretContext*> asyncContexts;

		[[nodiscard]]
		RunResult run(BytecodeOffset position = 0);
	};

} // namespace ds