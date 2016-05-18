#ifndef MS_RTC_RTP_RECEIVER_H
#define MS_RTC_RTP_RECEIVER_H

#include "common.h"
#include "RTC/RtpKind.h"
#include "RTC/RtpParameters.h"
#include "RTC/RtpPacket.h"
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
			virtual void onRtpReceiverParameters(RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters) = 0;
			virtual void onRtpPacket(RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) = 0;
			virtual void onRtpReceiverClosed(RtpReceiver* rtpReceiver) = 0;
		};

	public:
		RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, std::string& kind);
		virtual ~RtpReceiver();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void SetTransport(RTC::Transport* transport);
		RTC::Transport* GetTransport();
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetRtpParameters();
		void ReceiveRtpPacket(RTC::RtpPacket* packet);

	public:
		// Passed by argument.
		uint32_t rtpReceiverId;
		// Others.
		RTC::RtpKind kind;

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
	RTC::RtpParameters* RtpReceiver::GetRtpParameters()
	{
		return this->rtpParameters;
	}
}

#endif
