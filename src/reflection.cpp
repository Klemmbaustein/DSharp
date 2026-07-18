#include <ds/reflection.hpp>
#include <ds/interpreter.hpp>

using namespace ds;

RuntimeClass* TypeInfo::create(InterpretContext* context) const
{
	auto cls = RuntimeClass::allocateClass(this->bodySize,
		this->hash, &context->usedVTable->at(this->vTableOffset));

	context->pushValue(cls);

	auto stack = context->stackPos;
	context->run(this->constructor);

	if (stack == context->stackPos && context->stackPos != 0)
	{
		return context->popValue<RuntimeClass*>();
	}
	return nullptr;
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

std::pair<bool, Int> ds::ReflectInfo::tryCast(TypeId toCheck, TypeId superClass) const
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
	auto t2 = this->types.find(superClass);
	if (t2 != this->types.end())
	{
		for (auto& i : t2->second.interfaces)
		{
			if (i.first == toCheck)
			{
				return { true, -i.second - Int(sizeof(RuntimeClass)) * 2 };
			}
			else
			{
				auto [success, offset] = tryCast(i.first, toCheck);
				if (success)
				{
					return { true, -offset - Int(sizeof(RuntimeClass)) * 2 };
				}
			}
		}
	}

	return { false, 0 };
}