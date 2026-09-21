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
			 * Maximum length of a probation RTP packet.
			 */
			static constexpr size_t ProbationPacketMaxLength{ 1400 };

		public:
			explicit ProbationGenerator();
			~ProbationGenerator();

		public:
			RTP::Packet* GetNextPacket(size_t len);

			size_t GetProbationPacketMinLength() const
			{
				return this->probationPacketMinLength;
			}

		private:
			// Allocated by this.
			std::unique_ptr<RTP::Packet> probationPacket;
			// Others.
			// The length of the probation RTP packet without payload or padding.
			size_t probationPacketMinLength{ 0 };
		};
	} // namespace RTP
} // namespace RTC

#endif
