#define MS_CLASS "mocks::RTC::SCTP::MockTransmissionControlBlockContext"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/RTC/SCTP/association/MockTransmissionControlBlockContext.hpp"
#include "Logger.hpp"

alignas(4) static thread_local uint8_t FactoryBuffer[65536];

namespace mocks
{
	namespace RTC
	{
		namespace SCTP
		{
			std::unique_ptr<::RTC::SCTP::Packet> MockTransmissionControlBlockContext::CreatePacket() const
			{
				MS_TRACE();

				auto packet = std::unique_ptr<::RTC::SCTP::Packet>{ ::RTC::SCTP::Packet::Factory(
					FactoryBuffer, this->sctpOptions.mtu) };

				packet->SetSourcePort(this->sctpOptions.sourcePort);
				packet->SetDestinationPort(this->sctpOptions.destinationPort);

				return packet;
			}

			bool MockTransmissionControlBlockContext::SendPacket(::RTC::SCTP::Packet* packet)
			{
				MS_TRACE();

				const bool sent =
				  this->associationListener.OnAssociationSendData(packet->GetBuffer(), packet->GetLength());

				return sent;
			}
		} // namespace SCTP
	} // namespace RTC
} // namespace mocks
