#ifndef MS_RTC_TRANSPORT_H
#define MS_RTC_TRANSPORT_H

#include "common.h"
#include "RTC/TransportSource.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include "RTC/TCPConnection.h"
#include "RTC/ICEServer.h"
#include "RTC/STUNMessage.h"
#include "RTC/DTLSRole.h"
#include "RTC/DTLSHandler.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include "RTC/SRTPProfile.h"
#include "RTC/SRTPSession.h"
#include <vector>
#include <list>
#include <string>

namespace RTC
{
	/**
	 * A RTC::Transport handles (in the most complex case as in WebRTC):
	 * - A RTC::ICEServer to handle received ICE requests and notify ICE connections.
	 * - A RTC::DTLSHandler to handle DTLS packets and notify the DTLS connection
	 *   state along with SRTP keys material.
	 * - Many RTC::UDPSocket or RTC::TCPServer instances to listen for data coming
	 *   from the peer.
	 * - A pair of RTC::SRTPSession instances for both outbound and inbound traffic
	 *   that encrypt/decrypt SRTP and SRTCP packets.
	 * - A list of validated RTC::TransportSource instances (UDP tuple or TCP
	 *   connection) from which incoming data is accepted (otherwise discarded).
	 * - A RTC::TransportSource marked as "sending source" which is the chosen one
	 *   to send data to the peer.
	 *
	 * Considerations:
	 * - Received ICE requests trigger ICE responses that are sent back to the peer
	 *   through the same RTC::TransportSource from which the requests were received.
	 * - Outgoing DTLS data is sent to the currently selected sending source.
	 * - The sending source is updated at any time a packet is received from a valid
	 *   source that does not match the current sending source.
	 * - The Reset() method clear all the valid sources, resets the DTLS status and
	 *   closes the SRTP sessions.
	 */
	class Transport :
		public RTC::UDPSocket::Listener,
		public RTC::TCPServer::Listener,
		public RTC::TCPConnection::Reader,
		public RTC::ICEServer::Listener,
		public RTC::DTLSHandler::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onRTPPacket(RTC::Transport* transport, RTC::RTPPacket* packet) = 0;
			virtual void onRTCPPacket(RTC::Transport* transport, RTC::RTCPPacket* packet) = 0;
		};

	public:
		// NOTE: Maximum value is 1<<31.
		enum Flag
		{
			ICE   = 1<<0,
			DTLS  = 1<<1,
			SRTP  = 1<<2
		};

	public:
		static Transport* NewWebRTC(Listener* listener);

	private:
		static size_t maxSources;

	public:
		Transport(Listener* listener, int flags);
		virtual ~Transport();

		void AddUDPSocket(RTC::UDPSocket* socket);
		void AddTCPServer(RTC::TCPServer* server);
		void SetUserData(void* userData);
		void* GetUserData();
		void Reset();
		void Close();
		void SetLocalDTLSRole(RTC::DTLSRole role);
		void SetRemoteDTLSFingerprint(RTC::FingerprintHash hash, std::string& fingerprint);
		void SetLocalSRTPKey(SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len);
		void SetRemoteSRTPKey(SRTPProfile srtp_profile, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len);
		void SendRTPPacket(RTC::RTPPacket* packet);
		void SendRTCPPacket(RTC::RTCPPacket* packet);
		bool IsReadyForMedia();
		void Dump();

	private:
		bool HasFlagICE();
		bool HasFlagDTLS();
		bool HasFlagSRTP();
		bool SetSendingSource(RTC::TransportSource* source);
		bool IsValidSource(RTC::TransportSource* source);
		bool RemoveSource(RTC::TransportSource* source);
		void RunDTLSHandlerIfReady();

	/* Private methods to unify UDP and TCP behavior. */
	private:
		void onSTUNDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);
		void onDTLSDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);
		void onRTPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);
		void onRTCPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);

	/* Pure virtual methods inherited from RTC::UDPSocket::Listener. */
	public:
		virtual void onSTUNDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;
		virtual void onDTLSDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;
		virtual void onRTPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;
		virtual void onRTCPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;

	/* Pure virtual methods inherited from RTC::TCPServer::Listener. */
	public:
		virtual void onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer) override;

	/* Pure virtual methods inherited from RTC::TCPConnection::Listener. */
	public:
		virtual void onSTUNDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;
		virtual void onDTLSDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;
		virtual void onRTPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;
		virtual void onRTCPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;

	/* Pure virtual methods inherited from RTC::ICEServer::Listener. */
	public:
		virtual void onOutgoingSTUNMessage(RTC::ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source) override;
		virtual void onICEValidPair(RTC::ICEServer* iceServer, RTC::TransportSource* source, bool has_use_candidate) override;

	/* Pure virtual methods inherited from RTC::DTLSHandler::Listener. */
	public:
		virtual void onOutgoingDTLSData(RTC::DTLSHandler* dtlsHandler, const MS_BYTE* data, size_t len) override;
		virtual void onDTLSConnected(RTC::DTLSHandler* dtlsHandler) override;
		virtual void onDTLSDisconnected(RTC::DTLSHandler* dtlsHandler) override;
		virtual void onDTLSFailed(RTC::DTLSHandler* dtlsHandler) override;
		virtual void onSRTPKeyMaterial(RTC::DTLSHandler* dtlsHandler, RTC::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len) override;
		virtual void onDTLSApplicationData(RTC::DTLSHandler* dtlsHandler, const MS_BYTE* data, size_t len) override;

	private:
		// Allocated by this:
		RTC::ICEServer* iceServer = nullptr;
		RTC::DTLSHandler* dtlsHandler = nullptr;
		RTC::SRTPSession* srtpRecvSession = nullptr;
		RTC::SRTPSession* srtpSendSession = nullptr;
		// Passed by argument:
		Listener* listener = nullptr;
		std::vector<RTC::UDPSocket*> udpSockets;
		std::vector<RTC::TCPServer*> tcpServers;
		void* userData = nullptr;
		// Others:
		int flags = 0;
		std::list<RTC::TransportSource> sources;
		RTC::TransportSource* sendingSource = nullptr;
		RTC::DTLSRole dtlsRole = RTC::DTLSRole::NONE;
		bool isIcePaired = false;
		bool isIcePairedWithUseCandidate = false;
	};

	/* Inline methods. */

	inline
	bool Transport::IsReadyForMedia()
	{
		if (HasFlagDTLS() && !this->dtlsHandler->IsConnected())
			return false;

		if (HasFlagSRTP() && (!this->srtpRecvSession || !this->srtpSendSession))
			return false;

		return (this->sendingSource != nullptr);
	}

	inline
	bool Transport::HasFlagICE()
	{
		return this->flags & Flag::ICE;
	}

	inline
	bool Transport::HasFlagDTLS()
	{
		return this->flags & Flag::DTLS;
	}

	inline
	bool Transport::HasFlagSRTP()
	{
		return this->flags & Flag::SRTP;
	}

	inline
	void Transport::SetUserData(void* userData)
	{
		this->userData = userData;
	}

	inline
	void* Transport::GetUserData()
	{
		return this->userData;
	}

	/**
	 * Set the given source as a valid one and mark it as the sending source for
	 * outgoing data.
	 *
	 * @param source: A RTC::TransportSource (which is a UDP tuple or TCP connection).
	 * @return:       true if the given source was not an already valid source,
	 *                false otherwise.
	 */
	inline
	bool Transport::SetSendingSource(RTC::TransportSource* source)
	{
		// If the give source is already a valid one return.
		// NOTE: The IsValidSource() method will also mark it as sending source.
		if (IsValidSource(source))
			return false;

		// If there are Transport::maxSources sources then close and remove the last one.
		if (this->sources.size() == Transport::maxSources)
		{
			RTC::TransportSource* last_source = &(*this->sources.end());

			last_source->Close();
			this->sources.pop_back();
		}

		// Add the new source at the beginning.
		this->sources.push_front(*source);

		// Set it as sending source (make it point to the first source in the list).
		this->sendingSource = &(*this->sources.begin());

		// If it is UDP then we must store the remote address (until now it is
		// just a pointer that will be freed soon).
		if (this->sendingSource->IsUDP())
			this->sendingSource->StoreUdpRemoteAddress();

		return true;
	}

	/**
	 * Check whether the given source is contained in the list of valid sources for
	 * this transport. In case it is, the given source is also marked as sending
	 * source for the outgoing data.
	 *
	 * @param source:  The RTC::TransportSource to check.
	 * @return:        true if it is a valid source, false otherwise.
	 */
	inline
	bool Transport::IsValidSource(RTC::TransportSource* source)
	{
		// If there is no sending source yet then we know that the sources list
		// is empty, so return false.
		if (!this->sendingSource)
			return false;

		// First check the current sending source. If it matches then return true.
		if (this->sendingSource->Compare(source))
			return true;

		// Otherwise check other valid source in the sources list.
		for (auto it = this->sources.begin(); it != this->sources.end(); ++it)
		{
			RTC::TransportSource* valid_source = &(*it);

			// If found set it as sending source.
			if (valid_source->Compare(source))
			{
				this->sendingSource = valid_source;
				return true;
			}
		}

		return false;
	}

	/**
	 * Remove the given source from the list of valid sources. Also update the
	 * sending source if the given source was the sending source and make it point
	 * to the next valid source (if any).
	 *
	 * @param source:  The RTC::TransportSource to remove.
	 * @return:        true if the given source was present in the list of valid
	 *                 sources, false otherwise.
	 */
	inline
	bool Transport::RemoveSource(RTC::TransportSource* source)
	{
		for (auto it = this->sources.begin(); it != this->sources.end(); ++it)
		{
			RTC::TransportSource* valid_source = &(*it);

			// If the source was a valid source then remove it.
			if (valid_source->Compare(source))
			{
				this->sources.erase(it);

				// If it was the sending source then unset it and set the sending
				// source to the first valid source (if any).
				if (this->sendingSource == valid_source)
				{
					if (this->sources.begin() != this->sources.end())
					{
						this->sendingSource = &(*this->sources.begin());
					}
					else
					{
						this->sendingSource = nullptr;
						if (HasFlagICE())
						{
							// This is just useful is there are just TCP sources.
							this->isIcePaired = false;
							this->isIcePairedWithUseCandidate = false;
						}
					}
				}

				return true;
			}
		}

		return false;
	}
}  // namespace RTC

#endif
