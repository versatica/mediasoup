#ifndef MS_RTC_SCTP_HEARTBEAT_HANDLER_HPP
#define MS_RTC_SCTP_HEARTBEAT_HANDLER_HPP

#include "common.hpp"
#include "SharedInterface.hpp"
#include "RTC/SCTP/association/TCBContext.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * HeartbeatHandler handles all logic around sending heartbeats and receiving
		 * the responses, as well as receiving incoming heartbeat requests.
		 *
		 * Heartbeats are sent on idle connections to ensure that the connection is
		 * still healthy and to measure the RTT. If a number of heartbeats time out,
		 * the connection will eventually be closed.
		 */
		class HeartbeatHandler : public BackoffTimerHandleInterface::Listener
		{
		public:
			HeartbeatHandler(
			  AssociationListener& associationListener,
			  const SctpOptions& sctpOptions,
			  SharedInterface* shared,
			  TCBContext* tcbContext);

			~HeartbeatHandler() override;

		public:
			/**
			 * Called when the heartbeat interval timer should be restarted. This is
			 * generally done every time data is sent, which makes the timer expire
			 * when the connection is idle.
			 */
			void RestartTimer();

			/**
			 * Called on received HEARTBEAT_REQUEST Chunk.
			 */
			void HandleReceivedHeartbeatRequestChunk(
			  const HeartbeatRequestChunk* receivedHeartbeatRequestChunk);

			/**
			 * Called on received HEARTBEAT_ACK Chunk.
			 */
			void HandleReceivedHeartbeatAckChunk(const HeartbeatAckChunk* receivedHeartbeatAckChunk);

		private:
			void OnIntervalTimer(uint64_t& baseTimeoutMs, bool& stop);

			void OnTimeoutTimer(uint64_t& baseTimeoutMs, bool& stop);

			/* Pure virtual methods inherited from BackoffTimerHandleInterface::Listener. */
		public:
			void OnTimer(BackoffTimerHandleInterface* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			AssociationListener& associationListener;
			const SctpOptions sctpOptions;
			SharedInterface* shared;
			TCBContext* tcbContext{ nullptr };
			// The time for a connection to be idle before a heartbeat is sent.
			const uint64_t intervalDurationMs{ 0 };
			// Adding RTT to the duration will add some jitter, which is good in
			// production, but less good in unit tests, which is why it can be disabled.
			const bool intervalDurationShouldIncludeRtt{ false };
			const std::unique_ptr<BackoffTimerHandleInterface> intervalTimer;
			const std::unique_ptr<BackoffTimerHandleInterface> timeoutTimer;
		};
	} // namespace SCTP
} // namespace RTC

#endif
