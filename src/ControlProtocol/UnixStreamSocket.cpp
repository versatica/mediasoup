#define MS_CLASS "ControlProtocol::UnixStreamSocket"

#include "ControlProtocol/UnixStreamSocket.h"
#include "Logger.h"
#include <cstring>  // std::memmove()

namespace ControlProtocol
{
	/* Instance methods. */

	UnixStreamSocket::UnixStreamSocket(Listener* listener, int fd) :
		::UnixStreamSocket::UnixStreamSocket(fd, 65536),
		listener(listener)
	{
		MS_TRACE();

		// Initiate a ControlProtocol::Parser.
		this->parser = new ControlProtocol::Parser();
	}

	UnixStreamSocket::~UnixStreamSocket()
	{
		MS_TRACE();

		if (this->parser)
			delete this->parser;
	}

	void UnixStreamSocket::userOnUnixStreamRead(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// Be ready to parse more than a single message in a single TCP chunk.
		while (true)
		{
			ControlProtocol::Message* msg;

			// Tell the parser to parse the buffer starting from the beginning of the latest
			// detected message until the end of the data in the buffer.
			msg = this->parser->Parse(this->buffer + this->msgStart, this->bufferDataLen - this->msgStart);
			size_t parsed_len = parser->GetParsedLen();

			// Message parsed.
			if (msg)
			{
				// Notify the listener.
				this->listener->onControlProtocolMessage(this, msg, this->buffer + this->msgStart, parsed_len);

				// Free the message.
				delete msg;

				// If there is no more space available in the buffer and that is because
				// the latest parsed message filled it, then empty the full buffer.
				if ((this->msgStart + parsed_len) == this->bufferSize)
				{
					MS_DEBUG("no more space in the buffer, emptying the buffer data");

					this->msgStart = 0;
					this->bufferDataLen = 0;
				}
				// If there is still space in the buffer, set the beginning of the next
				// parsing to the next position after the parsed message.
				else
				{
					this->msgStart += parsed_len;
				}

				// Reset the parser.
				this->parser->Reset();

				// If there is more data in the buffer after the parsed message
				// then parse again. Otherwise break here and wait for more data.
				if (this->bufferDataLen > this->msgStart)
				{
					MS_DEBUG("there is more data after the parsed message, continue parsing");
					continue;
				}
				else
				{
					break;
				}
			}

			// Parsing error. Close the pipe.
			else if (this->parser->HasError())
			{
				MS_ERROR("parsing error, closing the pipe");

				// Close the pipe.
				Close();

				// Exit the parsing loop.
				break;
			}

			// Message not finished.
			else
			{
				// Check if the buffer is full.
				if (this->bufferDataLen == this->bufferSize)
				{
					// First case: the uncomplete message does not begin at position 0 of
					// the buffer, so move the message to the position 0.
					if (this->msgStart != 0)
					{
						MS_DEBUG("no more space in the buffer, moving parsed bytes to the beginning of the buffer and wait for more data");

						std::memmove(this->buffer, this->buffer + this->msgStart, parsed_len);
						this->msgStart = 0;
						this->bufferDataLen = parsed_len;
					}
					// Second case: the uncomplete message begins at position 0 of the buffer.
					// The message is too big, so close the pipe.
					else
					{
						MS_ERROR("no more space in the buffer for the unfinished message being parsed, closing the pipe");

						// Close the socket.
						Close();
					}
				}
				// The buffer is not full.
				else
				{
					MS_DEBUG("message not finished yet, waiting for more data");
				}

				// Exit the parsing loop.
				break;
			}
		}  // while()
	}

	void UnixStreamSocket::userOnUnixStreamSocketClosed(bool is_closed_by_peer)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->onControlProtocolUnixStreamSocketClosed(this, is_closed_by_peer);
	}
}  // namespace ControlProtocol
