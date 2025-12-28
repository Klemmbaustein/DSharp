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
std::optional<std::string> ds::TypeMember::getParameterValue(const std::string& name) const
{
	for (auto& i : this->parameterData)
	{
		size_t equals = i.find_first_of('=');

		if (i.substr(0, equals) == name)
		{
			return i.substr(equals + 1);
		}
	}

	return {};
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