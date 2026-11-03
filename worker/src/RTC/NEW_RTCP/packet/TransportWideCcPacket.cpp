#define MS_CLASS "RTC::NEW_RTCP::TransportWideCcPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/TransportWideCcPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SeqManager.hpp"
#include <cstdlib> // std::abs()
#include <cstring> // std::memmove()

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		TransportWideCcPacket* TransportWideCcPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::RTPFB)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			const auto messageType =
			  static_cast<AnyTransportLayerFeedbackPacket::MessageType>(buffer[0] & 0x1F);

			if (messageType != AnyTransportLayerFeedbackPacket::MessageType::TCC)
			{
				MS_WARN_DEV("invalid message type");

				return nullptr;
			}

			return TransportWideCcPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		TransportWideCcPacket* TransportWideCcPacket::Factory(
		  uint8_t* buffer, size_t bufferLength, bool includeDeltas)
		{
			MS_TRACE();

			if (bufferLength < TransportWideCcPacket::TransportWideCcHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new TransportWideCcPacket(buffer, bufferLength, includeDeltas);

			packet->InitializeHeader(
			  Packet::PacketType::RTPFB,
			  static_cast<uint8_t>(AnyTransportLayerFeedbackPacket::MessageType::TCC),
			  TransportWideCcPacket::TransportWideCcHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSenderSsrc(0);
			packet->SetMediaSsrc(0);
			Utils::Byte::Set2Bytes(buffer, 12, 0);
			packet->SetPacketStatusCount(0);
			Utils::Byte::Set3Bytes(buffer, 16, 0);
			packet->SetFeedbackPacketCount(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum TransportWideCcPacket length.

			return packet;
		}

		TransportWideCcPacket* TransportWideCcPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			if (contentLength < TransportWideCcPacket::TransportWideCcHeaderLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "TransportWideCcPacket length must be equal or greater than %zu",
				  TransportWideCcPacket::TransportWideCcHeaderLength);

				return nullptr;
			}

			const uint16_t packetStatusCount = Utils::Byte::Get2Bytes(buffer, 14);

			std::vector<DeltaSize> deltaSizes;
			LastChunk lastChunk;
			size_t offset{ TransportWideCcPacket::TransportWideCcHeaderLength };

			deltaSizes.reserve(packetStatusCount);

			while (deltaSizes.size() < packetStatusCount)
			{
				if (offset + TransportWideCcPacket::ChunkLength > contentLength)
				{
					MS_WARN_TAG(rtcp, "no space for packet chunks");

					return nullptr;
				}

				const uint16_t chunk = Utils::Byte::Get2Bytes(buffer, offset);

				offset += TransportWideCcPacket::ChunkLength;

				lastChunk.Decode(chunk, packetStatusCount - deltaSizes.size());
				lastChunk.AppendTo(deltaSizes);
			}

			size_t deltasLength{ 0 };

			for (const DeltaSize deltaSize : deltaSizes)
			{
				deltasLength += deltaSize;
			}

			// Receive deltas are included if there is space for all of them.
			const bool hasDeltas = contentLength >= offset + deltasLength;

			int64_t deltaTicksSum{ 0 };

			if (hasDeltas)
			{
				for (const DeltaSize deltaSize : deltaSizes)
				{
					switch (deltaSize)
					{
						case 0:
						{
							break;
						}

						case 1:
						{
							deltaTicksSum += Utils::Byte::Get1Byte(buffer, offset);
							offset += deltaSize;

							break;
						}

						case 2:
						{
							deltaTicksSum += static_cast<int16_t>(Utils::Byte::Get2Bytes(buffer, offset));
							offset += deltaSize;

							break;
						}

						default:
						{
							MS_WARN_TAG(rtcp, "invalid delta size %" PRIu8, deltaSize);

							return nullptr;
						}
					}
				}
			}

			auto* const packet =
			  new TransportWideCcPacket(const_cast<uint8_t*>(buffer), bufferLength, hasDeltas);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			packet->lastTimestampUs =
			  packet->GetBaseTimeUs() + (deltaTicksSum * TransportWideCcPacket::DeltaTickUs);
			packet->lastChunk = lastChunk;

			return packet;
		}

		/* Instance methods. */

		TransportWideCcPacket::TransportWideCcPacket(uint8_t* buffer, size_t bufferLength, bool hasDeltas)
		  : AnyTransportLayerFeedbackPacket(buffer, bufferLength), hasDeltas(hasDeltas)
		{
			MS_TRACE();

			SetLength(TransportWideCcPacket::TransportWideCcHeaderLength);
		}

		TransportWideCcPacket::~TransportWideCcPacket()
		{
			MS_TRACE();
		}

		void TransportWideCcPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::TransportWideCcPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  message type: %" PRIu8 " (%s)",
			  static_cast<uint8_t>(GetMessageType()),
			  AnyTransportLayerFeedbackPacket::MessageTypeToString(GetMessageType()).c_str());
			DumpFeedbackCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  base sequence number: %" PRIu16, GetBaseSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  packet status count: %" PRIu16, GetPacketStatusCount());
			MS_DUMP_CLEAN(indentation, "  base time: %" PRIi64 " us", GetBaseTimeUs());
			MS_DUMP_CLEAN(indentation, "  feedback packet count: %" PRIu8, GetFeedbackPacketCount());
			MS_DUMP_CLEAN(indentation, "  has deltas: %s", HasDeltas() ? "yes" : "no");
			for (const auto& receivedRtpPacket : GetReceivedRtpPackets())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - received RTP packet: seq: %" PRIu16 ", delta ticks: %" PRIi16,
				  receivedRtpPacket.sequenceNumber,
				  receivedRtpPacket.deltaTicks);
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::TransportWideCcPacket>");
		}

		TransportWideCcPacket* TransportWideCcPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new TransportWideCcPacket(buffer, bufferLength, this->hasDeltas);

			CloneInto(clonedPacket);
			SoftCloneInto(clonedPacket);

			return clonedPacket;
		}

		void TransportWideCcPacket::SetBase(uint16_t baseSequenceNumber, int64_t referenceTimestampUs)
		{
			MS_TRACE();

			MS_ASSERT(GetPacketStatusCount() == 0, "cannot set base once RTP packets have been added");

			auto* const buffer = const_cast<uint8_t*>(GetBuffer());

			Utils::Byte::Set2Bytes(buffer, 12, baseSequenceNumber);
			Utils::Byte::Set3Bytes(
			  buffer,
			  16,
			  static_cast<uint32_t>(
			    (referenceTimestampUs % TransportWideCcPacket::TimeWrapPeriodUs) /
			    TransportWideCcPacket::BaseTimeTickUs));

			this->lastTimestampUs = GetBaseTimeUs();
		}

		void TransportWideCcPacket::SetFeedbackPacketCount(uint8_t count)
		{
			MS_TRACE();

			Utils::Byte::Set1Byte(const_cast<uint8_t*>(GetBuffer()), 19, count);
		}

		bool TransportWideCcPacket::AddReceivedRtpPacket(uint16_t sequenceNumber, int64_t timestampUs)
		{
			MS_TRACE();

			// Set delta to zero if receive deltas are not included, this simplifies
			// the encoding process.
			int16_t delta{ 0 };

			if (this->hasDeltas)
			{
				int64_t timestamp = timestampUs;

				// Convert to ticks and round.
				if (this->lastTimestampUs > timestamp)
				{
					const int64_t diff = this->lastTimestampUs - timestamp;

					// Round up to a multiple of the wrap period.
					timestamp += ((diff + TransportWideCcPacket::TimeWrapPeriodUs - 1) /
					              TransportWideCcPacket::TimeWrapPeriodUs) *
					             TransportWideCcPacket::TimeWrapPeriodUs;
				}

				MS_ASSERT(timestamp >= this->lastTimestampUs, "timestamp lower than last timestamp");

				int64_t deltaFull =
				  (timestamp - this->lastTimestampUs) % TransportWideCcPacket::TimeWrapPeriodUs;

				if (deltaFull > TransportWideCcPacket::TimeWrapPeriodUs / 2)
				{
					deltaFull -= TransportWideCcPacket::TimeWrapPeriodUs;
					deltaFull -= TransportWideCcPacket::DeltaTickUs / 2;
				}
				else
				{
					deltaFull += TransportWideCcPacket::DeltaTickUs / 2;
				}

				deltaFull /= TransportWideCcPacket::DeltaTickUs;

				delta = static_cast<int16_t>(deltaFull);

				// If larger than 16 bits signed, it cannot be represented, so a new
				// feedback packet is needed.
				if (delta != deltaFull)
				{
					MS_WARN_TAG(rtcp, "delta value too large (>= 2^16 ticks)");

					return false;
				}
			}

			const auto nextSequenceNumber =
			  static_cast<uint16_t>(GetBaseSequenceNumber() + GetPacketStatusCount());

			if (sequenceNumber != nextSequenceNumber)
			{
				const auto lastSequenceNumber = static_cast<uint16_t>(nextSequenceNumber - 1);

				if (!RTC::SeqManager<uint16_t>::IsSeqHigherThan(sequenceNumber, lastSequenceNumber))
				{
					return false;
				}

				const auto numMissingRtpPackets = static_cast<uint16_t>(sequenceNumber - nextSequenceNumber);

				if (!AddMissingRtpPackets(numMissingRtpPackets))
				{
					return false;
				}
			}

			const DeltaSize deltaSize = (delta >= 0 && delta <= 0xFF) ? 1 : 2;

			if (!AddDeltaSize(deltaSize))
			{
				return false;
			}

			if (this->hasDeltas)
			{
				size_t chunksLength;
				size_t deltasLength;

				// NOTE: The computed receive deltas length already includes the delta
				// of this RTP packet.
				GetChunksAndDeltasLength(chunksLength, deltasLength);

				auto* const ptr = GetChunksPointer() + chunksLength + deltasLength - deltaSize;

				if (deltaSize == 1)
				{
					Utils::Byte::Set1Byte(ptr, 0, static_cast<uint8_t>(delta));
				}
				else
				{
					Utils::Byte::Set2Bytes(ptr, 0, static_cast<uint16_t>(delta));
				}

				SetVariableLengthValueLength(
				  TransportWideCcPacket::TransportWideCcHeaderLength - Packet::CommonHeaderLength +
				  chunksLength + deltasLength);
			}

			this->lastTimestampUs += delta * TransportWideCcPacket::DeltaTickUs;

			return true;
		}

		std::vector<TransportWideCcPacket::ReceivedRtpPacket> TransportWideCcPacket::GetReceivedRtpPackets() const
		{
			MS_TRACE();

			std::vector<DeltaSize> deltaSizes;
			const size_t chunksLength       = DecodeChunks(deltaSizes);
			const auto* const deltasPointer = GetChunksPointer() + chunksLength;

			uint16_t sequenceNumber = GetBaseSequenceNumber();
			size_t offset{ 0 };
			std::vector<ReceivedRtpPacket> receivedRtpPackets;

			for (const DeltaSize deltaSize : deltaSizes)
			{
				if (deltaSize > 0)
				{
					int16_t deltaTicks{ 0 };

					if (this->hasDeltas)
					{
						if (deltaSize == 1)
						{
							deltaTicks = Utils::Byte::Get1Byte(deltasPointer, offset);
						}
						else
						{
							deltaTicks = static_cast<int16_t>(Utils::Byte::Get2Bytes(deltasPointer, offset));
						}

						offset += deltaSize;
					}

					receivedRtpPackets.push_back({ .sequenceNumber = sequenceNumber, .deltaTicks = deltaTicks });
				}

				++sequenceNumber;
			}

			return receivedRtpPackets;
		}

		void TransportWideCcPacket::ForAllRtpPackets(
		  const std::function<void(uint16_t sequenceNumber, std::optional<int64_t> deltaSinceBaseUs)>&
		    handler) const
		{
			MS_TRACE();

			const auto receivedRtpPackets = GetReceivedRtpPackets();
			const auto endSequenceNumber =
			  static_cast<uint16_t>(GetBaseSequenceNumber() + GetPacketStatusCount());

			int64_t deltaSinceBaseUs{ 0 };
			size_t receivedIdx{ 0 };

			for (uint16_t sequenceNumber = GetBaseSequenceNumber(); sequenceNumber != endSequenceNumber;
			     ++sequenceNumber)
			{
				if (receivedIdx < receivedRtpPackets.size() && receivedRtpPackets[receivedIdx].sequenceNumber == sequenceNumber)
				{
					const auto& receivedRtpPacket = receivedRtpPackets[receivedIdx];

					deltaSinceBaseUs += receivedRtpPacket.deltaTicks * TransportWideCcPacket::DeltaTickUs;

					handler(sequenceNumber, deltaSinceBaseUs);

					++receivedIdx;
				}
				else
				{
					handler(sequenceNumber, std::nullopt);
				}
			}

			MS_ASSERT(receivedIdx == receivedRtpPackets.size(), "not all received RTP packets were handled");
		}

		int64_t TransportWideCcPacket::GetBaseTimeUs() const
		{
			MS_TRACE();

			return TransportWideCcPacket::TimeWrapPeriodUs +
			       (static_cast<int64_t>(Utils::Byte::Get3Bytes(GetBuffer(), 16)) *
			        TransportWideCcPacket::BaseTimeTickUs);
		}

		int64_t TransportWideCcPacket::GetBaseDeltaUs(int64_t previousTimestampUs) const
		{
			MS_TRACE();

			int64_t delta = GetBaseTimeUs() - previousTimestampUs;

			// Compensate for wrap around.
			if (std::abs(delta - TransportWideCcPacket::TimeWrapPeriodUs) < std::abs(delta))
			{
				// Wrap backwards.
				delta -= TransportWideCcPacket::TimeWrapPeriodUs;
			}
			else if (std::abs(delta + TransportWideCcPacket::TimeWrapPeriodUs) < std::abs(delta))
			{
				// Wrap forwards.
				delta += TransportWideCcPacket::TimeWrapPeriodUs;
			}

			return delta;
		}

		TransportWideCcPacket* TransportWideCcPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket =
			  new TransportWideCcPacket(const_cast<uint8_t*>(buffer), GetLength(), this->hasDeltas);

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}

		void TransportWideCcPacket::SoftCloneInto(Packet* packet) const
		{
			MS_TRACE();

			auto* const transportWideCcPacket = static_cast<TransportWideCcPacket*>(packet);

			transportWideCcPacket->lastTimestampUs = this->lastTimestampUs;
			transportWideCcPacket->lastChunk       = this->lastChunk;

			Packet::SoftCloneInto(packet);
		}

		void TransportWideCcPacket::SetPacketStatusCount(uint16_t count)
		{
			MS_TRACE();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 14, count);
		}

		size_t TransportWideCcPacket::DecodeChunks(std::vector<DeltaSize>& deltaSizes) const
		{
			MS_TRACE();

			const auto* const chunksPointer  = GetChunksPointer();
			const uint16_t packetStatusCount = GetPacketStatusCount();

			LastChunk decoder;
			size_t chunksLength{ 0 };

			deltaSizes.reserve(packetStatusCount);

			while (deltaSizes.size() < packetStatusCount)
			{
				const uint16_t chunk = Utils::Byte::Get2Bytes(chunksPointer, chunksLength);

				chunksLength += TransportWideCcPacket::ChunkLength;

				decoder.Decode(chunk, packetStatusCount - deltaSizes.size());
				decoder.AppendTo(deltaSizes);
			}

			return chunksLength;
		}

		void TransportWideCcPacket::GetChunksAndDeltasLength(size_t& chunksLength, size_t& deltasLength) const
		{
			MS_TRACE();

			std::vector<DeltaSize> deltaSizes;

			chunksLength = DecodeChunks(deltaSizes);
			deltasLength = 0;

			if (!this->hasDeltas)
			{
				return;
			}

			for (const DeltaSize deltaSize : deltaSizes)
			{
				deltasLength += deltaSize;
			}
		}

		bool TransportWideCcPacket::HasSpaceFor(size_t contentLength) const
		{
			MS_TRACE();

			return (
			  contentLength <= TransportWideCcPacket::MaxLength &&
			  Utils::Byte::PadTo4Bytes(contentLength) <= GetBufferLength());
		}

		void TransportWideCcPacket::InsertChunks(
		  size_t chunksLength, size_t deltasLength, size_t numberOfChunks)
		{
			MS_TRACE();

			auto* const chunksPointer = GetChunksPointer();

			std::memmove(
			  chunksPointer + chunksLength + (numberOfChunks * TransportWideCcPacket::ChunkLength),
			  chunksPointer + chunksLength,
			  deltasLength);
		}

		bool TransportWideCcPacket::AddDeltaSize(DeltaSize deltaSize)
		{
			MS_TRACE();

			const uint16_t packetStatusCount = GetPacketStatusCount();

			if (packetStatusCount == TransportWideCcPacket::MaxReportedRtpPackets)
			{
				return false;
			}

			size_t chunksLength;
			size_t deltasLength;

			GetChunksAndDeltasLength(chunksLength, deltasLength);

			const size_t contentLength =
			  TransportWideCcPacket::TransportWideCcHeaderLength + chunksLength + deltasLength;
			const size_t addChunkLength =
			  this->lastChunk.IsEmpty() ? TransportWideCcPacket::ChunkLength : 0;
			auto* const chunksPointer = GetChunksPointer();

			if (!HasSpaceFor(contentLength + deltaSize + addChunkLength))
			{
				return false;
			}

			if (this->lastChunk.CanAdd(deltaSize))
			{
				// Open a slot for the last chunk if it was empty.
				if (addChunkLength > 0)
				{
					InsertChunks(chunksLength, deltasLength, 1);
				}

				this->lastChunk.Add(deltaSize);

				Utils::Byte::Set2Bytes(
				  chunksPointer,
				  chunksLength + addChunkLength - TransportWideCcPacket::ChunkLength,
				  this->lastChunk.EncodeLast());

				SetPacketStatusCount(static_cast<uint16_t>(packetStatusCount + 1));
				SetVariableLengthValueLength(contentLength + addChunkLength - Packet::CommonHeaderLength);

				return true;
			}

			if (!HasSpaceFor(contentLength + deltaSize + TransportWideCcPacket::ChunkLength))
			{
				return false;
			}

			// The last chunk is full, so emit it into its slot and open a slot for a
			// new one.
			Utils::Byte::Set2Bytes(
			  chunksPointer, chunksLength - TransportWideCcPacket::ChunkLength, this->lastChunk.Emit());

			InsertChunks(chunksLength, deltasLength, 1);

			this->lastChunk.Add(deltaSize);

			Utils::Byte::Set2Bytes(chunksPointer, chunksLength, this->lastChunk.EncodeLast());

			SetPacketStatusCount(static_cast<uint16_t>(packetStatusCount + 1));
			SetVariableLengthValueLength(
			  contentLength + TransportWideCcPacket::ChunkLength - Packet::CommonHeaderLength);

			return true;
		}

		bool TransportWideCcPacket::AddMissingRtpPackets(size_t numMissingRtpPackets)
		{
			MS_TRACE();

			const size_t newPacketStatusCount = GetPacketStatusCount() + numMissingRtpPackets;

			if (newPacketStatusCount > TransportWideCcPacket::MaxReportedRtpPackets)
			{
				return false;
			}

			size_t chunksLength;
			size_t deltasLength;

			GetChunksAndDeltasLength(chunksLength, deltasLength);

			const size_t contentLength =
			  TransportWideCcPacket::TransportWideCcHeaderLength + chunksLength + deltasLength;
			auto* const chunksPointer = GetChunksPointer();

			if (!this->lastChunk.IsEmpty())
			{
				while (numMissingRtpPackets > 0 && this->lastChunk.CanAdd(0))
				{
					this->lastChunk.Add(0);

					--numMissingRtpPackets;
				}

				if (numMissingRtpPackets == 0)
				{
					Utils::Byte::Set2Bytes(
					  chunksPointer,
					  chunksLength - TransportWideCcPacket::ChunkLength,
					  this->lastChunk.EncodeLast());

					SetPacketStatusCount(static_cast<uint16_t>(newPacketStatusCount));

					return true;
				}

				Utils::Byte::Set2Bytes(
				  chunksPointer, chunksLength - TransportWideCcPacket::ChunkLength, this->lastChunk.Emit());
			}

			MS_ASSERT(this->lastChunk.IsEmpty(), "last chunk is not empty");

			const size_t fullChunks   = numMissingRtpPackets / LastChunk::MaxRunLengthCapacity;
			const size_t partialChunk = numMissingRtpPackets % LastChunk::MaxRunLengthCapacity;
			const size_t numChunks    = fullChunks + (partialChunk > 0 ? 1 : 0);

			if (!HasSpaceFor(contentLength + (TransportWideCcPacket::ChunkLength * numChunks)))
			{
				SetPacketStatusCount(static_cast<uint16_t>(newPacketStatusCount - numMissingRtpPackets));

				return false;
			}

			InsertChunks(chunksLength, deltasLength, numChunks);

			// T = 0, S = 0, run length = MaxRunLengthCapacity, see EncodeRunLength().
			for (size_t idx{ 0 }; idx < fullChunks; ++idx)
			{
				Utils::Byte::Set2Bytes(
				  chunksPointer,
				  chunksLength + (idx * TransportWideCcPacket::ChunkLength),
				  LastChunk::MaxRunLengthCapacity);
			}

			this->lastChunk.AddMissingRtpPackets(partialChunk);

			if (partialChunk > 0)
			{
				Utils::Byte::Set2Bytes(
				  chunksPointer,
				  chunksLength + (fullChunks * TransportWideCcPacket::ChunkLength),
				  this->lastChunk.EncodeLast());
			}

			SetPacketStatusCount(static_cast<uint16_t>(newPacketStatusCount));
			SetVariableLengthValueLength(
			  contentLength + (TransportWideCcPacket::ChunkLength * numChunks) - Packet::CommonHeaderLength);

			return true;
		}

		/* LastChunk instance methods. */

		bool TransportWideCcPacket::LastChunk::IsEmpty() const
		{
			MS_TRACE();

			return this->size == 0;
		}

		void TransportWideCcPacket::LastChunk::Clear()
		{
			MS_TRACE();

			this->size          = 0;
			this->allSame       = true;
			this->hasLargeDelta = false;
		}

		bool TransportWideCcPacket::LastChunk::CanAdd(DeltaSize deltaSize) const
		{
			MS_TRACE();

			MS_ASSERT(deltaSize <= 2, "invalid delta size");

			if (this->size < LastChunk::MaxTwoBitCapacity)
			{
				return true;
			}

			if (this->size < LastChunk::MaxOneBitCapacity && !this->hasLargeDelta && deltaSize != LastChunk::Large)
			{
				return true;
			}

			if (this->size < LastChunk::MaxRunLengthCapacity && this->allSame && this->deltaSizes[0] == deltaSize)
			{
				return true;
			}

			return false;
		}

		void TransportWideCcPacket::LastChunk::Add(DeltaSize deltaSize)
		{
			MS_TRACE();

			MS_ASSERT(CanAdd(deltaSize), "cannot add delta size");

			if (this->size < LastChunk::MaxVectorCapacity)
			{
				this->deltaSizes[this->size] = deltaSize;
			}

			++this->size;

			this->allSame       = this->allSame && deltaSize == this->deltaSizes[0];
			this->hasLargeDelta = this->hasLargeDelta || deltaSize == LastChunk::Large;
		}

		void TransportWideCcPacket::LastChunk::AddMissingRtpPackets(size_t numMissing)
		{
			MS_TRACE();

			MS_ASSERT(this->size == 0, "last chunk is not empty");
			MS_ASSERT(this->allSame, "last chunk delta sizes are not all the same");
			MS_ASSERT(!this->hasLargeDelta, "last chunk has large delta");
			MS_ASSERT(numMissing < LastChunk::MaxRunLengthCapacity, "too many missing RTP packets");

			std::ranges::fill(this->deltaSizes, DeltaSize{ 0 });

			this->size = numMissing;
		}

		uint16_t TransportWideCcPacket::LastChunk::Emit()
		{
			MS_TRACE();

			MS_ASSERT(!CanAdd(0) || !CanAdd(1) || !CanAdd(2), "last chunk is not full");

			if (this->allSame)
			{
				const uint16_t chunk = EncodeRunLength();

				Clear();

				return chunk;
			}

			if (this->size == LastChunk::MaxOneBitCapacity)
			{
				const uint16_t chunk = EncodeOneBit();

				Clear();

				return chunk;
			}

			MS_ASSERT(this->size >= LastChunk::MaxTwoBitCapacity, "last chunk too small");

			const uint16_t chunk = EncodeTwoBit(LastChunk::MaxTwoBitCapacity);

			// Remove `MaxTwoBitCapacity` encoded delta sizes. Shift remaining delta
			// sizes and recalculate `allSame` and `hasLargeDelta`.
			this->size -= LastChunk::MaxTwoBitCapacity;
			this->allSame       = true;
			this->hasLargeDelta = false;

			for (size_t idx{ 0 }; idx < this->size; ++idx)
			{
				const DeltaSize deltaSize = this->deltaSizes[LastChunk::MaxTwoBitCapacity + idx];

				this->deltaSizes[idx] = deltaSize;
				this->allSame         = this->allSame && deltaSize == this->deltaSizes[0];
				this->hasLargeDelta   = this->hasLargeDelta || deltaSize == LastChunk::Large;
			}

			return chunk;
		}

		uint16_t TransportWideCcPacket::LastChunk::EncodeLast() const
		{
			MS_TRACE();

			MS_ASSERT(this->size > 0, "last chunk is empty");

			if (this->allSame)
			{
				return EncodeRunLength();
			}

			if (this->size <= LastChunk::MaxTwoBitCapacity)
			{
				return EncodeTwoBit(this->size);
			}

			return EncodeOneBit();
		}

		void TransportWideCcPacket::LastChunk::Decode(uint16_t chunk, size_t maxSize)
		{
			MS_TRACE();

			if ((chunk & 0x8000) == 0)
			{
				DecodeRunLength(chunk, maxSize);
			}
			else if ((chunk & 0x4000) == 0)
			{
				DecodeOneBit(chunk, maxSize);
			}
			else
			{
				DecodeTwoBit(chunk, maxSize);
			}
		}

		void TransportWideCcPacket::LastChunk::AppendTo(std::vector<DeltaSize>& deltaSizes) const
		{
			MS_TRACE();

			if (this->allSame)
			{
				deltaSizes.insert(deltaSizes.end(), this->size, this->deltaSizes[0]);
			}
			else
			{
				deltaSizes.insert(
				  deltaSizes.end(),
				  this->deltaSizes.begin(),
				  this->deltaSizes.begin() + static_cast<std::ptrdiff_t>(this->size));
			}
		}

		/**
		 * One Bit Status Vector Chunk.
		 *
		 *  0                   1
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |T|S|       symbol list         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - T = 1.
		 * - S = 0.
		 * - Symbol list = 14 entries where 0 = not received, 1 = received with
		 *   1-byte delta.
		 */
		uint16_t TransportWideCcPacket::LastChunk::EncodeOneBit() const
		{
			MS_TRACE();

			MS_ASSERT(!this->hasLargeDelta, "last chunk has large delta");
			MS_ASSERT(this->size <= LastChunk::MaxOneBitCapacity, "last chunk too big");

			uint16_t chunk{ 0x8000 };

			for (size_t idx{ 0 }; idx < this->size; ++idx)
			{
				chunk |= this->deltaSizes[idx] << (LastChunk::MaxOneBitCapacity - 1 - idx);
			}

			return chunk;
		}

		void TransportWideCcPacket::LastChunk::DecodeOneBit(uint16_t chunk, size_t maxSize)
		{
			MS_TRACE();

			MS_ASSERT((chunk & 0xC000) == 0x8000, "not a One Bit Status Vector Chunk");

			this->size          = std::min(LastChunk::MaxOneBitCapacity, maxSize);
			this->hasLargeDelta = false;
			this->allSame       = false;

			for (size_t idx{ 0 }; idx < this->size; ++idx)
			{
				this->deltaSizes[idx] = (chunk >> (LastChunk::MaxOneBitCapacity - 1 - idx)) & 0x01;
			}
		}

		/**
		 * Two Bit Status Vector Chunk.
		 *
		 *  0                   1
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |T|S|       symbol list         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - T = 1.
		 * - S = 1.
		 * - Symbol list = 7 entries of two bits each.
		 */
		uint16_t TransportWideCcPacket::LastChunk::EncodeTwoBit(size_t count) const
		{
			MS_TRACE();

			MS_ASSERT(count <= this->size, "count greater than last chunk size");

			uint16_t chunk{ 0xC000 };

			for (size_t idx{ 0 }; idx < count; ++idx)
			{
				chunk |= this->deltaSizes[idx] << (2 * (LastChunk::MaxTwoBitCapacity - 1 - idx));
			}

			return chunk;
		}

		void TransportWideCcPacket::LastChunk::DecodeTwoBit(uint16_t chunk, size_t maxSize)
		{
			MS_TRACE();

			MS_ASSERT((chunk & 0xC000) == 0xC000, "not a Two Bit Status Vector Chunk");

			this->size          = std::min(LastChunk::MaxTwoBitCapacity, maxSize);
			this->hasLargeDelta = true;
			this->allSame       = false;

			for (size_t idx{ 0 }; idx < this->size; ++idx)
			{
				this->deltaSizes[idx] = (chunk >> (2 * (LastChunk::MaxTwoBitCapacity - 1 - idx))) & 0x03;
			}
		}

		/**
		 * Run Length Chunk.
		 *
		 *  0                   1
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |T| S |       Run Length        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - T = 0.
		 * - S = symbol.
		 * - Run Length = Unsigned integer denoting the run length of the symbol.
		 */
		uint16_t TransportWideCcPacket::LastChunk::EncodeRunLength() const
		{
			MS_TRACE();

			MS_ASSERT(this->allSame, "last chunk delta sizes are not all the same");
			MS_ASSERT(this->size <= LastChunk::MaxRunLengthCapacity, "last chunk too big");

			return static_cast<uint16_t>((this->deltaSizes[0] << 13) | this->size);
		}

		void TransportWideCcPacket::LastChunk::DecodeRunLength(uint16_t chunk, size_t maxSize)
		{
			MS_TRACE();

			MS_ASSERT((chunk & 0x8000) == 0, "not a Run Length Chunk");

			this->size = std::min<size_t>(chunk & 0x1FFF, maxSize);

			const DeltaSize deltaSize = (chunk >> 13) & 0x03;

			this->hasLargeDelta = deltaSize >= LastChunk::Large;
			this->allSame       = true;

			// To make it consistent with Add(), populate delta sizes beyond the
			// first one.
			for (size_t idx{ 0 }; idx < std::min<size_t>(this->size, LastChunk::MaxVectorCapacity); ++idx)
			{
				this->deltaSizes[idx] = deltaSize;
			}
		}
	} // namespace NEW_RTCP
} // namespace RTC
