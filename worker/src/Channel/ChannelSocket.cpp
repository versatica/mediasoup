#define MS_CLASS "Channel::ChannelSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelSocket.hpp"
#include "FBS/log.h"
#include "FBS/message.h"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "SharedInterface.hpp"
#include <cstring> // std::memcpy(), std::memmove()
#include <exception>

namespace Channel
{
	static constexpr size_t MessageMaxLen{ 8388608 };
	static constexpr size_t PayloadMaxLen{ MessageMaxLen - 4 };

	/* Static methods for UV callbacks. */

	// libuv imposes the callback signature.
	// NOLINTNEXTLINE(misc-const-correctness)
	inline static void onAsync(uv_async_t* handle)
	{
		MS_TRACE_STD();

		while (static_cast<ChannelSocket*>(handle->data)->CallbackRead())
		{
			// Read while there are new messages.
		}
	}

	inline static void onCloseAsync(uv_handle_t* handle)
	{
		MS_TRACE_STD();

		delete reinterpret_cast<uv_async_t*>(handle);
	}

	/* Instance methods. */

#if defined(MS_TEST) || defined(MS_FUZZER)
	ChannelSocket::ChannelSocket()
	{
		MS_TRACE_STD();
	}
#endif

	ChannelSocket::ChannelSocket(int consumerFd, int producerFd)
	  : consumerSocket(std::make_unique<ConsumerSocket>(consumerFd, MessageMaxLen, this)),
	    producerSocket(std::make_unique<ProducerSocket>(producerFd, MessageMaxLen))
	{
		MS_TRACE_STD();
	}

	ChannelSocket::ChannelSocket(
	  ChannelReadFn channelReadFn,
	  ChannelReadCtx channelReadCtx,
	  ChannelWriteFn channelWriteFn,
	  ChannelWriteCtx channelWriteCtx)
	  : channelReadFn(channelReadFn),
	    channelReadCtx(channelReadCtx),
	    channelWriteFn(channelWriteFn),
	    channelWriteCtx(channelWriteCtx),
	    uvReadHandle(new uv_async_t)
	{
		MS_TRACE_STD();

		int err;

		this->uvReadHandle->data = static_cast<void*>(this);

		err = uv_async_init(DepLibUV::GetLoop(), this->uvReadHandle, onAsync);

		if (err != 0)
		{
			delete this->uvReadHandle;
			this->uvReadHandle = nullptr;

			MS_THROW_ERROR_STD("uv_async_init() failed: %s", uv_strerror(err));
		}

		err = uv_async_send(this->uvReadHandle);

		if (err != 0)
		{
			// NOTE: The handle is already initialized in the loop, so it must be
			// closed (which deletes it in its close callback) rather than deleted.
			uv_close(
			  reinterpret_cast<uv_handle_t*>(this->uvReadHandle), static_cast<uv_close_cb>(onCloseAsync));

			this->uvReadHandle = nullptr;

			MS_THROW_ERROR_STD("uv_async_send() failed: %s", uv_strerror(err));
		}
	}

	ChannelSocket::~ChannelSocket()
	{
		MS_TRACE_STD();

		if (!this->closed)
		{
			Close();
		}
	}

	void ChannelSocket::Close()
	{
		MS_TRACE_STD();

		if (this->closed)
		{
			return;
		}

		this->closed = true;

		if (this->uvReadHandle)
		{
			uv_close(
			  reinterpret_cast<uv_handle_t*>(this->uvReadHandle), static_cast<uv_close_cb>(onCloseAsync));
		}

		if (this->consumerSocket)
		{
			this->consumerSocket->Close();
		}

		if (this->producerSocket)
		{
			this->producerSocket->Close();
		}
	}

	void ChannelSocket::SetListener(Listener* listener)
	{
		MS_TRACE_STD();

		this->listener = listener;
	}

	void ChannelSocket::SetShared(SharedInterface* shared)
	{
		MS_TRACE_STD();

		this->shared = shared;
	}

	void ChannelSocket::Send(const uint8_t* data, uint32_t dataLen)
	{
		MS_TRACE_STD();

		if (this->closed)
		{
			return;
		}

		if (dataLen > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		SendImpl(data, dataLen);
	}

	void ChannelSocket::SendLog(const char* data, uint32_t dataLen)
	{
		MS_TRACE_STD();

		if (this->closed)
		{
			return;
		}

		if (dataLen > PayloadMaxLen)
		{
			MS_ERROR_STD("message too big");

			return;
		}

		auto& builder = this->bufferBuilder;

		const auto log     = FBS::Log::CreateLogDirect(builder, data);
		const auto message = FBS::Message::CreateMessage(builder, FBS::Message::Body::Log, log.Union());

		builder.FinishSizePrefixed(message);

		Send(builder.GetBufferPointer(), builder.GetSize());

		builder.Clear();
	}

	bool ChannelSocket::CallbackRead()
	{
		MS_TRACE_STD();

		if (this->closed)
		{
			return false;
		}

		uint8_t* msg{ nullptr };
		uint32_t msgLen;
		size_t msgCtx;

		// Try to read next message using `channelReadFn`, message, its length and
		// context will be stored in provided arguments.
		const ChannelReadFreeFn free = this->channelReadFn(
		  std::addressof(msg),
		  std::addressof(msgLen),
		  std::addressof(msgCtx),
		  this->uvReadHandle,
		  this->channelReadCtx);

		// Non-null free function pointer means message was successfully read above
		// and will need to be freed later.
		if (free)
		{
			// NOTE: Take the arrival time before the message is parsed, so that it
			// doesn't include the cost of processing it.
			const int64_t receivedAtUs = this->shared->GetTimeUs();

			const auto* message = FBS::Message::GetMessage(msg);

#if MS_LOG_DEV_LEVEL == 3
			const auto s = flatbuffers::FlatBufferToString(
			  reinterpret_cast<uint8_t*>(msg), FBS::Message::MessageTypeTable());

			MS_DUMP("%s", s.c_str());
#endif

			if (message->data_type() == FBS::Message::Body::Request)
			{
				std::unique_ptr<ChannelRequest> request;

				try
				{
					request = std::make_unique<ChannelRequest>(this, message->data_as<FBS::Request::Request>());

					// Notify the listener.
					this->listener->HandleRequest(request.get());
				}
				catch (const MediaSoupTypeError& error)
				{
					if (request)
					{
						request->TypeError(error.what());
					}
				}
				catch (const MediaSoupNotFoundError& error)
				{
					if (request)
					{
						request->NotFoundError(error.what());
					}
				}
				catch (const MediaSoupError& error)
				{
					if (request)
					{
						request->Error(error.what());
					}
				}
				catch (const std::exception& error)
				{
					MS_ABORT("request failed with unexpected error: %s", error.what());
				}
			}
			else if (message->data_type() == FBS::Message::Body::Notification)
			{
				try
				{
					const auto notification = std::make_unique<ChannelNotification>(
					  message->data_as<FBS::Notification::Notification>(), receivedAtUs);

					// Notify the listener.
					this->listener->HandleNotification(notification.get());
				}
				catch (const MediaSoupError& error)
				{
					MS_ERROR("notification failed: %s", error.what());
				}
				catch (const std::exception& error)
				{
					MS_ABORT("notification failed with unexpected error: %s", error.what());
				}
			}
			else
			{
				MS_ERROR("discarding wrong Channel data");
			}

			// Message needs to be freed using stored function pointer.
			free(msg, msgLen, msgCtx);
		}

		// Return `true` if something was processed.
		return free != nullptr;
	}

	void ChannelSocket::SendImpl(const uint8_t* payload, uint32_t payloadLen)
	{
		MS_TRACE_STD();

		// Write using function call if provided.
		if (this->channelWriteFn)
		{
			this->channelWriteFn(payload, payloadLen, this->channelWriteCtx);
		}
		else if (this->producerSocket)
		{
			this->producerSocket->Write(payload, payloadLen);
		}
	}

	void ChannelSocket::OnConsumerSocketMessage(
	  const ConsumerSocket* /*consumerSocket*/, char* msg, size_t /*msgLen*/)
	{
		MS_TRACE_STD();

		// NOTE: Take the arrival time before the message is parsed, so that it
		// doesn't include the cost of processing it.
		const int64_t receivedAtUs = this->shared->GetTimeUs();

		const auto* message = FBS::Message::GetMessage(msg);

#if MS_LOG_DEV_LEVEL == 3
		const auto s = flatbuffers::FlatBufferToString(
		  reinterpret_cast<uint8_t*>(msg), FBS::Message::MessageTypeTable());

		MS_DUMP("%s", s.c_str());
#endif

		if (message->data_type() == FBS::Message::Body::Request)
		{
			std::unique_ptr<ChannelRequest> request;

			try
			{
				request = std::make_unique<ChannelRequest>(this, message->data_as<FBS::Request::Request>());

				// Notify the listener.
				this->listener->HandleRequest(request.get());
			}
			catch (const MediaSoupTypeError& error)
			{
				if (request)
				{
					request->TypeError(error.what());
				}
			}
			catch (const MediaSoupNotFoundError& error)
			{
				if (request)
				{
					request->NotFoundError(error.what());
				}
			}
			catch (const MediaSoupError& error)
			{
				if (request)
				{
					request->Error(error.what());
				}
			}
			catch (const std::exception& error)
			{
				MS_ABORT("request failed with unexpected error: %s", error.what());
			}
		}
		else if (message->data_type() == FBS::Message::Body::Notification)
		{
			try
			{
				const auto notification = std::make_unique<ChannelNotification>(
				  message->data_as<FBS::Notification::Notification>(), receivedAtUs);

				// Notify the listener.
				this->listener->HandleNotification(notification.get());
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR("notification failed: %s", error.what());
			}
			catch (const std::exception& error)
			{
				MS_ABORT("notification failed with unexpected error: %s", error.what());
			}
		}
		else
		{
			MS_ERROR("discarding wrong Channel data");
		}
	}

	void ChannelSocket::OnConsumerSocketClosed(const ConsumerSocket* /*consumerSocket*/)
	{
		MS_TRACE_STD();

		this->listener->OnChannelClosed(this);
	}

	/* Instance methods. */

	ConsumerSocket::ConsumerSocket(int fd, size_t bufferSize, Listener* listener)
	  : UnixStreamSocketHandle(fd, bufferSize, UnixStreamSocketHandle::Role::CONSUMER),
	    listener(listener)
	{
		MS_TRACE_STD();
	}

	ConsumerSocket::~ConsumerSocket()
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
			{
				return;
			}

			const size_t readLen = this->bufferDataLen - msgStart;

			if (readLen < sizeof(uint32_t))
			{
				// Incomplete data.
				break;
			}

			uint32_t msgLen;

			// Read message length.
			std::memcpy(std::addressof(msgLen), this->buffer + msgStart, sizeof(uint32_t));

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
			this->bufferDataLen -= msgStart;

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

	/* Instance methods. */

	ProducerSocket::ProducerSocket(int fd, size_t bufferSize)
	  : UnixStreamSocketHandle(fd, bufferSize, UnixStreamSocketHandle::Role::PRODUCER)
	{
		MS_TRACE_STD();
	}
} // namespace Channel
