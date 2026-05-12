#ifndef MS_MOCKS_RTC_SCTP_MOCK_TRANSMISSION_CONTROL_BLOCK_HPP
#define MS_MOCKS_RTC_SCTP_MOCK_TRANSMISSION_CONTROL_BLOCK_HPP

#include "common.hpp"
#include "RTC/SCTP/association/TransmissionControlBlockContextInterface.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/public/AssociationListenerInterface.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"

namespace mocks
{
	namespace RTC
	{
		namespace SCTP
		{
			class MockTransmissionControlBlockContext
			  : public ::RTC::SCTP::TransmissionControlBlockContextInterface
			{
			public:
				explicit MockTransmissionControlBlockContext(
				  ::RTC::SCTP::AssociationListenerInterface& associationListener,
				  const ::RTC::SCTP::SctpOptions& sctpOptions)
				  : associationListener(associationListener), sctpOptions(sctpOptions)
				{
				}

			public:
				bool IsAssociationEstablished() const override
				{
					return this->associationEstablished;
				}

				uint32_t GetLocalInitialTsn() const override
				{
					// TODO: Implement this.
					return 0;
				}

				uint32_t GetRemoteInitialTsn() const override
				{
					// TODO: Implement this.
					return 0;
				}

				void ObserveRttMs(uint64_t rttMs) override
				{
					// TODO: Implement this.
				}

				uint64_t GetCurrentRtoMs() const override
				{
					// TODO: Implement this.
					return 0;
				}

				bool IncrementTxErrorCounter(std::string_view /*reason*/) override
				{
					// TODO: Implement this.
					return false;
				}

				void ClearTxErrorCounter() override
				{
					// TODO: Implement this.
				}

				bool HasTooManyTxErrors() const override
				{
					// TODO: Implement this.
					return false;
				}

				std::unique_ptr<::RTC::SCTP::Packet> CreatePacket() const override;

				bool SendPacket(::RTC::SCTP::Packet* packet) override;

				// Methods for testing.
			public:
				void SetAssociationEstablished(bool associationEstablished)
				{
					this->associationEstablished = associationEstablished;
				}

			private:
				// Passed by argument.
				::RTC::SCTP::AssociationListenerInterface& associationListener;
				const ::RTC::SCTP::SctpOptions sctpOptions;
				// Others.
				bool associationEstablished{ false };
			};
		} // namespace SCTP
	} // namespace RTC
} // namespace mocks

#endif
