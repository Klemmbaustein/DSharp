#include <ds/parser/types/taskType.hpp>
#include <ds/parser/types/builtinClassFunction.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/modules/system.async.hpp>

using namespace ds;

ds::TaskType::TaskType(Type* baseType)
{
	this->name = baseType ? ("task<" + Type::toString(baseType) + ">") : "task";
	this->baseType = baseType;
	this->size = sizeof(Pointer);
	this->vTableOffset = UINT32_MAX;
	this->baseType = baseType;

	this->members.push_back(ClassMember{
		.name = "completed",
		.offset = offsetof(modules::system::async::Task, completed),
		.type = BoolType::getInstance(),
		});
}

ExpressionResult ds::TaskType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::TaskType::compileAwait(ExpressionResult taskExpr, ExpressionResult returnTaskExpr,
	ParsedScope* with)
{
	returnTaskExpr.code.addBuffer(taskExpr.code);
	auto endLabel = std::make_shared<BytecodeJumpLabel>("endAwait");

	returnTaskExpr.code.addNew<BytecodeAwait>(Size(baseType ? baseType->size : 0), endLabel.get());
	returnTaskExpr.code.addBuffer(with->compileScopeExit(with->functionDepth, false, false));
	returnTaskExpr.code.addOperation(BytecodeOp::returnAsync);
	returnTaskExpr.code.add(endLabel);

	returnTaskExpr.type = baseType;
	return returnTaskExpr;
}

ExpressionResult ds::TaskType::compileTask()
{
	ExpressionResult result;
	result.valid = true;
	result.type = this;
	result.code.addNew<BytecodeCallNative>("system::async::task.newEmpty");
	return result;
}

ExpressionResult ds::TaskType::compileCompleteTask()
{
	ExpressionResult result;
	result.valid = true;
	result.type = this;
	result.code.pushInt(this->baseType ? this->baseType->size : 0);
	result.code.addNew<BytecodeCallNative>("system::async::task.complete");
	return result;
}
