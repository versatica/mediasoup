#define MS_CLASS "RTC::SCTP::PacketSender"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/PacketSender.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		PacketSender::PacketSender(Listener& listener, AssociationListener& associationListener)
		  : listener(listener), associationListener(associationListener)
		{
			MS_TRACE();
		}

		PacketSender::~PacketSender()
		{
			MS_TRACE();
		}

		bool PacketSender::SendPacket(Packet* packet, bool writeChecksum)
		{
			MS_TRACE();

			if (writeChecksum)
			{
				packet->WriteCRC32cChecksum();
			}

			// TODO: SCTP: For testing purposes. Must be removed.
			{
				MS_DUMP(">>> sending SCTP packet:");

				packet->Dump();
			}

			const bool sent =
			  this->associationListener.OnAssociationSendData(packet->GetBuffer(), packet->GetLength());

			this->listener.OnPacketSenderPacketSent(this, packet, sent);

			return sent;
		}
	} // namespace SCTP
} // namespace RTC
