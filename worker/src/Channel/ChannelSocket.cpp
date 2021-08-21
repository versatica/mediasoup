#define MS_CLASS "Channel::ChannelSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelSocket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cmath>   // std::ceil()
#include <cstdio>  // sprintf()
#include <cstring> // std::memcpy(), std::memmove()
extern "C"
{
#include <netstring.h>
}

namespace Channel
{
	/* Static. */

	// netstring length for a 4194304 bytes payload.
	static constexpr size_t NsMessageMaxLen{ 4194313 };
	static constexpr size_t NsPayloadMaxLen{ 4194304 };

	/* Instance methods. */
	ChannelSocket::ChannelSocket(int consumerFd, int producerFd)
	  : consumerSocket(consumerFd, NsMessageMaxLen, this), producerSocket(producerFd, NsMessageMaxLen)
	{
		MS_TRACE_STD();

		this->writeBuffer = static_cast<uint8_t*>(std::malloc(NsMessageMaxLen));
	}

	ChannelSocket::~ChannelSocket()
	{
		MS_TRACE_STD();

		std::free(this->writeBuffer);

		if (!this->closed)
			Close();
	}

	void ChannelSocket::Close()
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		this->closed = true;

		this->consumerSocket.Close();
		this->producerSocket.Close();
	}

	void ChannelSocket::SetListener(Listener* listener)
	{
		MS_TRACE_STD();

		this->listener = listener;
	}

	void ChannelSocket::Send(json& jsonMessage)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		std::string message = jsonMessage.dump();

		if (message.length() > NsPayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(message.c_str(), static_cast<uint32_t>(message.length()));
	}

	void ChannelSocket::SendLog(char* message, uint32_t messageLen)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		if (messageLen > NsPayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(message, messageLen);
	}

	inline void ChannelSocket::SendImpl(const void* nsPayload, uint32_t nsPayloadLen)
	{
		MS_TRACE_STD();

		std::memcpy(this->writeBuffer, &nsPayloadLen, 4);

		if (nsPayloadLen != 0)
		{
			std::memcpy(this->writeBuffer + 4, nsPayload, nsPayloadLen);
		}

		size_t nsLen = 4 + nsPayloadLen;

		this->producerSocket.Write(this->writeBuffer, nsLen);
	}

	void ChannelSocket::OnConsumerSocketMessage(ConsumerSocket* /*consumerSocket*/, char* msg, size_t msgLen)
	{
		MS_TRACE_STD();

		try
		{
			json jsonMessage = json::parse(msg, msg + msgLen);
			auto* request    = new Channel::ChannelRequest(this, jsonMessage);

			// Notify the listener.
			try
			{
				this->listener->OnChannelRequest(this, request);
			}
			catch (const MediaSoupTypeError& error)
			{
				request->TypeError(error.what());
			}
			catch (const MediaSoupError& error)
			{
				request->Error(error.what());
			}

			// Delete the Request.
			delete request;
		}
		catch (const json::parse_error& error)
		{
			MS_ERROR_STD("JSON parsing error: %s", error.what());
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR_STD("discarding wrong Channel request");
		}
	}

	void ChannelSocket::OnConsumerSocketClosed(ConsumerSocket* /*consumerSocket*/)
	{
		MS_TRACE_STD();

		this->listener->OnChannelClosed(this);
	}

	ConsumerSocket::ConsumerSocket(int fd, size_t bufferSize, Listener* listener)
	  : ::UnixStreamSocket(fd, bufferSize, ::UnixStreamSocket::Role::CONSUMER), listener(listener)
	{
		MS_TRACE_STD();
	}

	void ConsumerSocket::UserOnUnixStreamRead()
	{
		MS_TRACE_STD();

		// Be ready to parse more than a single message in a single chunk.
		while (true)
		{
			if (IsClosed())
				return;

			size_t readLen = this->bufferDataLen - this->msgStart;
			char* msgStart = nullptr;
			size_t msgLen;
			int nsRet = netstring_read(
			  reinterpret_cast<char*>(this->buffer + this->msgStart), readLen, &msgStart, &msgLen);

			if (nsRet != 0)
			{
				switch (nsRet)
				{
					case NETSTRING_ERROR_TOO_SHORT:
					{
						// Check if the buffer is full.
						if (this->bufferDataLen == this->bufferSize)
						{
							// First case: the incomplete message does not begin at position 0 of
							// the buffer, so move the incomplete message to the position 0.
							if (this->msgStart != 0)
							{
								std::memmove(this->buffer, this->buffer + this->msgStart, readLen);
								this->msgStart      = 0;
								this->bufferDataLen = readLen;
							}
							// Second case: the incomplete message begins at position 0 of the buffer.
							// The message is too big, so discard it.
							else
							{
								MS_ERROR_STD(
								  "no more space in the buffer for the unfinished message being parsed, "
								  "discarding it");

								this->msgStart      = 0;
								this->bufferDataLen = 0;
							}
						}

						// Otherwise the buffer is not full, just wait.
						return;
					}

					case NETSTRING_ERROR_TOO_LONG:
					{
						MS_ERROR_STD("NETSTRING_ERROR_TOO_LONG");

						break;
					}

					case NETSTRING_ERROR_NO_COLON:
					{
						MS_ERROR_STD("NETSTRING_ERROR_NO_COLON");

						break;
					}

					case NETSTRING_ERROR_NO_COMMA:
					{
						MS_ERROR_STD("NETSTRING_ERROR_NO_COMMA");

						break;
					}

					case NETSTRING_ERROR_LEADING_ZERO:
					{
						MS_ERROR_STD("NETSTRING_ERROR_LEADING_ZERO");

						break;
					}

					case NETSTRING_ERROR_NO_LENGTH:
					{
						MS_ERROR_STD("NETSTRING_ERROR_NO_LENGTH");

						break;
					}
				}

				// Error, so reset and exit the parsing loop.
				this->msgStart      = 0;
				this->bufferDataLen = 0;

				return;
			}

			// If here it means that msgStart points to the beginning of a message
			// with msgLen bytes length, so recalculate readLen.
			readLen =
			  reinterpret_cast<const uint8_t*>(msgStart) - (this->buffer + this->msgStart) + msgLen + 1;

			this->listener->OnConsumerSocketMessage(this, msgStart, msgLen);

			// If there is no more space available in the buffer and that is because
			// the latest parsed message filled it, then empty the full buffer.
			if ((this->msgStart + readLen) == this->bufferSize)
			{
				this->msgStart      = 0;
				this->bufferDataLen = 0;
			}
			// If there is still space in the buffer, set the beginning of the next
			// parsing to the next position after the parsed message.
			else
			{
				this->msgStart += readLen;
			}

			// If there is more data in the buffer after the parsed message
			// then parse again. Otherwise break here and wait for more data.
			if (this->bufferDataLen > this->msgStart)
			{
				continue;
			}

			break;
		}
	}

	void ConsumerSocket::UserOnUnixStreamSocketClosed()
	{
		MS_TRACE_STD();

		// Notify the listener.
		this->listener->OnConsumerSocketClosed(this);
	}

	ProducerSocket::ProducerSocket(int fd, size_t bufferSize)
	  : ::UnixStreamSocket(fd, bufferSize, ::UnixStreamSocket::Role::PRODUCER)
	{
		MS_TRACE_STD();
	}
} // namespace Channel
