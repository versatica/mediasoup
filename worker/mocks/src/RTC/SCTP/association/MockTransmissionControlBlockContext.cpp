#define MS_CLASS "mocks::RTC::SCTP::MockTransmissionControlBlockContext"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/RTC/SCTP/association/MockTransmissionControlBlockContext.hpp"
#include "Logger.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"

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
					sctpCommon::FactoryBuffer, this->sctpOptions.mtu) };

				packet->SetSourcePort(this->sctpOptions.sourcePort);
				packet->SetDestinationPort(this->sctpOptions.destinationPort);

				return packet;
			}

			bool MockTransmissionControlBlockContext::SendPacket(::RTC::SCTP::Packet* packet)
			{
				const bool sent =
				  this->associationListener.OnAssociationSendData(packet->GetBuffer(), packet->GetLength());

				return sent;
			}
		} // namespace SCTP
	} // namespace RTC
} // namespace mocks
