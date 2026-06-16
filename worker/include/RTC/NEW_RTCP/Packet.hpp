#ifndef MS_RTC_NEW_RTCP_PACKET_HPP
#define MS_RTC_NEW_RTCP_PACKET_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * RTCP Packet.
		 *
		 * @see RFC 3550.
		 *
		 * @remarks
		 * - This class represents a single RTCP packet and not a compound packet.
		 */
		class Packet : public Serializable
		{
		public:
			/**
			 * RTP Common Header.
			 *
			 * @see RFC 3550.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |V=2|P|   RC    |      PT       |            length             |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *
			 * @remarks
			 * - This struct is guaranteed to be aligned to 4 bytes.
			 */
			struct CommonHeader
			{
#if defined(MS_LITTLE_ENDIAN)
				uint8_t count : 5;
				uint8_t padding : 1;
				uint8_t version : 2;
#elif defined(MS_BIG_ENDIAN)
				uint8_t version : 2;
				uint8_t padding : 1;
				uint8_t count : 5;
#endif
				uint8_t packetType;
				uint16_t length;
			};

		public:
			/**
			 * Length (in bytes) of the RTCP Common Header.
			 */
			static constexpr size_t CommonHeaderLength{ 4 };

			/**
			 * Whether given buffer could be a valid RTCP packet.
			 */
			static bool IsRtcp(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse an RTCP packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the packet.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create an RTCP packet.
			 */
			static Packet* Factory(uint8_t* buffer, size_t bufferLength);
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
