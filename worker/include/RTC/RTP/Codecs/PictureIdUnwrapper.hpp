#ifndef MS_RTC_RTP_CODECS_PICTURE_ID_UNWRAPPER_HPP
#define MS_RTC_RTP_CODECS_PICTURE_ID_UNWRAPPER_HPP

#include "common.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/**
			 * Extends the pictureIds of a VP8 or VP9 stream into 15 bits.
			 *
			 * @remarks
			 * - The sender picks how wide every pictureId is, 7 or 15 bits, and is
			 *   free to change it from one packet to the next. A stream numbering its
			 *   pictures with 7 bits hence wraps every 128 of them.
			 * - Taken as it comes, every pictureId past such a wrap would look older
			 *   than the ones before it, which is what tells a reordered packet apart.
			 */
			class PictureIdUnwrapper
			{
			private:
				/**
				 * Highest value each pictureId width holds.
				 */
				static constexpr uint32_t OneByteMaxPictureId{ 0x7F };
				static constexpr uint32_t TwoBytesMaxPictureId{ 0x7FFF };

			public:
				/**
				 * Forgets the pictureIds seen so far, so that the next one starts the
				 * sequence. Needed when the sender restarts the stream, since the
				 * pictures to come may be numbered by another encoder and may not even
				 * be as wide.
				 */
				void Reset()
				{
					this->maxPictureId.reset();
				}

				/**
				 * @param pictureId - PictureId just as it came.
				 * @param hasTwoBytesPictureId - Whether the sender gave it 15 bits rather
				 *   than 7.
				 * @returns The same pictureId extended to 15 bits, that is, carrying
				 *   the cycles a 7 bits one has already wrapped.
				 */
				uint16_t Unwrap(uint16_t pictureId, bool hasTwoBytesPictureId)
				{
					if (!this->maxPictureId.has_value())
					{
						this->maxPictureId            = pictureId;
						this->maxHasTwoBytesPictureId = hasTwoBytesPictureId;

						return pictureId;
					}

					const uint32_t max = this->maxPictureId.value();
					// Only the bits that both pictureIds carry can be compared, so the
					// distance is taken in the narrower of the two widths. That is what
					// keeps the sequence going when the sender changes the width.
					const uint32_t maxValue = hasTwoBytesPictureId && this->maxHasTwoBytesPictureId
					                            ? PictureIdUnwrapper::TwoBytesMaxPictureId
					                            : PictureIdUnwrapper::OneByteMaxPictureId;
					const uint32_t distance = (pictureId - max) & maxValue;
					// More than half a cycle forwards means the pictureId is behind the
					// highest one instead, that is, that the packet was reordered.
					const bool isOlder   = distance > maxValue / 2;
					const auto unwrapped = static_cast<uint16_t>(
					  (max + distance - (isOlder ? maxValue + 1 : 0)) &
					  PictureIdUnwrapper::TwoBytesMaxPictureId);

					if (!isOlder)
					{
						this->maxPictureId            = unwrapped;
						this->maxHasTwoBytesPictureId = hasTwoBytesPictureId;
					}

					return unwrapped;
				}

			private:
				/**
				 * Highest pictureId unwrapped so far.
				 */
				std::optional<uint16_t> maxPictureId;
				/**
				 * Whether the sender gave 15 bits to `maxPictureId`.
				 */
				bool maxHasTwoBytesPictureId{ false };
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
