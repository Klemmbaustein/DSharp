#pragma once
#include <string>
#include <set>
#include <map>
#include <ds/typeId.hpp>
#include "tokens.hpp"

namespace ds
{
	struct ParsedFile;

	/**
	* @brief
	* A language attribute
	*
	* An attribute that can be applied to modules, functions and types in the programming language.
	* Attributes modify compiler behavior and provide metadata.
	*/
	class Attribute
	{
	public:
		std::string name;
		std::string moduleName;

		std::set<std::string> attributeParameters;

		TypeId getType() const;

		virtual ~Attribute() = default;
	};

	/**
	* @brief
	* Something that can have an attribute
	*
	* This provides functions for describing something that has language attributes.
	*/
	class Attributable
	{
	public:

		struct AttribInfo
		{
			AttribInfo(std::vector<Token> attributeTokens)
			{
				this->attributeTokens = attributeTokens;
			}
			std::vector<Token> attributeTokens;
			std::map<Token, Token> parameters;
			std::vector<std::string> parametersToString() const;

			Attribute* attribute = nullptr;
		};
		void addAttributes(const std::vector<AttribInfo>& newAttributes);

		void resolveAttributes(ParsedFile* file, ErrorContext* errors);

		void clearAttributes();

		template<typename T>
		const AttribInfo* getAttribute() const
		{
			for (auto& i : attributes)
			{
				if (dynamic_cast<T*>(i.attribute))
				{
					return &i;
				}
			}
			return nullptr;
		}

		std::vector<AttribInfo> attributes;
	};
} // namespace ds