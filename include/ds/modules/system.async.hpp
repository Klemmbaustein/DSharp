#pragma once
#include <ds/native/nativeModule.hpp>
#include <mutex>

namespace ds::modules::system
{
	namespace async
	{
		NativeModule createModule(LanguageContext* to);

		struct Task;

		using TaskCallback = void (*)(ClassRef<Task>, InterpretContext*, void*);

		struct Task
		{
			Bool completed;
			union
			{
				InterpretContext* awaiter;
				void* awaitNativeData;
			};
			TaskCallback awaitNative;
			uint8_t* resultBuffer;
			std::mutex taskMutex;
		};

		void pushTaskResult(Task* target, size_t size, InterpretContext* to);

		RuntimeClass* emptyTask();

		void attachTaskResultBuffer(ClassRef<Task> task, void* data, size_t length);

		template <typename T>
		void attachTaskResult(ClassRef<Task> task, T value)
		{
			if (sizeof(T))
			{
				attachTaskResultBuffer(task, &value, sizeof(T));
			}
		}

		template <typename T>
		RuntimeClass* completedTask(T result)
		{
			ClassRef<Task> t = emptyTask();

			if (sizeof(T))
			{
				attachTaskResultBuffer(t, &result, sizeof(T));
			}
			t->completed = true;
			return t.classPtr;
		}

		template <typename T>
		T getTaskResult(ClassRef<Task> task)
		{
			return *reinterpret_cast<T*>(task->resultBuffer);
		}

		RuntimeClass* completedTask();

		void completeTaskWithBuffer(ClassRef<Task> task, size_t size, InterpretContext* context);

		template <typename T>
		void completeTask(ClassRef<Task> task, T result, InterpretContext* context)
		{
			attachTaskResult(task, result);
			completeTaskWithBuffer(task, sizeof(T), context);
		}

		void completeTask(ClassRef<Task> task, InterpretContext* context);

	} // namespace async
} // namespace ds::modules::system