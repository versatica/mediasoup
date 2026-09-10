#ifndef MS_RTC_BWE_SEND_PACKET_HISTORY_HPP
#define MS_RTC_BWE_SEND_PACKET_HISTORY_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <map>
#include <tuple>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Holds every packet that was sent until a feedback reports on it.
		 *
		 * Packets are indexed by a sequence number of its own, handed out by this
		 * history, so that nothing about how a feedback identifies a packet leaks
		 * in here. Each packet also keeps its SSRC and RTP sequence number, which
		 * is what allows a feedback that identifies packets that way to be resolved
		 * against this very same history.
		 *
		 * It also tracks how many bytes are in flight, which are those that were
		 * sent and no feedback has reported on yet.
		 *
		 * @see https://datatracker.ietf.org/doc/html/draft-holmer-rmcat-transport-wide-cc-extensions
		 * @see https://datatracker.ietf.org/doc/html/rfc8888
		 */
		class SendPacketHistory
		{
		public:
			struct SendPacketHistoryOptions
			{
				/**
				 * Time a packet is kept for, beyond which a feedback reporting on it
				 * arrives too late to be of any use.
				 */
				int64_t windowDurationUs{ 60 * 1000 * 1000 };
			};

		private:
			struct SentPacketEntry
			{
				Types::SentPacket sentPacket;
				uint32_t ssrc{ 0 };
				uint16_t seq{ 0 };
			};

			struct SsrcAndSeq
			{
				bool operator<(const SsrcAndSeq& other) const
				{
					return std::tie(this->ssrc, this->seq) < std::tie(other.ssrc, other.seq);
				}

				uint32_t ssrc{ 0 };
				uint16_t seq{ 0 };
			};

		public:
			SendPacketHistory();

			explicit SendPacketHistory(SendPacketHistoryOptions options);

			/**
			 * Take note of a packet that has just been sent.
			 *
			 * @param size - Size of the packet including overhead up to the IP layer
			 *   (bytes).
			 * @param audio - Whether it's an audio packet. False for video, padding
			 *   and RTX.
			 * @returns The sequence number given to the packet, which is what a
			 *   feedback has to be resolved into.
			 */
			int64_t AddPacket(uint32_t ssrc, uint16_t seq, size_t size, bool audio, int64_t sentAtUs);

			/**
			 * Resolve what a feedback reported about a single packet.
			 *
			 * @param received - Whether the feedback reported the packet as received.
			 * @returns The packet as it was sent, or no value if this history doesn't
			 *   hold it anymore.
			 *
			 * @remarks
			 * - Every packet up to the given one stops counting as in flight, since
			 *   the feedback settles whether it arrived or not.
			 * - A packet reported as received is dropped, while one reported as lost
			 *   is kept, since a later feedback may still report it as received.
			 */
			std::optional<Types::SentPacket> RetrievePacket(int64_t sequenceNumber, bool received);

			/**
			 * Same, for a feedback that identifies the packet by its SSRC and RTP
			 * sequence number rather than by the sequence number given to it.
			 */
			std::optional<Types::SentPacket> RetrievePacket(uint32_t ssrc, uint16_t seq, bool received);

			/**
			 * Sequence number this history gave to the latest packet added, or no
			 * value if no packet was ever added.
			 *
			 * @remarks
			 * - It's the sequence number of this history, not the RTP one, so it
			 *   never wraps around. It's also the highest a feedback carrying only
			 *   the lowest bits of it can possibly stand for.
			 */
			std::optional<int64_t> GetLastSequenceNumber() const;

			/**
			 * Bytes that were sent and no feedback has reported on yet.
			 */
			size_t GetOutstandingBytes() const
			{
				return this->outstandingBytes;
			}

		private:
			void RemoveOutstandingBytes(const SentPacketEntry& entry);

			/**
			 * Drop the mapping of a packet, unless the same SSRC and RTP sequence
			 * number were sent again afterwards and hence took it over.
			 */
			void RemoveSsrcAndSeq(const SentPacketEntry& entry);

		private:
			const SendPacketHistoryOptions options;
			std::map<int64_t, SentPacketEntry> history;
			std::map<SsrcAndSeq, int64_t> ssrcAndSeqToSequenceNumber;
			int64_t nextSequenceNumber{ 0 };
			// Sequence numbers are never negative, so this sits below all of them.
			int64_t lastAckSequenceNumber{ -1 };
			size_t outstandingBytes{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
