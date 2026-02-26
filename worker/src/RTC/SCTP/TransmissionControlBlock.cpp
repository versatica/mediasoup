#define MS_CLASS "RTC::SCTP::TransmissionControlBlock"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/TransmissionControlBlock.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		thread_local static uint8_t PacketFactoryBuffer[RTC::Consts::MaxSafeMtuSizeForSctp];

		/* Instance methods. */

		TransmissionControlBlock::TransmissionControlBlock(
		  const SctpOptions& sctpOptions,
		  uint32_t localVerificationTag,
		  uint32_t remoteVerificationTag,
		  uint32_t localInitialTsn,
		  uint32_t remoteInitialTsn,
		  uint32_t remoteAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities)
		  : sctpOptions(sctpOptions), localVerificationTag(localVerificationTag),
		    remoteVerificationTag(remoteVerificationTag), localInitialTsn(localInitialTsn),
		    remoteInitialTsn(remoteInitialTsn),
		    remoteAdvertisedReceiverWindowCredit(remoteAdvertisedReceiverWindowCredit), tieTag(tieTag),
		    negotiatedCapabilities(negotiatedCapabilities)
		{
			MS_TRACE();
		}

		TransmissionControlBlock::~TransmissionControlBlock()
		{
			MS_TRACE();
		}

		void TransmissionControlBlock::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::TransmissionControlBlock>");
			MS_DUMP_CLEAN(indentation, "  local verification tag: %" PRIu32, this->localVerificationTag);
			MS_DUMP_CLEAN(indentation, "  remote verification tag: %" PRIu32, this->remoteVerificationTag);
			MS_DUMP_CLEAN(indentation, "  local initial tsn: %" PRIu32, this->localInitialTsn);
			MS_DUMP_CLEAN(indentation, "  remote initial tsn: %" PRIu32, this->remoteInitialTsn);
			MS_DUMP_CLEAN(
			  indentation,
			  "  remote advertised receiver window credit: %" PRIu32,
			  this->remoteAdvertisedReceiverWindowCredit);
			MS_DUMP_CLEAN(indentation, "  tie-tag: %" PRIu64, this->tieTag);
			this->negotiatedCapabilities.Dump(indentation + 1);
			MS_DUMP_CLEAN(indentation, "</SCTP::TransmissionControlBlock>");
		}

		std::unique_ptr<Packet> TransmissionControlBlock::CreatePacket() const
		{
			MS_TRACE();

			return CreatePacketWithVerificationTag(this->remoteVerificationTag);
		}

		std::unique_ptr<Packet> TransmissionControlBlock::CreatePacketWithVerificationTag(
		  uint32_t verificationTag) const
		{
			MS_TRACE();

			auto packet =
			  std::unique_ptr<Packet>(Packet::Factory(PacketFactoryBuffer, sizeof(PacketFactoryBuffer)));

			packet->SetSourcePort(this->sctpOptions.sourcePort);
			packet->SetDestinationPort(this->sctpOptions.destinationPort);
			packet->SetVerificationTag(verificationTag);

			return packet;
		}
	} // namespace SCTP
} // namespace RTC
