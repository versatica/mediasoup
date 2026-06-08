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
				using GetCurrentRtoMsAction = std::function<uint64_t()>;

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
					this->observeRttMsCallCount++;
				}

				uint64_t GetCurrentRtoMs() const override
				{
					if (!this->getCurrentRtoMsOnceActions.empty())
					{
						auto action = std::move(this->getCurrentRtoMsOnceActions.front());

						this->getCurrentRtoMsOnceActions.pop();

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
				 * - Must be called before expecting calls to `ObserveRttMs()`.
				 */
				MockTransmissionControlBlockContext& ExpectObserveRttMsCalledTimes(size_t times)
				{
					this->observeRttMsCallCount         = 0;
					this->expectedObserveRttMsCallCount = times;

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

				MockTransmissionControlBlockContext& WillGetCurrentRtoMsOnce(GetCurrentRtoMsAction action)
				{
					this->getCurrentRtoMsOnceActions.push(std::move(action));

					return *this;
				}

				mocks::VerificationResult VerifyExpectations() const
				{
					if (
					  this->expectedObserveRttMsCallCount.has_value() &&
					  this->observeRttMsCallCount != this->expectedObserveRttMsCallCount.value())
					{
						return { .ok           = false,
							       .errorMessage = "ObserveRttMs() call count mismatch [expected:" +
							                       std::to_string(this->expectedObserveRttMsCallCount.value()) +
							                       ", got:" + std::to_string(this->observeRttMsCallCount) + "]" };
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

				// ObserveRttMs().
				size_t observeRttMsCallCount{ 0 };
				std::optional<size_t> expectedObserveRttMsCallCount;

				// GetCurrentRtoMs().
				mutable std::queue<GetCurrentRtoMsAction> getCurrentRtoMsOnceActions;

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
