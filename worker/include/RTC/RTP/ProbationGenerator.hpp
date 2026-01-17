#ifndef MS_RTC_RTP_PROBATION_GENERATOR_HPP
#define MS_RTC_RTP_PROBATION_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		class ProbationGenerator
		{
		public:
			/**
			 * Maximum length of a probation RTP Packet.
			 */
			static const size_t ProbationPacketMaxLength{ 1400 };
			/**
			 * SSRC of the probation RTP stream.
			 */
			static const uint32_t Ssrc{ 1234 };
			/**
			 * Codec payload type of the probation RTP stream.
			 */
			static const uint8_t PayloadType{ 127u };

		public:
			explicit ProbationGenerator();
			~ProbationGenerator();

		public:
			RTC::RTP::Packet* GetNextPacket(size_t len);

			size_t GetProbationPacketMinLength() const
			{
				return this->probationPacketMinLength;
			}

		private:
			// Allocated by this.
			RTC::RTP::Packet* probationPacket{ nullptr };
			// Others.
			// The length of the probation RTP Packet without payload or padding.
			size_t probationPacketMinLength{ 0 };
		};
	} // namespace RTP
} // namespace RTC

#endif
