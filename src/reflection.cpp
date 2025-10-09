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
