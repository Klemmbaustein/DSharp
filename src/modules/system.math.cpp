#include <ds/modules/system.math.hpp>
#include <ds/language.hpp>
#include <cmath>

using namespace ds;

static void math_sin(InterpretContext* context)
{
	context->pushValue<Float>(std::sin(context->popValue<Float>()));
}

static void math_cos(InterpretContext* context)
{
	context->pushValue<Float>(std::cos(context->popValue<Float>()));
}

static void math_tan(InterpretContext* context)
{
	context->pushValue<Float>(std::tan(context->popValue<Float>()));
}

template<typename T>
static void math_pow(InterpretContext* context)
{
	auto y = context->popValue<T>();
	auto x = context->popValue<T>();

	context->pushValue<T>(std::pow(x, y));
}

static void math_sqrt(InterpretContext* context)
{
	context->pushValue<Float>(std::sqrt(context->popValue<Float>()));
}

static void math_floor(InterpretContext* context)
{
	context->pushValue<Float>(std::floor(context->popValue<Float>()));
}

static void math_ceil(InterpretContext* context)
{
	context->pushValue<Float>(std::ceil(context->popValue<Float>()));
}

static void math_round(InterpretContext* context)
{
	context->pushValue<Float>(std::round(context->popValue<Float>()));
}

template <typename T>
static void math_clamp(InterpretContext* context)
{
	T max = context->popValue<T>();
	T min = context->popValue<T>();
	T val = context->popValue<T>();

	context->pushValue<T>(val < min ? min : (val > max ? max : val));
}

template <typename T>
static void math_abs(InterpretContext* context)
{
	T value = context->popValue<T>();

	context->pushValue<T>(std::abs(value));
}

template <typename T>
static void math_min(InterpretContext* context)
{
	T min = context->popValue<T>();
	T val = context->popValue<T>();

	context->pushValue<T>(val < min ? min : val);
}

template <typename T>
static void math_max(InterpretContext* context)
{
	T max = context->popValue<T>();
	T val = context->popValue<T>();

	context->pushValue<T>(val > max ? max : val);
}

NativeModule ds::modules::system::math::createModule(LanguageContext* to)
{
	NativeModule out;
	out.name = "system::math";

	auto floatInst = to->registry->getEntry<FloatType>();
	auto intInst = to->registry->getEntry<IntType>();

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "sin", &math_sin));

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "cos", &math_cos));

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "tan", &math_tan));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(floatInst, "x"), ds::FunctionArgument(floatInst, "y") },
		floatInst, "powF", &math_pow<Float>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(intInst, "x"), ds::FunctionArgument(intInst, "y") },
		intInst, "powI", &math_pow<Int>));

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "sqrt", &math_sqrt));

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "floor", &math_floor));

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "ceil", &math_ceil));

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "round", &math_round));

	out.addFunction(NativeFunction({ ds::FunctionArgument(floatInst, "value") },
		floatInst, "absF", &math_abs<Float>));

	out.addFunction(NativeFunction({ ds::FunctionArgument(intInst, "value") },
		floatInst, "absI", &math_abs<Int>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(floatInst, "value"),
			ds::FunctionArgument(floatInst, "min"),
			ds::FunctionArgument(floatInst, "max") },
		floatInst, "clampF", &math_clamp<Float>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(intInst, "value"),
			ds::FunctionArgument(intInst, "min"),
			ds::FunctionArgument(intInst, "max") },
		intInst, "clampI", &math_clamp<Int>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(floatInst, "value"),
			ds::FunctionArgument(floatInst, "min"),
			ds::FunctionArgument(floatInst, "max") },
		floatInst, "clampF", &math_clamp<Float>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(intInst, "value"),
			ds::FunctionArgument(intInst, "min"),
			ds::FunctionArgument(intInst, "max") },
		intInst, "clampI", &math_clamp<Int>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(floatInst, "value"),
			ds::FunctionArgument(floatInst, "min") },
		floatInst, "minF", &math_min<Float>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(intInst, "value"),
			ds::FunctionArgument(intInst, "min") },
		intInst, "minI", &math_min<Int>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(floatInst, "value"),
			ds::FunctionArgument(floatInst, "max") },
		floatInst, "maxF", &math_max<Float>));

	out.addFunction(NativeFunction(
		{ ds::FunctionArgument(intInst, "value"),
			ds::FunctionArgument(intInst, "max") },
		intInst, "maxI", &math_max<Int>));

	return out;
}
