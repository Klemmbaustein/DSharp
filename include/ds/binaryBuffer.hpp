#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>

namespace ds
{
	struct BinaryBuffer
	{
		void add(const uint8_t* data, size_t size)
		{
			if (size == 0)
				return;

			size_t writeTo = this->buffer.size();
			this->buffer.resize(writeTo + size);
			memcpy(&this->buffer[writeTo], data, size);
		}

		bool get(uint8_t* data, size_t size)
		{
			if (this->buffer.size() < size + streamPos)
				return false;

			memcpy(data, &this->buffer[streamPos], size);
			streamPos += size;
			return true;
		}

		template <typename T>
		T getValue()
		{
			T Out = *(T*)&this->buffer[streamPos];
			streamPos += sizeof(T);
			return Out;
		}

		template <typename T>
		void addValue(const T& value)
		{
			size_t writeTo = this->buffer.size();
			this->buffer.resize(writeTo + sizeof(value));
			*((T*)&this->buffer[writeTo]) = value;
		}

		void addBuffer(const BinaryBuffer& other)
		{
			add(other.buffer.data(), other.buffer.size());
		}

		bool empty() const
		{
			return streamPos >= buffer.size();
		}

		std::string toString() const;

		std::vector<uint8_t> buffer;
		size_t streamPos = 0;

		void clear()
		{
			buffer.clear();
			streamPos = 0;
		}
	};

	struct BinaryBufferRef
	{
		BinaryBufferRef(BinaryBuffer* buf, size_t streamPos)
		{
			this->buf = buf;
			this->streamPos = streamPos;
		}

		BinaryBufferRef()
		{

		}

		bool get(uint8_t* data, size_t size)
		{
			if (buf->buffer.size() < size + streamPos)
				return false;

			memcpy(data, &buf->buffer[streamPos], size);
			streamPos += size;
			return true;
		}

		template <typename T>
		T getValue()
		{
			T Out = *(T*)&buf->buffer[streamPos];
			streamPos += sizeof(T);
			return Out;
		}

		bool empty() const
		{
			return streamPos >= buf->buffer.size();
		}

		BinaryBuffer* buf = nullptr;
		size_t streamPos = 0;
	};

} // namespace ds