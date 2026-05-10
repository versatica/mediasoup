#ifndef MS_MOCKS_RTC_SCTP_MOCK_SEND_QUEUE_HPP
#define MS_MOCKS_RTC_SCTP_MOCK_SEND_QUEUE_HPP

#include "common.hpp"
#include "test/include/catch2Macros.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include <queue>
#include <stdexcept>

namespace mocks
{
	namespace RTC
	{
		namespace SCTP
		{
			class MockSendQueue : public ::RTC::SCTP::SendQueueInterface
			{
			public:
				using ProduceAction =
				  std::function<std::optional<DataToSend>(uint64_t /*nowMs*/, size_t /*maxLength*/)>;

				struct DiscardExpectation
				{
					// If std::nullopt, don't check it.
					std::optional<uint16_t> streamId;
					// If std::nullopt, don't check it.
					std::optional<uint32_t> outgoingMessageId;
					bool returnValue{ false };
				};

			public:
				MockSendQueue() = default;

				~MockSendQueue() override = default;

			public:
				void EnableMessageInterleaving(bool /*enabled*/) override
				{
				}

				std::optional<DataToSend> Produce(uint64_t nowMs, size_t maxLength) override
				{
					++this->produceCallCount;

					if (!this->produceOnceActions.empty())
					{
						auto action = std::move(this->produceOnceActions.front());

						this->produceOnceActions.pop();

						return action(nowMs, maxLength);
					}
					else if (this->produceRepeatedlyAction)
					{
						return this->produceRepeatedlyAction(nowMs, maxLength);
					}
					else
					{
						return std::nullopt;
					}
				}

				bool Discard(uint16_t streamId, uint32_t outgoingMessageId) override
				{
					++this->discardCallCount;

					if (this->discardExpectations.empty())
					{
						throw std::logic_error("Discard() called but no expectation was set");
					}

					auto discardExpectation = this->discardExpectations.front();

					this->discardExpectations.pop();

					if (discardExpectation.streamId.has_value() && discardExpectation.streamId.value() != streamId)
					{
						throw std::logic_error(
						  "Discard() called with unexpected streamId [expected:" +
						  std::to_string(discardExpectation.streamId.value()) +
						  ", got:" + std::to_string(streamId) + "]");
					}

					if (
					  discardExpectation.outgoingMessageId.has_value() &&
					  discardExpectation.outgoingMessageId.value() != outgoingMessageId)
					{
						throw std::logic_error(
						  "Discard() called with unexpected outgoingMessageId [expected:" +
						  std::to_string(discardExpectation.outgoingMessageId.value()) +
						  ", got:" + std::to_string(outgoingMessageId) + "]");
					}

					return discardExpectation.returnValue;
				}

				void PrepareResetStream(uint16_t /*streamId*/) override
				{
				}

				bool HasStreamsReadyToBeReset() const override
				{
					return false;
				}

				std::vector<uint16_t> GetStreamsReadyToBeReset() override
				{
					return {};
				}

				void CommitResetStreams() override
				{
				}

				void RollbackResetStreams() override
				{
				}

				void Reset() override
				{
				}

				size_t GetStreamBufferedAmount(uint16_t /*streamId*/) const override
				{
					return 0;
				}

				size_t GetTotalBufferedAmount() const override
				{
					return 0;
				}

				size_t GetStreamBufferedAmountLowThreshold(uint16_t /*streamId*/) const override
				{
					return 0;
				}

				void SetStreamBufferedAmountLowThreshold(uint16_t /*streamId*/, size_t /*bytes*/) override
				{
				}

				// Helpers for tests.
			public:
				MockSendQueue& WillProduceOnce(ProduceAction action)
				{
					this->produceOnceActions.push(std::move(action));

					return *this;
				}

				MockSendQueue& WillProduceRepeatedly(ProduceAction action)
				{
					this->produceRepeatedlyAction = std::move(action);

					return *this;
				}

				/**
				 * @remarks
				 * - Must be called before expecting calls to `Produce()`.
				 */
				MockSendQueue& ExpectProduceCalledTimes(size_t times)
				{
					this->produceCallCount         = 0;
					this->expectedProduceCallCount = times;

					return *this;
				}

				MockSendQueue& WillDiscardOnce(uint16_t streamId, uint32_t outgoingMessageId, bool returnValue)
				{
					this->discardExpectations.push({ streamId, outgoingMessageId, returnValue });

					return *this;
				}

				/**
				 * @remarks
				 * - Must be called before expecting calls to `Discard()`.
				 */
				MockSendQueue& ExpectDiscardCalledTimes(size_t times)
				{
					this->discardCallCount         = 0;
					this->expectedDiscardCallCount = times;

					return *this;
				}

				test::VerificationResult VerifyExpectations() const
				{
					if (
					  this->expectedProduceCallCount.has_value() &&
					  this->produceCallCount != this->expectedProduceCallCount.value())
					{
						return { .ok           = false,
							       .errorMessage = "Produce() call count mismatch [expected:" +
							                       std::to_string(this->expectedProduceCallCount.value()) +
							                       ", got:" + std::to_string(this->produceCallCount) + "]" };
					}
					else if (
					  this->expectedDiscardCallCount.has_value() &&
					  this->discardCallCount != this->expectedDiscardCallCount.value())
					{
						return { .ok           = false,
							       .errorMessage = "Discard() call count mismatch [expected:" +
							                       std::to_string(this->expectedDiscardCallCount.value()) +
							                       ", got:" + std::to_string(this->discardCallCount) + "]" };
					}
					else if (!this->discardExpectations.empty())
					{
						return { .ok           = false,
							       .errorMessage = "Discard() has " +
							                       std::to_string(this->discardExpectations.size()) +
							                       " unconsumed expectation(s)" };
					}
					else
					{
						return { .ok = true, .errorMessage = "" };
					}
				}

			private:
				// Produce().
				std::queue<ProduceAction> produceOnceActions;
				ProduceAction produceRepeatedlyAction;
				size_t produceCallCount{ 0 };
				std::optional<size_t> expectedProduceCallCount;

				// Discard().
				std::queue<DiscardExpectation> discardExpectations;
				size_t discardCallCount{ 0 };
				std::optional<size_t> expectedDiscardCallCount;
			};
		} // namespace SCTP
	} // namespace RTC
} // namespace mocks

#endif
