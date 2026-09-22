#ifndef MS_RTC_BWE_SEND_PACKET_HISTORY_HPP
#define MS_RTC_BWE_SEND_PACKET_HISTORY_HPP

#include "common.hpp"
#include "Utils/UnwrappedSequenceNumber.hpp"
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

			/**
			 * Everything about a packet that this history has to be told, since none
			 * of it can be read back from the bytes that go on the wire.
			 */
			struct AddPacketOptions
			{
				uint32_t ssrc{ 0 };
				/**
				 * RTP sequence number of the packet, which is what a feedback that
				 * doesn't carry the one above identifies it by.
				 */
				uint16_t seq{ 0 };
				/**
				 * Size of the packet including the overhead up to the IP layer (bytes).
				 */
				size_t size{ 0 };
				/**
				 * Whether it's an audio packet. False for video, padding and RTX.
				 */
				bool isAudio{ false };
				bool isRetransmission{ false };
				/**
				 * SSRC of the packet a retransmission carries, which is the SSRC above
				 * itself when it goes without RTX. No value when the packet isn't a
				 * retransmission.
				 */
				std::optional<uint32_t> originalSsrc;
				/**
				 * Whether the packet is marked as ECT(1), which asks the network to
				 * report congestion instead of dropping the packet.
				 *
				 * @see https://www.rfc-editor.org/rfc/rfc9331.html
				 */
				bool sentWithEct1{ false };
				/**
				 * The burst the packet belongs to, or no value if it isn't part of one.
				 */
				std::optional<Types::ProbeCluster> probeCluster;
				/**
				 * Time at which the packet is taken note of, which is not yet the time
				 * at which it was sent.
				 */
				int64_t createdAtUs{ 0 };
			};

			/**
			 * A packet as this history holds it, which is what it was sent with plus
			 * what identifies it on the wire.
			 */
			struct SentPacketEntry
			{
				Types::SentPacket sentPacket;
				/**
				 * Time at which the packet was taken note of, which is what makes it
				 * fall out of the window. It comes before the packet was sent, and by
				 * an unbounded amount when the socket cannot take the datagram right
				 * away.
				 */
				int64_t createdAtUs{ 0 };
				uint32_t ssrc{ 0 };
				uint16_t seq{ 0 };
				bool isRetransmission{ false };
				/**
				 * Whether what a feedback reports as the arrival time of this packet
				 * cannot be told from that of another one, which happens when the very
				 * same SSRC and RTP sequence number were sent more than once.
				 *
				 * @remark
				 * - Nothing in this estimator reads it. It is only of use to SCReAM,
				 *   another way of estimating the bandwidth, which discards the delay
				 *   of a packet it cannot tell apart.
				 *
				 * @link https://datatracker.ietf.org/doc/draft-johansson-ccwg-rfc8298bis-screamv2
				 */
				bool ambiguousReceiveTime{ false };
				/**
				 * Whether the packet was sent marked as ECT(1), which asks the network
				 * to report congestion instead of dropping the packet.
				 *
				 * @remark
				 * - Only a feedback that reports ECN carries it back, which is the one
				 *   of RFC 8888 and not the transport-cc one.
				 *
				 * @see https://datatracker.ietf.org/doc/html/rfc8888
				 * @see https://www.rfc-editor.org/rfc/rfc9331.html
				 */
				bool sentWithEct1{ false };
				/**
				 * Whether a feedback already reported this packet as lost, which is
				 * what tells a first loss from the same loss reported again.
				 *
				 * @remark
				 * - Only a feedback that reports on a whole range of packets makes use
				 *   of it, which is the one of RFC 8888 and not the transport-cc one.
				 *
				 * @see https://datatracker.ietf.org/doc/html/rfc8888
				 */
				bool previouslyReportedLost{ false };
			};

		private:
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
			 * Take note of a packet that is about to be sent.
			 *
			 * @returns The sequence number given to the packet, whose lowest 16 bits
			 *   are what has to go into the transport-wide sequence number RTP header
			 *   extension, and which is what a feedback resolves into.
			 */
			int64_t AddPacket(const AddPacketOptions& options);

			/**
			 * Recover the sequence number this history gave a packet out of the
			 * lowest 16 bits of it that the wire carries.
			 *
			 * @remarks
			 * - Each number is resolved against the previous one resolved, be it of a
			 *   packet taken note of or of another feedback, which is what carries the
			 *   count across a turn of the wire's 16 bits. So one further than half
			 *   that range away from the previous cannot be told apart from the same
			 *   number a turn later.
			 */
			int64_t UnwrapSequenceNumber(uint16_t wrappedSequenceNumber);

			/**
			 * Take note that the packet with the given sequence number has left.
			 *
			 * @returns The packet as it was sent, or no value if this history doesn't
			 *   hold it anymore or it had already left.
			 *
			 * @remarks
			 * - This is what makes the packet count as in flight, and what settles how
			 *   many bytes were in flight when it left.
			 * - A packet sent under a sequence number that already left is not taken
			 *   note of twice, so that its bytes are not counted twice either.
			 */
			std::optional<Types::SentPacket> ProcessSentPacket(int64_t sequenceNumber, int64_t sentAtUs);

			/**
			 * Take note of bytes that left without being tracked one by one, which is
			 * what packets carrying no sequence number of this history amount to.
			 *
			 * @remarks
			 * - They are held until the next tracked packet leaves and are attributed
			 *   to it, so that whoever measures the acknowledged bitrate doesn't
			 *   believe the link carried less than it did.
			 */
			void ProcessSentUntrackedPacket(size_t size, int64_t sentAtUs);

			/**
			 * Resolve what a feedback reported about a single packet.
			 *
			 * @param received - Whether the feedback reported the packet as received.
			 * @returns The packet as this history holds it, or no value if it doesn't
			 *   hold it anymore or it was never reported as sent.
			 *
			 * @remarks
			 * - Every packet up to the given one stops counting as in flight, since
			 *   the feedback settles whether it arrived or not.
			 * - A packet reported as received is dropped, while one reported as lost
			 *   is kept, since a later feedback may still report it as received.
			 */
			std::optional<SentPacketEntry> RetrievePacket(int64_t sequenceNumber, bool received);

			/**
			 * Same, for a feedback that identifies the packet by its SSRC and RTP
			 * sequence number rather than by the sequence number given to it.
			 */
			std::optional<SentPacketEntry> RetrievePacket(uint32_t ssrc, uint16_t seq, bool received);

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
			// Hands out the sequence numbers and recovers them out of the lowest bits
			// the wire carries, which keeps both to a single count.
			Utils::UnwrappedSequenceNumber<uint16_t>::Unwrapper sequenceNumberUnwrapper;
			uint16_t nextWrappedSequenceNumber{ 0 };
			// Highest sequence number ever taken note of, or no value if none was.
			std::optional<int64_t> highestSequenceNumber;
			// Sequence numbers are never negative, so this sits below all of them.
			int64_t lastAckSequenceNumber{ -1 };
			size_t outstandingBytes{ 0 };
			// Bytes that left untracked and no packet has been attributed yet.
			size_t pendingUntrackedBytes{ 0 };
			// Time at which the latest tracked packet left, or no value if none has.
			std::optional<int64_t> lastSendTimeUs;
			// Time at which the latest untracked bytes left, or no value if none
			// have.
			std::optional<int64_t> lastUntrackedSendTimeUs;
		};
	} // namespace BWE
} // namespace RTC

#endif
