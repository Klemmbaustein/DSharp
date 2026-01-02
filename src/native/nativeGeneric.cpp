#include <ds/native/nativeGeneric.hpp>

ds::GenericData::GenericData(InterpretContext* context)
{
	this->typeSize = context->popValue<Size>();
	this->id = context->popValue<TypeId>();
}
