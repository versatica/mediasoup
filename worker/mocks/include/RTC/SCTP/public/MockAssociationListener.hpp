#ifndef MS_MOCKS_RTC_SCTP_MOCK_ASSOCIATION_LISTENER_HPP
#define MS_MOCKS_RTC_SCTP_MOCK_ASSOCIATION_LISTENER_HPP

#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <string>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		class MockAssociationListener : public AssociationListener
		{
		public:
			bool OnAssociationSendData(const uint8_t* data, size_t len) override
			{
				sentPackets.emplace_back(data, data + len);

				return true;
			}

			void OnAssociationConnecting() override
			{
				this->connecting = true;
			}

			void OnAssociationConnected() override
			{
				this->connecting = false;
				this->connected  = true;
			}

			void OnAssociationFailed(Types::ErrorKind errorKind, std::string_view errorMessage) override
			{
				this->connecting         = false;
				this->connected          = false;
				this->failed             = true;
				this->failedErrorKind    = errorKind;
				this->failedErrorMessage = errorMessage;
			}

			void OnAssociationClosed(Types::ErrorKind errorKind, std::string_view errorMessage) override
			{
				this->connecting         = false;
				this->connected          = false;
				this->closed             = true;
				this->closedErrorKind    = errorKind;
				this->closedErrorMessage = errorMessage;
			}

			void OnAssociationRestarted() override
			{
				this->restarted = true;
			}

			void OnAssociationError(Types::ErrorKind errorKind, std::string_view errorMessage) override
			{
				this->connecting          = false;
				this->connected           = false;
				this->errored             = true;
				this->erroredErrorKind    = errorKind;
				this->erroredErrorMessage = errorMessage;
			}

			void OnAssociationMessageReceived(Message message) override
			{
				this->receivedMessages.emplace_back(std::move(message));
			}

			void OnAssociationStreamsResetPerformed(std::span<const uint16_t> /*outboundStreamIds*/) override
			{
			}

			void OnAssociationStreamsResetFailed(
			  std::span<const uint16_t> /*outboundStreamIds*/, std::string_view /*errorMessage*/) override
			{
			}

			void OnAssociationInboundStreamsReset(std::span<const uint16_t> /*inboundStreamIds*/) override
			{
			}

			void OnAssociationStreamBufferedAmountLow(uint16_t /*streamId*/) override
			{
			}

			void OnAssociationTotalBufferedAmountLow() override
			{
			}

			bool OnAssociationIsTransportReadyForSctp() override
			{
				return this->transportReady;
			}

		public:
			// Observable state for tests.
			bool connecting{ false };
			bool connected{ false };
			bool restarted{ false };
			bool failed{ false };
			Types::ErrorKind failedErrorKind;
			std::string failedErrorMessage;
			bool closed{ false };
			Types::ErrorKind closedErrorKind;
			std::string closedErrorMessage;
			bool errored{ false };
			Types::ErrorKind erroredErrorKind;
			std::string erroredErrorMessage;
			std::vector<std::vector<uint8_t>> sentPackets;
			std::vector<Message> receivedMessages;
			bool transportReady{ true };
		};
	} // namespace SCTP
} // namespace RTC

#endif
