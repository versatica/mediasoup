#define MS_CLASS "PayloadChannel::PayloadChannelSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/PayloadChannelSocket.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "PayloadChannel/PayloadChannelRequest.hpp"
#include <cmath>   // std::ceil()
#include <cstdio>  // sprintf()
#include <cstring> // std::memcpy(), std::memmove()

namespace PayloadChannel
{
	/* Static. */
	inline static void onAsync(uv_handle_t* handle)
	{
		while (static_cast<PayloadChannelSocket*>(handle->data)->CallbackRead())
		{
			// Read while there are new messages.
		}
	}

	inline static void onClose(uv_handle_t* handle)
	{
		delete handle;
	}

	// Binary length for a 4194304 bytes payload.
	static constexpr size_t MessageMaxLen{ 4194308 };
	static constexpr size_t PayloadMaxLen{ 4194304 };

	/* Instance methods. */
	PayloadChannelSocket::PayloadChannelSocket(int consumerFd, int producerFd)
	  : consumerSocket(new ConsumerSocket(consumerFd, MessageMaxLen, this)),
	    producerSocket(new ProducerSocket(producerFd, MessageMaxLen)),
	    writeBuffer(static_cast<uint8_t*>(std::malloc(MessageMaxLen)))
	{
		MS_TRACE();
	}

	PayloadChannelSocket::PayloadChannelSocket(
	  PayloadChannelReadFn payloadChannelReadFn,
	  PayloadChannelReadCtx payloadChannelReadCtx,
	  PayloadChannelWriteFn payloadChannelWriteFn,
	  PayloadChannelWriteCtx payloadChannelWriteCtx)
	  : payloadChannelReadFn(payloadChannelReadFn), payloadChannelReadCtx(payloadChannelReadCtx),
	    payloadChannelWriteFn(payloadChannelWriteFn), payloadChannelWriteCtx(payloadChannelWriteCtx)
	{
		MS_TRACE();

		int err;

		this->uvReadHandle       = new uv_async_t;
		this->uvReadHandle->data = static_cast<void*>(this);

		err =
		  uv_async_init(DepLibUV::GetLoop(), this->uvReadHandle, reinterpret_cast<uv_async_cb>(onAsync));

		if (err != 0)
		{
			delete this->uvReadHandle;
			this->uvReadHandle = nullptr;

			MS_THROW_ERROR("uv_async_init() failed: %s", uv_strerror(err));
		}

		err = uv_async_send(this->uvReadHandle);

		if (err != 0)
		{
			delete this->uvReadHandle;
			this->uvReadHandle = nullptr;

			MS_THROW_ERROR("uv_async_send() failed: %s", uv_strerror(err));
		}
	}

	PayloadChannelSocket::~PayloadChannelSocket()
	{
		MS_TRACE();

		std::free(this->writeBuffer);
		delete this->ongoingNotification;

		if (!this->closed)
			Close();

		delete this->consumerSocket;
		delete this->producerSocket;
	}

	void PayloadChannelSocket::Close()
	{
		MS_TRACE_STD();

		if (this->closed)
			return;

		this->closed = true;

		if (this->uvReadHandle)
		{
			uv_close(reinterpret_cast<uv_handle_t*>(this->uvReadHandle), static_cast<uv_close_cb>(onClose));
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

	void PayloadChannelSocket::SetListener(Listener* listener)
	{
		MS_TRACE();

		this->listener = listener;
	}

	void PayloadChannelSocket::Send(json& jsonMessage, const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		if (this->closed)
			return;

		std::string message = jsonMessage.dump();

		if (message.length() > PayloadMaxLen)
		{
			MS_ERROR("message too big");

			return;
		}
		else if (payloadLen > PayloadMaxLen)
		{
			MS_ERROR("payload too big");

			return;
		}

		SendImpl(
		  reinterpret_cast<const uint8_t*>(message.c_str()),
		  static_cast<uint32_t>(message.length()),
		  payload,
		  static_cast<uint32_t>(payloadLen));
	}

	void PayloadChannelSocket::Send(json& jsonMessage)
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

	bool PayloadChannelSocket::CallbackRead()
	{
		MS_TRACE();

		if (this->closed)
			return false;

		uint8_t* message{ nullptr };
		uint32_t messageLen;
		size_t messageCtx;

		uint8_t* payload{ nullptr };
		uint32_t payloadLen;
		size_t payloadCapacity;

		auto free = this->payloadChannelReadFn(
		  &message,
		  &messageLen,
		  &messageCtx,
		  &payload,
		  &payloadLen,
		  &payloadCapacity,
		  this->uvReadHandle,
		  this->payloadChannelReadCtx);

		if (free)
		{
			try
			{
				json jsonData = json::parse(message, message + static_cast<size_t>(messageLen));

				if (PayloadChannelRequest::IsRequest(jsonData))
				{
					try
					{
						auto* request = new PayloadChannel::PayloadChannelRequest(this, jsonData);
						request->SetPayload(payload, payloadLen);

						// Notify the listener.
						try
						{
							this->listener->OnPayloadChannelRequest(this, request);
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
						MS_ERROR("discarding wrong Payload Channel notification");
					}
				}

				else if (Notification::IsNotification(jsonData))
				{
					try
					{
						auto* notification = new PayloadChannel::Notification(jsonData);
						notification->SetPayload(payload, payloadLen);

						// Notify the listener.
						try
						{
							this->listener->OnPayloadChannelNotification(this, notification);
						}
						catch (const MediaSoupError& error)
						{
							MS_ERROR("notification failed: %s", error.what());
						}

						// Delete the Notification.
						delete notification;
					}
					catch (const json::parse_error& error)
					{
						MS_ERROR_STD("JSON parsing error: %s", error.what());
					}
					catch (const MediaSoupError& error)
					{
						MS_ERROR("discarding wrong Payload Channel notification");
					}
				}

				else
				{
					MS_ERROR("discarding wrong Payload Channel data");
				}
			}
			catch (const json::parse_error& error)
			{
				MS_ERROR("JSON parsing error: %s", error.what());
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR("discarding wrong Channel request");
			}

			free(message, messageLen, messageCtx);
			free(payload, payloadLen, payloadCapacity);
		}

		return free != nullptr;
	}

	inline void PayloadChannelSocket::SendImpl(const uint8_t* message, uint32_t messageLen)
	{
		MS_TRACE();

		// Write using function call if provided.
		if (this->payloadChannelWriteFn)
		{
			this->payloadChannelWriteFn(message, messageLen, nullptr, 0, this->payloadChannelWriteCtx);
		}
		else
		{
			std::memcpy(this->writeBuffer, &messageLen, sizeof(uint32_t));

			if (messageLen != 0)
			{
				std::memcpy(this->writeBuffer + sizeof(uint32_t), message, messageLen);
			}

			size_t len = sizeof(uint32_t) + messageLen;

			this->producerSocket->Write(this->writeBuffer, len);
		}
	}

	inline void PayloadChannelSocket::SendImpl(
	  const uint8_t* message, uint32_t messageLen, const uint8_t* payload, uint32_t payloadLen)
	{
		MS_TRACE();

		// Write using function call if provided.
		if (this->payloadChannelWriteFn)
		{
			this->payloadChannelWriteFn(
			  message, messageLen, payload, payloadLen, this->payloadChannelWriteCtx);
		}
		else
		{
			SendImpl(message, messageLen);
			SendImpl(payload, payloadLen);
		}
	}

	void PayloadChannelSocket::OnConsumerSocketMessage(
	  ConsumerSocket* /*consumerSocket*/, char* msg, size_t msgLen)
	{
		MS_TRACE();

		if (!this->ongoingNotification && !this->ongoingRequest)
		{
			json jsonData = json::parse(msg, msg + msgLen);
			if (PayloadChannelRequest::IsRequest(jsonData))
			{
				try
				{
					this->ongoingRequest = new PayloadChannel::PayloadChannelRequest(this, jsonData);
				}
				catch (const json::parse_error& error)
				{
					MS_ERROR_STD("JSON parsing error: %s", error.what());
				}
				catch (const MediaSoupError& error)
				{
					MS_ERROR("discarding wrong Payload Channel notification");
				}
			}

			else if (Notification::IsNotification(jsonData))
			{
				try
				{
					this->ongoingNotification = new PayloadChannel::Notification(jsonData);
				}
				catch (const json::parse_error& error)
				{
					MS_ERROR_STD("JSON parsing error: %s", error.what());
				}
				catch (const MediaSoupError& error)
				{
					MS_ERROR("discarding wrong Payload Channel notification");
				}
			}

			else
			{
				MS_ERROR("discarding wrong Payload Channel data");
			}
		}
		else if (this->ongoingNotification)
		{
			this->ongoingNotification->SetPayload(reinterpret_cast<const uint8_t*>(msg), msgLen);

			// Notify the listener.
			try
			{
				this->listener->OnPayloadChannelNotification(this, this->ongoingNotification);
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR("notification failed: %s", error.what());
			}

			// Delete the Notification.
			delete this->ongoingNotification;
			this->ongoingNotification = nullptr;
		}
		else if (this->ongoingRequest)
		{
			this->ongoingRequest->SetPayload(reinterpret_cast<const uint8_t*>(msg), msgLen);

			// Notify the listener.
			try
			{
				this->listener->OnPayloadChannelRequest(this, this->ongoingRequest);
			}
			catch (const MediaSoupTypeError& error)
			{
				this->ongoingRequest->TypeError(error.what());
			}
			catch (const MediaSoupError& error)
			{
				this->ongoingRequest->Error(error.what());
			}

			// Delete the Request.
			delete this->ongoingRequest;
			this->ongoingRequest = nullptr;
		}
	}

	void PayloadChannelSocket::OnConsumerSocketClosed(ConsumerSocket* /*consumerSocket*/)
	{
		MS_TRACE();

		this->listener->OnPayloadChannelClosed(this);
	}

	ConsumerSocket::ConsumerSocket(int fd, size_t bufferSize, Listener* listener)
	  : ::UnixStreamSocket(fd, bufferSize, ::UnixStreamSocket::Role::CONSUMER), listener(listener)
	{
		MS_TRACE();
	}

	ConsumerSocket::~ConsumerSocket()
	{
		MS_TRACE();
	}

	void ConsumerSocket::UserOnUnixStreamRead()
	{
		MS_TRACE();

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
		MS_TRACE();

		// Notify the listener.
		this->listener->OnConsumerSocketClosed(this);
	}

	ProducerSocket::ProducerSocket(int fd, size_t bufferSize)
	  : ::UnixStreamSocket(fd, bufferSize, ::UnixStreamSocket::Role::PRODUCER)
	{
		MS_TRACE();
	}
} // namespace PayloadChannel
