#ifndef MS_RTC_RTP_SHARED_PACKET_HPP
#define MS_RTC_RTP_SHARED_PACKET_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		/**
		 * Shared ownership of a cloned RTP packet.
		 */
		using SharedPacket = std::shared_ptr<RTP::Packet>;

		/**
		 * Clone the given RTP packet into a newly allocated buffer and return shared
		 * ownership of the clone.
		 *
		 * @remarks
		 * The buffer allocated for the clone is deallocated by the clone itself
		 * (via its buffer released listener) once it's destroyed, so the caller just
		 * needs to drop every reference to the returned SharedPacket.
		 */
		SharedPacket ClonePacket(const RTP::Packet* packet);

		/**
		 * Assert that the given SharedPacket holds a clone of the given RTP packet.
		 *
		 * @remarks
		 * A SharedPacket corresponds to a single packet received by a Producer, so
		 * this must be asserted wherever an already existing clone (made by another
		 * Consumer of the same Producer) is reused rather than a new one made.
		 */
		void AssertSamePacket(const SharedPacket& sharedPacket, const RTP::Packet* otherPacket);
	} // namespace RTP
} // namespace RTC

#endif
