#define MS_CLASS "RTC::RTCP::FeedbackRtpTransport"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/SeqManager.hpp"
#include <sstream>

namespace RTC
{
	namespace RTCP
	{
		/* Static members. */

		size_t FeedbackRtpTransportPacket::fixedHeaderSize{ 8u };
		uint16_t FeedbackRtpTransportPacket::maxMissingPackets{ (1 << 13) - 1 };
		uint16_t FeedbackRtpTransportPacket::maxPacketStatusCount{ (1 << 16) - 1 };
		uint16_t FeedbackRtpTransportPacket::maxPacketDelta{ 0x7FFF };

		std::map<FeedbackRtpTransportPacket::Status, std::string> FeedbackRtpTransportPacket::Status2String = {
			{ FeedbackRtpTransportPacket::Status::NotReceived, "NR" },
			{ FeedbackRtpTransportPacket::Status::SmallDelta, "SD" },
			{ FeedbackRtpTransportPacket::Status::LargeDelta, "LD" }
		};

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
			this->referenceTime       = static_cast<int32_t>(Utils::Byte::Get3Bytes(data, 4));
			this->feedbackPacketCount = Utils::Byte::Get1Byte(data, 7);

			// TODO: Parse.
		}

		FeedbackRtpTransportPacket::~FeedbackRtpTransportPacket()
		{
			MS_TRACE();

			for (auto* chunk : this->chunks)
			{
				delete chunk;
			}
			this->chunks.clear();
		}

		bool FeedbackRtpTransportPacket::AddPacket(
		  uint16_t sequenceNumber, uint64_t timestamp, size_t maxRtcpPacketLen)
		{
			MS_TRACE();

			MS_ASSERT(!IsFull(), "packet is full");

			// Let's see if we must set our base.
			if (this->highestTimestamp == 0u)
			{
				MS_DEBUG_DEV("setting base");

				this->baseSequenceNumber    = sequenceNumber + 1;
				this->referenceTime         = static_cast<int32_t>((timestamp & 0x1FFFFFC0) / 64);
				this->highestSequenceNumber = sequenceNumber;
				this->highestTimestamp      = (timestamp >> 6) * 64; // IMPORTANT: Loose precision.

				return true;
			}

			// If the wide sequence number of the new packet is lower than the highest seen,
			// ignore it.
			// NOTE: Not very spec compliant but libwebrtc does it.
			// Also ignore if the sequence number matches the highest seen.
			if (!RTC::SeqManager<uint16_t>::IsSeqHigherThan(sequenceNumber, this->highestSequenceNumber))
			{
				return true;
			}

			// Check if there are too many missing packets.
			{
				auto missingPackets = sequenceNumber - (this->highestSequenceNumber + 1);

				if (missingPackets > FeedbackRtpTransportPacket::maxMissingPackets)
				{
					MS_WARN_DEV("RTP missing packet number exceeded");

					return false;
				}
			}

			// Deltas are represented as multiples of 250us.
			// NOTE: Read it as uint 64 to detect long elapsed times.
			uint64_t delta64 = (timestamp - this->highestTimestamp) * 4;

			if (delta64 > FeedbackRtpTransportPacket::maxPacketDelta)
			{
				MS_WARN_DEV(
				  "RTP packet delta exceeded [highestTimestamp:%" PRIu64 ", timestamp:%" PRIu64 "]",
				  this->highestTimestamp,
				  timestamp);

				return false;
			}

			// Delta in 16 bits.
			auto delta = static_cast<uint16_t>(delta64);

			// Check whether another chunks and corresponding delta infos could be added.
			{
				auto size = GetSize();

				// Maximum size needed for another chunk and its delta infos.
				size += sizeof(uint16_t);
				size += sizeof(uint16_t) * 7;

				// 32 bits padding.
				size += (-size) & 3;

				if (size > maxRtcpPacketLen)
				{
					MS_WARN_DEV("maximum packet size exceeded");

					return false;
				}
			}

			// Fill a chunk.
			FillChunk(this->highestSequenceNumber, sequenceNumber, delta);

			// Update highest sequence number and timestamp seen.
			this->highestSequenceNumber = sequenceNumber;
			this->highestTimestamp      = timestamp;

			// Add entry to received packets container.
			this->receivedPackets.emplace_back(sequenceNumber, delta);

			return true;
		}

		void FeedbackRtpTransportPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<FeedbackRtpTransportPacket>");
			MS_DUMP("  base sequence         : %" PRIu16, this->baseSequenceNumber);
			MS_DUMP("  packet status count   : %" PRIu16, this->packetStatusCount);
			MS_DUMP("  reference time        : %" PRIi32, this->referenceTime);
			MS_DUMP("  feedback packet count : %" PRIu8, this->feedbackPacketCount);
			MS_DUMP("  size                  : %zu", GetSize());

			for (auto* chunk : this->chunks)
			{
				chunk->Dump();
			}

			if (this->receivedPackets.size() != this->deltas.size())
			{
				MS_ERROR(
				  "received packets and number of deltas mismatch [packets:%zu, deltas:%zu]",
				  this->receivedPackets.size(),
				  this->deltas.size());

				for (auto& packet : this->receivedPackets)
				{
					MS_DUMP("  seqNumber:%d, delta(ms):%d", packet.sequenceNumber, packet.delta / 4);
				}
			}
			else
			{
				auto receivedPacketIt = this->receivedPackets.begin();
				auto deltaIt          = this->deltas.begin();

				MS_DUMP("  <Deltas>");

				while (receivedPacketIt != this->receivedPackets.end())
				{
					MS_DUMP(
					  "    seqNumber:%" PRIu16 ", delta(ms):%" PRIu16,
					  receivedPacketIt->sequenceNumber,
					  static_cast<uint16_t>(receivedPacketIt->delta / 4));

					if (receivedPacketIt->delta != *deltaIt)
						MS_ERROR("delta block does not coincide with the received value");

					receivedPacketIt++;
					deltaIt++;
				}

				MS_DUMP("  </Deltas>");
			}

			MS_DUMP("</FeedbackRtpTransportPacket>");
		}

		size_t FeedbackRtpTransportPacket::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Add chunks for status packets that may not be represented yet.
			AddPendingChunks();

			size_t offset = FeedbackPacket::Serialize(buffer);

			// Base sequence number.
			Utils::Byte::Set2Bytes(buffer, offset, this->baseSequenceNumber);
			offset += 2;

			// Packet status count.
			Utils::Byte::Set2Bytes(buffer, offset, this->packetStatusCount);
			offset += 2;

			// Reference time.
			Utils::Byte::Set3Bytes(buffer, offset, static_cast<uint32_t>(this->referenceTime));
			offset += 3;

			// Feedback packet count.
			Utils::Byte::Set1Byte(buffer, offset, this->feedbackPacketCount);
			offset += 1;

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
					Utils::Byte::Set1Byte(buffer, offset, delta);
					offset += sizeof(uint8_t);
				}
				else
				{
					Utils::Byte::Set2Bytes(buffer, offset, delta);
					offset += sizeof(uint16_t);
				}
			}

			// 32 bits padding.
			size_t padding = (-offset) & 3;

			for (size_t i{ 0u }; i < padding; ++i)
			{
				buffer[offset + i] = 0u;
			}

			offset += padding;

			return offset;
		}

		void FeedbackRtpTransportPacket::FillChunk(
		  uint16_t previousSequenceNumber, uint16_t sequenceNumber, uint16_t delta)
		{
			MS_TRACE();

			auto missingPackets = static_cast<uint16_t>(sequenceNumber - (previousSequenceNumber + 1));

			// MS_DEBUG_DEV(
			//   "[sequenceNumber:%" PRIu16 ", delta:%" PRIu16 ", missingPackets:%" PRIu16
			//   ", packetStatusCount:%" PRIu16 "]",
			//   sequenceNumber,
			//   delta,
			//   missingPackets,
			//   this->packetStatusCount);

			if (missingPackets > 0)
			{
				// Create a long run chunk before processing this packet, if needed.
				if (this->context.statuses.size() >= 7 && this->context.allSameStatus)
				{
					CreateRunLengthChunk(this->context.currentStatus, this->context.statuses.size());

					this->context.statuses.clear();
					this->context.currentStatus = Status::None;
				}

				this->context.currentStatus = Status::NotReceived;
				size_t representedPackets{ 0u };

				// Fill statuses vector.
				for (uint8_t i{ 0u }; i < missingPackets && this->context.statuses.size() < 7; ++i)
				{
					this->context.statuses.emplace_back(Status::NotReceived);
					representedPackets++;
				}

				// Create a two bit vector if needed.
				if (this->context.statuses.size() == 7)
				{
					// Fill a vector chunk.
					CreateTwoBitVectorChunk(this->context.statuses);

					this->context.statuses.clear();
					this->context.currentStatus = Status::None;
				}

				missingPackets -= representedPackets;

				// Not all missing packets have been represented.
				if (missingPackets != 0)
				{
					// Fill a run length chunk with the remaining missing packets.
					CreateRunLengthChunk(Status::NotReceived, missingPackets);

					this->context.statuses.clear();
					this->context.currentStatus = Status::None;
				}
			}

			Status status = (delta <= 255) ? Status::SmallDelta : Status::LargeDelta;

			// Create a long run chunk before processing this packet, if needed.
			// clang-format off
			if (
				this->context.statuses.size() >= 7 &&
				this->context.allSameStatus &&
				status != this->context.currentStatus
			)
			// clang-format on
			{
				CreateRunLengthChunk(this->context.currentStatus, this->context.statuses.size());

				this->context.statuses.clear();
			}

			this->context.statuses.emplace_back(status);
			this->deltas.emplace_back(delta);
			this->size += (status == Status::SmallDelta) ? sizeof(uint8_t) : sizeof(uint16_t);

			// Update context info.

			// clang-format off
			if (
				this->context.currentStatus == Status::None ||
				(this->context.allSameStatus && this->context.currentStatus == status)
			)
			// clang-format on
			{
				this->context.allSameStatus = true;
			}
			else
			{
				this->context.allSameStatus = false;
			}

			this->context.currentStatus = status;

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
				CreateTwoBitVectorChunk(this->context.statuses);

				this->context.statuses.clear();
			}
		}

		void FeedbackRtpTransportPacket::CreateRunLengthChunk(Status status, uint16_t count)
		{
			auto* chunk = new RunLengthChunk(status, count);

			this->chunks.push_back(chunk);
			this->packetStatusCount += count;
			this->size += sizeof(uint16_t);
		}

		void FeedbackRtpTransportPacket::CreateTwoBitVectorChunk(std::vector<Status>& statuses)
		{
			auto* chunk = new TwoBitVectorChunk(statuses);

			this->chunks.push_back(chunk);
			this->packetStatusCount += statuses.size();
			this->size += sizeof(uint16_t);
		}

		void FeedbackRtpTransportPacket::AddPendingChunks()
		{
			// No pending status packets.
			if (this->context.statuses.size() == 0)
				return;

			if (this->context.allSameStatus)
			{
				CreateRunLengthChunk(this->context.currentStatus, this->context.statuses.size());

				this->context.statuses.clear();
			}
			else
			{
				MS_ASSERT(this->context.statuses.size() < 7, "already 7 status packets present");

				CreateTwoBitVectorChunk(this->context.statuses);

				this->context.statuses.clear();
			}
		}

		FeedbackRtpTransportPacket::RunLengthChunk::RunLengthChunk(uint16_t buffer)
		{
			MS_TRACE();

			MS_ASSERT(buffer & 0x8000, "invalid run length chunk");

			this->status = static_cast<Status>((buffer >> 13) & 0x03);
			this->count  = buffer & 0x1FFF;
		}

		void FeedbackRtpTransportPacket::RunLengthChunk::Dump()
		{
			MS_TRACE();

			MS_DUMP("  <FeedbackRtpTransportPacket::RunLengthChunk>");
			MS_DUMP("    status : %s", FeedbackRtpTransportPacket::Status2String[this->status].c_str());
			MS_DUMP("    count  : %" PRIu16, this->count);
			MS_DUMP("  </FeedbackRtpTransportPacket::RunLengthChunk>");
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

			MS_ASSERT(buffer & 0xC000, "invalid two bit vector chunk");

			for (size_t i{ 0u }; i < 7; ++i)
			{
				auto status = static_cast<Status>((buffer >> 2 * (7 - 1 - i)) & 0x03);

				this->statuses.emplace_back(status);
			}
		}

		void FeedbackRtpTransportPacket::TwoBitVectorChunk::Dump()
		{
			MS_TRACE();

			std::ostringstream out;

			// Dump status slots.
			for (auto status : this->statuses)
			{
				out << "|" << FeedbackRtpTransportPacket::Status2String[status];
			}

			// Dump empty slots.
			for (size_t i{ this->statuses.size() }; i < 7; ++i)
			{
				out << "|--";
			}

			out << "|";

			MS_DUMP("  <FeedbackRtpTransportPacket::TwoBitVectorChunk>");
			MS_DUMP("    %s", out.str().c_str());
			MS_DUMP("  </FeedbackRtpTransportPacket::TwoBitVectorChunk>");
		}

		size_t FeedbackRtpTransportPacket::TwoBitVectorChunk::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			MS_ASSERT(this->statuses.size() <= 7, "packet info size must be 7 or less");

			uint16_t bytes{ 0x8000 };
			uint8_t i{ 12u };

			bytes |= 0x01 << 14;

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
