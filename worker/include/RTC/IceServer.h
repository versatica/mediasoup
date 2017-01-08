#ifndef MS_RTC_Ice_SERVER_H
#define MS_RTC_Ice_SERVER_H

#include "common.h"
#include "RTC/StunMessage.h"
#include "RTC/TransportTuple.h"
#include <string>
#include <list>

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
			DISCONNECTED
		};

	public:
		class Listener
		{
		public:
			/**
			 * These callbacks are guaranteed to be called before ProcessStunMessage()
			 * returns, so the given pointers are still usable.
			 */
			virtual void onOutgoingStunMessage(IceServer* iceServer, RTC::StunMessage* msg, RTC::TransportTuple* tuple) = 0;
			virtual void onIceSelectedTuple(IceServer* iceServer, RTC::TransportTuple* tuple) = 0;
			virtual void onIceConnected(IceServer* iceServer) = 0;
			virtual void onIceCompleted(IceServer* iceServer) = 0;
			virtual void onIceDisconnected(IceServer* iceServer) = 0;
		};

	private:
		static uint8_t stunSerializeBuffer[];

	public:
		IceServer(Listener* listener, const std::string& usernameFragment, const std::string& password);

		void Close();
		void ProcessStunMessage(RTC::StunMessage* msg, RTC::TransportTuple* tuple);
		std::string& GetUsernameFragment();
		std::string& GetPassword();
		IceState GetState();
		bool IsValidTuple(RTC::TransportTuple* tuple);
		void RemoveTuple(RTC::TransportTuple* tuple);
		// This should be just called in 'connected' or completed' state
		// and the given tuple must be an already valid tuple.
		void ForceSelectedTuple(RTC::TransportTuple* tuple);

	private:
		void HandleTuple(RTC::TransportTuple* tuple, bool has_use_candidate);
		/**
		 * Store the given tuple and return its stored address.
		 */
		RTC::TransportTuple* AddTuple(RTC::TransportTuple* tuple);
		/**
		 * If the given tuple exists return its stored address, nullptr otherwise.
		 */
		RTC::TransportTuple* HasTuple(RTC::TransportTuple* tuple);
		/**
		 * Set the given tuple as the selected tuple.
		 * NOTE: The given tuple MUST be already stored within the list.
		 */
		void SetSelectedTuple(RTC::TransportTuple* stored_tuple);

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Others.
		std::string usernameFragment;
		std::string password;
		IceState state = IceState::NEW;
		std::list<RTC::TransportTuple> tuples;
		RTC::TransportTuple* selectedTuple = nullptr;
	};

	/* Inline methods. */

	inline
	std::string& IceServer::GetUsernameFragment()
	{
		return this->usernameFragment;
	}

	inline
	std::string& IceServer::GetPassword()
	{
		return this->password;
	}

	inline
	IceServer::IceState IceServer::GetState()
	{
		return this->state;
	}
}

#endif
