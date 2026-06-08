#ifndef MS_MOCKS_RTC_SCTP_MOCK_ASSOCIATION_LISTENER_HPP
#define MS_MOCKS_RTC_SCTP_MOCK_ASSOCIATION_LISTENER_HPP

#include "RTC/SCTP/public/AssociationListenerInterface.hpp"
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mocks
{
	namespace RTC
	{
		namespace SCTP
		{
			class MockAssociationListener : public ::RTC::SCTP::AssociationListenerInterface
			{
			public:
				bool OnAssociationSendData(const uint8_t* data, size_t len) override
				{
					this->sentPackets.emplace_back(data, data + len);

					return this->sendDataResult;
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

				void OnAssociationFailed(
				  ::RTC::SCTP::Types::ErrorKind errorKind, std::string_view errorMessage) override
				{
					this->connecting         = false;
					this->connected          = false;
					this->failed             = true;
					this->failedErrorKind    = errorKind;
					this->failedErrorMessage = errorMessage;
				}

				void OnAssociationClosed(
				  ::RTC::SCTP::Types::ErrorKind errorKind, std::string_view errorMessage) override
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

				void OnAssociationError(
				  ::RTC::SCTP::Types::ErrorKind errorKind, std::string_view errorMessage) override
				{
					this->connecting          = false;
					this->connected           = false;
					this->errored             = true;
					this->erroredErrorKind    = errorKind;
					this->erroredErrorMessage = errorMessage;
				}

				void OnAssociationMessageReceived(::RTC::SCTP::Message message) override
				{
					this->receivedMessages.emplace_back(std::move(message));
				}

				void OnAssociationStreamsResetPerformed(std::span<const uint16_t> outboundStreamIds) override
				{
					++this->onStreamsResetPerformedCalls;

					for (const auto streamId : outboundStreamIds)
					{
						this->streamsResetPerformed.insert(streamId);
					}
				}

				void OnAssociationStreamsResetFailed(
				  std::span<const uint16_t> outboundStreamIds, std::string_view /*errorMessage*/) override
				{
					++this->onStreamsResetFailedCalls;

					for (const auto streamId : outboundStreamIds)
					{
						this->streamsResetFailed.insert(streamId);
					}
				}

				void OnAssociationInboundStreamsReset(std::span<const uint16_t> inboundStreamIds) override
				{
					++this->onInboundStreamsResetCalls;

					for (const auto streamId : inboundStreamIds)
					{
						this->inboundStreamsReset.insert(streamId);
					}
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
					this->onAssociationLifecycleMessageFullySentCalls.insert(lifecycleId);
				}

				void OnAssociationLifecycleMessageExpired(uint64_t lifecycleId, bool maybeDelivered) override
				{
					this->onAssociationLifecycleMessageExpiredCalls[lifecycleId] = maybeDelivered;
				}

				void OnAssociationLifecycleMessageDelivered(uint64_t lifecycleId) override
				{
					this->onAssociationLifecycleMessageDeliveredCalls.insert(lifecycleId);
				}

				void OnAssociationLifecycleMessageEnd(uint64_t lifecycleId) override
				{
					this->onAssociationLifecycleMessageEndCalls.insert(lifecycleId);
				}

				// Methods for testing.
			public:
				bool IsConnecting() const
				{
					return this->connecting;
				}

				bool IsConnected() const
				{
					return this->connected;
				}

				bool HasRestarted() const
				{
					return this->restarted;
				}

				bool HasFailed() const
				{
					return this->failed;
				}

				::RTC::SCTP::Types::ErrorKind GetFailedErrorKind() const
				{
					return this->failedErrorKind;
				}

				const std::string& GetFailedErrorMessage() const
				{
					return this->failedErrorMessage;
				}

				bool IsClosed() const
				{
					return this->closed;
				}

				::RTC::SCTP::Types::ErrorKind GetClosedErrorKind() const
				{
					return this->closedErrorKind;
				}

				const std::string& GetClosedErrorMessage() const
				{
					return this->closedErrorMessage;
				}

				bool HasErrored() const
				{
					return this->errored;
				}

				::RTC::SCTP::Types::ErrorKind GetErroredErrorKind() const
				{
					return this->erroredErrorKind;
				}

				const std::string& GetErroredErrorMessage() const
				{
					return this->erroredErrorMessage;
				}

				bool HasOnStreamBufferedAmountLowBeenCalledWithStreamId(uint64_t streamId) const
				{
					return this->onStreamBufferedAmountLowCalls.contains(streamId);
				}

				size_t CountOnStreamBufferedAmountLowCallsWithStreamId(uint64_t streamId) const
				{
					if (!this->onStreamBufferedAmountLowCalls.contains(streamId))
					{
						return 0;
					}

					return this->onStreamBufferedAmountLowCalls.at(streamId);
				}

				size_t CountOnTotalBufferedAmountLowCalls() const
				{
					return this->onTotalBufferedAmountLowCalls;
				}

				size_t CountOnStreamsResetPerformedCalls() const
				{
					return this->onStreamsResetPerformedCalls;
				}

				bool HasStreamsResetPerformedForStreamId(uint16_t streamId) const
				{
					return this->streamsResetPerformed.contains(streamId);
				}

				size_t CountOnStreamsResetFailedCalls() const
				{
					return this->onStreamsResetFailedCalls;
				}

				bool HasStreamsResetFailedForStreamId(uint16_t streamId) const
				{
					return this->streamsResetFailed.contains(streamId);
				}

				size_t CountOnInboundStreamsResetCalls() const
				{
					return this->onInboundStreamsResetCalls;
				}

				bool HasInboundStreamsResetForStreamId(uint16_t streamId) const
				{
					return this->inboundStreamsReset.contains(streamId);
				}

				/**
				 * Sets the value that `OnAssociationSendData()` will return, allowing
				 * tests to simulate a failed send.
				 */
				void SetSendDataResult(bool sendDataResult)
				{
					this->sendDataResult = sendDataResult;
				}

				bool HasSentPackets() const
				{
					return !this->sentPackets.empty();
				}

				bool HasReceivedMessages() const
				{
					return !this->receivedMessages.empty();
				}

				std::vector<uint8_t> ConsumeFirstSentPacket()
				{
					if (this->sentPackets.empty())
					{
						return {};
					}

					const auto packet = std::move(this->sentPackets.front());

					this->sentPackets.pop_front();

					return packet;
				}

				std::optional<::RTC::SCTP::Message> ConsumeFirstReceivedMessage()
				{
					if (this->receivedMessages.empty())
					{
						return std::nullopt;
					}

					auto message = std::move(this->receivedMessages.front());

					this->receivedMessages.pop_front();

					return message;
				}

				bool IsTransportReady() const
				{
					return this->transportReady;
				}

				bool HasOnAssociationLifecycleMessageFullySentBeenCalledWithLifecycleId(uint64_t lifecycleId) const
				{
					return this->onAssociationLifecycleMessageFullySentCalls.contains(lifecycleId);
				}

				bool HasOnAssociationLifecycleMessageExpiredSentBeenCalledWithLifecycleId(
				  uint64_t lifecycleId, bool maybeDelivered) const
				{
					if (!this->onAssociationLifecycleMessageExpiredCalls.contains(lifecycleId))
					{
						return false;
					}

					return this->onAssociationLifecycleMessageExpiredCalls.at(lifecycleId) == maybeDelivered;
				}

				bool HasOnAssociationLifecycleMessageDeliveredBeenCalledWithLifecycleId(uint64_t lifecycleId) const
				{
					return this->onAssociationLifecycleMessageDeliveredCalls.contains(lifecycleId);
				}

				bool HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(uint64_t lifecycleId) const
				{
					return this->onAssociationLifecycleMessageEndCalls.contains(lifecycleId);
				}

			private:
				// Observable state for tests.
				bool connecting{ false };
				bool connected{ false };
				bool restarted{ false };
				bool failed{ false };
				::RTC::SCTP::Types::ErrorKind failedErrorKind;
				std::string failedErrorMessage;
				bool closed{ false };
				::RTC::SCTP::Types::ErrorKind closedErrorKind;
				std::string closedErrorMessage;
				bool errored{ false };
				::RTC::SCTP::Types::ErrorKind erroredErrorKind;
				std::string erroredErrorMessage;
				std::map<uint16_t /*streamId*/, size_t /*count*/> onStreamBufferedAmountLowCalls;
				size_t onTotalBufferedAmountLowCalls{ 0 };
				size_t onStreamsResetPerformedCalls{ 0 };
				std::set<uint16_t /*streamId*/> streamsResetPerformed;
				size_t onStreamsResetFailedCalls{ 0 };
				std::set<uint16_t /*streamId*/> streamsResetFailed;
				size_t onInboundStreamsResetCalls{ 0 };
				std::set<uint16_t /*streamId*/> inboundStreamsReset;
				bool sendDataResult{ true };
				std::deque<std::vector<uint8_t>> sentPackets;
				std::deque<::RTC::SCTP::Message> receivedMessages;
				bool transportReady{ true };
				std::set<uint64_t /*lifecycleId*/> onAssociationLifecycleMessageFullySentCalls;
				std::map<uint64_t /*lifecycleId*/, bool /*maybeDelivered*/> onAssociationLifecycleMessageExpiredCalls;
				std::set<uint64_t /*lifecycleId*/> onAssociationLifecycleMessageDeliveredCalls;
				std::set<uint64_t /*lifecycleId*/> onAssociationLifecycleMessageEndCalls;
			};
		} // namespace SCTP
	} // namespace RTC
} // namespace mocks

#endif
