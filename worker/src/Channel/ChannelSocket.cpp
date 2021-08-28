#define MS_CLASS "Channel::ChannelSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelSocket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cmath>   // std::ceil()
#include <cstdio>  // sprintf()
#include <cstring> // std::memcpy(), std::memmove()

namespace Channel
{
	/* Static. */

	// Binary length for a 4194304 bytes payload.
	static constexpr size_t MessageMaxLen{ 4194308 };
	static constexpr size_t PayloadMaxLen{ 4194304 };

	/* Instance methods. */
	ChannelSocket::ChannelSocket(int consumerFd, int producerFd)
	  : consumerSocket(consumerFd, MessageMaxLen, this), producerSocket(producerFd, MessageMaxLen)
	{
		MS_TRACE_STD();

		this->writeBuffer = static_cast<uint8_t*>(std::malloc(MessageMaxLen));
	}

	ChannelSocket::ChannelSocket(
	  int consumerFd, int producerFd, ChannelWriteFn channelWriteFn, ChannelWriteCtx channelWriteCtx)
	  : consumerSocket(consumerFd, MessageMaxLen, this), producerSocket(producerFd, MessageMaxLen),
	    channelWriteFn(channelWriteFn), channelWriteCtx(channelWriteCtx)
	{
		MS_TRACE_STD();

		this->writeBuffer = static_cast<uint8_t*>(std::malloc(MessageMaxLen));
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

		if (message.length() > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(
		  reinterpret_cast<const uint8_t*>(message.c_str()), static_cast<uint32_t>(message.length()));
	}

	void ChannelSocket::SendLog(const char* message, uint32_t messageLen)
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		if (messageLen > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(reinterpret_cast<const uint8_t*>(message), messageLen);
	}

	inline void ChannelSocket::SendImpl(const uint8_t* payload, uint32_t payloadLen)
	{
		MS_TRACE_STD();

		// Write using function call if provided.
		if (this->channelWriteFn)
		{
			this->channelWriteFn(payload, payloadLen, this->channelWriteCtx);
		}
		else
		{
			std::memcpy(this->writeBuffer, &payloadLen, sizeof(uint32_t));

			if (payloadLen != 0)
			{
				std::memcpy(this->writeBuffer + sizeof(uint32_t), payload, payloadLen);
			}

			size_t len = sizeof(uint32_t) + payloadLen;

			this->producerSocket.Write(this->writeBuffer, len);
		}
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

		size_t msgStart{ 0 };

		// Be ready to parse more than a single message in a single chunk.
		while (true)
		{
			if (IsClosed())
				return;

			size_t readLen = this->bufferDataLen - msgStart;

			if (readLen < sizeof(uint32_t))
			{
				// Incomplete data.
				break;
			}

			uint32_t msgLen;
			// Read message length.
			std::memcpy(&msgLen, this->buffer + msgStart, sizeof(uint32_t));

			if (readLen < sizeof(uint32_t) + static_cast<size_t>(msgLen))
			{
				// Incomplete data.
				break;
			}

			this->listener->OnConsumerSocketMessage(
			  this,
			  reinterpret_cast<char*>(this->buffer + msgStart + sizeof(uint32_t)),
			  static_cast<size_t>(msgLen));

			msgStart += sizeof(uint32_t) + static_cast<size_t>(msgLen);
		}

		if (msgStart != 0)
		{
			this->bufferDataLen = this->bufferDataLen - msgStart;

			if (this->bufferDataLen != 0)
			{
				std::memmove(this->buffer, this->buffer + msgStart, this->bufferDataLen);
			}
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
