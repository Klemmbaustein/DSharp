#include <ds/native/nativeGeneric.hpp>

ds::GenericData::GenericData(InterpretContext* context)
{
	this->isClassType = context->popValue<Bool>();
	this->typeSize = context->popValue<Size>();
	this->id = context->popValue<TypeId>();
}
