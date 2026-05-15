#include <ds/reflection.hpp>
#include <ds/interpreter.hpp>

using namespace ds;

RuntimeClass* TypeInfo::create(InterpretContext* context) const
{
	auto cls = RuntimeClass::allocateClass(this->bodySize,
		this->hash, &context->usedVTable->at(this->vTableOffset));

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
		if (t->second.superClass)
		{
			if (t->second.superClass == superClass)
			{
				return true;
			}
			else if (isSubclassOf(t->second.superClass, superClass))
			{
				return true;
			}
		}
		for (auto& i : t->second.interfaces)
		{
			if (i.first == superClass)
			{
				return true;
			}
			else if (isSubclassOf(i.first, superClass))
			{
				return true;
			}
		}
	}

	return false;
}

std::pair<bool, BytecodeOffset> ds::ReflectInfo::tryCast(TypeId toCheck, TypeId superClass) const
{
	auto t = this->types.find(toCheck);

	if (t != this->types.end())
	{
		if (t->second.superClass)
		{
			if (t->second.superClass == superClass)
			{
				return { true, 0 };
			}
			else
			{
				auto [success, offset] = tryCast(t->second.superClass, superClass);
				if (success)
				{
					return { true, offset };
				}
			}
		}
		for (auto& i : t->second.interfaces)
		{
			if (i.first == superClass)
			{
				return { true, i.second };
			}
			else
			{
				auto [success, offset] = tryCast(i.first, superClass);
				if (success)
				{
					return { true, offset };
				}
			}
		}
	}

	return { false, 0 };
}