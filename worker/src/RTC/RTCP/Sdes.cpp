#define MS_CLASS "RTC::RTCP::Sdes"
#define MS_LOG_DEV

#include "RTC/RTCP/Sdes.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Item Class variables. */

		// clang-format off
	std::map<SdesItem::Type, std::string> SdesItem::type2String =
	{
		{ SdesItem::Type::END,   "END"   },
		{ SdesItem::Type::CNAME, "CNAME" },
		{ SdesItem::Type::NAME,  "NAME"  },
		{ SdesItem::Type::EMAIL, "EMAIL" },
		{ SdesItem::Type::PHONE, "PHONE" },
		{ SdesItem::Type::LOC,   "LOC"   },
		{ SdesItem::Type::TOOL,  "TOOL"  },
		{ SdesItem::Type::NOTE,  "NOTE"  },
		{ SdesItem::Type::PRIV,  "PRIV"  }
	};
		// clang-format on

		/* Class methods. */

		SdesItem* SdesItem::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

			// data size must be >= header + length value.
			if (sizeof(uint8_t) * 2 + header->length > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for SDES item, discarded");

				return nullptr;
			}

			return new SdesItem(header);
		}

		const std::string& SdesItem::Type2String(SdesItem::Type type)
		{
			static const std::string unknown("UNKNOWN");

			if (type2String.find(type) == type2String.end())
				return unknown;

			return type2String[type];
		}

		/* Instance methods. */

		SdesItem::SdesItem(SdesItem::Type type, size_t len, const char* value)
		{
			MS_TRACE();

			// Allocate memory.
			this->raw.reset(new uint8_t[2 + len]);

			// Update the header pointer.
			this->header = reinterpret_cast<Header*>(this->raw.get());

			this->header->type   = type;
			this->header->length = len;

			// Copy the value into raw.
			std::memcpy(this->header->value, value, this->header->length);
		}

		void SdesItem::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SdesItem>");
			MS_DUMP("  type   : %s", SdesItem::Type2String(this->GetType()).c_str());
			MS_DUMP("  length : %" PRIu8, this->header->length);
			MS_DUMP("  value  : %.*s", this->header->length, this->header->value);
			MS_DUMP("</SdesItem>");
		}

		size_t SdesItem::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Add minimum header.
			std::memcpy(buffer, this->header, 2);

			// Copy the content.
			std::memcpy(buffer + 2, this->header->value, this->header->length);

			return 2 + this->header->length;
		}

		/* Class methods. */

		SdesChunk* SdesChunk::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// data size must be > SSRC field.
			if (sizeof(uint32_t) /* ssrc */ > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for SDES chunk, discarded");

				return nullptr;
			}

			std::unique_ptr<SdesChunk> chunk(new SdesChunk(Utils::Byte::Get4Bytes(data, 0)));

			size_t offset = sizeof(uint32_t) /* ssrc */;

			while (len - offset > 0)
			{
				SdesItem* item = SdesItem::Parse(data + offset, len - offset);
				if (item)
				{
					if (item->GetType() == SdesItem::Type::END)
						return chunk.release();

					chunk->AddItem(item);
					offset += item->GetSize();
				}
				else
				{
					return chunk.release();
				}
			}

			return chunk.release();
		}

		/* Instance methods. */

		size_t SdesChunk::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			std::memcpy(buffer, &this->ssrc, sizeof(this->ssrc));

			size_t offset = sizeof(this->ssrc);

			for (auto item : this->items)
			{
				offset += item->Serialize(buffer + offset);
			}

			// 32 bits padding.
			size_t padding = (-offset) & 3;
			for (size_t i = 0; i < padding; ++i)
			{
				buffer[offset + i] = 0;
			}

			return offset + padding;
		}

		void SdesChunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SdesChunk>");
			MS_DUMP("  ssrc : %" PRIu32, (uint32_t)ntohl(this->ssrc));
			for (auto item : this->items)
			{
				item->Dump();
			}
			MS_DUMP("</SdesChunk>");
		}

		/* Class methods. */

		SdesPacket* SdesPacket::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			Packet::CommonHeader* header = (Packet::CommonHeader*)data;
			std::unique_ptr<SdesPacket> packet(new SdesPacket());
			size_t offset = sizeof(Packet::CommonHeader);
			uint8_t count = header->count;

			while ((count--) && (len - offset > 0))
			{
				SdesChunk* chunk = SdesChunk::Parse(data + offset, len - offset);
				if (chunk)
				{
					packet->AddChunk(chunk);
					offset += chunk->GetSize();
				}
				else
				{
					return packet.release();
				}
			}

			return packet.release();
		}

		/* Instance methods. */

		size_t SdesPacket::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			size_t offset = Packet::Serialize(buffer);

			for (auto chunk : this->chunks)
			{
				offset += chunk->Serialize(buffer + offset);
			}

			return offset;
		}

		void SdesPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SdesPacket>");
			for (auto chunk : this->chunks)
			{
				chunk->Dump();
			}
			MS_DUMP("</SdesPacket>");
		}
	}
}
