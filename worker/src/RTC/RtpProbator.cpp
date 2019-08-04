#define MS_CLASS "RTC::RtpProbator"
// #define MS_LOG_DEV

#include "RTC/RtpProbator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cmath>   // std::ceil(), std::round(), std::floor()
#include <cstring> // std::memcpy()
#include <vector>

namespace RTC
{
	/* Static. */

	// clang-format off
	// Minimum target bitrate desired.
	static constexpr uint32_t MinBitrate{ 50000u };
	// Minimum probation interval (in ms).
	static constexpr uint64_t MinProbationInterval{ 1u };
	// Duration of each probation step (in ms).
	static constexpr uint64_t StepDuration{ 500u };
	// Number of RTP packets to send in each step at the calculated bitrate.
	static constexpr uint32_t StepNumPackets{ 50u };
	// Bitrate jump between steps (in bps).
	static constexpr uint32_t StepBitrateJump{ 150000u };
	// Probation RTP header.
	static uint8_t ProbationPacketHeader[] =
	{
		0b10010000, 0b01111111, 0, 0, // PayloadType: 127, Sequence Number: 0
		0, 0, 0, 0,                   // Timestamp: 0
		0, 0, 0, 0,                   // SSRC: 0
		0xBE, 0xDE, 0, 2,             // Header Extension (One-Byte Extensions)
		0, 0, 0, 0,                   // Space for abs-send-time extension.
		0, 0, 0, 0                    // Space for transport-wide-cc-01 extension.
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

		// Set fixed SSRC.
		this->probationPacket->SetSsrc(RTC::RtpProbatorSsrc);

		// Set random initial RTP seq number and timestamp.
		this->probationPacket->SetSequenceNumber(
		  static_cast<uint16_t>(Utils::Crypto::GetRandomUInt(0, 65535)));
		this->probationPacket->SetTimestamp(Utils::Crypto::GetRandomUInt(0, 4294967295));

		// Add BWE related RTP header extensions.
		static uint8_t buffer[4096];

		std::vector<RTC::RtpPacket::GenericExtension> extensions;
		uint8_t extenLen;
		uint8_t* bufferPtr{ buffer };

		// Add http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time.
		// NOTE: Just the corresponding id and space for its value.
		{
			extenLen = 3u;

			extensions.emplace_back(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME), extenLen, bufferPtr);

			bufferPtr += extenLen;
		}

		// Add http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01.
		// NOTE: Just the corresponding id and space for its value.
		{
			extenLen = 2u;

			extensions.emplace_back(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01),
			  extenLen,
			  bufferPtr);

			// Not needed since this is the latest added extension.
			// bufferPtr += extenLen;
		}

		// Set the extensions into the packet using One-Byte format.
		this->probationPacket->SetExtensions(1, extensions);

		// Set our abs-send-time extension id.
		this->probationPacket->SetAbsSendTimeExtensionId(
		  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME));

		// Set our transport-wide-cc-01 extension id.
		this->probationPacket->SetTransportWideCc01ExtensionId(
		  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01));

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

		MS_ASSERT(!this->running, "already running");

		this->running = true;

		if (bitrate < MinBitrate)
		{
			MS_DEBUG_TAG(
			  bwe, "too low bitrate:%" PRIu32 ", using minimum bitrate:%" PRIu32, bitrate, MinBitrate);

			bitrate = MinBitrate;
		}

		this->targetBitrate = bitrate;
		this->numSteps = static_cast<uint16_t>(std::ceil(bitrate / static_cast<float>(StepBitrateJump)));
		this->currentStep = 0u; // Begin with 0 on purpose.

		double targetPacketsPerSecond = bitrate / (this->probationPacketLen * 8.0f);

		this->targetRtpInterval = static_cast<uint64_t>(std::floor(1000.0f / targetPacketsPerSecond));

		if (this->targetRtpInterval < MinProbationInterval)
			this->targetRtpInterval = MinProbationInterval;

		MS_DEBUG_TAG(
		  bwe,
		  "probation started [targetBitrate:%" PRIu32 ", numSteps:%" PRIu16
		  ", targetPacketsPerSecond:%f, targetRtpInterval:%" PRIu64 "]",
		  this->targetBitrate,
		  this->numSteps,
		  targetPacketsPerSecond,
		  this->targetRtpInterval);

		ReloadProbation();
	}

	void RtpProbator::Stop()
	{
		MS_TRACE();

		if (!this->running)
			return;

		this->running = false;

		this->rtpPeriodicTimer->Stop();

		this->targetBitrate     = 0u;
		this->numSteps          = 0u;
		this->currentStep       = 0u;
		this->stepStartedAt     = 0u;
		this->stepNumPacket     = 0u;
		this->targetRtpInterval = 0u;

		MS_DEBUG_TAG(bwe, "probation stopped");
	}

	void RtpProbator::ReloadProbation()
	{
		MS_TRACE();

		uint64_t rtpInterval;
		uint32_t bitrate;

		if (this->currentStep == 0u)
		{
			bitrate = MinBitrate;

			double targetPacketsPerSecond = bitrate / (this->probationPacketLen * 8.0f);

			rtpInterval = static_cast<uint64_t>(std::floor(1000.0f / targetPacketsPerSecond));
		}
		else
		{
			rtpInterval = static_cast<uint64_t>(std::floor(
			  this->targetRtpInterval *
			  static_cast<float>(this->numSteps / static_cast<float>(this->currentStep))));

			bitrate = static_cast<uint32_t>(std::round(
			  this->targetBitrate *
			  static_cast<float>(this->targetRtpInterval / static_cast<float>(rtpInterval))));
		}

		MS_DEBUG_TAG(
		  bwe,
		  "[currentStep:%" PRIu16 "/%" PRIu16 ", bitrate:%" PRIu32 ", rtpInterval:%" PRIu64 "]",
		  this->currentStep,
		  this->numSteps,
		  bitrate,
		  rtpInterval);

		this->stepStartedAt = DepLibUV::GetTime();
		this->stepNumPacket = 0u;

		this->rtpPeriodicTimer->Start(rtpInterval, rtpInterval);
	}

	inline void RtpProbator::OnTimer(Timer* /*timer*/)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		// Just send up to StepNumPackets per step.
		if (++this->stepNumPacket <= StepNumPackets)
		{
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

			// Notify the listener.
			this->listener->OnRtpProbatorSendRtpPacket(this, this->probationPacket);
		}
		else if (this->stepNumPacket == StepNumPackets + 1)
		{
			// This is to avoid entering this condition again.
			++this->stepNumPacket;

			uint64_t elapsedStepTime = now - this->stepStartedAt;

			if (elapsedStepTime < StepDuration)
			{
				uint64_t remainingStepDuration = StepDuration - (now - this->stepStartedAt);

				this->rtpPeriodicTimer->Start(remainingStepDuration, 0);

				// TODO: REMOVE
				// MS_DUMP(
				// 	"no more packets sent on this step [currentStep:%" PRIu16 ", remainingStepDuration:%"
				// PRIu64 "]", 	this->currentStep, 	remainingStepDuration);

				return;
			}
		}

		// If step duration elapsed or the timer does not currently have 'repeat', move to
		// next step or end.
		// NOTE: In theory the second condition should never happen, but just in case.
		if (now - this->stepStartedAt >= StepDuration || !this->rtpPeriodicTimer->GetRepeat())
		{
			// TODO: REMOVE
			// if (now - this->stepStartedAt >= StepDuration)
			// 	MS_DUMP("----------------------- now - this->stepStartedAt >= StepDuration");
			// else
			// 	MS_DUMP("----------------------------------------------- GET_REPEAT = 0 !!!");

			if (this->currentStep == 0u)
			{
				++this->currentStep;

				ReloadProbation();
			}
			else
			{
				if (this->currentStep + 1 > this->numSteps)
				{
					Stop();

					this->listener->OnRtpProbatorEnded(this);

					return;
				}

				this->listener->OnRtpProbatorStep(this);

				// If the listener stopped us, exit.
				if (!IsRunning())
					return;

				++this->currentStep;

				ReloadProbation();
			}
		}
	}
} // namespace RTC
