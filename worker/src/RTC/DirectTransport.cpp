#define MS_CLASS "RTC::DirectTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/DirectTransport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	DirectTransport::DirectTransport(const std::string& id, RTC::Transport::Listener* listener, json& data)
	  : RTC::Transport::Transport(id, listener, data)
	{
		MS_TRACE();
	}

	DirectTransport::~DirectTransport()
	{
		MS_TRACE();
	}

	void DirectTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);
	}

	void DirectTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJsonStats(jsonArray);

		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "direct-transport";
	}

	void DirectTransport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		// Pass it to the parent class.
		RTC::Transport::HandleRequest(request);
	}

	void DirectTransport::HandleNotification(PayloadChannel::Notification* notification)
	{
		MS_TRACE();

		// Pass it to the parent class.
		RTC::Transport::HandleNotification(notification);
	}

	inline bool DirectTransport::IsConnected() const
	{
		return true;
	}

	void DirectTransport::SendRtpPacket(RTC::RtpPacket* /*packet*/, RTC::Transport::onSendCallback* /*cb*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void DirectTransport::SendRtcpPacket(RTC::RTCP::Packet* /*packet*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void DirectTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* /*packet*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void DirectTransport::SendSctpData(const uint8_t* /*data*/, size_t /*len*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void DirectTransport::RecvStreamClosed(uint32_t /*ssrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void DirectTransport::SendStreamClosed(uint32_t /*ssrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}
} // namespace RTC
