#define MS_CLASS "RTC::RTCP::FeedbackRtpTransportPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Static members. */

		size_t FeedbackRtpTransportPacket::fixedHeaderSize{ 8u };
		uint16_t FeedbackRtpTransportPacket::maxMissingPackets{ (1 << 13) - 1 };
		uint16_t FeedbackRtpTransportPacket::maxPacketStatusCount{ (1 << 16) - 1 };
		uint16_t FeedbackRtpTransportPacket::maxPacketDelta{ (1 << 16) - 1 };

		/* Class methods. */

		FeedbackRtpTransportPacket* FeedbackRtpTransportPacket::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) + FeedbackRtpTransportPacket::fixedHeaderSize > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for FeedbackRtpTransportPacket packet, discarded");

				return nullptr;
			}

			auto* commonHeader = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			std::unique_ptr<FeedbackRtpTransportPacket> packet(new FeedbackRtpTransportPacket(commonHeader));

			return packet.release();
		}

		/* Instance methods. */

		FeedbackRtpTransportPacket::FeedbackRtpTransportPacket(CommonHeader* commonHeader)
		  : FeedbackRtpPacket(commonHeader)
		{
			MS_TRACE();

			// TODO: uncomment.
			// size_t len = static_cast<size_t>(ntohs(commonHeader->length) + 1) * 4;

			// Make data point to the packet specific info.
			auto* data = reinterpret_cast<uint8_t*>(commonHeader) + sizeof(CommonHeader) +
			             sizeof(FeedbackPacket::Header);

			this->baseSequenceNumber  = Utils::Byte::Get2Bytes(data, 0);
			this->packetStatusCount   = Utils::Byte::Get2Bytes(data, 2);
			this->referenceTimeMs     = Utils::Byte::Get3Bytes(data, 4);
			this->feedbackPacketCount = Utils::Byte::Get1Byte(data, 7);

			// TODO: Parse.
		}

		bool FeedbackRtpTransportPacket::AddPacket(
		  uint16_t wideSeqNumber, uint64_t timestamp, size_t maxRtcpPacketLen)
		{
			MS_TRACE();

			MS_ASSERT(!IsFull(), "packet is full");

			uint16_t delta = 0;

			// Let's see if we must update our pre base.
			if (!hasPreBase)
			{
				MS_DEBUG_DEV("setting pre base");

				this->hasPreBase            = true;
				this->preBaseSequenceNumber = wideSeqNumber;
				this->preReferenceTimeMs    = timestamp;

				return true;
			}
			// Ensure this can be the base. Update pre base otherwise.
			// clang-format off
			else if (
				this->receivedPackets.empty() &&
				wideSeqNumber != this->preBaseSequenceNumber + 1
			)
			// clang-format on
			{
				MS_WARN_DEV("not valid as base, resetting pre base");

				this->preBaseSequenceNumber = wideSeqNumber;
				this->preReferenceTimeMs    = timestamp;

				return true;
			}
			// This is the base (but let's see).
			else if (this->receivedPackets.empty())
			{
				// Not a valid base. Use it as pre base.
				if (!CheckDelta(this->preReferenceTimeMs, timestamp))
				{
					MS_WARN_DEV(
					  "RTP packet delta exceeded, not valid as base, resetting pre base [preReferenceTimeMs:%" PRIu64
					  ", timestamp:%" PRIu64 "]",
					  this->preReferenceTimeMs,
					  timestamp);

					this->preBaseSequenceNumber = wideSeqNumber;
					this->preReferenceTimeMs    = timestamp;

					return true;
				}

				MS_DEBUG_DEV("setting base");

				this->baseSequenceNumber = wideSeqNumber;
				this->referenceTimeMs    = timestamp;

				delta = (timestamp - this->preReferenceTimeMs) * 1000 / 250;

				FillChunk(this->preBaseSequenceNumber, wideSeqNumber, delta);
			}
			else
			{
				auto lastSequenceNumber = this->receivedPackets.back().sequenceNumber;

				// If the wide sequence number of the new packet is lower than the highest seen,
				// ignore it.
				// NOTE: Not very spec compliant but libwebrtc does it.
				if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(wideSeqNumber, lastSequenceNumber))
					return true;

				if (!CheckMissingPackets(lastSequenceNumber, wideSeqNumber))
				{
					MS_WARN_DEV("RTP missing packet number exceeded");

					return false;
				}

				if (!CheckDelta(this->lastTimestamp, timestamp))
				{
					MS_WARN_DEV(
					  "RTP packet delta exceeded [lastTimestamp:%" PRIu64 ", timestamp:%" PRIu64 "]",
					  this->lastTimestamp,
					  timestamp);

					return false;
				}

				if (!CheckSize(maxRtcpPacketLen))
				{
					MS_WARN_DEV("maximum packet size exceeded");

					return false;
				}

				// TODO: This is really necessary?
				if (this->lastTimestamp == timestamp)
				{
					delta = 0u;
				}
				// Deltas are represented as multiples of 250us.
				else
				{
					delta = (timestamp - this->lastTimestamp) * 1000 / 250;
				}

				uint16_t previousSequenceNumber = this->receivedPackets.back().sequenceNumber;

				FillChunk(previousSequenceNumber, wideSeqNumber, delta);
			}

			// Store last timestamp.
			this->lastTimestamp = timestamp;

			// Add entry to received packets container.
			this->receivedPackets.emplace_back(wideSeqNumber, delta);

			return true;
		}

		size_t FeedbackRtpTransportPacket::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Leave the necessary offset for the common header.
			size_t offset = 12;

			Utils::Byte::Set2Bytes(buffer, offset, this->baseSequenceNumber);
			offset += 2;

			// We don't know the packet status count yet.
			size_t packetStatusCountOffset = offset;

			offset += 2;

			// Reference time is represented in multiples of 64ms.
			auto referenceTime = (this->referenceTimeMs / 64) & 0xFFFFFF;

			Utils::Byte::Set3Bytes(buffer, offset, referenceTime);
			offset += 3;

			Utils::Byte::Set1Byte(buffer, offset, this->feedbackPacketCount);
			offset += 1;

			// Check if there are pending status packets.
			if (this->context.statuses.size() > 0)
			{
				if (this->context.allSameStatus)
				{
					auto chunk =
					  CreateRunLengthChunk(this->context.currentStatus, this->context.statuses.size());

					this->chunks.push_back(chunk);
					this->packetStatusCount += this->context.statuses.size();
					this->size += sizeof(uint16_t);
					this->context.statuses.clear();
				}
				else
				{
					Status currentStatus = this->context.statuses.front();
					size_t count         = 0;

					for (auto status : this->context.statuses)
					{
						if (status == currentStatus)
						{
							count++;
						}
						else
						{
							auto chunk = CreateRunLengthChunk(currentStatus, count);

							this->chunks.push_back(chunk);
							this->packetStatusCount += count;
							this->size += sizeof(uint16_t);

							currentStatus = status;
							count         = 1;
						}
					}

					auto chunk = CreateRunLengthChunk(currentStatus, count);

					this->chunks.push_back(chunk);
					this->packetStatusCount += count;
					this->size += sizeof(uint16_t);
					this->context.statuses.clear();
				}
			}

			// Now we have the final size, serialize the common header.
			FeedbackPacket::Serialize(buffer);

			// Serialize chunks.
			for (auto* chunk : this->chunks)
			{
				offset += chunk->Serialize(buffer + offset);
			}

			// Serialize deltas.
			for (auto delta : this->deltas)
			{
				if (delta <= 255)
				{
					std::memcpy(buffer + offset, &delta, sizeof(uint8_t));
					offset += sizeof(uint8_t);
				}
				else
				{
					std::memcpy(buffer + offset, &delta, sizeof(uint16_t));
					offset += sizeof(uint16_t);
				}
			}

			// Write the packet count.
			Utils::Byte::Set2Bytes(buffer, packetStatusCountOffset, this->packetStatusCount);

			// 32 bits padding.
			size_t padding = (-offset) & 3;

			for (size_t i{ 0u }; i < padding; ++i)
			{
				buffer[offset + i] = 0u;
			}

			offset += padding;

			return offset;
		}

		void FeedbackRtpTransportPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<FeedbackRtpTransportPacket>");
			MS_DUMP("  pre base sequence     : %" PRIu16, this->preBaseSequenceNumber);
			MS_DUMP("  base sequence         : %" PRIu16, this->baseSequenceNumber);
			MS_DUMP("  packet status count   : %" PRIu16, this->packetStatusCount);
			MS_DUMP("  reference time        : %" PRIu64, this->referenceTimeMs);
			MS_DUMP("  feedback packet count : %" PRIu8, this->feedbackPacketCount);
			MS_DUMP("  size                  : %zu", GetSize());
			MS_DUMP("</FeedbackRtpTransportPacket>");
		}

		void FeedbackRtpTransportPacket::FillChunk(
		  uint16_t previousSequenceNumber, uint16_t sequenceNumber, uint16_t delta)
		{
			MS_TRACE();

			auto missingPackets = static_cast<uint16_t>(sequenceNumber - (previousSequenceNumber + 1));
			Status status;

			// MS_DEBUG_DEV(
			//   "[sequenceNumber:%" PRIu16 ", delta:%" PRIu16 ", missingPackets:%" PRIu16
			//   ", packetStatusCount:%" PRIu16 "]",
			//   sequenceNumber,
			//   delta,
			//   missingPackets,
			//   this->packetStatusCount);

			if (missingPackets > 0)
			{
				status = Status::NotReceived;

				// Create a long run chunk before processing this packet, if needed.
				// clang-format off
				if (
					this->context.statuses.size() >= 7 &&
					this->context.allSameStatus
					&& status != this->context.currentStatus
				)
				// clang-format on
				{
					this->context.currentStatus = Status::None;

					auto chunk =
					  CreateRunLengthChunk(this->context.currentStatus, this->context.statuses.size());

					this->chunks.push_back(chunk);
					this->packetStatusCount += this->context.statuses.size();
					this->size += sizeof(uint16_t);
					this->context.statuses.clear();
				}

				size_t representedPackets = 0;

				// Fill status vector.
				for (uint8_t i{ 0u }; i < missingPackets && this->context.statuses.size() < 7; ++i)
				{
					this->context.statuses.emplace_back(Status::NotReceived);
					representedPackets++;
				}

				missingPackets -= representedPackets;

				// Not all missing packets have been represented.
				if (missingPackets != 0)
				{
					// Fill a vector chunk.
					auto vectorChunk = CreateTwoBitVectorChunk(this->context.statuses);

					this->chunks.push_back(vectorChunk);
					this->packetStatusCount += 7;
					this->size += sizeof(uint16_t);
					this->context.statuses.clear();

					// Fill a run length chunk with the remaining missing packets.
					auto runLengthChunk = CreateRunLengthChunk(Status::NotReceived, missingPackets);

					this->chunks.push_back(runLengthChunk);
					this->packetStatusCount += missingPackets;
					this->size += sizeof(uint16_t);
					this->context.statuses.clear();
					this->context.currentStatus = Status::None;
				}
			}
			else
			{
				status = (delta <= 255) ? Status::SmallDelta : Status::LargeDelta;

				// Create a long run chunk before processing this packet, if needed.
				// clang-format off
				if (
					this->context.statuses.size() >= 7 &&
					this->context.allSameStatus &&
					status != this->context.currentStatus
				)
				// clang-format on
				{
					auto chunk =
					  CreateRunLengthChunk(this->context.currentStatus, this->context.statuses.size());

					this->chunks.push_back(chunk);
					this->packetStatusCount += this->context.statuses.size();
					this->size += sizeof(uint16_t);
					this->context.statuses.clear();
					this->context.currentStatus = Status::None;
				}

				this->context.statuses.emplace_back(status);
				this->deltas.emplace_back(delta);
				this->size += (status == Status::SmallDelta) ? sizeof(uint8_t) : sizeof(uint16_t);
			}

			// Update context info.
			this->context.allSameStatus = (this->context.currentStatus == this->context.allSameStatus);

			// Not enough packet infos for creating a chunk.
			if (this->context.statuses.size() < 7)
			{
				return;
			}
			// 7 packet infos with heterogeneous status, create the chunk.
			else if (this->context.statuses.size() == 7 && !this->context.allSameStatus)
			{
				// Reset current status.
				this->context.currentStatus = Status::None;

				// Fill a vector chunk and return.
				auto chunk = CreateTwoBitVectorChunk(this->context.statuses);

				this->chunks.push_back(chunk);
				this->packetStatusCount += 7;
				this->size += sizeof(uint16_t);
				this->context.statuses.clear();
			}
		}

		bool FeedbackRtpTransportPacket::CheckMissingPackets(
		  uint16_t previousSequenceNumber, uint16_t nextSecuenceNumber)
		{
			MS_TRACE();

			// Check missing packet limits.
			auto missingPackets = nextSecuenceNumber - (previousSequenceNumber + 1);

			// Check if there are too many missing packets.
			return (missingPackets <= FeedbackRtpTransportPacket::maxMissingPackets);
		}

		bool FeedbackRtpTransportPacket::CheckDelta(uint16_t previousTimestamp, uint16_t nextTimestamp)
		{
			MS_TRACE();

			// Delta since last received RTP packet in milliseconds.
			auto deltaMs = nextTimestamp - previousTimestamp;

			// Deltas are represented as multiples of 250us.
			auto delta = deltaMs * 1000 / 250;

			// Check if there is too much delta since previous RTP packet.
			return (delta <= FeedbackRtpTransportPacket::maxPacketDelta);
		}

		// Check whether another chunks and corresponding delta infos could be added.
		bool FeedbackRtpTransportPacket::CheckSize(size_t maxRtcpPacketLen)
		{
			MS_TRACE();

			auto size = GetSize();

			// Maximum size needed for another chunk and its delta infos.
			size += sizeof(uint16_t);
			size += sizeof(uint16_t) * 7;

			// 32 bits padding.
			size += (-size) & 3;

			return (size <= maxRtcpPacketLen);
		}

		FeedbackRtpTransportPacket::RunLengthChunk::RunLengthChunk(uint16_t buffer)
		{
			MS_TRACE();

			MS_ASSERT(buffer & 0x8000, "invalid run length chunk");

			this->status = static_cast<Status>((buffer >> 13) & 0x03);
			this->count  = buffer & 0x1FFF;
		}

		size_t FeedbackRtpTransportPacket::RunLengthChunk::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			uint16_t bytes{ 0x0000 };

			bytes |= this->status << 13;
			bytes |= this->count & 0x1FFF;

			Utils::Byte::Set2Bytes(buffer, 0, bytes);

			return sizeof(uint16_t);
		}

		FeedbackRtpTransportPacket::TwoBitVectorChunk::TwoBitVectorChunk(uint16_t buffer)
		{
			MS_TRACE();

			MS_ASSERT(buffer & 0xC000, "Invalid two bit vector chunk");

			for (size_t i{ 0u }; i < 7; ++i)
			{
				auto status = static_cast<Status>((buffer >> 2 * (7 - 1 - i)) & 0x03);

				this->statuses.emplace_back(status);
			}
		}

		size_t FeedbackRtpTransportPacket::TwoBitVectorChunk::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			MS_ASSERT(this->statuses.size() == 7, "packet info size must be 7");

			uint16_t bytes{ 0x8000 };

			bytes |= 0x01 << 14;

			uint8_t i{ 12u };

			for (auto status : this->statuses)
			{
				bytes |= status << i;
				i -= 2;
			}

			Utils::Byte::Set2Bytes(buffer, 0, bytes);

			return sizeof(uint16_t);
		}
	} // namespace RTCP
} // namespace RTC
