#define MS_CLASS "RTC::SCTP::PacketSender"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/PacketSender.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		PacketSender::PacketSender(Listener* listener, AssociationListenerInterface& associationListener)
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

			if (packet->GetChunksCount() == 0)
			{
				return false;
			}

			if (writeChecksum)
			{
				packet->WriteCRC32cChecksum();
			}

			MS_ASSERT(!packet->NeedsConsolidation(), "cannot send a SCTP packet that needs consolidation");

			const bool sent =
			  this->associationListener.OnAssociationSendData(packet->GetBuffer(), packet->GetLength());

			this->listener->OnPacketSenderPacketSent(this, packet, sent);

			if (!sent)
			{
				MS_WARN_TAG(sctp, "couldn't send SCTP Packet");
			}

			return sent;
		}
	} // namespace SCTP
} // namespace RTC
