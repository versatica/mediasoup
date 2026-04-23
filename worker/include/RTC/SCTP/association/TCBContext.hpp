#ifndef MS_RTC_SCTP_TCB_CONTEXT_HPP
#define MS_RTC_SCTP_TCB_CONTEXT_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include <string_view>

namespace RTC
{
	namespace SCTP
	{
		class TCBContext
		{
		public:
			virtual ~TCBContext() = default;

			/**
			 * Indicates if the SCTP Association has been established.
			 */
			virtual bool IsAssociationEstablished() const = 0;

			/**
			 * The value of the Initiate Tag field the peer put in its INIT or
			 * INIT_ACK Chunk.
			 */
			virtual uint32_t GetLocalInitialTsn() const = 0;

			/**
			 * The value of the Initial TSN field the peer put in its INIT or
			 * INIT_ACK Chunk.
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
			 * reason.
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

			virtual void Send(Packet* packet) = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
