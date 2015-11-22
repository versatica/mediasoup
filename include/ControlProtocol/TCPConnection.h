#ifndef MS_CONTROL_PROTOCOL_TCP_CONNECTION_H
#define MS_CONTROL_PROTOCOL_TCP_CONNECTION_H

#include "common.h"
#include "handles/TCPConnection.h"
#include "ControlProtocol/Parser.h"
#include "ControlProtocol/Message.h"

namespace ControlProtocol
{
	class TCPConnection : public ::TCPConnection
	{
	public:
		class Reader
		{
		public:
			virtual void onControlProtocolMessage(ControlProtocol::TCPConnection* connection, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len) = 0;
		};

	public:
		TCPConnection(Reader* reader, size_t bufferSize);
		virtual ~TCPConnection();

	/* Pure virtual methods inherited from ::TCPConnection. */
	public:
		virtual void userOnTCPConnectionRead(const MS_BYTE* data, size_t len) override;

	private:
		// Allocated by this:
		ControlProtocol::Parser* parser = nullptr;
		// Passed by argument:
		Reader* reader = nullptr;
		// Others:
		size_t msgStart = 0;  // Where the latest message starts.
	};
}  // namespace ControlProtocol

#endif
