#define MS_CLASS "RTC::PipeTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/PipeTransport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()

namespace RTC
{
	/* Static. */

	// NOTE: PipeTransport just allows AES_CM_128_HMAC_SHA1_80 SRTP crypto suite.
	RTC::SrtpSession::CryptoSuite PipeTransport::srtpCryptoSuite{
		RTC::SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_80
	};
	std::string PipeTransport::srtpCryptoSuiteString{ "AES_CM_128_HMAC_SHA1_80" };
	size_t PipeTransport::srtpMasterLength{ 30 };

	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	PipeTransport::PipeTransport(const std::string& id, RTC::Transport::Listener* listener, json& data)
	  : RTC::Transport::Transport(id, listener, data)
	{
		MS_TRACE();

		auto jsonListenIpIt = data.find("listenIp");

		if (jsonListenIpIt == data.end())
			MS_THROW_TYPE_ERROR("missing listenIp");
		else if (!jsonListenIpIt->is_object())
			MS_THROW_TYPE_ERROR("wrong listenIp (not an object)");

		auto jsonIpIt = jsonListenIpIt->find("ip");

		if (jsonIpIt == jsonListenIpIt->end())
			MS_THROW_TYPE_ERROR("missing listenIp.ip");
		else if (!jsonIpIt->is_string())
			MS_THROW_TYPE_ERROR("wrong listenIp.ip (not an string)");

		this->listenIp.ip.assign(jsonIpIt->get<std::string>());

		// This may throw.
		Utils::IP::NormalizeIp(this->listenIp.ip);

		auto jsonAnnouncedIpIt = jsonListenIpIt->find("announcedIp");

		if (jsonAnnouncedIpIt != jsonListenIpIt->end())
		{
			if (!jsonAnnouncedIpIt->is_string())
				MS_THROW_TYPE_ERROR("wrong listenIp.announcedIp (not an string");

			this->listenIp.announcedIp.assign(jsonAnnouncedIpIt->get<std::string>());
		}

		uint16_t port{ 0 };
		auto jsonPortIt = data.find("port");

		if (jsonPortIt != data.end())
		{
			if (!(jsonPortIt->is_number() && Utils::Json::IsPositiveInteger(*jsonPortIt)))
				MS_THROW_TYPE_ERROR("wrong port (not a positive number)");

			port = jsonPortIt->get<uint16_t>();
		}

		auto jsonEnableRtxIt = data.find("enableRtx");

		if (jsonEnableRtxIt != data.end() && jsonEnableRtxIt->is_boolean())
			this->rtx = jsonEnableRtxIt->get<bool>();

		auto jsonEnableSrtpIt = data.find("enableSrtp");

		// clang-format off
		if (
			jsonEnableSrtpIt != data.end() &&
			jsonEnableSrtpIt->is_boolean() &&
			jsonEnableSrtpIt->get<bool>()
		)
		// clang-format on
		{
			this->srtpKey       = Utils::Crypto::GetRandomString(PipeTransport::srtpMasterLength);
			this->srtpKeyBase64 = Utils::String::Base64Encode(this->srtpKey);
		}

		try
		{
			// This may throw.
			if (port != 0)
				this->udpSocket = new RTC::UdpSocket(this, this->listenIp.ip, port);
			else
				this->udpSocket = new RTC::UdpSocket(this, this->listenIp.ip);
		}
		catch (const MediaSoupError& error)
		{
			// Must delete everything since the destructor won't be called.

			delete this->udpSocket;
			this->udpSocket = nullptr;

			throw;
		}
	}

	PipeTransport::~PipeTransport()
	{
		MS_TRACE();

		delete this->udpSocket;
		this->udpSocket = nullptr;

		delete this->tuple;
		this->tuple = nullptr;

		delete this->srtpSendSession;
		this->srtpSendSession = nullptr;

		delete this->srtpRecvSession;
		this->srtpRecvSession = nullptr;
	}

	void PipeTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

		// Add tuple.
		if (this->tuple)
		{
			this->tuple->FillJson(jsonObject["tuple"]);
		}
		else
		{
			jsonObject["tuple"] = json::object();
			auto jsonTupleIt    = jsonObject.find("tuple");

			if (this->listenIp.announcedIp.empty())
				(*jsonTupleIt)["localIp"] = this->udpSocket->GetLocalIp();
			else
				(*jsonTupleIt)["localIp"] = this->listenIp.announcedIp;

			(*jsonTupleIt)["localPort"] = this->udpSocket->GetLocalPort();
			(*jsonTupleIt)["protocol"]  = "udp";
		}

		// Add rtx.
		jsonObject["rtx"] = this->rtx;

		// Add srtpParameters.
		if (HasSrtp())
		{
			jsonObject["srtpParameters"] = json::object();
			auto jsonSrtpParametersIt    = jsonObject.find("srtpParameters");

			(*jsonSrtpParametersIt)["cryptoSuite"] = PipeTransport::srtpCryptoSuiteString;
			(*jsonSrtpParametersIt)["keyBase64"]   = this->srtpKeyBase64;
		}
	}

	void PipeTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJsonStats(jsonArray);

		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "pipe-transport";

		if (this->tuple)
		{
			this->tuple->FillJson(jsonObject["tuple"]);
		}
		else
		{
			// Add tuple.
			jsonObject["tuple"] = json::object();
			auto jsonTupleIt    = jsonObject.find("tuple");

			if (this->listenIp.announcedIp.empty())
				(*jsonTupleIt)["localIp"] = this->udpSocket->GetLocalIp();
			else
				(*jsonTupleIt)["localIp"] = this->listenIp.announcedIp;

			(*jsonTupleIt)["localPort"] = this->udpSocket->GetLocalPort();
			(*jsonTupleIt)["protocol"]  = "udp";
		}
	}

	void PipeTransport::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::TRANSPORT_CONNECT:
			{
				// Ensure this method is not called twice.
				if (this->tuple)
					MS_THROW_ERROR("connect() already called");

				try
				{
					std::string ip;
					uint16_t port{ 0u };
					std::string srtpKeyBase64;

					auto jsonSrtpParametersIt = request->data.find("srtpParameters");

					if (!HasSrtp() && jsonSrtpParametersIt != request->data.end())
					{
						MS_THROW_TYPE_ERROR("invalid srtpParameters (SRTP not enabled)");
					}
					else if (HasSrtp())
					{
						// clang-format off
						if (
							jsonSrtpParametersIt == request->data.end() ||
							!jsonSrtpParametersIt->is_object()
						)
						// clang-format on
						{
							MS_THROW_TYPE_ERROR("missing srtpParameters (SRTP enabled)");
						}

						auto jsonCryptoSuiteIt = jsonSrtpParametersIt->find("cryptoSuite");

						// clang-format off
						if (
							jsonCryptoSuiteIt == jsonSrtpParametersIt->end() ||
							!jsonCryptoSuiteIt->is_string()
						)
						// clang-format on
						{
							MS_THROW_TYPE_ERROR("missing srtpParameters.cryptoSuite)");
						}

						// NOTE: We just use AES_CM_128_HMAC_SHA1_80 as SRTP crypto suite in
						// PipeTransport.
						if (jsonCryptoSuiteIt->get<std::string>() != PipeTransport::srtpCryptoSuiteString)
						{
							MS_THROW_TYPE_ERROR("invalid/unsupported srtpParameters.cryptoSuite");
						}

						auto jsonKeyBase64It = jsonSrtpParametersIt->find("keyBase64");

						// clang-format off
						if (
							jsonKeyBase64It == jsonSrtpParametersIt->end() ||
							!jsonKeyBase64It->is_string()
						)
						// clang-format on
						{
							MS_THROW_TYPE_ERROR("missing srtpParameters.keyBase64)");
						}

						srtpKeyBase64 = jsonKeyBase64It->get<std::string>();

						size_t outLen;
						// This may throw.
						auto* srtpKey = Utils::String::Base64Decode(srtpKeyBase64, outLen);

						if (outLen != PipeTransport::srtpMasterLength)
							MS_THROW_TYPE_ERROR("invalid decoded SRTP key length");

						auto* srtpLocalKey  = new uint8_t[PipeTransport::srtpMasterLength];
						auto* srtpRemoteKey = new uint8_t[PipeTransport::srtpMasterLength];

						std::memcpy(srtpLocalKey, this->srtpKey.c_str(), PipeTransport::srtpMasterLength);
						std::memcpy(srtpRemoteKey, srtpKey, PipeTransport::srtpMasterLength);

						try
						{
							this->srtpSendSession = new RTC::SrtpSession(
							  RTC::SrtpSession::Type::OUTBOUND,
							  PipeTransport::srtpCryptoSuite,
							  srtpLocalKey,
							  PipeTransport::srtpMasterLength);
						}
						catch (const MediaSoupError& error)
						{
							delete[] srtpLocalKey;
							delete[] srtpRemoteKey;

							MS_THROW_ERROR("error creating SRTP sending session: %s", error.what());
						}

						try
						{
							this->srtpRecvSession = new RTC::SrtpSession(
							  RTC::SrtpSession::Type::INBOUND,
							  PipeTransport::srtpCryptoSuite,
							  srtpRemoteKey,
							  PipeTransport::srtpMasterLength);
						}
						catch (const MediaSoupError& error)
						{
							delete[] srtpLocalKey;
							delete[] srtpRemoteKey;

							MS_THROW_ERROR("error creating SRTP receiving session: %s", error.what());
						}

						delete[] srtpLocalKey;
						delete[] srtpRemoteKey;
					}

					auto jsonIpIt = request->data.find("ip");

					if (jsonIpIt == request->data.end() || !jsonIpIt->is_string())
						MS_THROW_TYPE_ERROR("missing ip");

					ip = jsonIpIt->get<std::string>();

					// This may throw.
					Utils::IP::NormalizeIp(ip);

					auto jsonPortIt = request->data.find("port");

					// clang-format off
					if (
						jsonPortIt == request->data.end() ||
						!Utils::Json::IsPositiveInteger(*jsonPortIt)
					)
					// clang-format on
					{
						MS_THROW_TYPE_ERROR("missing port");
					}

					port = jsonPortIt->get<uint16_t>();

					int err;

					switch (Utils::IP::GetFamily(ip))
					{
						case AF_INET:
						{
							err = uv_ip4_addr(
							  ip.c_str(),
							  static_cast<int>(port),
							  reinterpret_cast<struct sockaddr_in*>(&this->remoteAddrStorage));

							if (err != 0)
								MS_THROW_ERROR("uv_ip4_addr() failed: %s", uv_strerror(err));

							break;
						}

						case AF_INET6:
						{
							err = uv_ip6_addr(
							  ip.c_str(),
							  static_cast<int>(port),
							  reinterpret_cast<struct sockaddr_in6*>(&this->remoteAddrStorage));

							if (err != 0)
								MS_THROW_ERROR("uv_ip6_addr() failed: %s", uv_strerror(err));

							break;
						}

						default:
						{
							MS_THROW_ERROR("invalid IP '%s'", ip.c_str());
						}
					}

					// Create the tuple.
					this->tuple = new RTC::TransportTuple(
					  this->udpSocket, reinterpret_cast<struct sockaddr*>(&this->remoteAddrStorage));

					if (!this->listenIp.announcedIp.empty())
						this->tuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);
				}
				catch (const MediaSoupError& error)
				{
					delete this->tuple;
					this->tuple = nullptr;

					delete this->srtpSendSession;
					this->srtpSendSession = nullptr;

					delete this->srtpRecvSession;
					this->srtpRecvSession = nullptr;

					throw;
				}

				// Tell the caller about the selected local DTLS role.
				json data = json::object();

				this->tuple->FillJson(data["tuple"]);

				request->Accept(data);

				// Assume we are connected (there is no much more we can do to know it)
				// and tell the parent class.
				RTC::Transport::Connected();

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Transport::HandleRequest(request);
			}
		}
	}

	void PipeTransport::HandleNotification(PayloadChannel::Notification* notification)
	{
		MS_TRACE();

		// Pass it to the parent class.
		RTC::Transport::HandleNotification(notification);
	}

	inline bool PipeTransport::IsConnected() const
	{
		return this->tuple;
	}

	inline bool PipeTransport::HasSrtp() const
	{
		return !this->srtpKey.empty();
	}

	void PipeTransport::SendRtpPacket(
	  RTC::Consumer* /*consumer*/, RTC::RtpPacket* packet, RTC::Transport::onSendCallback* cb)
	{
		MS_TRACE();

		if (!IsConnected())
		{
			if (cb)
			{
				(*cb)(false);
				delete cb;
			}

			return;
		}

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		if (HasSrtp() && !this->srtpSendSession->EncryptRtp(&data, &intLen))
		{
			if (cb)
			{
				(*cb)(false);
				delete cb;
			}

			return;
		}

		auto len = static_cast<size_t>(intLen);

		this->tuple->Send(data, len, cb);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PipeTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		if (HasSrtp() && !this->srtpSendSession->EncryptRtcp(&data, &intLen))
			return;

		auto len = static_cast<size_t>(intLen);

		this->tuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PipeTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		if (HasSrtp() && !this->srtpSendSession->EncryptRtcp(&data, &intLen))
			return;

		auto len = static_cast<size_t>(intLen);

		this->tuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PipeTransport::SendMessage(
	  RTC::DataConsumer* dataConsumer, uint32_t ppid, const uint8_t* msg, size_t len, onQueuedCallback* cb)
	{
		MS_TRACE();

		this->sctpAssociation->SendSctpMessage(dataConsumer, ppid, msg, len, cb);
	}

	void PipeTransport::SendSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		this->tuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PipeTransport::RecvStreamClosed(uint32_t ssrc)
	{
		MS_TRACE();

		if (this->srtpRecvSession)
		{
			this->srtpRecvSession->RemoveStream(ssrc);
		}
	}

	void PipeTransport::SendStreamClosed(uint32_t ssrc)
	{
		MS_TRACE();

		if (this->srtpSendSession)
		{
			this->srtpSendSession->RemoveStream(ssrc);
		}
	}

	inline void PipeTransport::OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Increase receive transmission.
		RTC::Transport::DataReceived(len);

		// Check if it's RTCP.
		if (RTC::RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataReceived(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RTC::RtpPacket::IsRtp(data, len))
		{
			OnRtpDataReceived(tuple, data, len);
		}
		// Check if it's SCTP.
		else if (RTC::SctpAssociation::IsSctp(data, len))
		{
			OnSctpDataReceived(tuple, data, len);
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

	inline void PipeTransport::OnRtpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Decrypt the SRTP packet.
		auto intLen = static_cast<int>(len);

		if (HasSrtp() && !this->srtpRecvSession->DecryptSrtp(const_cast<uint8_t*>(data), &intLen))
		{
			RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, static_cast<size_t>(intLen));

			if (!packet)
			{
				MS_WARN_TAG(srtp, "DecryptSrtp() failed due to an invalid RTP packet");
			}
			else
			{
				MS_WARN_TAG(
				  srtp,
				  "DecryptSrtp() failed [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetPayloadType(),
				  packet->GetSequenceNumber());

				delete packet;
			}

			return;
		}

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, static_cast<size_t>(intLen));

		if (!packet)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Verify that the packet's tuple matches our tuple.
		if (!this->tuple->Compare(tuple))
		{
			MS_DEBUG_TAG(rtp, "ignoring RTP packet from unknown IP:port");

			// Remove this SSRC.
			RecvStreamClosed(packet->GetSsrc());

			delete packet;

			return;
		}

		// Pass the packet to the parent transport.
		RTC::Transport::ReceiveRtpPacket(packet);
	}

	inline void PipeTransport::OnRtcpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Decrypt the SRTCP packet.
		auto intLen = static_cast<int>(len);

		if (HasSrtp() && !this->srtpRecvSession->DecryptSrtcp(const_cast<uint8_t*>(data), &intLen))
		{
			return;
		}

		// Verify that the packet's tuple matches our tuple.
		if (!this->tuple->Compare(tuple))
		{
			MS_DEBUG_TAG(rtcp, "ignoring RTCP packet from unknown IP:port");

			return;
		}

		RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, static_cast<size_t>(intLen));

		if (!packet)
		{
			MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

			return;
		}

		// Pass the packet to the parent transport.
		RTC::Transport::ReceiveRtcpPacket(packet);
	}

	inline void PipeTransport::OnSctpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Verify that the packet's tuple matches our tuple.
		if (!this->tuple->Compare(tuple))
		{
			MS_DEBUG_TAG(sctp, "ignoring SCTP packet from unknown IP:port");

			return;
		}

		// Pass it to the parent transport.
		RTC::Transport::ReceiveSctpData(data, len);
	}

	inline void PipeTransport::OnUdpSocketPacketReceived(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketReceived(&tuple, data, len);
	}
} // namespace RTC
