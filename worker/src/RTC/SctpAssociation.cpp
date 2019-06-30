#define MS_CLASS "RTC::SctpAssociation"
#define MS_LOG_DEV

#include "RTC/SctpAssociation.hpp"
#include "DepUsrSCTP.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
// #include "Utils.hpp"
// #include "Channel/Notifier.hpp"

/* clang-format off */
uint16_t event_types[] =
{
	SCTP_ADAPTATION_INDICATION,
	SCTP_ASSOC_CHANGE,
	SCTP_ASSOC_RESET_EVENT,
	SCTP_PEER_ADDR_CHANGE,
	SCTP_REMOTE_ERROR,
	SCTP_SHUTDOWN_EVENT,
	SCTP_SEND_FAILED_EVENT,
	SCTP_STREAM_RESET_EVENT,
	SCTP_STREAM_CHANGE_EVENT
};
/* clang-format on */

inline static int onRecvSctpData(
  struct socket* /*sock*/,
  union sctp_sockstore /*addr*/,
  void* data,
  size_t datalen,
  struct sctp_rcvinfo rcv,
  int flags,
  void* ulp_info)
{
	auto* sctpAssociation = static_cast<RTC::SctpAssociation*>(ulp_info);

	if (sctpAssociation == nullptr)
		return 0;

	uint16_t streamId = rcv.rcv_sid;

	MS_DEBUG_DEV(
	  "Message received: [length:%zu, stream:%" PRIu16 ", SSN:%" PRIu16 ", TSN:%" PRIu32
	  ", PPID:%" PRIu32 ", context:%" PRIu32,
	  datalen,
	  rcv.rcv_sid,
	  rcv.rcv_ssn,
	  rcv.rcv_tsn,
	  ntohl(rcv.rcv_ppid),
	  rcv.rcv_context);

	if (flags & MSG_NOTIFICATION)
		sctpAssociation->OnUsrSctpReceiveSctpNotification((union sctp_notification*)data, datalen);
	else
		sctpAssociation->OnUsrSctpReceiveSctpData(streamId, (uint8_t*)data, datalen);

	return 1;
}

namespace RTC
{
	/* Instance methods. */

	SctpAssociation::SctpAssociation(Listener* listener, uint16_t numSctpStreams, uint32_t maxSctpMessageSize)
	  : listener(listener), numSctpStreams(numSctpStreams), maxSctpMessageSize(maxSctpMessageSize)
	{
		MS_TRACE();
	}

	bool SctpAssociation::Run()
	{
		int ret;

		// Without this function call usrsctp_bind will fail as follows:
		// "usrsctp_bind() failed: Can't assign requested address"
		// TODO: should we unregister on destructor?
		usrsctp_register_address((void*)this);

		// Disable explicit congestion notifications (ecn).
		usrsctp_sysctl_set_sctp_ecn_enable(0);

		this->socket =
		  usrsctp_socket(AF_CONN, SOCK_STREAM, IPPROTO_SCTP, onRecvSctpData, nullptr, 0, (void*)this);

		if (this->socket == nullptr)
			MS_THROW_ERROR("usrsctp_socket() failed: %s", std::strerror(errno));

		usrsctp_set_ulpinfo(this->socket, (void*)this);

		// Make the socket non-blocking.
		ret = usrsctp_set_non_blocking(this->socket, 1);
		if (ret < 0)
			MS_THROW_ERROR("usrsctp_set_non_blocking() failed: %s", std::strerror(errno));

		// Set SO_LINGER.
		struct linger linger_opt;
		linger_opt.l_onoff  = 1;
		linger_opt.l_linger = 0;

		ret = usrsctp_setsockopt(this->socket, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));
		if (ret < 0)
			MS_THROW_ERROR("usrsctp_setsockopt(SO_LINGER) failed: %s", std::strerror(errno));

		// Set SCTP_NODELAY.
		uint32_t noDelay = 1;
		ret = usrsctp_setsockopt(this->socket, IPPROTO_SCTP, SCTP_NODELAY, &noDelay, sizeof(noDelay));
		if (ret < 0)
			MS_THROW_ERROR("usrsctp_setsockopt(SCTP_NODELAY) failed: %s", std::strerror(errno));

		// Enable events.
		struct sctp_event event;
		memset(&event, 0, sizeof(event));
		event.se_assoc_id = SCTP_ALL_ASSOC;
		event.se_on       = 1;
		for (size_t i = 0; i < sizeof(event_types) / sizeof(uint16_t); i++)
		{
			event.se_type = event_types[i];
			ret = usrsctp_setsockopt(this->socket, IPPROTO_SCTP, SCTP_EVENT, &event, sizeof(event));
			if (ret < 0)
				MS_THROW_ERROR("usrsctp_setsockopt(SCTP_EVENT) failed: %s", std::strerror(errno));
		}

		// Init message.
		struct sctp_initmsg initmsg;
		memset(&initmsg, 0, sizeof(struct sctp_initmsg));
		// TODO: Set proper values.
		initmsg.sinit_num_ostreams  = 1023;
		initmsg.sinit_max_instreams = 1023;

		ret = usrsctp_setsockopt(
		  this->socket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(struct sctp_initmsg));
		if (ret < 0)
			MS_THROW_ERROR("usrsctp_setsockopt(SCTP_INITMSG) failed: %s", std::strerror(errno));

		// Server side.
		struct sockaddr_conn sconn;
		memset(&sconn, 0, sizeof(struct sockaddr_conn));
		sconn.sconn_family = AF_CONN;
		sconn.sconn_port   = htons(5000);
		sconn.sconn_addr   = (void*)this;
#ifdef HAVE_SCONN_LEN
		rconn.sconn_len = sizeof(struct sockaddr_conn);
#endif

		ret = usrsctp_bind(this->socket, (struct sockaddr*)&sconn, sizeof(struct sockaddr_conn));

		if (ret < 0)
			MS_THROW_ERROR("usrsctp_bind() failed: %s", std::strerror(errno));

		// Client side.
		struct sockaddr_conn rconn;
		memset(&rconn, 0, sizeof(struct sockaddr_conn));
		rconn.sconn_family = AF_CONN;
		rconn.sconn_port   = htons(5000);
		rconn.sconn_addr   = (void*)this;
		rconn.sconn_len    = sizeof(struct sockaddr_conn);

		ret = usrsctp_connect(this->socket, (struct sockaddr*)&rconn, sizeof(struct sockaddr_conn));

		if (ret < 0 && errno != EINPROGRESS)
			MS_THROW_ERROR("usrsctp_connect() failed: %s", std::strerror(errno));

		DepUsrSCTP::IncreaseSctpAssociations();

		// TMP.
		return true;
	}

	SctpAssociation::~SctpAssociation()
	{
		MS_TRACE();

		usrsctp_close(this->socket);

		DepUsrSCTP::DecreaseSctpAssociations();
	}

	void SctpAssociation::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add port (always 5000).
		jsonObject["port"] = 5000;

		// Add numStreams.
		jsonObject["numStreams"] = this->numSctpStreams;

		// Add maxMessageSize.
		jsonObject["maxMessageSize"] = this->maxSctpMessageSize;
	}

	void SctpAssociation::ProcessSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (len > this->maxSctpMessageSize)
		{
			MS_WARN_TAG(sctp, "incoming data size exceeds maxSctpMessageSize value");
			return;
		}

		usrsctp_conninput((void*)this, data, len, 0);
	}

	void SctpAssociation::SendSctpMessage(RTC::DataConsumer* dataConsumer, const uint8_t* msg, size_t len)
	{
		MS_TRACE();

		if (len > this->maxSctpMessageSize)
		{
			MS_WARN_TAG(sctp, "outgoing message size exceeds maxSctpMessageSize value");
			return;
		}

		auto parameters = dataConsumer->GetSctpStreamParameters();

		// Fill stcp_sendv_spa.
		struct sctp_sendv_spa spa;

		memset(&spa, 0, sizeof(struct sctp_sendv_spa));
		spa.sendv_sndinfo.snd_sid = parameters.streamId;

		if (parameters.ordered)
			spa.sendv_sndinfo.snd_flags = SCTP_EOR;
		else
			spa.sendv_sndinfo.snd_flags = SCTP_EOR | SCTP_UNORDERED;

		spa.sendv_sndinfo.snd_ppid = htonl((uint8_t)RTC::Data::Type::STRING);
		spa.sendv_flags            = SCTP_SEND_SNDINFO_VALID;

		// TODO.
		// spa.sendv_prinfo.pr_policy.
		// spa.sendv_prinfo.pr_value.

		int ret = usrsctp_sendv(
		  this->socket, msg, len, nullptr, 0, &spa, (socklen_t)sizeof(struct sctp_sendv_spa), SCTP_SENDV_SPA, 0);

		if (ret < 0)
			MS_ERROR("error sending usctp data: %s", std::strerror(errno));
	}

	void SctpAssociation::OnUsrSctpSendSctpData(void* buffer, size_t len)
	{
		MS_TRACE();

		const uint8_t* data = static_cast<uint8_t*>(buffer);

		// TODO: accounting, rate, etc?

		this->listener->OnSctpAssociationSendData(this, data, len);
	}

	void SctpAssociation::OnUsrSctpReceiveSctpData(uint16_t streamId, const uint8_t* msg, size_t len)
	{
		MS_DUMP_DATA(msg, len);

		this->listener->OnSctpAssociationMessageReceived(this, streamId, msg, len);
	}

	void SctpAssociation::OnUsrSctpReceiveSctpNotification(union sctp_notification* notification, size_t len)
	{
		if (notification->sn_header.sn_length != (uint32_t)len)
			return;

		switch (notification->sn_header.sn_type)
		{
			case SCTP_ADAPTATION_INDICATION:
			{
				MS_DEBUG_TAG(
				  sctp,
				  "sctp adaptation indication [%x]",
				  notification->sn_adaptation_event.sai_adaptation_ind);

				break;
			}
			case SCTP_ASSOC_CHANGE:
			{
				switch (notification->sn_assoc_change.sac_state)
				{
					case SCTP_COMM_UP:
					{
						this->state = SctpState::CONNECTED;

						MS_DEBUG_TAG(
						  sctp,
						  "sctp association connected, streams [in:%" PRIu16 " , out:%" PRIu16 "]",
						  notification->sn_assoc_change.sac_inbound_streams,
						  notification->sn_assoc_change.sac_outbound_streams);

						break;
					}

					case SCTP_COMM_LOST:
					{
						this->state = SctpState::CLOSED;

						if (notification->sn_header.sn_length > 0)
						{
							static const size_t bufferSize{ 1024 };
							static char buffer[bufferSize];
							uint32_t len = notification->sn_header.sn_length;

							for (uint32_t i{ 0 }; i < len; i++)
							{
								std::snprintf(
								  buffer, bufferSize, " 0x%02x", notification->sn_assoc_change.sac_info[i]);
							}

							MS_WARN_TAG(sctp, "stcp communication lost");
						}

						MS_DEBUG_TAG(sctp, "sctp association disconnected");

						break;
					}

					case SCTP_RESTART:
					{
						this->state = SctpState::CONNECTED;

						MS_DEBUG_TAG(
						  sctp,
						  "sctp remote association restarted, streams [in:%" PRIu16 " , out:%" PRIu16 "]",
						  notification->sn_assoc_change.sac_inbound_streams,
						  notification->sn_assoc_change.sac_outbound_streams);

						break;
					}

					case SCTP_SHUTDOWN_COMP:
					{
						this->state = SctpState::CLOSED;

						MS_DEBUG_TAG(sctp, "sctp association gracefully closed");

						break;
					}

					case SCTP_CANT_STR_ASSOC:
					{
						this->state = SctpState::CLOSED;

						if (notification->sn_header.sn_length > 0)
						{
							static const size_t bufferSize{ 1024 };
							static char buffer[bufferSize];
							uint32_t len = notification->sn_header.sn_length;

							for (uint32_t i{ 0 }; i < len; i++)
							{
								std::snprintf(
								  buffer, bufferSize, " 0x%02x", notification->sn_assoc_change.sac_info[i]);
							}

							MS_WARN_TAG(sctp, "stcp setup failed: '%s'", buffer);
						}

						break;
					}

					default:
					{
						break;
					}
				}

				break;
			}
			// https://tools.ietf.org/html/rfc6525#section-6.1.2.
			case SCTP_ASSOC_RESET_EVENT:
			{
				MS_DEBUG_TAG(sctp, "association reset event received");

				break;
			}
			// https://tools.ietf.org/html/rfc6458#section-6.1.2.
			// Only applicable to multi-homed associations.
			case SCTP_PEER_ADDR_CHANGE:
			{
				break;
			}

			// An Operation Error is not considered fatal in and of itself, but may be
			// used with an ABORT chunk to report a fatal condition.
			case SCTP_REMOTE_ERROR:
			{
				static const size_t bufferSize{ 1024 };
				static char buffer[bufferSize];

				uint32_t len = notification->sn_remote_error.sre_length - sizeof(struct sctp_remote_error);

				for (uint32_t i{ 0 }; i < len; i++)
				{
					std::snprintf(buffer, bufferSize, "0x%02x", notification->sn_remote_error.sre_data[i]);
				}

				MS_WARN_TAG(
				  sctp,
				  "remote association error [type:0x%04x, data:%s]",
				  notification->sn_remote_error.sre_error,
				  buffer);

				break;
			}

			// When a peer sends a SHUTDOWN, SCTP delivers this notification to
			// inform the application that it should cease sending data.
			case SCTP_SHUTDOWN_EVENT:
			{
				MS_DEBUG_TAG(sctp, "remote association shutdown");

				// TODO: Change the state and notify?

				break;
			}

			case SCTP_SEND_FAILED_EVENT:
			{
				static const size_t bufferSize{ 1024 };
				static char buffer[bufferSize];

				uint32_t len =
				  notification->sn_send_failed_event.ssfe_length - sizeof(struct sctp_send_failed_event);
				for (uint32_t i{ 0 }; i < len; i++)
				{
					std::snprintf(buffer, bufferSize, "0x%02x", notification->sn_send_failed_event.ssfe_data[i]);
				}

				MS_WARN_TAG(
				  sctp,
				  "sctp message sent failure [streamId:%" PRIu16 " ,ppid:%" PRIu32
				  ", sent:%s, error:0x%08x, info:%s]",
				  notification->sn_send_failed_event.ssfe_info.snd_sid,
				  ntohl(notification->sn_send_failed_event.ssfe_info.snd_ppid),
				  notification->sn_send_failed_event.ssfe_flags & SCTP_DATA_SENT ? "yes" : "no",
				  notification->sn_send_failed_event.ssfe_error,
				  buffer);

				break;
			}

			case SCTP_STREAM_RESET_EVENT:
			{
				MS_DEBUG_TAG(sctp, "stream reset event received");

				break;
			}

			case SCTP_STREAM_CHANGE_EVENT:
			{
				MS_DEBUG_TAG(
				  sctp,
				  "sctp stream changed, streams [in:%" PRIu16 " , out:%" PRIu16 ", flags:%x]",
				  notification->sn_strchange_event.strchange_instrms,
				  notification->sn_strchange_event.strchange_outstrms,
				  notification->sn_strchange_event.strchange_flags);

				break;
			}

			default:
			{
				MS_DEBUG_TAG(
				  sctp, "unhandled sctp event received [type:%" PRIu16 "]", notification->sn_header.sn_type);

				break;
			}
		}
	}
} // namespace RTC
