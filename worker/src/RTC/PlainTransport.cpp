#define MS_CLASS "RTC::PlainTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/PlainTransport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/ChannelNotifier.hpp"

namespace RTC
{
	/* Static. */

	// NOTE: PlainTransport allows AES_CM_128_HMAC_SHA1_80 and
	// AES_CM_128_HMAC_SHA1_32 SRTP crypto suites.
	// clang-format off
	absl::flat_hash_map<std::string, RTC::SrtpSession::CryptoSuite> PlainTransport::string2SrtpCryptoSuite =
	{
		{ "AES_CM_128_HMAC_SHA1_80", RTC::SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_80 },
		{ "AES_CM_128_HMAC_SHA1_32", RTC::SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_32 }
	};
	absl::flat_hash_map<RTC::SrtpSession::CryptoSuite, std::string> PlainTransport::srtpCryptoSuite2String =
	{
		{ RTC::SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_80, "AES_CM_128_HMAC_SHA1_80" },
		{ RTC::SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_32, "AES_CM_128_HMAC_SHA1_32" }
	};
	// clang-format on
	size_t PlainTransport::srtpMasterLength{ 30 };

	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	PlainTransport::PlainTransport(const std::string& id, RTC::Transport::Listener* listener, json& data)
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

		auto jsonRtcpMuxIt = data.find("rtcpMux");

		if (jsonRtcpMuxIt != data.end())
		{
			if (!jsonRtcpMuxIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong rtcpMux (not a boolean)");

			this->rtcpMux = jsonRtcpMuxIt->get<bool>();
		}

		auto jsonComediaIt = data.find("comedia");

		if (jsonComediaIt != data.end())
		{
			if (!jsonComediaIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong comedia (not a boolean)");

			this->comedia = jsonComediaIt->get<bool>();
		}

		auto jsonEnableSrtpIt = data.find("enableSrtp");

		// clang-format off
		if (
			jsonEnableSrtpIt != data.end() &&
			jsonEnableSrtpIt->is_boolean() &&
			jsonEnableSrtpIt->get<bool>()
		)
		// clang-format on
		{
			auto jsonSrtpCryptoSuiteIt = data.find("srtpCryptoSuite");

			if (jsonSrtpCryptoSuiteIt == data.end() || !jsonSrtpCryptoSuiteIt->is_string())
				MS_THROW_TYPE_ERROR("missing srtpCryptoSuite)");

			// Ensure it's a crypto suite supported by us.
			auto it =
			  PlainTransport::string2SrtpCryptoSuite.find(jsonSrtpCryptoSuiteIt->get<std::string>());

			if (it == PlainTransport::string2SrtpCryptoSuite.end())
				MS_THROW_TYPE_ERROR("invalid/unsupported srtpCryptoSuite");

			// NOTE: The SRTP crypto suite may change later on connect().
			this->srtpCryptoSuite = it->second;
			this->srtpKey         = Utils::Crypto::GetRandomString(PlainTransport::srtpMasterLength);
			this->srtpKeyBase64   = Utils::String::Base64Encode(this->srtpKey);
		}

		try
		{
			// This may throw.
			// This may throw.
			if (port != 0)
				this->udpSocket = new RTC::UdpSocket(this, this->listenIp.ip, port);
			else
				this->udpSocket = new RTC::UdpSocket(this, this->listenIp.ip);

			if (!this->rtcpMux)
			{
				// This may throw.
				this->rtcpUdpSocket = new RTC::UdpSocket(this, this->listenIp.ip);
			}
		}
		catch (const MediaSoupError& error)
		{
			delete this->udpSocket;
			this->udpSocket = nullptr;

			delete this->rtcpUdpSocket;
			this->rtcpUdpSocket = nullptr;

			throw;
		}
	}

	PlainTransport::~PlainTransport()
	{
		MS_TRACE();

		delete this->udpSocket;
		this->udpSocket = nullptr;

		delete this->rtcpUdpSocket;
		this->rtcpUdpSocket = nullptr;

		delete this->tuple;
		this->tuple = nullptr;

		delete this->rtcpTuple;
		this->rtcpTuple = nullptr;

		delete this->srtpSendSession;
		this->srtpSendSession = nullptr;

		delete this->srtpRecvSession;
		this->srtpRecvSession = nullptr;
	}

	void PlainTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

		// Add rtcpMux.
		jsonObject["rtcpMux"] = this->rtcpMux;

		// Add comedia.
		jsonObject["comedia"] = this->comedia;

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

		// Add rtcpTuple.
		if (!this->rtcpMux)
		{
			if (this->rtcpTuple)
			{
				this->rtcpTuple->FillJson(jsonObject["rtcpTuple"]);
			}
			else
			{
				jsonObject["rtcpTuple"] = json::object();
				auto jsonRtcpTupleIt    = jsonObject.find("rtcpTuple");

				if (this->listenIp.announcedIp.empty())
					(*jsonRtcpTupleIt)["localIp"] = this->rtcpUdpSocket->GetLocalIp();
				else
					(*jsonRtcpTupleIt)["localIp"] = this->listenIp.announcedIp;

				(*jsonRtcpTupleIt)["localPort"] = this->rtcpUdpSocket->GetLocalPort();
				(*jsonRtcpTupleIt)["protocol"]  = "udp";
			}
		}

		// Add srtpParameters.
		if (HasSrtp())
		{
			jsonObject["srtpParameters"] = json::object();
			auto jsonSrtpParametersIt    = jsonObject.find("srtpParameters");

			(*jsonSrtpParametersIt)["cryptoSuite"] =
			  PlainTransport::srtpCryptoSuite2String[this->srtpCryptoSuite];
			(*jsonSrtpParametersIt)["keyBase64"] = this->srtpKeyBase64;
		}
	}

	void PlainTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJsonStats(jsonArray);

		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "plain-rtp-transport";

		// Add rtcpMux.
		jsonObject["rtcpMux"] = this->rtcpMux;

		// Add comedia.
		jsonObject["comedia"] = this->comedia;

		if (this->tuple)
		{
			// Add tuple.
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

		// Add rtcpTuple.
		if (!this->rtcpMux && this->rtcpTuple)
			this->rtcpTuple->FillJson(jsonObject["rtcpTuple"]);
	}

	void PlainTransport::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::TRANSPORT_CONNECT:
			{
				// Ensure this method is not called twice.
				if (this->connectCalled)
					MS_THROW_ERROR("connect() already called");

				try
				{
					std::string ip;
					uint16_t port{ 0u };
					uint16_t rtcpPort{ 0u };
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

						// Ensure it's a crypto suite supported by us.
						auto it =
						  PlainTransport::string2SrtpCryptoSuite.find(jsonCryptoSuiteIt->get<std::string>());

						if (it == PlainTransport::string2SrtpCryptoSuite.end())
							MS_THROW_TYPE_ERROR("invalid/unsupported srtpParameters.cryptoSuite");

						// Update out SRTP crypto suite wuth the one used by the remote.
						this->srtpCryptoSuite = it->second;

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

						if (outLen != PlainTransport::srtpMasterLength)
							MS_THROW_TYPE_ERROR("invalid decoded SRTP key length");

						auto* srtpLocalKey  = new uint8_t[PlainTransport::srtpMasterLength];
						auto* srtpRemoteKey = new uint8_t[PlainTransport::srtpMasterLength];

						std::memcpy(srtpLocalKey, this->srtpKey.c_str(), PlainTransport::srtpMasterLength);
						std::memcpy(srtpRemoteKey, srtpKey, PlainTransport::srtpMasterLength);

						try
						{
							this->srtpSendSession = new RTC::SrtpSession(
							  RTC::SrtpSession::Type::OUTBOUND,
							  this->srtpCryptoSuite,
							  srtpLocalKey,
							  PlainTransport::srtpMasterLength);
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
							  this->srtpCryptoSuite,
							  srtpRemoteKey,
							  PlainTransport::srtpMasterLength);
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

					if (!this->comedia)
					{
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

						auto jsonRtcpPortIt = request->data.find("rtcpPort");

						// clang-format off
						if (
							jsonRtcpPortIt != request->data.end() &&
							Utils::Json::IsPositiveInteger(*jsonRtcpPortIt)
						)
						// clang-format on
						{
							if (this->rtcpMux)
								MS_THROW_TYPE_ERROR("cannot set rtcpPort with rtcpMux enabled");

							rtcpPort = jsonRtcpPortIt->get<uint16_t>();
						}
						else
						{
							if (!this->rtcpMux)
								MS_THROW_TYPE_ERROR("missing rtcpPort (required with rtcpMux disabled)");
						}

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

						if (!this->rtcpMux)
						{
							switch (Utils::IP::GetFamily(ip))
							{
								case AF_INET:
								{
									err = uv_ip4_addr(
									  ip.c_str(),
									  static_cast<int>(rtcpPort),
									  reinterpret_cast<struct sockaddr_in*>(&this->rtcpRemoteAddrStorage));

									if (err != 0)
										MS_THROW_ERROR("uv_ip4_addr() failed: %s", uv_strerror(err));

									break;
								}

								case AF_INET6:
								{
									err = uv_ip6_addr(
									  ip.c_str(),
									  static_cast<int>(rtcpPort),
									  reinterpret_cast<struct sockaddr_in6*>(&this->rtcpRemoteAddrStorage));

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
							this->rtcpTuple = new RTC::TransportTuple(
							  this->rtcpUdpSocket,
							  reinterpret_cast<struct sockaddr*>(&this->rtcpRemoteAddrStorage));

							if (!this->listenIp.announcedIp.empty())
								this->rtcpTuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);
						}
					}
				}
				catch (const MediaSoupError& error)
				{
					delete this->tuple;
					this->tuple = nullptr;

					delete this->rtcpTuple;
					this->rtcpTuple = nullptr;

					delete this->srtpSendSession;
					this->srtpSendSession = nullptr;

					delete this->srtpRecvSession;
					this->srtpRecvSession = nullptr;

					throw;
				}

				this->connectCalled = true;

				// Tell the caller about the selected local DTLS role.
				json data = json::object();

				if (this->tuple)
					this->tuple->FillJson(data["tuple"]);

				if (!this->rtcpMux && this->rtcpTuple)
					this->rtcpTuple->FillJson(data["rtcpTuple"]);

				if (HasSrtp())
				{
					data["srtpParameters"]    = json::object();
					auto jsonSrtpParametersIt = data.find("srtpParameters");

					(*jsonSrtpParametersIt)["cryptoSuite"] =
					  PlainTransport::srtpCryptoSuite2String[this->srtpCryptoSuite];
					(*jsonSrtpParametersIt)["keyBase64"] = this->srtpKeyBase64;
				}

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

	void PlainTransport::HandleNotification(PayloadChannel::Notification* notification)
	{
		MS_TRACE();

		// Pass it to the parent class.
		RTC::Transport::HandleNotification(notification);
	}

	inline bool PlainTransport::IsConnected() const
	{
		return this->tuple;
	}

	inline bool PlainTransport::HasSrtp() const
	{
		return !this->srtpKey.empty();
	}

	inline bool PlainTransport::IsSrtpReady() const
	{
		return HasSrtp() && this->srtpSendSession && this->srtpRecvSession;
	}

	void PlainTransport::SendRtpPacket(
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

	void PlainTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		if (HasSrtp() && !this->srtpSendSession->EncryptRtcp(&data, &intLen))
			return;

		auto len = static_cast<size_t>(intLen);

		if (this->rtcpMux)
			this->tuple->Send(data, len);
		else if (this->rtcpTuple)
			this->rtcpTuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PlainTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		if (HasSrtp() && !this->srtpSendSession->EncryptRtcp(&data, &intLen))
			return;

		auto len = static_cast<size_t>(intLen);

		if (this->rtcpMux)
			this->tuple->Send(data, len);
		else if (this->rtcpTuple)
			this->rtcpTuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PlainTransport::SendMessage(
	  RTC::DataConsumer* dataConsumer, uint32_t ppid, const uint8_t* msg, size_t len, onQueuedCallback* cb)
	{
		MS_TRACE();

		this->sctpAssociation->SendSctpMessage(dataConsumer, ppid, msg, len, cb);
	}

	void PlainTransport::SendSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		this->tuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PlainTransport::RecvStreamClosed(uint32_t ssrc)
	{
		MS_TRACE();

		if (this->srtpRecvSession)
		{
			this->srtpRecvSession->RemoveStream(ssrc);
		}
	}

	void PlainTransport::SendStreamClosed(uint32_t ssrc)
	{
		MS_TRACE();

		if (this->srtpSendSession)
		{
			this->srtpSendSession->RemoveStream(ssrc);
		}
	}

	inline void PlainTransport::OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
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

	inline void PlainTransport::OnRtpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (HasSrtp() && !IsSrtpReady())
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

		// If we don't have a RTP tuple yet, check whether comedia mode is set.
		if (!this->tuple)
		{
			if (!this->comedia)
			{
				MS_DEBUG_TAG(rtp, "ignoring RTP packet while not connected");

				// Remove this SSRC.
				RecvStreamClosed(packet->GetSsrc());

				delete packet;

				return;
			}

			MS_DEBUG_TAG(rtp, "setting RTP tuple (comedia mode enabled)");

			auto wasConnected = IsConnected();

			this->tuple = new RTC::TransportTuple(tuple);

			if (!this->listenIp.announcedIp.empty())
				this->tuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);

			// If not yet connected do it now.
			if (!wasConnected)
			{
				// Notify the Node PlainTransport.
				json data = json::object();

				this->tuple->FillJson(data["tuple"]);

				Channel::ChannelNotifier::Emit(this->id, "tuple", data);

				RTC::Transport::Connected();
			}
		}
		// Otherwise, if RTP tuple is set, verify that it matches the origin
		// of the packet.
		else if (!this->tuple->Compare(tuple))
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

	inline void PlainTransport::OnRtcpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (HasSrtp() && !IsSrtpReady())
			return;

		// Decrypt the SRTCP packet.
		auto intLen = static_cast<int>(len);

		if (HasSrtp() && !this->srtpRecvSession->DecryptSrtcp(const_cast<uint8_t*>(data), &intLen))
		{
			return;
		}

		// If we don't have a RTP tuple yet, check whether RTCP-mux and comedia
		// mode are set.
		if (this->rtcpMux && !this->tuple)
		{
			if (!this->comedia)
			{
				MS_DEBUG_TAG(rtcp, "ignoring RTCP packet while not connected");

				return;
			}

			MS_DEBUG_TAG(rtp, "setting RTP tuple (comedia mode enabled)");

			auto wasConnected = IsConnected();

			this->tuple = new RTC::TransportTuple(tuple);

			if (!this->listenIp.announcedIp.empty())
				this->tuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);

			// If not yet connected do it now.
			if (!wasConnected)
			{
				// Notify the Node PlainTransport.
				json data = json::object();

				this->tuple->FillJson(data["tuple"]);

				Channel::ChannelNotifier::Emit(this->id, "tuple", data);

				RTC::Transport::Connected();
			}
		}
		// Otherwise, if RTCP-mux is unset and RTCP tuple is unset, set it if we
		// are in comedia mode.
		else if (!this->rtcpMux && !this->rtcpTuple)
		{
			if (!this->comedia)
			{
				MS_DEBUG_TAG(rtcp, "ignoring RTCP packet while not connected");

				return;
			}

			MS_DEBUG_TAG(rtcp, "setting RTCP tuple (comedia mode enabled)");

			this->rtcpTuple = new RTC::TransportTuple(tuple);

			if (!this->listenIp.announcedIp.empty())
				this->rtcpTuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);

			// Notify the Node PlainTransport.
			json data = json::object();

			this->rtcpTuple->FillJson(data["rtcpTuple"]);

			Channel::ChannelNotifier::Emit(this->id, "rtcptuple", data);
		}
		// If RTCP-mux verify that the packet's tuple matches our RTP tuple.
		else if (this->rtcpMux && !this->tuple->Compare(tuple))
		{
			MS_DEBUG_TAG(rtcp, "ignoring RTCP packet from unknown IP:port");

			return;
		}
		// If no RTCP-mux verify that the packet's tuple matches our RTCP tuple.
		else if (!this->rtcpMux && !this->rtcpTuple->Compare(tuple))
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

	inline void PlainTransport::OnSctpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// If we don't have a RTP tuple yet, check whether comedia mode is set.
		if (!this->tuple)
		{
			if (!this->comedia)
			{
				MS_DEBUG_TAG(sctp, "ignoring SCTP packet while not connected");

				return;
			}

			MS_DEBUG_TAG(sctp, "setting RTP/SCTP tuple (comedia mode enabled)");

			auto wasConnected = IsConnected();

			this->tuple = new RTC::TransportTuple(tuple);

			if (!this->listenIp.announcedIp.empty())
				this->tuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);

			// If not yet connected do it now.
			if (!wasConnected)
			{
				// Notify the Node PlainTransport.
				json data = json::object();

				this->tuple->FillJson(data["tuple"]);

				Channel::ChannelNotifier::Emit(this->id, "tuple", data);

				RTC::Transport::Connected();
			}
		}
		// Otherwise, if RTP tuple is set, verify that it matches the origin
		// of the packet.
		if (!this->tuple->Compare(tuple))
		{
			MS_DEBUG_TAG(sctp, "ignoring SCTP packet from unknown IP:port");

			return;
		}

		// Pass it to the parent transport.
		RTC::Transport::ReceiveSctpData(data, len);
	}

	inline void PlainTransport::OnUdpSocketPacketReceived(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketReceived(&tuple, data, len);
	}
} // namespace RTC
