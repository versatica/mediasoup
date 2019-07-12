#define MS_CLASS "Channel::UnixStreamSocket"
// #define MS_LOG_DEV

#include "Channel/UnixStreamSocket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cmath>   // std::ceil()
#include <cstdio>  // sprintf()
#include <cstring> // std::memcpy(), std::memmove()
extern "C" {
#include <netstring.h>
}

namespace Channel
{
	/* Static. */

	// netstring length for a 4194304 bytes payload.
	static constexpr size_t NsMessageMaxLen{ 4194313 };
	static constexpr size_t NsPayloadMaxLen{ 4194304 };
	static uint8_t WriteBuffer[NsMessageMaxLen];

	/* Instance methods. */

	UnixStreamSocket::UnixStreamSocket(int fd)
	  : ::UnixStreamSocket::UnixStreamSocket(fd, NsMessageMaxLen)
	{
		MS_TRACE_STD();
	}

	void UnixStreamSocket::SetListener(Listener* listener)
	{
		MS_TRACE_STD();

		this->listener = listener;
	}

	void UnixStreamSocket::Send(json& jsonMessage)
	{
		if (IsClosed())
			return;

		std::string nsPayload = jsonMessage.dump();
		size_t nsPayloadLen   = nsPayload.length();
		size_t nsNumLen;
		size_t nsLen;

		if (nsPayloadLen > NsPayloadMaxLen)
		{
			MS_ERROR_STD("mesage too big");

			return;
		}

		if (nsPayloadLen == 0)
		{
			nsNumLen       = 1;
			WriteBuffer[0] = '0';
			WriteBuffer[1] = ':';
			WriteBuffer[2] = ',';
		}
		else
		{
			nsNumLen = static_cast<size_t>(std::ceil(std::log10(static_cast<double>(nsPayloadLen) + 1)));
			std::sprintf(reinterpret_cast<char*>(WriteBuffer), "%zu:", nsPayloadLen);
			std::memcpy(WriteBuffer + nsNumLen + 1, nsPayload.c_str(), nsPayloadLen);
			WriteBuffer[nsNumLen + nsPayloadLen + 1] = ',';
		}

		nsLen = nsNumLen + nsPayloadLen + 2;

		Write(WriteBuffer, nsLen);
	}

	void UnixStreamSocket::SendLog(char* nsPayload, size_t nsPayloadLen)
	{
		if (IsClosed())
			return;

		// MS_TRACE_STD();

		size_t nsNumLen;
		size_t nsLen;

		if (nsPayloadLen > NsPayloadMaxLen)
		{
			MS_ERROR_STD("mesage too big");

			return;
		}

		if (nsPayloadLen == 0)
		{
			nsNumLen       = 1;
			WriteBuffer[0] = '0';
			WriteBuffer[1] = ':';
			WriteBuffer[2] = ',';
		}
		else
		{
			nsNumLen = static_cast<size_t>(std::ceil(std::log10(static_cast<double>(nsPayloadLen) + 1)));
			std::sprintf(reinterpret_cast<char*>(WriteBuffer), "%zu:", nsPayloadLen);
			std::memcpy(WriteBuffer + nsNumLen + 1, nsPayload, nsPayloadLen);
			WriteBuffer[nsNumLen + nsPayloadLen + 1] = ',';
		}

		nsLen = nsNumLen + nsPayloadLen + 2;

		Write(WriteBuffer, nsLen);
	}

	void UnixStreamSocket::SendBinary(const uint8_t* nsPayload, size_t nsPayloadLen)
	{
		if (IsClosed())
			return;

		size_t nsNumLen;
		size_t nsLen;

		if (nsPayloadLen > NsPayloadMaxLen)
		{
			MS_ERROR_STD("mesage too big");

			return;
		}

		if (nsPayloadLen == 0)
		{
			nsNumLen       = 1;
			WriteBuffer[0] = '0';
			WriteBuffer[1] = ':';
			WriteBuffer[2] = ',';
		}
		else
		{
			nsNumLen = static_cast<size_t>(std::ceil(std::log10(static_cast<double>(nsPayloadLen) + 1)));
			std::sprintf(reinterpret_cast<char*>(WriteBuffer), "%zu:", nsPayloadLen);
			std::memcpy(WriteBuffer + nsNumLen + 1, nsPayload, nsPayloadLen);
			WriteBuffer[nsNumLen + nsPayloadLen + 1] = ',';
		}

		nsLen = nsNumLen + nsPayloadLen + 2;

		Write(WriteBuffer, nsLen);
	}

	void UnixStreamSocket::UserOnUnixStreamRead()
	{
		MS_TRACE_STD();

		// Be ready to parse more than a single message in a single TCP chunk.
		while (true)
		{
			if (IsClosed())
				return;

			size_t readLen  = this->bufferDataLen - this->msgStart;
			char* jsonStart = nullptr;
			size_t jsonLen;
			int nsRet = netstring_read(
			  reinterpret_cast<char*>(this->buffer + this->msgStart), readLen, &jsonStart, &jsonLen);

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

			// If here it means that jsonStart points to the beginning of a JSON string
			// with jsonLen bytes length, so recalculate readLen.
			readLen =
			  reinterpret_cast<const uint8_t*>(jsonStart) - (this->buffer + this->msgStart) + jsonLen + 1;

			try
			{
				json jsonRequest = json::parse(jsonStart, jsonStart + jsonLen);

				Channel::Request* request{ nullptr };

				try
				{
					request = new Channel::Request(this, jsonRequest);
				}
				catch (const MediaSoupError& error)
				{
					MS_ERROR_STD("discarding wrong Channel request");
				}

				if (request != nullptr)
				{
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
			}
			catch (const json::parse_error& error)
			{
				MS_ERROR_STD("JSON parsing error: %s", error.what());
			}

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

	void UnixStreamSocket::UserOnUnixStreamSocketClosed(bool isClosedByPeer)
	{
		MS_TRACE_STD();

		// Notify the listener.
		if (isClosedByPeer)
			this->listener->OnChannelRemotelyClosed(this);
	}
} // namespace Channel
