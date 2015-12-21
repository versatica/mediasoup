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
	class ICEServer
	{
	public:
		class Listener
		{
		public:
			/**
			 * These callbacks are guaranteed to be called before ProcessSTUNMessage()
			 * returns, so the given pointers are still usable.
			 */
			virtual void onOutgoingSTUNMessage(ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source) = 0;
			virtual void onICEValidPair(ICEServer* iceServer, RTC::TransportSource* source, bool has_use_candidate) = 0;
		};

	public:
		ICEServer(Listener* listener);

		void ProcessSTUNMessage(RTC::STUNMessage* msg, RTC::TransportSource* source);
		std::string& GetLocalUsername();
		std::string& GetLocalPassword();
		void SetUserData(void* userData);
		void* GetUserData();
		void Close();

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		void* userData = nullptr;
		// Others.
		std::string localUsername;
		std::string localPassword;
	};

	/* Inline methods. */

	inline
	std::string& ICEServer::GetLocalUsername()
	{
		return this->localUsername;
	}

	inline
	std::string& ICEServer::GetLocalPassword()
	{
		return this->localPassword;
	}

	inline
	void ICEServer::SetUserData(void* userData)
	{
		this->userData = userData;
	}

	inline
	void* ICEServer::GetUserData()
	{
		return this->userData;
	}
}

#endif
