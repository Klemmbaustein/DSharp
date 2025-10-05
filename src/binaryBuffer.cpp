#include <ds/binaryBuffer.hpp>
#include <format>

std::string ds::BinaryBuffer::toString() const
{
	std::string out;

	for (auto i = this->buffer.rbegin(); i < this->buffer.rend(); i++)
	{
#ifdef HAS_CPP_FORMAT
		out.append(std::format("{:02x}", *i));
#endif
	}
	return out;
}
