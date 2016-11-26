#define MS_CLASS "RTC::RTCP::Sdes"

#include "RTC/RTCP/Sdes.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC { namespace RTCP
{

	/* Sdes Item Class variables. */

	std::map<SdesItem::Type, std::string> SdesItem::type2String =
	{
		{ END,   "END"   },
		{ CNAME, "CNAME" },
		{ NAME,  "NAME"  },
		{ EMAIL, "EMAIL" },
		{ PHONE, "PHONE" },
		{ LOC,   "LOC"   },
		{ TOOL,  "TOOL"  },
		{ NOTE,  "NOTE"  },
		{ PRIV,  "PRIV"  },
	};

	/* SDES Item Class methods. */

	SdesItem* SdesItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

		// data size must be >= header + length value.
		if (sizeof(uint8_t)*2 + header->length > len)
		{
				MS_WARN("not enough space for SDES item, discarded");

				return nullptr;
		}

		return new SdesItem(header);
	}

	const std::string& SdesItem::Type2String(Type type)
	{
		static const std::string unknown("UNKNOWN");

		if (type2String.find(type) == type2String.end())
			return unknown;

		return type2String[type];
	}

	/* SDES Item Instance methods. */

	SdesItem::SdesItem(Type type, size_t len, const char* value)
	{
		MS_TRACE_STD();

		// Allocate memory.
		this->raw = new uint8_t[2 + len];

		// Update the header pointer.
		this->header = (Header*)(this->raw);

		this->header->type = type;
		this->header->length = len;

		// Copy the value into raw
		std::memcpy(this->header->value, value, this->header->length);
	}

	void SdesItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Sdes Item>");
		MS_WARN("\t\t\ttype: %s", Type2String(this->GetType()).c_str());
		MS_WARN("\t\t\tlength: %u", this->header->length);
		MS_WARN("\t\t\tvalue: %.*s", this->header->length, this->header->value);
		MS_WARN("\t\t</Sdes Item>");
	}

	void SdesItem::Serialize()
	{
		MS_TRACE_STD();

		if (this->raw)
			delete this->raw;

		this->raw = new uint8_t[2 + this->header->length];

		this->Serialize(this->raw);
	}

	size_t SdesItem::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		// Add minimum header.
		std::memcpy(data, this->header, 2);

		// Copy the content.
		std::memcpy(data+2, this->header->value, this->header->length);

		return 2+this->header->length;
	}


	/* SDES Chunk Class methods. */

	SdesChunk* SdesChunk::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// data size must be > SSRC field.
		if (sizeof(uint32_t) /* ssrc */ > len)
		{
			MS_WARN("not enough space for SDES chunk, discarded");

			return nullptr;
		}

		std::auto_ptr<SdesChunk> chunk(new SdesChunk(Utils::Byte::Get4Bytes(data, 0)));

		size_t offset = sizeof(uint32_t) /* ssrc */;

		while (len - offset > 0)
		{
			SdesItem* item = SdesItem::Parse(data+offset, len-offset);
			if (item) {

				if (item->GetType() == SdesItem::END) {
					return chunk.release();
				}

				chunk->AddItem(item);
				offset += item->GetSize();
			} else {
				return chunk.release();
			}
		}

		return chunk.release();
	}

	/* SDES Chunk Instance methods. */

	void SdesChunk::Serialize()
	{
		MS_TRACE_STD();

		for (auto item : this->items) {
			item->Serialize();
		}
	}

	size_t SdesChunk::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		std::memcpy(data, &this->ssrc, sizeof(this->ssrc));

		size_t offset = sizeof(this->ssrc);

		for (auto item : this->items) {
			offset += item->Serialize(data + offset);
		}

		// 32 bits padding
		size_t padding = (-offset) & 3;
		for (size_t i = 0; i < padding; i++)
		{
			data[offset+i] = 0;
		}

		return offset+padding;
	}

	void SdesChunk::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<Sdes Chunk>");
		MS_WARN("\t\tssrc: %u", (uint32_t)ntohl(this->ssrc));

		for (auto item : this->items) {
			item->Dump();
		}

		MS_WARN("\t</Sdes Chunk>");
	}

	/* SDES Packet Class methods. */

	SdesPacket* SdesPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Packet::CommonHeader* header = (Packet::CommonHeader*)data;

		std::auto_ptr<SdesPacket> packet(new SdesPacket());

		size_t offset = sizeof(Packet::CommonHeader);

		uint8_t count = header->count;
		while ((count--) && (len - offset > 0))
		{
			SdesChunk* chunk = SdesChunk::Parse(data+offset, len-offset);
			if (chunk) {
				packet->AddChunk(chunk);
				offset += chunk->GetSize();
			} else {
				return packet.release();
			}
		}

		return packet.release();
	}

	/* SDES Packet Instance methods. */

	size_t SdesPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = Packet::Serialize(data);

		for(auto chunk : this->chunks) {
			offset += chunk->Serialize(data + offset);
		}

		return offset;
	}

	void SdesPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("<Sdes Packet>");

		for (auto chunk : this->chunks) {
			chunk->Dump();
		}

		MS_WARN("</Sdes Packet>");
	}

} } // RTP::RTCP
