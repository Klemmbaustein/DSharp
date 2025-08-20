#pragma once
#include <string>
#include "tokens.hpp"

namespace lang
{
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
			Attribute* attribute = nullptr;
		};
		void addAttributes(const std::vector<AttribInfo>& newAttributes);

		template<typename T>
		Attribute* getAttribute() const
		{
			for (auto& i : attributes)
			{
				if (dynamic_cast<T*>(i.attribute))
				{
					return i.attribute;
				}
			}
			return nullptr;
		}

		std::vector<AttribInfo> attributes;
	};
} // namespace lang