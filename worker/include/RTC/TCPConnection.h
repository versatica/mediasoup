#ifndef MS_RTC_TCP_CONNECTION_H
#define MS_RTC_TCP_CONNECTION_H

#include "common.h"
#include "handles/TCPConnection.h"

namespace RTC
{
	class TCPConnection :
		public ::TCPConnection
	{
	public:
		class Listener
		{
		public:
			virtual void onPacketRecv(RTC::TCPConnection *connection, const uint8_t* data, size_t len) = 0;
		};

	public:
		TCPConnection(Listener* listener, size_t bufferSize);
		virtual ~TCPConnection();

		void Send(const uint8_t* data, size_t len);

	/* Pure virtual methods inherited from ::TCPConnection. */
	public:
		virtual void userOnTCPConnectionRead() override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Others.
		size_t frameStart = 0;  // Where the latest frame starts.
	};
}

#endif
