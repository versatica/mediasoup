#define MS_CLASS "RTC::SctpAssociation"
#define MS_LOG_DEV

#include "RTC/SctpAssociation.hpp"
#include "DepUsrSCTP.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
// #include "Channel/Notifier.hpp"

/* Static methods for usrsctp callbacks. */

inline static int onRecvSctpData(
  struct socket* /*sock*/,
  union sctp_sockstore /*addr*/,
  void* data,
  size_t datalen,
  struct sctp_rcvinfo rcv,
  int /*flags*/,
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

		DepUsrSCTP::IncreaseSctpAssociations();
	}

	SctpAssociation::~SctpAssociation()
	{
		MS_TRACE();

		if (this->socket)
			usrsctp_close(this->socket);

		DepUsrSCTP::DecreaseSctpAssociations();
	}

	bool SctpAssociation::Run()
	{
		MS_TRACE();

		int ret;

		// Without this function call usrsctp_bind will fail as follows:
		// "usrsctp_bind() failed: Can't assign requested address"
		// TODO: should we unregister on destructor?
		// TODO: Let's use static_cast instead.
		usrsctp_register_address((void*)this);

		// Disable explicit congestion notifications (ecn).
		usrsctp_sysctl_set_sctp_ecn_enable(0);

		// TODO: Let's use static_cast instead.
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
		sconn.sconn_addr   = (void*)this; // TODO: Let's use static_cast instead.
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

		// TODO: Let's use static_cast or reinterpret_cast instead.
		ret = usrsctp_connect(this->socket, (struct sockaddr*)&rconn, sizeof(struct sockaddr_conn));

		if (ret < 0 && errno != EINPROGRESS)
			MS_THROW_ERROR("usrsctp_connect() failed: %s", std::strerror(errno));

		// TMP.
		return true;
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

		MS_ERROR("SctpAssociation::ProcessSctpData [id:%p]", this);

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

		MS_ERROR("SctpAssociation::SendSctpMessage [id:%p]", this);

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

		// TODO: Let's use static_cast or reinterpret_cast instead.
		int ret = usrsctp_sendv(
		  this->socket, msg, len, nullptr, 0, &spa, (socklen_t)sizeof(struct sctp_sendv_spa), SCTP_SENDV_SPA, 0);

		if (ret < 0)
			MS_ERROR("error sending usctp data: %s", std::strerror(errno));
	}

	void SctpAssociation::OnUsrSctpSendSctpData(void* buffer, size_t len)
	{
		MS_TRACE();

		MS_ERROR("OnUsrSctpSendSctpData [id:%p]", this);

		const uint8_t* data = static_cast<uint8_t*>(buffer);

		// TODO: accounting, rate, etc?

		this->listener->OnSctpAssociationSendData(this, data, len);
	}

	void SctpAssociation::OnUsrSctpReceiveSctpData(uint16_t streamId, const uint8_t* msg, size_t len)
	{
		MS_ERROR("SctpAssociation::OnUsrSctpReceiveSctpData");

		MS_DUMP_DATA(msg, len);

		this->listener->OnSctpAssociationMessageReceived(this, streamId, msg, len);
	}
} // namespace RTC
