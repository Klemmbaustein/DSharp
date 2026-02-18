#pragma once
#include <ds/class.hpp>
#include <ds/interpreter.hpp>

namespace ds
{
	/**
	 * @brief
	 * Wraps a callable D# class into a class that can be used with C++'s std::function,
	 * while properly doing reference counting.
	 *
	 * @tparam TReturn
	 * The return type of the function.
	 */
	template <typename TReturn>
	class CallableWrapper
	{
	public:

		CallableWrapper(RuntimeClass* callablePtr, InterpretContext* interpreter)
		{
			this->callablePtr = callablePtr;
			this->interpreter = interpreter;
		}

		void operator=(const CallableWrapper& other)
		{
			this->callablePtr = other.callablePtr;
			callablePtr->addRef();
			this->interpreter = other.interpreter;
		}
		CallableWrapper(const CallableWrapper& other)
		{
			this->callablePtr = other.callablePtr;
			callablePtr->addRef();
			this->interpreter = other.interpreter;
		}

		virtual ~CallableWrapper()
		{
			this->interpreter->destruct(this->callablePtr);
		}

		RuntimeClass* callablePtr;
		InterpretContext* interpreter;

		TReturn operator()() const
		{
			if constexpr (std::is_same_v<TReturn, void>)
			{
				interpreter->callVirtualMethodVoid(callablePtr, 1);
			}
			else
			{
				return interpreter->callVirtualMethod<TReturn>(callablePtr, 1);
			}
		}
	};
} // namespace ds