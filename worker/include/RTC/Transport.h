#ifndef MS_RTC_TRANSPORT_H
#define MS_RTC_TRANSPORT_H

#include "common.h"
#include "RTC/IceCandidate.h"
#include "RTC/IceServer.h"
#include "RTC/DTLSTransport.h"
#include "RTC/STUNMessage.h"
#include "RTC/TransportSource.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include "RTC/TCPConnection.h"
#include <string>
#include <vector>
#include <list>
#include <json/json.h>

namespace RTC
{
	class Transport :
		public RTC::UDPSocket::Listener,
		public RTC::TCPServer::Listener,
		public RTC::TCPConnection::Reader,
		public RTC::IceServer::Listener,
		public RTC::DTLSTransport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onTransportClosed(RTC::Transport* transport) = 0;
		};

	public:
		enum class IceComponent
		{
			RTP  = 1,
			RTCP = 2
		};

	private:
		static size_t maxSources;

	public:
		Transport(Listener* listener, unsigned int transportId, Json::Value& data, Transport* rtpTransport = nullptr);
		virtual ~Transport();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		std::string& GetIceUsernameFragment();
		std::string& GetIcePassword();
		Transport* CreateAssociatedTransport(unsigned int transportId);

	private:
		int SetSendingSource(RTC::TransportSource* source);
		int IsValidSource(RTC::TransportSource* source);
		bool RemoveSource(RTC::TransportSource* source);
		void RunDTLSTransportIfReady();

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

	/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		virtual void onOutgoingSTUNMessage(RTC::IceServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source) override;
		virtual void onICEValidPair(RTC::IceServer* iceServer, RTC::TransportSource* source, bool has_use_candidate) override;

	/* Pure virtual methods inherited from RTC::DTLSTransport::Listener. */
	public:
		virtual void onOutgoingDTLSData(RTC::DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len) override;
		virtual void onDTLSConnected(RTC::DTLSTransport* dtlsTransport) override;
		virtual void onDTLSDisconnected(RTC::DTLSTransport* dtlsTransport) override;
		virtual void onDTLSFailed(RTC::DTLSTransport* dtlsTransport) override;
		virtual void onSRTPKeyMaterial(RTC::DTLSTransport* dtlsTransport, RTC::SRTPSession::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len) override;
		virtual void onDTLSApplicationData(RTC::DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len) override;

	public:
		// Passed by argument.
		unsigned int transportId;
		// Others.
		bool hasIPv4udp = false;
		bool hasIPv6udp = false;
		bool hasIPv4tcp = false;
		bool hasIPv6tcp = false;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		RTC::Transport::IceComponent iceComponent;
		// Allocated by this.
		RTC::IceServer* iceServer = nullptr;
		std::vector<RTC::UDPSocket*> udpSockets;
		std::vector<RTC::TCPServer*> tcpServers;
		RTC::DTLSTransport* dtlsTransport = nullptr;
		// Others.
		bool allocated = false;
		// Others (ICE).
		std::vector<IceCandidate> iceLocalCandidates;
		std::list<RTC::TransportSource> sources;
		RTC::TransportSource* sendingSource = nullptr;
		bool icePaired = false;
		bool icePairedWithUseCandidate = false;
		// Others (DTLS).
		bool remoteDtlsParametersGiven = false;
		RTC::DTLSTransport::Role dtlsLocalRole = RTC::DTLSTransport::Role::SERVER;
	};

	/* Inline methods. */

	inline
	std::string& Transport::GetIceUsernameFragment()
	{
		return this->iceServer->GetUsernameFragment();
	}

	inline
	std::string& Transport::GetIcePassword()
	{
		return this->iceServer->GetPassword();
	}

	/**
	 * Set the given source as a valid one and mark it as the sending source for
	 * outgoing data.
	 *
	 * @param source: A RTC::TransportSource (which is a UDP tuple or TCP connection).
	 * @return:       true if the given source was not an already valid source,
	 *                false otherwise.
	 * @return:       1 if it's the current sending source, 2 if it's another
	 *                valid source, 0 if it's not a valid source.
	 */
	inline
	int Transport::SetSendingSource(RTC::TransportSource* source)
	{
		// Check whether this is already a valid source.
		// NOTE: The IsValidSource() method will also mark it as sending source.
		switch (IsValidSource(source))
		{
			// This is already the sending source.
			case 1:
				return 1;
			// This is another valid source.
			case 2:
				return 2;
		}

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

		return 0;
	}

	/**
	 * Check whether the given source is contained in the list of valid sources for
	 * this transport. In case it is, the given source is also marked as sending
	 * source for the outgoing data.
	 *
	 * @param source:  The RTC::TransportSource to check.
	 * @return:        1 if it's the current sending source, 2 if it's another
	 *                 valid source, 0 if it's not a valid source.
	 */
	inline
	int Transport::IsValidSource(RTC::TransportSource* source)
	{
		// If there is no sending source yet then we know that the sources list
		// is empty, so return 0.
		if (!this->sendingSource)
			return 0;

		// First check the current sending source. If it matches then return 1.
		if (this->sendingSource->Compare(source))
			return 1;

		// Otherwise check other valid source in the sources list.
		for (auto it = this->sources.begin(); it != this->sources.end(); ++it)
		{
			RTC::TransportSource* valid_source = &(*it);

			// If found set it as sending source and return 2.
			if (valid_source->Compare(source))
			{
				this->sendingSource = valid_source;
				return 2;
			}
		}

		// Otherwise it is not a valid source so return 0.
		return 0;
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
						// This is just useful is there are just TCP sources.
						this->icePaired = false;
						this->icePairedWithUseCandidate = false;

						// TODO: This should fire a 'icefailed' event.
					}
				}

				return true;
			}
		}

		return false;
	}
}

#endif
