#ifndef MS_RTC_RTP_PACKET_UTILS_HPP
#define MS_RTC_RTP_PACKET_UTILS_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		class PacketUtils
		{
		public:
			/**
			 * Clone the given RTP packet into a newly allocated buffer and return
			 * shared ownership of the clone.
			 *
			 * @remarks
			 * The buffer allocated for the clone is deallocated by the clone itself
			 * (via its buffer released listener) once it's destroyed, so the caller
			 * just needs to drop every reference to the returned shared pointer.
			 */
			static std::shared_ptr<RTP::Packet> ClonePacket(const RTP::Packet* packet);

			/**
			 * Assert that the given shared packet holds a clone of the given RTP
			 * packet.
			 *
			 * @remarks
			 * A shared packet corresponds to a single packet received by a Producer,
			 * so this must be asserted wherever an already existing clone (made by
			 * another Consumer of the same Producer) is reused rather than a new one
			 * made.
			 */
			static void AssertSamePacket(
			  const std::shared_ptr<RTP::Packet>& sharedPacket, const RTP::Packet* otherPacket);
		};
	} // namespace RTP
} // namespace RTC

#endif
