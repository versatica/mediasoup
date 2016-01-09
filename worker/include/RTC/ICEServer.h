#ifndef MS_RTC_ICE_SERVER_H
#define MS_RTC_ICE_SERVER_H

#include "common.h"
#include "RTC/STUNMessage.h"
#include "RTC/TransportTuple.h"
#include <string>
#include <list>

namespace RTC
{
	class ICEServer
	{
	public:
		enum class IceComponent
		{
			RTP  = 1,
			RTCP = 2
		};

	public:
		enum class IceState
		{
			NEW = 1,
			CONNECTED,
			COMPLETED
		};

	public:
		class Listener
		{
		public:
			/**
			 * These callbacks are guaranteed to be called before ProcessSTUNMessage()
			 * returns, so the given pointers are still usable.
			 */
			virtual void onOutgoingSTUNMessage(ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportTuple* tuple) = 0;
			virtual void onICESelectedTuple(ICEServer* iceServer, RTC::TransportTuple* tuple) = 0;
			virtual void onICEConnected(ICEServer* iceServer) = 0;
			virtual void onICECompleted(ICEServer* iceServer) = 0;
		};

	public:
		ICEServer(Listener* listener, ICEServer::IceComponent iceComponent, const std::string& usernameFragment, const std::string password);

		void Close();
		void ProcessSTUNMessage(RTC::STUNMessage* msg, RTC::TransportTuple* tuple);
		IceComponent GetComponent();
		std::string& GetUsernameFragment();
		std::string& GetPassword();
		IceState GetState();
		bool IsValidTuple(RTC::TransportTuple* tuple);
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
		IceComponent iceComponent;
		std::string usernameFragment;
		std::string password;
		IceState state = IceState::NEW;
		std::list<RTC::TransportTuple> tuples;
		RTC::TransportTuple* selectedTuple = nullptr;
	};

	/* Inline methods. */

	inline
	ICEServer::IceComponent ICEServer::GetComponent()
	{
		return this->iceComponent;
	}

	inline
	std::string& ICEServer::GetUsernameFragment()
	{
		return this->usernameFragment;
	}

	inline
	std::string& ICEServer::GetPassword()
	{
		return this->password;
	}

	inline
	ICEServer::IceState ICEServer::GetState()
	{
		return this->state;
	}
}

#endif
