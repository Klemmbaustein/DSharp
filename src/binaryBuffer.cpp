#include "binaryBuffer.hpp"
#include <format>

std::string lang::BinaryBuffer::toString() const
{
	std::string out;

	for (auto i = this->buffer.rbegin(); i < this->buffer.rend(); i++)
	{
		out.append(std::format("{:02x}", *i));
	}
	return out;
}
