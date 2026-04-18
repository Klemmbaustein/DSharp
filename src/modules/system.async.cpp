#include <ds/modules/system.async.hpp>
#include <ds/parser/types/taskType.hpp>
#include <ds/language.hpp>
#include <thread>

using namespace ds;
using namespace ds::modules::system::async;
using namespace std::chrono;

static void async_sleepAndReturn(InterpretContext* context)
{
	ClassRef<Task> task = emptyTask();
	task.classPtr->addRef();

	context->runtime->createBackgroundThread([task, context] {
		std::this_thread::sleep_for(milliseconds(1000));

		completeTask(task, context);
		context->destruct(task.classPtr);
	});

	context->pushValue(task);
}

static void task_delete(InterpretContext* context)
{
	ClassPtr<Task> task = context->popPtr<Task>();

	if (task->resultBuffer)
	{
		delete task->resultBuffer;
	}

	if (task->taskMutex)
	{
		delete task->taskMutex;
	}
}

static RuntimeFunction task_vTable = RuntimeFunction{
	.nativeFn = &task_delete,
};

static void task_newEmpty(InterpretContext* context)
{
	context->pushValue(emptyTask());
}

static void task_complete(InterpretContext* context)
{
	Size size = context->popValue<Size>();
	ClassRef<Task> task = context->popValue<RuntimeClass*>();

	if (size)
	{
		task->resultBuffer = new uint8_t[size];
		context->popBytes(task->resultBuffer, size);
		completeTaskWithBuffer(task, size, context);
	}
	else
	{
		completeTask(task, context);
	}

	context->destruct(task.classPtr);

	context->pushValue(task);
}

NativeModule ds::modules::system::async::createModule(LanguageContext* to)
{
	NativeModule out;
	out.name = "system::async";

	out.addFunction(NativeFunction({}, TaskType::getInstance(nullptr, to->registry), "sleep", &async_sleepAndReturn));

	out.addFunction(NativeFunction({}, nullptr, "task.newEmpty", &task_newEmpty));

	out.addFunction(NativeFunction({}, nullptr, "task.complete", &task_complete));

	return out;
}

void ds::modules::system::async::pushTaskResult(Task* target, size_t size, InterpretContext* to)
{
	to->pushBytes(target->resultBuffer, size);
}

RuntimeClass* ds::modules::system::async::emptyTask()
{
	ClassRef<Task> c = RuntimeClass::allocateClass(sizeof(Task), 0, &task_vTable);

	c->completed = false;
	c->awaiter = nullptr;
	c->resultBuffer = nullptr;
	c->taskMutex = new std::mutex();

	return c.classPtr;
}

void ds::modules::system::async::attachTaskResultBuffer(ClassRef<Task> task, void* data, size_t length)
{
	task->resultBuffer = new uint8_t[length];

	memcpy(task->resultBuffer, data, length);
}

RuntimeClass* ds::modules::system::async::completedTask()
{
	ClassRef<Task> t = emptyTask();
	t->completed = true;
	return t.classPtr;
}
void ds::modules::system::async::completeTaskWithBuffer(ClassRef<Task> task, size_t size, InterpretContext* context)
{
	{
		std::unique_lock l{ *task->taskMutex };

		if (task->awaiter && size)
			pushTaskResult(task.get(), size, task->awaiter);
		task->completed = true;
	}
	if (task->awaitNative)
	{
		task->awaitNative(task, context, task->awaitNativeData);
	}
	else if (task->awaiter)
	{
		task->awaiter->resumeSuspend();
	}
}
void ds::modules::system::async::completeTask(ClassRef<Task> task, InterpretContext* context)
{
	{
		task->taskMutex->lock();
		task->completed = true;
		task->taskMutex->unlock();
	}
	if (task->awaitNative)
	{
		task->awaitNative(task, context, task->awaitNativeData);
	}
	else if (task->awaiter)
	{
		task->awaiter->resumeSuspend();
	}
}