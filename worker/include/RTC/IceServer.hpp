#ifndef MS_RTC_ICE_SERVER_HPP
#define MS_RTC_ICE_SERVER_HPP

#include "common.hpp"
#include "RTC/StunMessage.hpp"
#include "RTC/TransportTuple.hpp"
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
			virtual ~Listener() {};

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
		const std::string& GetUsernameFragment() const;
		const std::string& GetPassword() const;
		IceState GetState() const;
		bool IsValidTuple(RTC::TransportTuple* tuple) const;
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
		RTC::TransportTuple* HasTuple(RTC::TransportTuple* tuple) const;
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

	/* Inline instance methods. */

	inline
	const std::string& IceServer::GetUsernameFragment() const
	{
		return this->usernameFragment;
	}

	inline
	const std::string& IceServer::GetPassword() const
	{
		return this->password;
	}

	inline
	IceServer::IceState IceServer::GetState() const
	{
		return this->state;
	}
}

#endif
