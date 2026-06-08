#ifndef MS_RTC_SCTP_PACKET_SENDER_HPP
#define MS_RTC_SCTP_PACKET_SENDER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/public/AssociationListenerInterface.hpp"

namespace RTC
{
	namespace SCTP
	{
		class PacketSender
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				virtual void OnPacketSenderPacketSent(
				  PacketSender* packetSender, const Packet* packet, bool sent) = 0;
			};

		public:
			PacketSender(Listener* listener, AssociationListenerInterface& associationListener);

			~PacketSender();

		public:
			/**
			 * Notifies the parent about a packet to be sent to the peer and returns a
			 * boolean indicating whether the packet was sent or not.
			 *
			 * This method also writes the packet checksum field depending on the value
			 * of `writeChecksum`.
			 *
			 * @remarks
			 * - This method does not delete the given `packet`. The caller must do it
			 *   after invoking this method.
			 */
			bool SendPacket(Packet* packet, bool writeChecksum = true);

		private:
			Listener* listener;
			AssociationListenerInterface& associationListener;
		};
	} // namespace SCTP
} // namespace RTC

#endif
