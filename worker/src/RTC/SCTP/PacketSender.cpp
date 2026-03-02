#define MS_CLASS "RTC::SCTP::PacketSender"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/PacketSender.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		PacketSender::PacketSender(Listener& listener, SocketListener& socketListener)
		  : listener(listener), socketListener(socketListener)
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

			const bool sent = this->socketListener.OnSocketSendSctpPacket(packet);

			this->listener.OnPacketSenderSentPacket(this, packet, sent);

			return sent;
		}
	} // namespace SCTP
} // namespace RTC
