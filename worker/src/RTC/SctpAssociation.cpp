#define MS_CLASS "RTC::SctpAssociation"
// #define MS_LOG_DEV

#include "RTC/SctpAssociation.hpp"
#include "DepUsrSCTP.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

/* SCTP events to which we are subscribing. */

/* clang-format off */
uint16_t EventTypes[] =
{
	SCTP_ADAPTATION_INDICATION,
	SCTP_ASSOC_CHANGE,
	SCTP_ASSOC_RESET_EVENT,
	SCTP_REMOTE_ERROR,
	SCTP_SHUTDOWN_EVENT,
	SCTP_SEND_FAILED_EVENT,
	SCTP_STREAM_RESET_EVENT,
	SCTP_STREAM_CHANGE_EVENT
};
/* clang-format on */

/* Static methods for usrsctp callbacks. */

inline static int onRecvSctpData(
  struct socket* /*sock*/,
  union sctp_sockstore /*addr*/,
  void* data,
  size_t dataLen,
  struct sctp_rcvinfo rcv,
  int flags,
  void* ulpInfo)
{
	auto* sctpAssociation = static_cast<RTC::SctpAssociation*>(ulpInfo);

	if (sctpAssociation == nullptr)
		return 0;

	if (flags & MSG_NOTIFICATION)
	{
		sctpAssociation->OnUsrSctpReceiveSctpNotification(
		  static_cast<union sctp_notification*>(data), dataLen);
	}
	else
	{
		uint16_t streamId = rcv.rcv_sid;
		uint8_t ppid      = ntohl(rcv.rcv_ppid);

		// TODO: Convert this to MS_DEBUG_DEV.
		MS_DEBUG_TAG(
		  sctp,
		  "data chunk received [length:%zu, streamId:%" PRIu16 ", SSN:%" PRIu16 ", TSN:%" PRIu32
		  ", PPID:%" PRIu32 ", context:%" PRIu32 ", flags:%d]",
		  dataLen,
		  rcv.rcv_sid,
		  rcv.rcv_ssn,
		  rcv.rcv_tsn,
		  ntohl(rcv.rcv_ppid),
		  rcv.rcv_context,
		  flags);

		sctpAssociation->OnUsrSctpReceiveSctpData(
		  streamId, ppid, flags, static_cast<uint8_t*>(data), dataLen);
	}

	return 1;
}

namespace RTC
{
	/* Instance methods. */

	SctpAssociation::SctpAssociation(Listener* listener, uint16_t numSctpStreams, size_t maxSctpMessageSize)
	  : listener(listener), numSctpStreams(numSctpStreams), maxSctpMessageSize(maxSctpMessageSize)
	{
		MS_TRACE();

		// Register ourselves in usrsctp.
		usrsctp_register_address(static_cast<void*>(this));

		DepUsrSCTP::IncreaseSctpAssociations();
	}

	SctpAssociation::~SctpAssociation()
	{
		MS_TRACE();

		if (this->socket)
		{
			usrsctp_set_ulpinfo(this->socket, nullptr);
			usrsctp_close(this->socket);
		}

		// Deregister ourselves from usrsctp.
		usrsctp_deregister_address(static_cast<void*>(this));

		DepUsrSCTP::DecreaseSctpAssociations();

		delete[] this->messageBuffer;
	}

	void SctpAssociation::Run()
	{
		MS_TRACE();

		MS_ASSERT(this->state == SctpState::NEW, "not in new SCTP state");

		try
		{
			int ret;

			// Disable explicit congestion notifications (ecn).
			usrsctp_sysctl_set_sctp_ecn_enable(0);

			this->socket = usrsctp_socket(
			  AF_CONN, SOCK_STREAM, IPPROTO_SCTP, onRecvSctpData, nullptr, 0, static_cast<void*>(this));

			if (this->socket == nullptr)
				MS_THROW_ERROR("usrsctp_socket() failed: %s", std::strerror(errno));

			usrsctp_set_ulpinfo(this->socket, static_cast<void*>(this));

			// Make the socket non-blocking.
			ret = usrsctp_set_non_blocking(this->socket, 1);

			if (ret < 0)
				MS_THROW_ERROR("usrsctp_set_non_blocking() failed: %s", std::strerror(errno));

			// Set SO_LINGER.
			struct linger lingerOpt; // NOLINT(cppcoreguidelines-pro-type-member-init)

			lingerOpt.l_onoff  = 1;
			lingerOpt.l_linger = 0;

			ret = usrsctp_setsockopt(this->socket, SOL_SOCKET, SO_LINGER, &lingerOpt, sizeof(lingerOpt));

			if (ret < 0)
				MS_THROW_ERROR("usrsctp_setsockopt(SO_LINGER) failed: %s", std::strerror(errno));

			// Set SCTP_ENABLE_STREAM_RESET.
			struct sctp_assoc_value av; // NOLINT(cppcoreguidelines-pro-type-member-init)

			av.assoc_id    = SCTP_ALL_ASSOC;
			av.assoc_value = 1;

			ret = usrsctp_setsockopt(this->socket, IPPROTO_SCTP, SCTP_ENABLE_STREAM_RESET, &av, sizeof(av));

			if (ret < 0)
				MS_THROW_ERROR(
				  "usrsctp_setsockopt(SCTP_ENABLE_STREAM_RESET) failed: %s", std::strerror(errno));

			// Set SCTP_NODELAY.
			uint32_t noDelay = 1;

			ret = usrsctp_setsockopt(this->socket, IPPROTO_SCTP, SCTP_NODELAY, &noDelay, sizeof(noDelay));

			if (ret < 0)
				MS_THROW_ERROR("usrsctp_setsockopt(SCTP_NODELAY) failed: %s", std::strerror(errno));

			// Enable events.
			struct sctp_event event; // NOLINT(cppcoreguidelines-pro-type-member-init)
			memset(&event, 0, sizeof(event));
			event.se_assoc_id = SCTP_ALL_ASSOC;
			event.se_on       = 1;

			for (size_t i{ 0 }; i < sizeof(EventTypes) / sizeof(uint16_t); ++i)
			{
				event.se_type = EventTypes[i];

				ret = usrsctp_setsockopt(this->socket, IPPROTO_SCTP, SCTP_EVENT, &event, sizeof(event));

				if (ret < 0)
					MS_THROW_ERROR("usrsctp_setsockopt(SCTP_EVENT) failed: %s", std::strerror(errno));
			}

			// Init message.
			struct sctp_initmsg initmsg; // NOLINT(cppcoreguidelines-pro-type-member-init)

			memset(&initmsg, 0, sizeof(struct sctp_initmsg));
			initmsg.sinit_num_ostreams  = this->numSctpStreams;
			initmsg.sinit_max_instreams = this->numSctpStreams;

			ret = usrsctp_setsockopt(this->socket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));

			if (ret < 0)
				MS_THROW_ERROR("usrsctp_setsockopt(SCTP_INITMSG) failed: %s", std::strerror(errno));

			// Server side.
			struct sockaddr_conn sconn; // NOLINT(cppcoreguidelines-pro-type-member-init)

			memset(&sconn, 0, sizeof(struct sockaddr_conn));
			sconn.sconn_family = AF_CONN;
			sconn.sconn_port   = htons(5000);
			sconn.sconn_addr   = static_cast<void*>(this);
#ifdef HAVE_SCONN_LEN
			rconn.sconn_len = sizeof(struct sockaddr_conn);
#endif

			ret = usrsctp_bind(
			  this->socket, reinterpret_cast<struct sockaddr*>(&sconn), sizeof(struct sockaddr_conn));

			if (ret < 0)
				MS_THROW_ERROR("usrsctp_bind() failed: %s", std::strerror(errno));

			// Client side.
			struct sockaddr_conn rconn; // NOLINT(cppcoreguidelines-pro-type-member-init)

			memset(&rconn, 0, sizeof(struct sockaddr_conn));
			rconn.sconn_family = AF_CONN;
			rconn.sconn_port   = htons(5000);
			rconn.sconn_addr   = static_cast<void*>(this);
#ifdef HAVE_SCONN_LEN
			rconn.sconn_len = sizeof(struct sockaddr_conn);
#endif

			ret = usrsctp_connect(
			  this->socket, reinterpret_cast<struct sockaddr*>(&rconn), sizeof(struct sockaddr_conn));

			if (ret < 0 && errno != EINPROGRESS)
				MS_THROW_ERROR("usrsctp_connect() failed: %s", std::strerror(errno));

			this->state = SctpState::CONNECTING;
			this->listener->OnSctpAssociationConnecting(this);
		}
		catch (const MediaSoupError& /*error*/)
		{
			this->state = SctpState::FAILED;
			this->listener->OnSctpAssociationFailed(this);
		}
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

#ifdef MS_LOG_DEV
		MS_DUMP_DATA(data, len);
#endif

		usrsctp_conninput(static_cast<void*>(this), data, len, 0);
	}

	void SctpAssociation::SendSctpMessage(
	  RTC::DataConsumer* dataConsumer, uint8_t ppid, const uint8_t* msg, size_t len)
	{
		MS_TRACE();

		// This must be controlled by the DataConsumer.
		MS_ASSERT(
		  len <= this->maxSctpMessageSize,
		  "given message exceeds maxSctpMessageSize value [maxSctpMessageSize:%zu, len:%zu]",
		  len,
		  this->maxSctpMessageSize);

		auto& parameters = dataConsumer->GetSctpStreamParameters();

		// Fill stcp_sendv_spa.
		struct sctp_sendv_spa spa; // NOLINT(cppcoreguidelines-pro-type-member-init)

		memset(&spa, 0, sizeof(struct sctp_sendv_spa));
		spa.sendv_sndinfo.snd_sid = parameters.streamId;

		if (parameters.ordered)
			spa.sendv_sndinfo.snd_flags = SCTP_EOR;
		else
			spa.sendv_sndinfo.snd_flags = SCTP_EOR | SCTP_UNORDERED;

		spa.sendv_sndinfo.snd_ppid = htonl(ppid);
		spa.sendv_flags            = SCTP_SEND_SNDINFO_VALID;

		// TODO: use SctpStreamParameters to indicate reliability:
		// https://tools.ietf.org/html/rfc3758
		spa.sendv_prinfo.pr_policy = SCTP_PR_SCTP_NONE;
		spa.sendv_prinfo.pr_value  = 0;

		int ret = usrsctp_sendv(
		  this->socket,
		  msg,
		  len,
		  nullptr,
		  0,
		  &spa,
		  static_cast<socklen_t>(sizeof(struct sctp_sendv_spa)),
		  SCTP_SENDV_SPA,
		  0);

		if (ret < 0)
			MS_ERROR("error sending SCTP message: %s", std::strerror(errno));
	}

	void SctpAssociation::DataProducerClosed(RTC::DataProducer* dataProducer)
	{
		MS_TRACE();

		auto streamId = dataProducer->GetSctpStreamParameters().streamId;

		// Send SCTP STREAM RESET to the remote.
		ResetOutgoingSctpStream(streamId);
	}

	void SctpAssociation::DataConsumerClosed(RTC::DataConsumer* dataConsumer)
	{
		MS_TRACE();

		auto streamId = dataConsumer->GetSctpStreamParameters().streamId;

		// Send SCTP STREAM RESET to the remote.
		ResetOutgoingSctpStream(streamId);
	}

	void SctpAssociation::ResetOutgoingSctpStream(uint16_t streamId)
	{
		MS_TRACE();

		// As per spec: https://tools.ietf.org/html/rfc6525#section-4.1
		socklen_t paramLen = sizeof(sctp_assoc_t) + (2 + 1) * sizeof(uint16_t);
		auto* srs          = static_cast<struct sctp_reset_streams*>(std::malloc(paramLen));

		srs->srs_flags          = SCTP_STREAM_RESET_OUTGOING;
		srs->srs_number_streams = 1;
		srs->srs_stream_list[0] = streamId; // No need for htonl().

		int ret = usrsctp_setsockopt(this->socket, IPPROTO_SCTP, SCTP_RESET_STREAMS, srs, paramLen);

		if (ret < 0)
		{
			MS_WARN_TAG(sctp, "usrsctp_setsockopt(SCTP_RESET_STREAMS) failed: %s", std::strerror(errno));
		}

		std::free(srs);
	}

	void SctpAssociation::OnUsrSctpSendSctpData(void* buffer, size_t len)
	{
		MS_TRACE();

		const uint8_t* data = static_cast<uint8_t*>(buffer);

#ifdef MS_LOG_DEV
		MS_DUMP_DATA(data, len);
#endif

		this->listener->OnSctpAssociationSendData(this, data, len);
	}

	void SctpAssociation::OnUsrSctpReceiveSctpData(
	  uint16_t streamId, uint8_t ppid, int flags, const uint8_t* data, size_t len)
	{
		// Ignore WebRTC DataChannel Control DATA chunks.
		if (ppid == 50)
		{
			MS_WARN_TAG(sctp, "ignoring SCTP data with ppid:50 (WebRTC DataChannel Control)");

			return;
		}

		auto eor = static_cast<bool>(flags & MSG_EOR);

		if (this->messageBufferLen + len > this->maxSctpMessageSize)
		{
			MS_WARN_TAG(
			  sctp,
			  "received message exceeds maxSctpMessageSize value [maxSctpMessageSize:%zu, len:%zu, EOR:%u]",
			  this->messageBufferLen + len,
			  this->maxSctpMessageSize,
			  eor ? 1 : 0);

			return;
		}

		// If end of message and there is no buffered data, notify it directly.
		if (eor && this->messageBufferLen == 0)
		{
			MS_DEBUG_DEV("directly notifying listener [eor:1, messageBufferLen:0]");

			this->listener->OnSctpAssociationMessageReceived(this, streamId, ppid, data, len);
		}
		// If end of message and there is buffered data, append data and notify buffer.
		else if (eor && this->messageBufferLen != 0)
		{
			std::memcpy(this->messageBuffer + this->messageBufferLen, data, len);
			this->messageBufferLen += len;

			MS_DEBUG_DEV("notifying listener [eor:1, messageBufferLen:%zu]", this->messageBufferLen);

			this->listener->OnSctpAssociationMessageReceived(
			  this, streamId, ppid, this->messageBuffer, this->messageBufferLen);

			this->messageBufferLen = 0;
		}
		// If non end of message, append data to the buffer.
		else if (!eor)
		{
			// Allocate the buffer if not already done.
			if (!this->messageBuffer)
				this->messageBuffer = new uint8_t[this->maxSctpMessageSize];

			std::memcpy(this->messageBuffer + this->messageBufferLen, data, len);
			this->messageBufferLen += len;

			MS_DEBUG_DEV("data buffered [eor:0, messageBufferLen:%zu]", this->messageBufferLen);
		}
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
				  "SCTP adaptation indication [%x]",
				  notification->sn_adaptation_event.sai_adaptation_ind);

				break;
			}

			case SCTP_ASSOC_CHANGE:
			{
				switch (notification->sn_assoc_change.sac_state)
				{
					case SCTP_COMM_UP:
					{
						MS_DEBUG_TAG(
						  sctp,
						  "SCTP association connected, streams [in:%" PRIu16 ", out:%" PRIu16 "]",
						  notification->sn_assoc_change.sac_inbound_streams,
						  notification->sn_assoc_change.sac_outbound_streams);

						if (this->state != SctpState::CONNECTED)
						{
							this->state = SctpState::CONNECTED;
							this->listener->OnSctpAssociationConnected(this);
						}

						break;
					}

					case SCTP_COMM_LOST:
					{
						if (notification->sn_header.sn_length > 0)
						{
							static const size_t BufferSize{ 1024 };
							static char buffer[BufferSize];

							uint32_t len = notification->sn_header.sn_length;

							for (uint32_t i{ 0 }; i < len; ++i)
							{
								std::snprintf(
								  buffer, BufferSize, " 0x%02x", notification->sn_assoc_change.sac_info[i]);
							}

							MS_DEBUG_TAG(sctp, "SCTP communication lost [info:%s]", buffer);
						}
						else
						{
							MS_DEBUG_TAG(sctp, "SCTP communication lost");
						}

						if (this->state != SctpState::CLOSED)
						{
							this->state = SctpState::CLOSED;
							this->listener->OnSctpAssociationClosed(this);
						}

						break;
					}

					case SCTP_RESTART:
					{
						MS_DEBUG_TAG(
						  sctp,
						  "SCTP remote association restarted, streams [in:%" PRIu16 ", out:%" PRIu16 "]",
						  notification->sn_assoc_change.sac_inbound_streams,
						  notification->sn_assoc_change.sac_outbound_streams);

						if (this->state != SctpState::CONNECTED)
						{
							this->state = SctpState::CONNECTED;
							this->listener->OnSctpAssociationConnected(this);
						}

						break;
					}

					case SCTP_SHUTDOWN_COMP:
					{
						MS_DEBUG_TAG(sctp, "SCTP association gracefully closed");

						if (this->state != SctpState::CLOSED)
						{
							this->state = SctpState::CLOSED;
							this->listener->OnSctpAssociationClosed(this);
						}

						break;
					}

					case SCTP_CANT_STR_ASSOC:
					{
						if (notification->sn_header.sn_length > 0)
						{
							static const size_t BufferSize{ 1024 };
							static char buffer[BufferSize];

							uint32_t len = notification->sn_header.sn_length;

							for (uint32_t i{ 0 }; i < len; i++)
							{
								std::snprintf(
								  buffer, BufferSize, " 0x%02x", notification->sn_assoc_change.sac_info[i]);
							}

							MS_WARN_TAG(sctp, "SCTP setup failed: %s", buffer);
						}

						if (this->state != SctpState::FAILED)
						{
							this->state = SctpState::FAILED;
							this->listener->OnSctpAssociationFailed(this);
						}

						break;
					}

					default:;
				}

				break;
			}

			// https://tools.ietf.org/html/rfc6525#section-6.1.2.
			case SCTP_ASSOC_RESET_EVENT:
			{
				MS_DEBUG_TAG(sctp, "SCTP association reset event received");

				break;
			}

			// An Operation Error is not considered fatal in and of itself, but may be
			// used with an ABORT chunk to report a fatal condition.
			case SCTP_REMOTE_ERROR:
			{
				static const size_t BufferSize{ 1024 };
				static char buffer[BufferSize];

				uint32_t len = notification->sn_remote_error.sre_length - sizeof(struct sctp_remote_error);

				for (uint32_t i{ 0 }; i < len; i++)
				{
					std::snprintf(buffer, BufferSize, "0x%02x", notification->sn_remote_error.sre_data[i]);
				}

				MS_WARN_TAG(
				  sctp,
				  "remote SCTP association error [type:0x%04x, data:%s]",
				  notification->sn_remote_error.sre_error,
				  buffer);

				break;
			}

			// When a peer sends a SHUTDOWN, SCTP delivers this notification to
			// inform the application that it should cease sending data.
			case SCTP_SHUTDOWN_EVENT:
			{
				MS_DEBUG_TAG(sctp, "remote SCTP association shutdown");

				if (this->state != SctpState::CLOSED)
				{
					this->state = SctpState::CLOSED;
					this->listener->OnSctpAssociationClosed(this);
				}

				break;
			}

			case SCTP_SEND_FAILED_EVENT:
			{
				static const size_t BufferSize{ 1024 };
				static char buffer[BufferSize];

				uint32_t len =
				  notification->sn_send_failed_event.ssfe_length - sizeof(struct sctp_send_failed_event);

				for (uint32_t i{ 0 }; i < len; i++)
				{
					std::snprintf(buffer, BufferSize, "0x%02x", notification->sn_send_failed_event.ssfe_data[i]);
				}

				MS_WARN_TAG(
				  sctp,
				  "SCTP message sent failure [streamId:%" PRIu16 ", ppid:%" PRIu32
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

				// NOTE: We may honor it and reply the remote with our own SCTP_RESET_STREAMS
				// notification. However we rely on signaling so our reset will be sent when
				// the corresponding DataProducer or DataConsumer is closed.

				break;
			}

			case SCTP_STREAM_CHANGE_EVENT:
			{
				MS_DEBUG_TAG(
				  sctp,
				  "SCTP stream changed, streams [in:%" PRIu16 ", out:%" PRIu16 ", flags:%x]",
				  notification->sn_strchange_event.strchange_instrms,
				  notification->sn_strchange_event.strchange_outstrms,
				  notification->sn_strchange_event.strchange_flags);

				break;
			}

			default:
			{
				// TODO: Move to MS_DEBUG_TAG once we know which the cases are.
				MS_WARN_TAG(
				  sctp, "unhandled SCTP event received [type:%" PRIu16 "]", notification->sn_header.sn_type);
			}
		}
	}
} // namespace RTC
