#ifndef MS_RTC_TCP_CONNECTION_H
#define MS_RTC_TCP_CONNECTION_H

#include "common.h"
#include "handles/TCPConnection.h"

namespace RTC
{
	class TCPConnection : public ::TCPConnection
	{
	public:
		class Reader
		{
		public:
			virtual void onSTUNDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) = 0;
			virtual void onDTLSDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) = 0;
			virtual void onRTPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) = 0;
			virtual void onRTCPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) = 0;
		};

	public:
		TCPConnection(Reader* reader, size_t bufferSize);
		virtual ~TCPConnection();

		void Send(const MS_BYTE* data, size_t len);

	/* Pure virtual methods inherited from ::TCPConnection. */
	public:
		virtual void userOnTCPConnectionRead() override;

	private:
		// Passed by argument:
		Reader* reader = nullptr;
		// Others:
		size_t frameStart = 0;  // Where the latest frame starts.
	};
}  // namespace RTC

#endif
