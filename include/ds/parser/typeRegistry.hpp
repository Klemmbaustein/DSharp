#pragma once
#include <vector>
#include <functional>
#include <map>
#include <ds/typeId.hpp>

namespace ds
{
	class Type;
	class ArrayType;

	class TypeRegistry
	{
	public:

		template <typename T>
		T* getEntry()
		{
			for (auto& i : this->types)
			{
				if (typeid(*i.type) == typeid(T))
				{
					return static_cast<T*>(i.type);
				}
			}

			T* newType;

			if constexpr (requires { new T(this); })
			{
				newType = new T(this);
			}
			else
			{
				newType = new T();
			}

			this->types.push_back(Entry(newType, this));
			return newType;
		}

		ArrayType* getArray(Type* base);

		template <typename T>
		T* getGenericEntry(TypeId type, std::vector<Type*> args, std::function<T*()> create)
		{
			auto found = genericTypes.find(type);

			if (found != genericTypes.end())
			{
				for (auto& t : found->second)
				{
					if (args.size() != t.genericEntries.size())
					{
						continue;
					}
					bool valid = true;

					for (size_t i = 0; i < args.size(); i++)
					{
						if (!compareTypes(t.genericEntries[i], args[i]))
						{
							valid = false;
						}
					}

					if (valid)
					{
						return static_cast<T*>(t.type);
					}
				}
			}

			auto newType = create();
			this->genericTypes[type].push_back(GenericEntry(newType, this, args));
			return newType;
		}

		template <typename T>
		T* getClassGenericEntry(std::vector<Type*> args, std::function<T*()> create)
		{
			for (auto& [_, category] : genericTypes)
			{
				for (auto& t : category)
				{
					if (typeid(*t.type) != typeid(T))
					{
						continue;
					}

					if (args.size() != t.genericEntries.size())
					{
						continue;
					}
					bool valid = true;

					for (size_t i = 0; i < args.size(); i++)
					{
						if (!compareTypes(t.genericEntries[i], args[i]))
						{
							valid = false;
						}
					}

					if (valid)
					{
						return static_cast<T*>(t.type);
					}
				}
			}

			T* newType = create();
			newType->applyName();
			this->genericTypes[newType->id].push_back(GenericEntry(newType, this, args));
			return newType;
		}

		template <typename T>
		bool ifTypeIs(Type* inType)
		{
			return compareTypes(getEntry<T>(), inType);
		}

		template <typename T>
		bool ifTypeIs(Type* inType, T*& outType)
		{
			outType = getEntry<T>();
			return compareTypes(outType, inType);
		}

		std::vector<Type*> getAllTypes();

		~TypeRegistry();

	private:
		bool compareTypes(Type* a, Type* b);

		struct Entry
		{
			Type* type = nullptr;
			TypeRegistry* owner = nullptr;
		};

		struct GenericEntry
		{
			Type* type = nullptr;
			TypeRegistry* owner = nullptr;
			std::vector<Type*> genericEntries;
		};

		std::vector<Entry> types;
		std::map<Type*, Entry> arrayTypes;

		std::map<TypeId, std::vector<GenericEntry>> genericTypes;
	};
} // namespace ds