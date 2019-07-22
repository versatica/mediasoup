#define MS_CLASS "RTC::RtpProbator"
#define MS_LOG_DEV

#include "RTC/RtpProbator.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()

namespace RTC
{
	/* Static. */

	static constexpr uint64_t MinProbationInterval{ 1u }; // In ms.
	// clang-format off
	static uint8_t ProbationPacketHeader[] =
	{
		0b10010000, 0b01111111, 0, 0, // PayloadType: 127, Sequence Number: 0
		0, 0, 0, 0,                   // Timestamp: 0
		0, 0, 0x04, 0xD2,             // SSRC: 1234
		0xBE, 0xDE, 0, 1,             // Header Extension (One-Byte Extensions)
		0, 0, 0, 0                    // Space for abs-send-time extension.
		// TODO: Add space for Transport-CC extension once implemented (this will
		// make RtpPacket::SetExtensions() not have to shift the payload).
	};
	// clang-format on

	/* Instance methods. */

	RtpProbator::RtpProbator(RTC::RtpProbator::Listener* listener, size_t probationPacketLen)
	  : listener(listener), probationPacketLen(probationPacketLen)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->probationPacketLen >= sizeof(ProbationPacketHeader), "probationPacketLen too small");

		// Allocate the probation RTP packet buffer.
		this->probationPacketBuffer = new uint8_t[this->probationPacketLen];

		// Copy the generic probation RTP packet header into the buffer.
		std::memcpy(this->probationPacketBuffer, ProbationPacketHeader, sizeof(ProbationPacketHeader));

		// Create the probation RTP packet.
		this->probationPacket =
		  RTC::RtpPacket::Parse(this->probationPacketBuffer, this->probationPacketLen);

		// Set random initial RTP seq number and timestamp.
		this->probationPacket->SetSequenceNumber(
		  static_cast<uint16_t>(Utils::Crypto::GetRandomUInt(0, 65535)));
		this->probationPacket->SetTimestamp(Utils::Crypto::GetRandomUInt(0, 4294967295));

		// Create the RTP periodic timer.
		this->rtpPeriodicTimer = new Timer(this);
	}

	RtpProbator::~RtpProbator()
	{
		MS_TRACE();

		// Delete the probation packet buffer.
		delete[] this->probationPacketBuffer;

		// Delete the probation RTP packet.
		delete this->probationPacket;

		// Delete the RTP periodic timer.
		delete this->rtpPeriodicTimer;
	}

	void RtpProbator::Start(uint32_t bitrate)
	{
		MS_TRACE();

		MS_ASSERT(!this->rtpPeriodicTimer->IsActive(), "already started");
		MS_ASSERT(bitrate != 0u, "bitrate cannot be 0");

		// Calculate a proper interval for sending RTP packets of size
		// RTC::RtpProbator::ProbationRtpPacketSize bytes in order to produce the
		// given bitrate.
		auto packetsPerSecond =
		  static_cast<double>(bitrate / (RTC::RtpProbator::ProbationRtpPacketSize * 8.0f));
		auto interval = static_cast<uint64_t>(1000.0f / packetsPerSecond);

		if (interval < MinProbationInterval)
			interval = MinProbationInterval;

		MS_DEBUG_TAG(
		  bwe,
		  "probation started [bitrate:%" PRIu32 ", packetsPerSecond:%f, interval:%" PRIu64 "]",
		  bitrate,
		  packetsPerSecond,
		  interval);

		this->rtpPeriodicTimer->Start(interval, interval);
	}

	void RtpProbator::Stop()
	{
		MS_TRACE();

		if (!this->rtpPeriodicTimer->IsActive())
			return;

		this->rtpPeriodicTimer->Stop();

		MS_DEBUG_TAG(bwe, "probation stopped");
	}

	inline void RtpProbator::OnTimer(Timer* /*timer*/)
	{
		MS_TRACE();

		// Increase RTP seq number and timestamp.
		auto seq       = this->probationPacket->GetSequenceNumber();
		auto timestamp = this->probationPacket->GetTimestamp();

		// TODO: Properly increment timestamp. We know the interval (ms) via
		// this->rtpPeriodicTimer->GetRepeat().
		// NOTE: Is it worth it?
		++seq;
		timestamp += 20u;

		this->probationPacket->SetSequenceNumber(seq);
		this->probationPacket->SetTimestamp(timestamp);

		// TODO
		if (seq % 500 == 0)
			MS_DEBUG_DEV("sending RTP probation packet [seq:%" PRIu16 "]", seq);

		// Notify the listener.
		this->listener->OnRtpProbatorSendRtpPacket(this, this->probationPacket);
	}
} // namespace RTC
