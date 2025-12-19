#include <ds/reflection.hpp>
#include <ds/interpreter.hpp>

using namespace ds;

RuntimeClass* TypeInfo::create(InterpretContext* context) const
{
	auto cls = RuntimeClass::allocateClass(this->bodySize, &context->runtime->vTable->at(this->vTableOffset));

	context->pushValue(cls);

	context->run(this->constructor);

	return context->popValue<RuntimeClass*>();
}

bool ds::ReflectInfo::isSubclassOf(TypeId toCheck, TypeId superClass) const
{
	auto t = this->types.find(toCheck);

	if (t != this->types.end())
	{
		for (auto& i : t->second.superClasses)
		{
			if (i == superClass)
			{
				return true;
			}
			else if (isSubclassOf(i, superClass))
			{
				return true;
			}
		}
	}

	return false;
}