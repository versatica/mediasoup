#ifndef MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_CONTEXT_INTERFACE_HPP
#define MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_CONTEXT_INTERFACE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include <string_view>

namespace RTC
{
	namespace SCTP
	{
		class TransmissionControlBlockContextInterface
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * Called when the transmission error counter has exceeded its limit and
				 * the association must therefore be closed.
				 *
				 * @remarks
				 * - It mirrors dcsctp's `CloseConnectionBecauseOfTooManyTransmissionErrors()`.
				 */
				virtual void OnTransmissionControlBlockTooManyTxErrors() = 0;
			};

		public:
			virtual ~TransmissionControlBlockContextInterface() = default;

			/**
			 * Indicates if the SCTP association has been established.
			 */
			virtual bool IsAssociationEstablished() const = 0;

			/**
			 * The value of the Initiate Tag field the peer put in its INIT or
			 * INIT-ACK chunk.
			 */
			virtual uint32_t GetLocalInitialTsn() const = 0;

			/**
			 * The value of the Initial TSN field the peer put in its INIT or
			 * INIT-ACK chunk.
			 */
			virtual uint32_t GetRemoteInitialTsn() const = 0;

			/**
			 * To be called when a RTT (ms) has been measured, to update the RTO
			 * value.
			 */
			virtual void ObserveRttMs(uint64_t rttMs) = 0;

			/**
			 * Returns the Retransmission Timeout (RTO) value.
			 */
			virtual uint64_t GetCurrentRtoMs() const = 0;

			/**
			 * Increments the transmission error counter, given a human readable
			 * reason. Returns `false` if the maximum error count has been reached,
			 * `true` otherwise.
			 */
			virtual bool IncrementTxErrorCounter(std::string_view reason) = 0;

			/**
			 * Clears the transmission error counter.
			 */
			virtual void ClearTxErrorCounter() = 0;

			/**
			 * Returns true if there have been too many retransmission errors.
			 */
			virtual bool HasTooManyTxErrors() const = 0;

			virtual std::unique_ptr<Packet> CreatePacket() const = 0;

			/**
			 * Sends a packet and returns a boolean indicating whether the packet was
			 * sent or not.
			 */
			virtual bool SendPacket(Packet* packet) = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
