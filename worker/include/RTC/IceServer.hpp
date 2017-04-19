#ifndef MS_RTC_ICE_SERVER_HPP
#define MS_RTC_ICE_SERVER_HPP

#include "common.hpp"
#include "RTC/StunMessage.hpp"
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
			DISCONNECTED
		};

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			/**
			 * These callbacks are guaranteed to be called before ProcessStunMessage()
			 * returns, so the given pointers are still usable.
			 */
			virtual void OnOutgoingStunMessage(
			    const RTC::IceServer* iceServer, const RTC::StunMessage* msg, RTC::TransportTuple* tuple) = 0;
			virtual void OnIceSelectedTuple(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) = 0;
			virtual void OnIceConnected(const RTC::IceServer* iceServer)    = 0;
			virtual void OnIceCompleted(const RTC::IceServer* iceServer)    = 0;
			virtual void OnIceDisconnected(const RTC::IceServer* iceServer) = 0;
		};

	public:
		IceServer(Listener* listener, std::string usernameFragment, std::string password);

		void Destroy();
		void ProcessStunMessage(RTC::StunMessage* msg, RTC::TransportTuple* tuple);
		const std::string& GetUsernameFragment() const;
		const std::string& GetPassword() const;
		void SetUsernameFragment(const std::string& usernameFragment);
		void SetPassword(const std::string& password);
		IceState GetState() const;
		bool IsValidTuple(const RTC::TransportTuple* tuple) const;
		void RemoveTuple(RTC::TransportTuple* tuple);
		// This should be just called in 'connected' or completed' state
		// and the given tuple must be an already valid tuple.
		void ForceSelectedTuple(const RTC::TransportTuple* tuple);

	private:
		void HandleTuple(RTC::TransportTuple* tuple, bool hasUseCandidate);
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
		IceState state{ IceState::NEW };
		std::list<RTC::TransportTuple> tuples;
		RTC::TransportTuple* selectedTuple{ nullptr };
	};

	/* Inline instance methods. */

	inline const std::string& IceServer::GetUsernameFragment() const
	{
		return this->usernameFragment;
	}

	inline const std::string& IceServer::GetPassword() const
	{
		return this->password;
	}

	inline void IceServer::SetUsernameFragment(const std::string& usernameFragment)
	{
		this->usernameFragment = usernameFragment;
	}

	inline void IceServer::SetPassword(const std::string& password)
	{
		this->password = password;
	}

	inline IceServer::IceState IceServer::GetState() const
	{
		return this->state;
	}
} // namespace RTC

#endif
