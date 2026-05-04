#ifndef MS_MOCKS_RTC_SCTP_MOCK_ASSOCIATION_LISTENER_HPP
#define MS_MOCKS_RTC_SCTP_MOCK_ASSOCIATION_LISTENER_HPP

#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
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

			void OnAssociationStreamBufferedAmountLow(uint16_t streamId) override
			{
				++this->onStreamBufferedAmountLowCalls[streamId];
			}

			void OnAssociationTotalBufferedAmountLow() override
			{
				++this->onTotalBufferedAmountLowCalls;
			}

			bool OnAssociationIsTransportReadyForSctp() override
			{
				return this->transportReady;
			}

			void OnAssociationLifecycleMessageFullySent(uint64_t lifecycleId) override
			{
				this->onAssociationLifecycleMessageFullySentLifecycleId = lifecycleId;
			}

			void OnAssociationLifecycleMessageExpired(uint64_t lifecycleId, bool maybeDelivered) override
			{
				// TODO: SCTP: REMOVE
				printf("------ OnAssociationLifecycleMessageExpired(lifecycleId:%" PRIu64 ")\n", lifecycleId);
				this->onAssociationLifecycleMessageExpiredLifecycleId    = lifecycleId;
				this->onAssociationLifecycleMessageExpiredMaybeDelivered = maybeDelivered;
			}

			void OnAssociationLifecycleMessageDelivered(uint64_t lifecycleId) override
			{
				this->onAssociationLifecycleMessageDeliveredLifecycleId = lifecycleId;
			}

			void OnAssociationLifecycleMessageEnd(uint64_t lifecycleId) override
			{
				// TODO: SCTP: REMOVE
				printf("------ OnAssociationLifecycleMessageEnd(lifecycleId:%" PRIu64 ")\n", lifecycleId);
				this->onAssociationLifecycleMessageEndLifecycleId = lifecycleId;
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
			std::unordered_map<uint16_t /*streamId*/, size_t /*cound*/> onStreamBufferedAmountLowCalls;
			size_t onTotalBufferedAmountLowCalls{ 0 };
			std::vector<std::vector<uint8_t>> sentPackets;
			std::vector<Message> receivedMessages;
			bool transportReady{ true };
			uint64_t onAssociationLifecycleMessageFullySentLifecycleId{ 0 };
			uint64_t onAssociationLifecycleMessageExpiredLifecycleId{ 0 };
			bool onAssociationLifecycleMessageExpiredMaybeDelivered{ false };
			uint64_t onAssociationLifecycleMessageDeliveredLifecycleId{ 0 };
			uint64_t onAssociationLifecycleMessageEndLifecycleId{ 0 };
		};
	} // namespace SCTP
} // namespace RTC

#endif
