#define MS_CLASS "RTC::RTP::Codecs::RED"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/RED.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Static. */

			static constexpr size_t NonFinalBlockHeaderLength{ 4 };
			static constexpr size_t FinalBlockHeaderLength{ 1 };

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
			static std::optional<size_t> IterateBlockHeaders(const uint8_t* data, size_t len, F&& fn)
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

				auto primaryBlockOffset = IterateBlockHeaders(
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

				bool allMapped{ true };

				// First validate the whole payload without modifying it.
				auto primaryBlockOffset = IterateBlockHeaders(
				  data,
				  len,
				  [data, &mapping, &allMapped](size_t offset)
				  {
					  const uint8_t payloadType = data[offset] & 0x7F;

					  if (mapping.find(payloadType) == mapping.end())
					  {
						  MS_WARN_DEV("unknown block payload type [payloadType:%" PRIu8 "]", payloadType);

						  allMapped = false;
					  }
				  });

				if (!primaryBlockOffset.has_value() || !allMapped)
				{
					return false;
				}

				// Then rewrite block payload types.
				IterateBlockHeaders(
				  data,
				  len,
				  [data, &mapping](size_t offset)
				  {
					  const uint8_t payloadType       = data[offset] & 0x7F;
					  const uint8_t mappedPayloadType = mapping.at(payloadType);

					  data[offset] = (data[offset] & 0x80) | (mappedPayloadType & 0x7F);
				  });

				return true;
			}

			bool RED::Unwrap(RTP::Packet* packet)
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

				// NOTE: SetPayload() uses memmove() so the source can overlap.
				packet->SetPayload(data + offset, len - offset);
				packet->SetPayloadType(payloadType);

				return true;
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
