#ifndef MS_MOCKS_RTC_SCTP_MOCK_TRANSMISSION_CONTROL_BLOCK_HPP
#define MS_MOCKS_RTC_SCTP_MOCK_TRANSMISSION_CONTROL_BLOCK_HPP

#include "common.hpp"
#include "RTC/SCTP/association/TransmissionControlBlockContextInterface.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/public/AssociationListenerInterface.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "mocks/include/mockTypes.hpp"
#include <queue>
#include <stdexcept>
#include <string>

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
				using GetCurrentRtoUsAction = std::function<int64_t()>;

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

				void ObserveRttUs(int64_t rttUs) override
				{
					this->observeRttUsCallCount++;
					this->observeRttUsCalledWith = rttUs;
				}

				int64_t GetCurrentRtoUs() const override
				{
					if (!this->getCurrentRtoUsOnceActions.empty())
					{
						auto action = std::move(this->getCurrentRtoUsOnceActions.front());

						this->getCurrentRtoUsOnceActions.pop();

						return action();
					}
					else
					{
						return 0;
					}
				}

				bool IncrementTxErrorCounter(std::string_view /*reason*/) override
				{
					this->incrementTxErrorCounterCallCount++;

					return false;
				}

				void ClearTxErrorCounter() override
				{
					this->incrementTxErrorCounterCallCount = 0;
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

				/**
				 * @remarks
				 * - Must be called before expecting calls to `ObserveRttUs()`.
				 */
				MockTransmissionControlBlockContext& ExpectObserveRttUsCalledTimes(size_t times)
				{
					this->observeRttUsCallCount         = 0;
					this->expectedObserveRttUsCallCount = times;

					return *this;
				}

				MockTransmissionControlBlockContext& ExpectObserveRttUsCalledWith(int64_t rttUs)
				{
					this->expectedObserveRttUsCalledWith = rttUs;

					return *this;
				}

				/**
				 * @remarks
				 * - Must be called before expecting calls to `IncrementTxErrorCounter()`.
				 */
				MockTransmissionControlBlockContext& ExpectIncrementTxErrorCounterCalledTimes(size_t times)
				{
					this->incrementTxErrorCounterCallCount         = 0;
					this->expectedIncrementTxErrorCounterCallCount = times;

					return *this;
				}

				MockTransmissionControlBlockContext& WillGetCurrentRtoUsOnce(GetCurrentRtoUsAction action)
				{
					this->getCurrentRtoUsOnceActions.push(std::move(action));

					return *this;
				}

				mocks::VerificationResult VerifyExpectations() const
				{
					if (
					  this->expectedObserveRttUsCallCount.has_value() &&
					  this->observeRttUsCallCount != this->expectedObserveRttUsCallCount.value())
					{
						return { .ok           = false,
						         .errorMessage = "ObserveRttUs() call count mismatch [expected:" +
							                       std::to_string(this->expectedObserveRttUsCallCount.value()) +
							                       ", got:" + std::to_string(this->observeRttUsCallCount) + "]" };
					}

					if (
					  this->expectedObserveRttUsCalledWith.has_value() &&
					  this->observeRttUsCalledWith != this->expectedObserveRttUsCalledWith)
					{
						return { .ok           = false,
						         .errorMessage = "ObserveRttUs() call mismatch [expected:" +
							                       std::to_string(this->expectedObserveRttUsCalledWith.value()) +
							                       ", got:" +
							                       (this->observeRttUsCalledWith.has_value()
							                          ? std::to_string(this->observeRttUsCalledWith.value())
							                          : "none") +
							                       "]" };
					}

					if (
					  this->expectedIncrementTxErrorCounterCallCount.has_value() &&
					  this->incrementTxErrorCounterCallCount !=
					    this->expectedIncrementTxErrorCounterCallCount.value())
					{
						return { .ok = false,
						         .errorMessage =
						           "IncrementTxErrorCounter() call count mismatch [expected:" +
						           std::to_string(this->expectedIncrementTxErrorCounterCallCount.value()) +
						           ", got:" + std::to_string(this->incrementTxErrorCounterCallCount) + "]" };
					}

					return { .ok = true, .errorMessage = "" };
				}

			private:
				// Passed by argument.
				::RTC::SCTP::AssociationListenerInterface& associationListener;
				const ::RTC::SCTP::SctpOptions sctpOptions;

				// ObserveRttUs().
				size_t observeRttUsCallCount{ 0 };
				std::optional<size_t> expectedObserveRttUsCallCount;
				std::optional<int64_t> observeRttUsCalledWith;
				std::optional<int64_t> expectedObserveRttUsCalledWith;

				// GetCurrentRtoUs().
				mutable std::queue<GetCurrentRtoUsAction> getCurrentRtoUsOnceActions;

				// IncrementTxErrorCounter().
				size_t incrementTxErrorCounterCallCount{ 0 };
				std::optional<size_t> expectedIncrementTxErrorCounterCallCount;

				// Others.
				bool associationEstablished{ false };
			};
		} // namespace SCTP
	} // namespace RTC
} // namespace mocks

#endif
