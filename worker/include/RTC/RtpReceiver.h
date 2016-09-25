#ifndef MS_RTC_RTP_RECEIVER_H
#define MS_RTC_RTP_RECEIVER_H

#include "common.h"
#include "RTC/RtpDictionaries.h"
#include "RTC/RtpPacket.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "RTC/RTCP/Feedback.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Transport;

	class RtpReceiver
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

	public:
		RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, RTC::Media::Kind kind);
		virtual ~RtpReceiver();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void SetTransport(RTC::Transport* transport);
		RTC::Transport* GetTransport();
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetParameters();
		void ReceiveRtpPacket(RTC::RtpPacket* packet);

		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpFeedback(RTC::RTCP::FeedbackPacket* packet);

	private:
		void FillRtpParameters();

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
		// Others.
		bool rtpRawEventEnabled = false;
		bool rtpObjectEventEnabled = false;

		// Receiver Report holding the RTP stats
		std::auto_ptr<RTC::RTCP::ReceiverReport> receiverReport;
	};

	/* Inline methods. */

	inline
	void RtpReceiver::SetTransport(RTC::Transport* transport)
	{
		this->transport = transport;
	}

	inline
	RTC::Transport* RtpReceiver::GetTransport()
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
	RTC::RtpParameters* RtpReceiver::GetParameters()
	{
		return this->rtpParameters;
	}

	inline
	RTC::RTCP::ReceiverReport* RtpReceiver::GetRtcpReceiverReport()
	{
		return this->receiverReport.release();
	};
}

#endif
