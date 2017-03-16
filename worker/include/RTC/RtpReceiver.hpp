#ifndef MS_RTC_RTP_RECEIVER_HPP
#define MS_RTC_RTP_RECEIVER_HPP

#include "common.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "Channel/Request.hpp"
#include "Channel/Notifier.hpp"
#include <string>
#include <map>
#include <json/json.h>

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Transport;

	class RtpReceiver :
		public RtpStreamRecv::Listener
	{
	public:
		/**
		 * RTC::Peer is the Listener.
		 */
		class Listener
		{
		public:
			virtual void onRtpReceiverParameters(RtpReceiver* rtpReceiver) = 0;
			virtual void onRtpReceiverParametersDone(RTC::RtpReceiver* rtpReceiver) = 0;
			virtual void onRtpPacket(RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) = 0;
			virtual void onRtpReceiverClosed(RtpReceiver* rtpReceiver) = 0;
		};

	private:
		static uint8_t rtcpBuffer[];

	public:
		RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, RTC::Media::Kind kind);
		virtual ~RtpReceiver();

		void Close();
		Json::Value toJson() const;
		void HandleRequest(Channel::Request* request);
		void SetTransport(RTC::Transport* transport);
		RTC::Transport* GetTransport() const;
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetParameters() const;
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void GetRtcp(RTC::RTCP::CompoundPacket *packet, uint64_t now);
		void ReceiveRtcpFeedback(RTC::RTCP::FeedbackPsPacket* packet);
		void ReceiveRtcpFeedback(RTC::RTCP::FeedbackRtpPacket* packet);

	private:
		void ClearRtpStreams();

	/* Pure virtual methods inherited from RTC::RtpStreamRecv::Listener. */
	public:
		virtual void onNackRequired(RTC::RtpStreamRecv* rtpStream, uint16_t seq, uint16_t bitmask) override;

	public:
		// Passed by argument.
		uint32_t rtpReceiverId;
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RTC::Transport* transport = nullptr;
		// Allocated by this.
		RTC::RtpParameters* rtpParameters = nullptr;
		std::map<uint32_t, RTC::RtpStreamRecv*> rtpStreams;
		// Others.
		bool rtpRawEventEnabled = false;
		bool rtpObjectEventEnabled = false;
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime = 0;
		uint16_t maxRtcpInterval;
	};

	/* Inline methods. */

	inline
	void RtpReceiver::SetTransport(RTC::Transport* transport)
	{
		this->transport = transport;
	}

	inline
	RTC::Transport* RtpReceiver::GetTransport() const
	{
		return this->transport;
	}

	inline
	void RtpReceiver::RemoveTransport(RTC::Transport* transport)
	{
		if (this->transport == transport)
			this->transport = nullptr;
	}

	inline
	RTC::RtpParameters* RtpReceiver::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline
	void RtpReceiver::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		auto it = this->rtpStreams.find(report->GetSsrc());
		if (it != this->rtpStreams.end())
		{
			auto rtpStream = it->second;

			rtpStream->ReceiveRtcpSenderReport(report);
		}
	}
}

#endif
