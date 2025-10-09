#pragma once
#include <ds/native/nativeModule.hpp>
#include <mutex>

namespace ds::modules::system
{
	namespace async
	{
		NativeModule createModule();

		struct Task
		{
			Bool completed;
			union
			{
				InterpretContext* awaiter;
				void* awaitNativeData;
			};
			void (*awaitNative)(void*);
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

		template<typename T>
		T getTaskResult(ClassRef<Task> task)
		{
			return *reinterpret_cast<T*>(task->resultBuffer);
		}

		RuntimeClass* completedTask();

		template <typename T>
		void completeTask(ClassRef<Task> task, T result)
		{
			{
				std::unique_lock l{ task->taskMutex };
				attachTaskResult(task, result);

				if (task->awaiter)
					pushTaskResult(task.get(), sizeof(T), task->awaiter);
				task->completed = true;
			}
			if (task->awaitNative)
			{
				task->awaitNative(task->awaitNativeData);
			}
			else if (task->awaiter)
			{
				task->awaiter->resumeSuspend();
			}
		}

		void completeTask(ClassRef<Task> task);

	} // namespace async
} // namespace ds::modules::system