#define MS_CLASS "RTC::RTCP::SdesPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/packet/SdesPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove()
#include <limits>  // std::numeric_limits

namespace RTC
{
	namespace RTCP
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
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::SDES)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return SdesPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		SdesPacket* SdesPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Packet::CommonHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new SdesPacket(buffer, bufferLength);

			packet->InitializeHeader(Packet::PacketType::SDES, 0, Packet::CommonHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum Packet length.

			return packet;
		}

		size_t SdesPacket::ComputeLengthFor(size_t chunksLength)
		{
			MS_TRACE();

			// The chunks go right after the common header, and each one is already
			// padded to 4 bytes.
			return Packet::CommonHeaderLength + chunksLength;
		}

		size_t SdesPacket::ComputeChunkLengthFor(std::initializer_list<size_t> itemValueLengths)
		{
			MS_TRACE();

			// The SSRC/CSRC (4 bytes).
			size_t length{ 4 };

			// Each item takes its type and length (1 byte each) and its value.
			for (const size_t itemValueLength : itemValueLengths)
			{
				length += 2 + itemValueLength;
			}

			// The null octet that terminates the list of items, and the padding up to
			// the next 32-bit boundary.
			return Utils::Byte::PadTo4Bytes(length + 1);
		}

		SdesPacket* SdesPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;
			auto* const packet         = new SdesPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			const uint8_t numberOfChunks = packet->GetCount();

			size_t offset{ Packet::CommonHeaderLength };

			for (uint8_t chunkIdx{ 0 }; chunkIdx < numberOfChunks; ++chunkIdx)
			{
				// SSRC/CSRC of the chunk.
				if (offset + 4 > contentLength)
				{
					MS_WARN_TAG(rtcp, "no space for SSRC/CSRC of chunk %" PRIu8, chunkIdx);

					delete packet;
					return nullptr;
				}

				offset += 4;

				// Items until the null octet that terminates the list.
				while (true)
				{
					if (offset >= contentLength)
					{
						MS_WARN_TAG(rtcp, "missing null octet terminating the items of chunk %" PRIu8, chunkIdx);

						delete packet;
						return nullptr;
					}

					if (Utils::Byte::Get1Byte(buffer, offset) == 0)
					{
						break;
					}

					if (offset + 2 > contentLength)
					{
						MS_WARN_TAG(rtcp, "no space for item header in chunk %" PRIu8, chunkIdx);

						delete packet;
						return nullptr;
					}

					const uint8_t itemLength = Utils::Byte::Get1Byte(buffer, offset + 1);

					if (offset + 2 + itemLength > contentLength)
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

				if (chunkEndOffset > contentLength)
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

			// The packet (excluding padding) must end right after the last chunk.
			if (offset != contentLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "packet length (%zu bytes, excluding padding) does not end right after the last chunk (%zu bytes)",
				  contentLength,
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

			auto* const clonedPacket = new SdesPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		std::vector<SdesPacket::Item> SdesPacket::GetItems() const
		{
			MS_TRACE();

			const auto* const buffer = GetBuffer();

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

			const size_t previousValueLength = GetVariableLengthValueLength();

			// NOTE: This may throw.
			SetVariableLengthValueLength(previousValueLength + 8);

			// Write the SSRC/CSRC.
			Utils::Byte::Set4Bytes(GetVariableLengthValuePointer(), previousValueLength, ssrc);

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
			auto* const itemPointer     = GetLastChunkItemsEndPointer();
			const size_t itemOffset     = itemPointer - GetBuffer();
			const size_t itemsEndOffset = itemOffset + 2 + value.size();

			// Room for the null octet terminating the list of items and the padding.
			const size_t newLength = Utils::Byte::PadTo4Bytes(itemsEndOffset + 1);

			// NOTE: This may throw.
			SetVariableLengthValueLength(newLength - Packet::CommonHeaderLength);

			// Write the item.
			Utils::Byte::Set1Byte(itemPointer, 0, static_cast<uint8_t>(type));
			Utils::Byte::Set1Byte(itemPointer, 1, static_cast<uint8_t>(value.size()));
			std::memmove(itemPointer + 2, value.data(), value.size());

			// Write the null octet terminating the list of items and the padding.
			FillPadding(newLength - itemsEndOffset);
		}

		SdesPacket* SdesPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket = new SdesPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}

		uint8_t* SdesPacket::GetLastChunkItemsEndPointer() const
		{
			MS_TRACE();

			const uint8_t* chunkPointer = GetVariableLengthValuePointer();
			const uint8_t* itemsEndPointer{ nullptr };

			for (uint8_t chunkIdx{ 0 }; chunkIdx < GetCount(); ++chunkIdx)
			{
				// Skip the SSRC/CSRC.
				const uint8_t* ptr = chunkPointer + 4;

				while (Utils::Byte::Get1Byte(ptr, 0) != 0)
				{
					ptr += 2 + Utils::Byte::Get1Byte(ptr, 1);
				}

				itemsEndPointer = ptr;

				// The null octet and the padding until the next 32-bit boundary.
				chunkPointer += Utils::Byte::PadTo4Bytes(static_cast<size_t>(ptr - chunkPointer) + 1);
			}

			return const_cast<uint8_t*>(itemsEndPointer);
		}
	} // namespace RTCP
} // namespace RTC
