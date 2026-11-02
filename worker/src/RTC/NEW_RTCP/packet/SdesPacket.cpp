#define MS_CLASS "RTC::NEW_RTCP::SdesPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/SdesPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove()
#include <limits>  // std::numeric_limits

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class variables. */

		// clang-format off
		const ankerl::unordered_dense::map<SdesPacket::ItemType, std::string> SdesPacket::ItemType2String =
		{
			{ SdesPacket::ItemType::CNAME, "CNAME" },
			{ SdesPacket::ItemType::NAME,  "NAME"  },
			{ SdesPacket::ItemType::EMAIL, "EMAIL" },
			{ SdesPacket::ItemType::PHONE, "PHONE" },
			{ SdesPacket::ItemType::LOC,   "LOC"   },
			{ SdesPacket::ItemType::TOOL,  "TOOL"  },
			{ SdesPacket::ItemType::NOTE,  "NOTE"  },
			{ SdesPacket::ItemType::PRIV,  "PRIV"  },
		};
		// clang-format on

		/* Class methods. */

		const std::string& SdesPacket::ItemTypeToString(ItemType itemType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			const auto it = SdesPacket::ItemType2String.find(itemType);

			if (it == SdesPacket::ItemType2String.end())
			{
				return Unknown;
			}

			const std::string& itemTypeString = it->second;

			return itemTypeString;
		}

		SdesPacket* SdesPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::SDES)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return SdesPacket::ParseStrict(buffer, bufferLength, packetLength);
		}

		SdesPacket* SdesPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Packet::CommonHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* packet = new SdesPacket(buffer, bufferLength);

			packet->InitializeHeader(Packet::PacketType::SDES, Packet::CommonHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum Packet length.

			return packet;
		}

		SdesPacket* SdesPacket::ParseStrict(const uint8_t* buffer, size_t bufferLength, size_t packetLength)
		{
			MS_TRACE();

			auto* packet = new SdesPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			const uint8_t numberOfChunks = packet->GetCount();

			size_t offset{ Packet::CommonHeaderLength };

			for (uint8_t chunkIdx{ 0 }; chunkIdx < numberOfChunks; ++chunkIdx)
			{
				// SSRC/CSRC of the chunk.
				if (offset + 4 > packetLength)
				{
					MS_WARN_TAG(rtcp, "no space for SSRC/CSRC of chunk %" PRIu8, chunkIdx);

					delete packet;
					return nullptr;
				}

				offset += 4;

				// Items until the null octet that terminates the list.
				while (true)
				{
					if (offset >= packetLength)
					{
						MS_WARN_TAG(rtcp, "missing null octet terminating the items of chunk %" PRIu8, chunkIdx);

						delete packet;
						return nullptr;
					}

					if (Utils::Byte::Get1Byte(buffer, offset) == 0)
					{
						break;
					}

					if (offset + 2 > packetLength)
					{
						MS_WARN_TAG(rtcp, "no space for item header in chunk %" PRIu8, chunkIdx);

						delete packet;
						return nullptr;
					}

					const uint8_t itemLength = Utils::Byte::Get1Byte(buffer, offset + 1);

					if (offset + 2 + itemLength > packetLength)
					{
						MS_WARN_TAG(
						  rtcp,
						  "item length (%" PRIu8 " bytes) exceeds the packet length in chunk %" PRIu8,
						  itemLength,
						  chunkIdx);

						delete packet;
						return nullptr;
					}

					offset += 2 + itemLength;
				}

				// The null octet and the padding until the next 32-bit boundary.
				const size_t chunkEndOffset = Utils::Byte::PadTo4Bytes(offset + 1);

				if (chunkEndOffset > packetLength)
				{
					MS_WARN_TAG(rtcp, "no space for padding of chunk %" PRIu8, chunkIdx);

					delete packet;
					return nullptr;
				}

				for (size_t idx{ offset }; idx < chunkEndOffset; ++idx)
				{
					if (Utils::Byte::Get1Byte(buffer, idx) != 0)
					{
						MS_WARN_TAG(rtcp, "non null octet in padding of chunk %" PRIu8, chunkIdx);

						delete packet;
						return nullptr;
					}
				}

				offset = chunkEndOffset;
			}

			// The packet must end right after the last chunk.
			if (offset != packetLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "packet length (%zu bytes) does not end right after the last chunk (%zu bytes)",
				  packetLength,
				  offset);

				delete packet;
				return nullptr;
			}

			return packet;
		}

		/* Instance methods. */

		SdesPacket::SdesPacket(uint8_t* buffer, size_t bufferLength) : Packet(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::CommonHeaderLength);
		}

		SdesPacket::~SdesPacket()
		{
			MS_TRACE();
		}

		void SdesPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::SdesPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  chunks: %" PRIu8, GetNumberOfChunks());
			MS_DUMP_CLEAN(indentation, "  items:");
			for (const auto& item : GetItems())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - ssrc: %" PRIu32 ", type: %" PRIu8 " (%s), value: \"%.*s\"",
				  item.ssrc,
				  static_cast<uint8_t>(item.type),
				  SdesPacket::ItemTypeToString(item.type).c_str(),
				  static_cast<int>(item.value.size()),
				  item.value.data());
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::SdesPacket>");
		}

		SdesPacket* SdesPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new SdesPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		std::vector<SdesPacket::Item> SdesPacket::GetItems() const
		{
			MS_TRACE();

			const auto* buffer = GetBuffer();

			std::vector<Item> items;
			size_t offset{ Packet::CommonHeaderLength };

			for (uint8_t chunkIdx{ 0 }; chunkIdx < GetCount(); ++chunkIdx)
			{
				const uint32_t ssrc = Utils::Byte::Get4Bytes(buffer, offset);

				offset += 4;

				while (Utils::Byte::Get1Byte(buffer, offset) != 0)
				{
					const auto type          = static_cast<ItemType>(Utils::Byte::Get1Byte(buffer, offset));
					const uint8_t itemLength = Utils::Byte::Get1Byte(buffer, offset + 1);

					items.push_back(
					  { .ssrc = ssrc,
						  .type = type,
						  .value =
						    std::string_view(reinterpret_cast<const char*>(buffer + offset + 2), itemLength) });

					offset += 2 + itemLength;
				}

				offset = Utils::Byte::PadTo4Bytes(offset + 1);
			}

			return items;
		}

		void SdesPacket::AddChunk(uint32_t ssrc)
		{
			MS_TRACE();

			if (GetCount() == SdesPacket::MaxChunks)
			{
				MS_THROW_TYPE_ERROR("cannot add more than %zu chunks", SdesPacket::MaxChunks);
			}

			const size_t previousLength = GetLength();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 8);

			// Write the SSRC/CSRC.
			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), previousLength, ssrc);

			// Write the null octet terminating the empty list of items and the
			// padding.
			FillPadding(4);

			// Update the counter field.
			SetCount(GetCount() + 1);
		}

		void SdesPacket::AddItem(ItemType type, std::string_view value)
		{
			MS_TRACE();

			if (GetCount() == 0)
			{
				MS_THROW_TYPE_ERROR("there is no chunk to add the item to");
			}

			// An item type of 0 terminates the list of items.
			if (static_cast<uint8_t>(type) == 0)
			{
				MS_THROW_TYPE_ERROR("item type cannot be 0");
			}

			if (value.size() > std::numeric_limits<uint8_t>::max())
			{
				MS_THROW_TYPE_ERROR("value length (%zu bytes) cannot be greater than 255", value.size());
			}

			// The new item replaces the null octet and the padding of the last chunk.
			const size_t itemOffset     = GetLastChunkItemsEndOffset();
			const size_t itemsEndOffset = itemOffset + 2 + value.size();

			// Room for the null octet terminating the list of items and the padding.
			const size_t newLength = Utils::Byte::PadTo4Bytes(itemsEndOffset + 1);

			// NOTE: This may throw.
			SetVariableLengthValueLength(newLength - Packet::CommonHeaderLength);

			auto* buffer = const_cast<uint8_t*>(GetBuffer());

			// Write the item.
			Utils::Byte::Set1Byte(buffer, itemOffset, static_cast<uint8_t>(type));
			Utils::Byte::Set1Byte(buffer, itemOffset + 1, static_cast<uint8_t>(value.size()));
			std::memmove(buffer + itemOffset + 2, value.data(), value.size());

			// Write the null octet terminating the list of items and the padding.
			FillPadding(newLength - itemsEndOffset);
		}

		SdesPacket* SdesPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedPacket = new SdesPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}

		size_t SdesPacket::GetLastChunkItemsEndOffset() const
		{
			MS_TRACE();

			const auto* buffer = GetBuffer();

			size_t offset{ Packet::CommonHeaderLength };
			size_t itemsEndOffset{ 0 };

			for (uint8_t chunkIdx{ 0 }; chunkIdx < GetCount(); ++chunkIdx)
			{
				// Skip the SSRC/CSRC.
				offset += 4;

				while (Utils::Byte::Get1Byte(buffer, offset) != 0)
				{
					offset += 2 + Utils::Byte::Get1Byte(buffer, offset + 1);
				}

				itemsEndOffset = offset;
				offset         = Utils::Byte::PadTo4Bytes(offset + 1);
			}

			return itemsEndOffset;
		}
	} // namespace NEW_RTCP
} // namespace RTC
