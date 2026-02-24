#define MS_CLASS "RTC::SCTP::SocketDeferredListener"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/SocketDeferredListener.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		SocketDeferredListener::ScopedDeferred::ScopedDeferred(SocketDeferredListener* deferredListener)
		  : deferredListener(deferredListener)
		{
			MS_TRACE();

			this->deferredListener->SetReady();
		}

		SocketDeferredListener::ScopedDeferred::~ScopedDeferred()
		{
			MS_TRACE();

			this->deferredListener->TriggerDeferredCallbacks();
		}

		SocketDeferredListener::SocketDeferredListener(SocketListener* innerListener)
		  : innerListener(innerListener)
		{
			MS_TRACE();

			this->deferredCallbacks.reserve(8);
		}

		void SocketDeferredListener::SetReady()
		{
			MS_TRACE();

			this->ready = true;
		}

		void SocketDeferredListener::TriggerDeferredCallbacks()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->ready = false;

			if (this->deferredCallbacks.empty())
			{
				return;
			}

			// Need to swap stored callbacks here. The caller may call into the library
			// from within a callback, and that might result in adding new callbacks to
			// this instance, and the vector can't be modified while iterated on.

			std::vector<std::pair<Callback, CallbackData>> localDeferredCallbacks;

			// Reserve a small buffer to prevent too much reallocation on growth.
			localDeferredCallbacks.reserve(8);

			localDeferredCallbacks.swap(this->deferredCallbacks);

			for (auto& [callback, data] : localDeferredCallbacks)
			{
				callback(std::move(data), this->innerListener);
			}
		}

		bool SocketDeferredListener::OnSocketSendSctpPacket(Socket* socket, Packet* packet)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			// Will not be deferred but called directly.
			return this->innerListener->OnSocketSendSctpPacket(socket, packet);
		}

		void SocketDeferredListener::OnSocketConnected(Socket* socket)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData /*data*/, SocketListener* listener)
			  {
				  listener->OnSocketConnected(socket);
			  },
			  std::monostate{});
		}

		void SocketDeferredListener::OnSocketClosed(Socket* socket)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData /*data*/, SocketListener* listener)
			  {
				  listener->OnSocketClosed(socket);
			  },
			  std::monostate{});
		}

		void SocketDeferredListener::OnSocketConnectionRestarted(Socket* socket)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData /*data*/, SocketListener* listener)
			  {
				  listener->OnSocketConnectionRestarted(socket);
			  },
			  std::monostate{});
		}

		void SocketDeferredListener::OnSocketError(
		  Socket* socket, Types::ErrorKind errorKind, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  Error error = std::get<Error>(std::move(data));
				  listener->OnSocketError(socket, error.errorKind, error.message);
			  },
			  Error{ .errorKind = errorKind, .message = std::string(errorMessage) });
		}

		void SocketDeferredListener::OnSocketAborted(
		  Socket* socket, Types::ErrorKind errorKind, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  Error error = std::get<Error>(std::move(data));
				  listener->OnSocketAborted(socket, error.errorKind, error.message);
			  },
			  Error{ .errorKind = errorKind, .message = std::string(errorMessage) });
		}

		void SocketDeferredListener::OnSocketMessageReceived(Socket* socket, Message message)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  listener->OnSocketMessageReceived(socket, std::get<Message>(std::move(data)));
			  },
			  std::move(message));
		}

		void SocketDeferredListener::OnSocketStreamsResetPerformed(
		  Socket* socket, std::span<const uint16_t> outboundStreamIds)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener->OnSocketStreamsResetPerformed(socket, streamReset.streamIds);
      },
			  StreamReset{ .streamIds = { outboundStreamIds.begin(), outboundStreamIds.end() } });
		}

		void SocketDeferredListener::OnSocketStreamsResetFailed(
		  Socket* socket, std::span<const uint16_t> outboundStreamIds, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener->OnSocketStreamsResetFailed(
				    socket, streamReset.streamIds, streamReset.errorMessage);
      },
			  StreamReset{ .streamIds    = { outboundStreamIds.begin(), outboundStreamIds.end() },
			               .errorMessage = std::string(errorMessage) });
		}

		void SocketDeferredListener::OnSocketInboundStreamsReset(
		  Socket* socket, std::span<const uint16_t> inboundStreamIds)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener->OnSocketInboundStreamsReset(socket, streamReset.streamIds);
      },
			  StreamReset{ .streamIds = { inboundStreamIds.begin(), inboundStreamIds.end() } });
		}

		void SocketDeferredListener::OnSocketBufferedAmountLow(Socket* socket, uint16_t streamId)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  listener->OnSocketBufferedAmountLow(socket, std::get<uint16_t>(std::move(data)));
			  },
			  streamId);
		}

		void SocketDeferredListener::OnSocketTotalBufferedAmountLow(Socket* socket)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  {
				  listener->OnSocketTotalBufferedAmountLow(socket);
			  },
			  std::monostate{});
		}
	} // namespace SCTP
} // namespace RTC
