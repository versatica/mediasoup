#ifndef MS_RTC_ICE_SERVER_H
#define MS_RTC_ICE_SERVER_H

#include "common.h"
#include "RTC/TransportSource.h"
#include "RTC/STUNMessage.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPConnection.h"
#include <string>

namespace RTC
{
	class IceServer
	{
	public:
		class Listener
		{
		public:
			/**
			 * These callbacks are guaranteed to be called before ProcessSTUNMessage()
			 * returns, so the given pointers are still usable.
			 */
			virtual void onOutgoingSTUNMessage(IceServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source) = 0;
			virtual void onICEValidPair(IceServer* iceServer, RTC::TransportSource* source, bool has_use_candidate) = 0;
		};

	public:
		IceServer(Listener* listener, std::string& usernameFragment, std::string password);

		void Close();
		void ProcessSTUNMessage(RTC::STUNMessage* msg, RTC::TransportSource* source);
		std::string& GetUsernameFragment();
		std::string& GetPassword();

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Others.
		std::string usernameFragment;
		std::string password;
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
}

#endif
