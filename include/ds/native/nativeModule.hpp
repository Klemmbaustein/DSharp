#pragma once
#include <ds/interpreter.hpp>
#include <ds/parser/function.hpp>
#include <ds/parser/module.hpp>
#include <ds/parser/types/classType.hpp>
#include "externalFunction.hpp"
#include "genericClassType.hpp"

namespace ds
{
	struct LanguageContext;

	struct NativeFunction : public Function
	{
		NativeFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			std::string name, ExternalFunctionPointer function)
		{
			this->arguments = arguments;
			this->name = name;
			this->function = function;
			this->returnType = returnType;
		}
		NativeFunction(std::vector<FunctionArgument> arguments, Type* returnType,
			const char* name, ExternalFunctionPointer function)
		{
			this->arguments = arguments;
			this->name = name;
			this->function = function;
			this->returnType = returnType;
		}

		NativeFunction(const NativeFunction&) = default;

		std::vector<FunctionArgument> arguments;
		std::string name;
		ExternalFunctionPointer function = nullptr;
		Type* returnType = nullptr;
		std::string moduleName;
		std::string className;
		ClassType* virtualType = nullptr;
		bytecodeOffset virtualId = 0;
		bool isDiscardable = false;

		ExpressionResult compileCall() override;
		std::vector<FunctionArgument> getArguments() override;
		Type* getReturnType() override;
		std::string getShortName() const override;
		std::string getFullName() const override;
		bool discardable() const override;

		virtual NativeFunction* create() const
		{
			return new NativeFunction(*this);
		}

		bool isVirtual() const override
		{
			return virtualId != 0;
		}

		virtual bytecodeOffset getVirtualOffset() const
		{
			return virtualId;
		}

		BytecodeBuffer compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const override;
	};

	struct NativeModule
	{
		std::string name;

		NativeFunction* addFunction(const NativeFunction& function);

		template <typename T>
		static RuntimeClass* makePointerClass(T* value)
		{
			ClassRef<T*> newObject = RuntimeClass::allocateClass(sizeof(T*), 0, nullptr);
			newObject.getValue() = value;
			return newObject.classPtr;
		}

		template <typename T>
		static RuntimeClass* makeClass(const T& value)
		{
			ClassRef<T> newObject = RuntimeClass::allocateClass(sizeof(T), 0, nullptr);
			newObject.getValue() = value;
			return newObject.classPtr;
		}

		Type* getType(const std::string& name);

		template <typename TNative>
		ClassType* createClass(std::string name, ClassType* derived = nullptr)
		{
			auto cls = new ClassType();
			cls->name = name;
			cls->classSize = sizeof(TNative);
			cls->vTableOffset = UINT32_MAX;

			if (derived)
			{
				cls->parents = { derived };

				for (auto& i : derived->members)
				{
					cls->members.push_back(i);
				}

				for (auto& i : derived->methods)
				{
					cls->methods.insert(i);
				}
			}

			this->types.push_back(cls);
			return cls;
		}

		template <typename TNative>
		NativeGenericClassType* createGenericClass(std::string name, std::vector<GenericArgument> args, ClassType* derived = nullptr)
		{
			auto cls = new NativeGenericClassType(args);
			cls->name = name;
			cls->classSize = sizeof(TNative);
			cls->vTableOffset = UINT32_MAX;

			if (derived)
			{
				cls->parents = { derived };

				for (auto& i : derived->members)
				{
					cls->members.push_back(i);
				}

				for (auto& i : derived->methods)
				{
					cls->methods.insert(i);
				}
			}

			this->types.push_back(cls);
			return cls;
		}

		EnumType* createEnum(std::string name);

		void addEnumIntValue(EnumType* type, std::string name, int value);

		template <typename T>
		void addEnumValue(EnumType* type, std::string name, T value)
		{
			addEnumIntValue(type, name, int(value));
		}

		ds::TypeId addAttribute(Attribute* attrib);

		void addClassConstructor(ClassType* type, const NativeFunction& function);
		void addClassMethod(ClassType* type, const NativeFunction& function);
		void addStructMethod(ClassType* type, const NativeFunction& function);
		void addClassVirtualMethod(ClassType* type, const NativeFunction& function, bytecodeOffset virtualId);

		void addType(Type* newType)
		{
			this->types.push_back(newType);
		}

		void initialize();

		Module create() const;

		const std::vector<NativeFunction*>& getFunctions() const
		{
			return this->functions;
		}

	private:
		std::vector<NativeFunction*> functions;
		std::vector<Attribute*> attributes;
		std::vector<Type*> types;
		std::vector<EnumType*> enums;
	};
} // namespace ds