#ifndef MS_RTC_PRODUCER_HPP
#define MS_RTC_PRODUCER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/ProducerListener.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "RTC/Transport.hpp"
#include "handles/Timer.hpp"
#include <json/json.h>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace RTC
{
	class Producer : public RtpStreamRecv::Listener, public Timer::Listener
	{
	public:
		struct RtpMapping
		{
			std::map<uint8_t, uint8_t> codecPayloadTypes;
			std::map<uint8_t, uint8_t> headerExtensionIds;
		};

	private:
		struct HeaderExtensionIds
		{
			uint8_t ssrcAudioLevel{ 0 }; // 0 means no ssrc-audio-level id.
			uint8_t absSendTime{ 0 };    // 0 means no abs-send-time id.
			uint8_t mid{ 0 };            // 0 means no MID id.
			uint8_t rid{ 0 };            // 0 means no RID id.
		};

	private:
		struct RtpStreamInfo
		{
			RTC::RtpStreamRecv* rtpStream{ nullptr };
			std::string rid{};
			RTC::RtpEncodingParameters::Profile profile{ RTC::RtpEncodingParameters::Profile::NONE };
			uint32_t rtxSsrc{ 0 };
			bool active{ false };
		};

	public:
		Producer(
		  Channel::Notifier* notifier,
		  uint32_t producerId,
		  RTC::Media::Kind kind,
		  RTC::Transport* transport,
		  RTC::RtpParameters& rtpParameters,
		  struct RtpMapping& rtpMapping,
		  bool paused);
		~Producer();

	public:
		Json::Value ToJson() const;
		Json::Value GetStats() const;
		void AddListener(RTC::ProducerListener* listener);
		void RemoveListener(RTC::ProducerListener* listener);
		void Pause();
		void Resume();
		void SetPreferredProfile(const RTC::RtpEncodingParameters::Profile profile);
		const RTC::RtpParameters& GetParameters() const;
		const struct RTC::Transport::HeaderExtensionIds& GetTransportHeaderExtensionIds() const;
		bool IsPaused() const;
		RTC::RtpEncodingParameters::Profile GetPreferredProfile() const;
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void RequestKeyFrame(bool force = false);
		const std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*>& GetActiveProfiles() const;

	private:
		void FillHeaderExtensionIds();
		void MayNeedNewStream(RTC::RtpPacket* packet);
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding, uint32_t ssrc);
		void ApplyRtpMapping(RTC::RtpPacket* packet) const;
		void ActivateStream(RTC::RtpStreamRecv* rtpStream);
		void DeactivateStream(RTC::RtpStreamRecv* rtpStream);
		// TODO: Remove?
		bool IsStreamActive(const RTC::RtpStream* rtpStream) const;

		/* Pure virtual methods inherited from RTC::RtpStreamRecv::Listener. */
	public:
		void OnRtpStreamRecvNackRequired(
		  RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers) override;
		void OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream) override;
		void OnRtpStreamInactive(RTC::RtpStream* rtpStream) override;
		void OnRtpStreamActive(RTC::RtpStream* rtpStream) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		uint32_t producerId{ 0 };
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		Channel::Notifier* notifier{ nullptr };
		RTC::Transport* transport{ nullptr };
		RTC::RtpParameters rtpParameters;
		struct RtpMapping rtpMapping;
		std::unordered_set<RTC::ProducerListener*> listeners;
		// Allocated by this.
		std::map<uint32_t, RtpStreamInfo> mapSsrcRtpStreamInfo;
		std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*> mapActiveProfiles;
		Timer* keyFrameRequestBlockTimer{ nullptr };
		// Others.
		struct RTC::Transport::HeaderExtensionIds transportHeaderExtensionIds;
		struct HeaderExtensionIds headerExtensionIds;
		bool paused{ false };
		bool isKeyFrameRequested{ false };
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
		// RTP preferred profile.
		RTC::RtpEncodingParameters::Profile preferredProfile{ RTC::RtpEncodingParameters::Profile::DEFAULT };
	};

	/* Inline methods. */

	inline void Producer::AddListener(RTC::ProducerListener* listener)
	{
		this->listeners.insert(listener);
	}

	inline void Producer::RemoveListener(RTC::ProducerListener* listener)
	{
		this->listeners.erase(listener);
	}

	inline void Producer::SetPreferredProfile(const RTC::RtpEncodingParameters::Profile profile)
	{
		this->preferredProfile = profile;
	}

	inline const RTC::RtpParameters& Producer::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline RTC::RtpEncodingParameters::Profile Producer::GetPreferredProfile() const
	{
		return this->preferredProfile;
	}

	inline const struct RTC::Transport::HeaderExtensionIds& Producer::GetTransportHeaderExtensionIds() const
	{
		return this->transportHeaderExtensionIds;
	}

	inline bool Producer::IsPaused() const
	{
		return this->paused;
	}

	inline void Producer::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		auto it = this->mapSsrcRtpStreamInfo.find(report->GetSsrc());
		if (it != this->mapSsrcRtpStreamInfo.end())
		{
			auto& info      = it->second;
			auto* rtpStream = info.rtpStream;

			rtpStream->ReceiveRtcpSenderReport(report);
		}
	}

	inline const std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*>& Producer::
	  GetActiveProfiles() const
	{
		return this->mapActiveProfiles;
	}
} // namespace RTC

#endif
