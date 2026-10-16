#define MS_CLASS "RTC::RTP::Codecs::RED"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/RED.hpp"
#include "Logger.hpp"
#include "RTC/RTP/Codecs/Opus.hpp"
#include <array>
#include <cstring> // std::memcpy(), std::memmove()
#include <utility> // std::pair

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Static. */

			static constexpr size_t NonFinalBlockHeaderLength{ 4 };
			static constexpr size_t FinalBlockHeaderLength{ 1 };
			// Maximum number of blocks (redundant ones plus the primary one).
			static constexpr size_t MaxBlocks{ 32 };

			/* Static helpers. */

			/**
			 * Iterate the block headers of the given RED payload calling the given
			 * function with the offset of each block header (being the last one the
			 * final block header).
			 *
			 * @return The offset at which the data of the primary block begins or
			 *   std::nullopt if the RED payload is invalid.
			 */
			template<typename F>
			static std::optional<size_t> iterateBlockHeaders(const uint8_t* data, size_t len, const F& fn)
			{
				size_t offset{ 0 };
				size_t blocksLength{ 0 };

				while (offset < len)
				{
					// F bit set, so this is a non final block header.
					if (data[offset] & 0x80)
					{
						if (offset + NonFinalBlockHeaderLength > len)
						{
							MS_WARN_DEV("not enough space for non final block header");

							return std::nullopt;
						}

						const size_t blockLength = ((data[offset + 2] & 0x03) << 8) | data[offset + 3];

						fn(offset);

						blocksLength += blockLength;
						offset += NonFinalBlockHeaderLength;
					}
					// Final block header.
					else
					{
						fn(offset);

						offset += FinalBlockHeaderLength;

						if (offset + blocksLength > len)
						{
							MS_WARN_DEV("not enough space for redundant blocks data");

							return std::nullopt;
						}

						return offset + blocksLength;
					}
				}

				MS_WARN_DEV("final block header not found");

				return std::nullopt;
			}

			/* Class methods. */

			bool RED::GetPrimaryBlock(const uint8_t* data, size_t len, uint8_t& payloadType, size_t& offset)
			{
				MS_TRACE();

				size_t lastBlockHeaderOffset{ 0 };

				auto primaryBlockOffset = iterateBlockHeaders(
				  data,
				  len,
				  [&lastBlockHeaderOffset](size_t offset)
				  {
					  lastBlockHeaderOffset = offset;
				  });

				if (!primaryBlockOffset.has_value())
				{
					return false;
				}

				payloadType = data[lastBlockHeaderOffset] & 0x7F;
				offset      = primaryBlockOffset.value();

				return true;
			}

			bool RED::MapPayloadTypes(
			  uint8_t* data, size_t len, const ankerl::unordered_dense::map<uint8_t, uint8_t>& mapping)
			{
				MS_TRACE();

				// Offset and mapped payload type of each block header, so the payload is
				// just modified once it has been entirely validated.
				std::array<std::pair<size_t, uint8_t>, MaxBlocks> mappedBlocks;
				size_t numBlocks{ 0 };
				bool valid{ true };

				auto primaryBlockOffset = iterateBlockHeaders(
				  data,
				  len,
				  [data, &mapping, &mappedBlocks, &numBlocks, &valid](size_t offset)
				  {
					  if (!valid)
					  {
						  return;
					  }

					  if (numBlocks == MaxBlocks)
					  {
						  MS_WARN_DEV("too many blocks");

						  valid = false;

						  return;
					  }

					  const uint8_t payloadType = data[offset] & 0x7F;
					  auto it                   = mapping.find(payloadType);

					  if (it == mapping.end())
					  {
						  MS_WARN_DEV("unknown block payload type [payloadType:%" PRIu8 "]", payloadType);

						  valid = false;

						  return;
					  }

					  mappedBlocks[numBlocks++] = { offset, it->second };
				  });

				if (!primaryBlockOffset.has_value() || !valid)
				{
					return false;
				}

				for (size_t i{ 0 }; i < numBlocks; ++i)
				{
					const auto [offset, mappedPayloadType] = mappedBlocks[i];

					data[offset] = (data[offset] & 0x80) | (mappedPayloadType & 0x7F);
				}

				return true;
			}

			bool RED::Unwrap(RTP::Packet* packet, RemovedData* removedData)
			{
				MS_TRACE();

				auto* data = packet->GetPayload();
				auto len   = packet->GetPayloadLength();
				uint8_t payloadType;
				size_t offset;

				if (!RED::GetPrimaryBlock(data, len, payloadType, offset))
				{
					return false;
				}

				if (removedData)
				{
					removedData->payloadType   = packet->GetPayloadType();
					removedData->paddingLength = packet->GetPaddingLength();
					removedData->data.assign(data, data + offset);
				}

				// NOTE: SetPayload() uses memmove() so the source can overlap.
				packet->SetPayload(data + offset, len - offset);
				packet->SetPayloadType(payloadType);

				return true;
			}

			void RED::Restore(RTP::Packet* packet, const RemovedData& removedData)
			{
				MS_TRACE();

				const size_t primaryBlockLength = packet->GetPayloadLength();
				const size_t removedLength      = removedData.data.size();

				// NOTE: The original packet fitted in its buffer so this cannot throw.
				packet->SetPayloadLength(removedLength + primaryBlockLength);

				auto* data = packet->GetPayload();

				std::memmove(data + removedLength, data, primaryBlockLength);
				std::memcpy(data, removedData.data.data(), removedLength);

				packet->SetPayloadType(removedData.payloadType);

				if (removedData.paddingLength > 0)
				{
					packet->SetPaddingLength(removedData.paddingLength);
				}
			}

			void RED::ProcessRtpPacket(RTP::Packet* packet, const RTC::RtpCodecMimeType& mimeType)
			{
				MS_TRACE();

				switch (mimeType.subtype)
				{
					case RTC::RtpCodecMimeType::Subtype::OPUS:
					case RTC::RtpCodecMimeType::Subtype::MULTIOPUS:
					{
						auto* data = packet->GetPayload();
						auto len   = packet->GetPayloadLength();
						uint8_t payloadType;
						size_t offset;

						if (!RED::GetPrimaryBlock(data, len, payloadType, offset))
						{
							return;
						}

						auto* payloadDescriptor = Opus::Parse(data + offset, len - offset);

						if (!payloadDescriptor)
						{
							return;
						}

						packet->SetPayloadDescriptorHandler(new Opus::PayloadDescriptorHandler(payloadDescriptor));

						break;
					}

					default:;
				}
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
