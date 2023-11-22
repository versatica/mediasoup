#ifndef MS_RTC_ICE_SERVER_HPP
#define MS_RTC_ICE_SERVER_HPP

#include "common.hpp"
#include "FBS/webRtcTransport.h"
#include "RTC/StunPacket.hpp"
#include "RTC/TransportTuple.hpp"
#include <list>
#include <string>

namespace RTC
{
	class IceServer
	{
	public:
		enum class IceState
		{
			NEW = 1,
			CONNECTED,
			COMPLETED,
			DISCONNECTED,
		};

	public:
		static IceState RoleFromFbs(FBS::WebRtcTransport::IceState state);
		static FBS::WebRtcTransport::IceState IceStateToFbs(IceState state);

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			/**
			 * These callbacks are guaranteed to be called before ProcessStunPacket()
			 * returns, so the given pointers are still usable.
			 */
			virtual void OnIceServerSendStunPacket(
			  const RTC::IceServer* iceServer, const RTC::StunPacket* packet, RTC::TransportTuple* tuple) = 0;
			virtual void OnIceServerLocalUsernameFragmentAdded(
			  const RTC::IceServer* iceServer, const std::string& usernameFragment) = 0;
			virtual void OnIceServerLocalUsernameFragmentRemoved(
			  const RTC::IceServer* iceServer, const std::string& usernameFragment) = 0;
			virtual void OnIceServerTupleAdded(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) = 0;
			virtual void OnIceServerTupleRemoved(
			  const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) = 0;
			virtual void OnIceServerSelectedTuple(
			  const RTC::IceServer* iceServer, RTC::TransportTuple* tuple)        = 0;
			virtual void OnIceServerConnected(const RTC::IceServer* iceServer)    = 0;
			virtual void OnIceServerCompleted(const RTC::IceServer* iceServer)    = 0;
			virtual void OnIceServerDisconnected(const RTC::IceServer* iceServer) = 0;
		};

	public:
		IceServer(Listener* listener, const std::string& usernameFragment, const std::string& password);
		~IceServer();

	public:
		void ProcessStunPacket(RTC::StunPacket* packet, RTC::TransportTuple* tuple);
		const std::string& GetUsernameFragment() const
		{
			return this->usernameFragment;
		}
		const std::string& GetPassword() const
		{
			return this->password;
		}
		IceState GetState() const
		{
			return this->state;
		}
		RTC::TransportTuple* GetSelectedTuple() const
		{
			return this->selectedTuple;
		}
		void RestartIce(const std::string& usernameFragment, const std::string& password)
		{
			if (!this->oldUsernameFragment.empty())
			{
				this->listener->OnIceServerLocalUsernameFragmentRemoved(this, this->oldUsernameFragment);
			}

			this->oldUsernameFragment = this->usernameFragment;
			this->usernameFragment    = usernameFragment;

			this->oldPassword = this->password;
			this->password    = password;

			this->remoteNomination = 0u;

			// Notify the listener.
			this->listener->OnIceServerLocalUsernameFragmentAdded(this, usernameFragment);

			// NOTE: Do not call listener->OnIceServerLocalUsernameFragmentRemoved()
			// yet with old usernameFragment. Wait until we receive a STUN packet
			// with the new one.
		}
		bool IsValidTuple(const RTC::TransportTuple* tuple) const;
		void RemoveTuple(RTC::TransportTuple* tuple);
		/**
		 * This should be just called in 'connected' or 'completed' state and the
		 * given tuple must be an already valid tuple.
		 */
		void MayForceSelectedTuple(const RTC::TransportTuple* tuple);

	private:
		void HandleTuple(
		  RTC::TransportTuple* tuple, bool hasUseCandidate, bool hasNomination, uint32_t nomination);
		/**
		 * Store the given tuple and return its stored address.
		 */
		RTC::TransportTuple* AddTuple(RTC::TransportTuple* tuple);
		/**
		 * If the given tuple exists return its stored address, nullptr otherwise.
		 */
		RTC::TransportTuple* HasTuple(const RTC::TransportTuple* tuple) const;
		/**
		 * Set the given tuple as the selected tuple.
		 * NOTE: The given tuple MUST be already stored within the list.
		 */
		void SetSelectedTuple(RTC::TransportTuple* storedTuple);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		std::string usernameFragment;
		std::string password;
		std::string oldUsernameFragment;
		std::string oldPassword;
		IceState state{ IceState::NEW };
		uint32_t remoteNomination{ 0u };
		std::list<RTC::TransportTuple> tuples;
		RTC::TransportTuple* selectedTuple{ nullptr };
	};
} // namespace RTC

#endif
