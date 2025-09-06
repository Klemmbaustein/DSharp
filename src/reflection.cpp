#include <reflection.hpp>
#include <interpreter.hpp>

using namespace lang;

RuntimeClass* TypeInfo::create(InterpretContext* context) const
{
	auto cls = RuntimeClass::allocateClass(this->bodySize, &context->vTable->at(this->vTableOffset));

	context->pushValue(cls);

	context->run(this->constructor);

	return context->popValue<RuntimeClass*>();
}
