#ifndef MS_RTC_NEW_RTCP_TRANSPORT_WIDE_CC_PACKET_HPP
#define MS_RTC_NEW_RTCP_TRANSPORT_WIDE_CC_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"
#include "Utils.hpp"
#include <array>
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Transport-wide Congestion Control Feedback Packet (RTPFB 205, FMT 15).
		 *
		 * @see draft-holmer-rmcat-transport-wide-cc-extensions-01.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|  FMT=15 |  PT=RTPFB=205 |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      SSRC of media source                     |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |      base sequence number     |      packet status count      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                 reference time                | fb pkt. count |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          packet chunk         |         packet chunk          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :                              ...                              :
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         packet chunk          |  recv delta   |  recv delta   |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :                              ...                              :
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Base sequence number (16 bits): The transport-wide sequence number
		 *   of the first RTP packet in this feedback.
		 * - Packet status count (16 bits): The number of RTP packets this
		 *   feedback contains status for, starting with the RTP packet identified
		 *   by the base sequence number.
		 * - Reference time (24 bits): Signed integer indicating an absolute
		 *   reference time in some (unknown) time base chosen by the sender of the
		 *   feedback, in multiples of 64 ms.
		 * - Feedback packet count (8 bits): A counter incremented by one for each
		 *   feedback packet sent.
		 * - Packet chunk (16 bits each): Run Length Chunk, One Bit Status Vector
		 *   Chunk or Two Bit Status Vector Chunk, with the status of the RTP
		 *   packets.
		 * - Receive delta (8 or 16 bits each): For each received RTP packet, the
		 *   delta to the previous received RTP packet (or to the reference time
		 *   for the first one), in multiples of 250 us.
		 *
		 * @remarks
		 * - Receive deltas may be omitted (when the media sender requests
		 *   feedback without timing information). See `HasDeltas()`.
		 */
		class TransportWideCcPacket : public AnyTransportLayerFeedbackPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Receive deltas are expressed in multiples of this (in microseconds).
			 */
			static constexpr int64_t DeltaTickUs{ 250 };
			/**
			 * Maximum number of RTP packets a feedback packet can contain status
			 * for.
			 */
			static constexpr size_t MaxReportedRtpPackets{ 0xFFFF };
			/**
			 * Length (in bytes) of the fixed header that precedes the packet
			 * chunks, RTCP Common Header included.
			 */
			static constexpr size_t TransportWideCcHeaderLength{ 20 };
			/**
			 * The reference time is expressed in multiples of this (in
			 * microseconds).
			 */
			static constexpr int64_t BaseTimeTickUs{ TransportWideCcPacket::DeltaTickUs * (1 << 8) };
			/**
			 * Period (in microseconds) after which the reference time wraps around.
			 */
			static constexpr int64_t TimeWrapPeriodUs{ TransportWideCcPacket::BaseTimeTickUs * (1 << 24) };

		private:
			/**
			 * Length (in bytes) of a packet chunk.
			 */
			static constexpr size_t ChunkLength{ 2 };
			/**
			 * Maximum length (in bytes) of the packet.
			 */
			static constexpr size_t MaxLength{ (1 << 16) * 4 };

		public:
			/**
			 * Received RTP packet.
			 */
			struct ReceivedRtpPacket
			{
				/**
				 * Transport-wide sequence number.
				 */
				uint16_t sequenceNumber;
				/**
				 * Receive delta in multiples of `DeltaTickUs`. It is 0 if the packet
				 * has no receive deltas.
				 */
				int16_t deltaTicks;
			};

		private:
			/**
			 * Number of bytes of the receive delta of an RTP packet status: 0 (RTP
			 * packet not received), 1 (RTP packet received, small delta) or 2 (RTP
			 * packet received, large or negative delta).
			 */
			using DeltaSize = uint8_t;

		private:
			/**
			 * Last packet chunk of the packet, which is the one where new RTP packet
			 * statuses are added.
			 */
			class LastChunk
			{
			public:
				/**
				 * Maximum number of RTP packet statuses in a Run Length Chunk.
				 */
				static constexpr size_t MaxRunLengthCapacity{ 0x1FFF };

			private:
				/**
				 * Maximum number of RTP packet statuses in a One Bit Status Vector
				 * Chunk.
				 */
				static constexpr size_t MaxOneBitCapacity{ 14 };
				/**
				 * Maximum number of RTP packet statuses in a Two Bit Status Vector
				 * Chunk.
				 */
				static constexpr size_t MaxTwoBitCapacity{ 7 };
				/**
				 * Maximum number of RTP packet statuses in a Status Vector Chunk.
				 */
				static constexpr size_t MaxVectorCapacity{ LastChunk::MaxOneBitCapacity };
				/**
				 * Delta size of an RTP packet received with a large or negative delta.
				 */
				static constexpr DeltaSize Large{ 2 };

			public:
				bool IsEmpty() const;

				void Clear();

				bool CanAdd(DeltaSize deltaSize) const;

				void Add(DeltaSize deltaSize);

				void AddMissingRtpPackets(size_t numMissing);

				/**
				 * Encode as many RTP packet statuses as fit in a packet chunk, remove
				 * them from this last chunk and return the encoded packet chunk.
				 */
				uint16_t Emit();

				/**
				 * Encode all RTP packet statuses into a packet chunk without removing
				 * them from this last chunk.
				 */
				uint16_t EncodeLast() const;

				void Decode(uint16_t chunk, size_t maxSize);

				/**
				 * Append the delta sizes of the RTP packet statuses of this last chunk
				 * to the given `deltaSizes`.
				 */
				void AppendTo(std::vector<DeltaSize>& deltaSizes) const;

			private:
				uint16_t EncodeOneBit() const;

				void DecodeOneBit(uint16_t chunk, size_t maxSize);

				uint16_t EncodeTwoBit(size_t count) const;

				void DecodeTwoBit(uint16_t chunk, size_t maxSize);

				uint16_t EncodeRunLength() const;

				void DecodeRunLength(uint16_t chunk, size_t maxSize);

			private:
				std::array<DeltaSize, LastChunk::MaxVectorCapacity> deltaSizes{};
				size_t size{ 0 };
				bool allSame{ true };
				bool hasLargeDelta{ false };
			};

		public:
			/**
			 * Parse a TransportWideCcPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static TransportWideCcPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a TransportWideCcPacket.
			 *
			 * @param buffer
			 * @param bufferLength - Could be greater than the packet real length.
			 * @param includeDeltas - Whether receive deltas are included in the
			 *   packet.
			 *
			 * @remarks
			 * - The packet is created with no RTP packet statuses. `SetBase()` must
			 *   be called before adding RTP packets with `AddReceivedRtpPacket()`.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than
			 *   `TransportWideCcHeaderLength`.
			 */
			static TransportWideCcPacket* Factory(uint8_t* buffer, size_t bufferLength, bool includeDeltas);

		private:
			/**
			 * Parse a TransportWideCcPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static TransportWideCcPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			TransportWideCcPacket(uint8_t* buffer, size_t bufferLength, bool hasDeltas);

		public:
			~TransportWideCcPacket() override;

			void Dump(int indentation = 0) const final;

			TransportWideCcPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			/**
			 * Set the base sequence number and the reference time.
			 *
			 * @param baseSequenceNumber - Transport-wide sequence number of the first
			 *   RTP packet in this feedback.
			 * @param referenceTimestampUs - Reference time (in microseconds).
			 */
			void SetBase(uint16_t baseSequenceNumber, int64_t referenceTimestampUs);

			/**
			 * Value of the Feedback packet count field.
			 */
			uint8_t GetFeedbackPacketCount() const
			{
				return Utils::Byte::Get1Byte(GetBuffer(), 19);
			}

			void SetFeedbackPacketCount(uint8_t count);

			/**
			 * Add the status of a received RTP packet, and of the RTP packets not
			 * received since the last added one.
			 *
			 * @param sequenceNumber - Transport-wide sequence number of the received
			 *   RTP packet.
			 * @param timestampUs - Time (in microseconds) at which the RTP packet was
			 *   received.
			 *
			 * @returns false if the RTP packet cannot be added (its sequence number
			 *   is not higher than the last added one, its receive delta cannot be
			 *   represented or there is no space for it). In that case a new
			 *   feedback packet is needed.
			 */
			[[nodiscard]] bool AddReceivedRtpPacket(uint16_t sequenceNumber, int64_t timestampUs);

			/**
			 * Received RTP packets, in transport-wide sequence number order.
			 */
			std::vector<ReceivedRtpPacket> GetReceivedRtpPackets() const;

			/**
			 * Call given `handler` for every RTP packet status in the packet, with
			 * the transport-wide sequence number and the receive time (in
			 * microseconds) relative to the reference time, or `std::nullopt` if the
			 * RTP packet was not received.
			 */
			void ForAllRtpPackets(
			  const std::function<void(uint16_t sequenceNumber, std::optional<int64_t> deltaSinceBaseUs)>&
			    handler) const;

			/**
			 * Value of the Base sequence number field.
			 */
			uint16_t GetBaseSequenceNumber() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 12);
			}

			/**
			 * Value of the Packet status count field.
			 */
			uint16_t GetPacketStatusCount() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 14);
			}

			/**
			 * Reference time (in microseconds).
			 *
			 * @remarks
			 * - It includes an extra `TimeWrapPeriodUs` so that RTP packets received
			 *   earlier than the reference time can be represented even if the
			 *   value of the Reference time field is 0.
			 */
			int64_t GetBaseTimeUs() const;

			/**
			 * Difference (in microseconds) between the reference time of this packet
			 * and given `previousTimestampUs`, compensating for the wrap around of
			 * the Reference time field.
			 */
			int64_t GetBaseDeltaUs(int64_t previousTimestampUs) const;

			/**
			 * Whether the packet includes receive deltas.
			 */
			bool HasDeltas() const
			{
				return this->hasDeltas;
			}

		private:
			TransportWideCcPacket* SoftClone(const uint8_t* buffer) const final;

			void SoftCloneInto(Packet* packet) const final;

			uint8_t* GetChunksPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + TransportWideCcPacket::TransportWideCcHeaderLength;
			}

			void SetPacketStatusCount(uint16_t count);

			/**
			 * Decode the packet chunks into the delta size of each RTP packet
			 * status.
			 */
			void DecodeChunks(std::vector<DeltaSize>& deltaSizes) const;

			/**
			 * Whether a packet with given `contentLength` (excluding padding) fits
			 * in the maximum packet length and in the buffer.
			 */
			bool HasSpaceFor(size_t contentLength) const;

			/**
			 * Make room for `numberOfChunks` packet chunks right after the existing
			 * ones by moving the receive deltas forward, and update the length of the
			 * packet.
			 *
			 * @remarks
			 * - The new packet chunks are not written, the caller must write them.
			 * - The caller must check that there is space for them.
			 */
			void InsertChunks(size_t numberOfChunks);

			[[nodiscard]] bool AddDeltaSize(DeltaSize deltaSize);

			[[nodiscard]] bool AddMissingRtpPackets(size_t numMissingRtpPackets);

		private:
			// Whether the packet includes receive deltas.
			const bool hasDeltas;
			// Length (in bytes) of the packet chunks in the buffer.
			size_t chunksLength{ 0 };
			// Length (in bytes) of the receive deltas in the buffer.
			size_t deltasLength{ 0 };
			// Time (in microseconds) of the last added RTP packet.
			int64_t lastTimestampUs{ 0 };
			// Last packet chunk, also encoded in the buffer.
			LastChunk lastChunk;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
