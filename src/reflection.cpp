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

std::optional<std::string> ds::AttributeData::getParameterValue(const std::string& name) const
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

CastResult ds::ReflectInfo::tryCast(TypeId toCheck, TypeId superClass) const
{
	auto t = this->types.find(toCheck);

	if (t != this->types.end())
	{
		if (t->second.superClass)
		{
			if (t->second.superClass == superClass)
			{
				return { true, false, 0 };
			}
			else
			{
				auto result = tryCast(t->second.superClass, superClass);
				if (result.success)
				{
					return result;
				}
			}
		}
		for (auto& i : t->second.interfaces)
		{
			if (i.first == superClass)
			{
				return { true, true, Int(i.second) };
			}
			else
			{
				auto result = tryCast(i.first, superClass);
				if (result.success)
				{
					return result;
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
				return { true, true, Int(-i.second - Int(sizeof(RuntimeClass)) * 2) };
			}
			else
			{
				auto result = tryCast(i.first, toCheck);
				if (result.success)
				{
					return { true, true, Int(-i.second - Int(sizeof(RuntimeClass)) * 2) };
				}
			}
		}
	}

	return { false, 0 };
}