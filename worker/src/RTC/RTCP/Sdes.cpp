#define MS_CLASS "RTC::RTCP::Sdes"
// #define MS_LOG_DEV_LEVEL 3

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
		absl::flat_hash_map<SdesItem::Type, std::string> SdesItem::type2String =
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
			auto* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

			// data size must be >= header + length value.
			if (len < HeaderSize || len < (1u * 2) + header->length)
			{
				MS_WARN_TAG(rtcp, "not enough space for SDES item, discarded");

				return nullptr;
			}

			if (header->type == SdesItem::Type::END)
				return nullptr;

			return new SdesItem(header);
		}

		const std::string& SdesItem::Type2String(SdesItem::Type type)
		{
			static const std::string Unknown("UNKNOWN");

			auto it = SdesItem::type2String.find(type);

			if (it == SdesItem::type2String.end())
				return Unknown;

			return it->second;
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
			if (len < 4u /* ssrc */)
			{
				MS_WARN_TAG(rtcp, "not enough space for SDES chunk, discarded");

				return nullptr;
			}

			uint32_t ssrc = Utils::Byte::Get4Bytes(data, 0);

			std::unique_ptr<SdesChunk> chunk(new SdesChunk(ssrc));

			size_t offset = 4u /* ssrc */;

			while (len > offset)
			{
				SdesItem* item = SdesItem::Parse(data + offset, len - offset);

				if (!item)
					break;

				chunk->AddItem(item);
				offset += item->GetSize();
			}

			return chunk.release();
		}

		/* Instance methods. */

		size_t SdesChunk::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Copy the SSRC.
			Utils::Byte::Set4Bytes(buffer, 0, this->ssrc);

			size_t offset{ 4u }; // ssrc.

			for (auto* item : this->items)
			{
				offset += item->Serialize(buffer + offset);
			}

			// 32 bits padding.
			size_t padding = (-offset) & 3;

			for (size_t i{ 0 }; i < padding; ++i)
			{
				buffer[offset + i] = 0;
			}

			return offset + padding;
		}

		void SdesChunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SdesChunk>");
			MS_DUMP("  ssrc : %" PRIu32, this->ssrc);
			for (auto* item : this->items)
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
			auto* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));
			std::unique_ptr<SdesPacket> packet(new SdesPacket(header));
			size_t offset = Packet::CommonHeaderSize;
			uint8_t count = header->count;

			while ((count-- != 0u) && (len > offset))
			{
				SdesChunk* chunk = SdesChunk::Parse(data + offset, len - offset);

				if (chunk != nullptr)
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

			for (auto* chunk : this->chunks)
			{
				offset += chunk->Serialize(buffer + offset);
			}

			return offset;
		}

		void SdesPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SdesPacket>");
			for (auto* chunk : this->chunks)
			{
				chunk->Dump();
			}
			MS_DUMP("</SdesPacket>");
		}
	} // namespace RTCP
} // namespace RTC
