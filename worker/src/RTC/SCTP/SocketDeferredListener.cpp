#define MS_CLASS "RTC::SCTP::SocketDeferredListener"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/SocketDeferredListener.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		SocketDeferredListener::ScopedDeferred::ScopedDeferred(SocketDeferredListener& deferredListener)
		  : deferredListener(deferredListener)
		{
			MS_TRACE();

			this->deferredListener.SetReady();
		}

		SocketDeferredListener::ScopedDeferred::~ScopedDeferred()
		{
			MS_TRACE();

			this->deferredListener.TriggerDeferredCallbacks();
		}

		SocketDeferredListener::SocketDeferredListener(SocketListener& innerListener)
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

		bool SocketDeferredListener::OnSocketSendPacket(Packet* packet)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			// Will not be deferred but called directly.
			return this->innerListener.OnSocketSendPacket(packet);
		}

		void SocketDeferredListener::OnSocketConnected()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData /*data*/, SocketListener& listener)
			  {
				  listener.OnSocketConnected();
			  },
			  std::monostate{});
		}

		void SocketDeferredListener::OnSocketClosed()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData /*data*/, SocketListener& listener)
			  {
				  listener.OnSocketClosed();
			  },
			  std::monostate{});
		}

		void SocketDeferredListener::OnSocketConnectionRestarted()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData /*data*/, SocketListener& listener)
			  {
				  listener.OnSocketConnectionRestarted();
			  },
			  std::monostate{});
		}

		void SocketDeferredListener::OnSocketError(Types::ErrorKind errorKind, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  Error error = std::get<Error>(std::move(data));
				  listener.OnSocketError(error.errorKind, error.message);
			  },
			  Error{ .errorKind = errorKind, .message = std::string(errorMessage) });
		}

		void SocketDeferredListener::OnSocketAborted(Types::ErrorKind errorKind, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  Error error = std::get<Error>(std::move(data));
				  listener.OnSocketAborted(error.errorKind, error.message);
			  },
			  Error{ .errorKind = errorKind, .message = std::string(errorMessage) });
		}

		void SocketDeferredListener::OnSocketMessageReceived(Message message)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  listener.OnSocketMessageReceived(std::get<Message>(std::move(data)));
			  },
			  std::move(message));
		}

		void SocketDeferredListener::OnSocketStreamsResetPerformed(std::span<const uint16_t> outboundStreamIds)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener.OnSocketStreamsResetPerformed(streamReset.streamIds);
      },
			  StreamReset{ .streamIds = { outboundStreamIds.begin(), outboundStreamIds.end() } });
		}

		void SocketDeferredListener::OnSocketStreamsResetFailed(
		  std::span<const uint16_t> outboundStreamIds, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener.OnSocketStreamsResetFailed(streamReset.streamIds, streamReset.errorMessage);
      },
			  StreamReset{ .streamIds    = { outboundStreamIds.begin(), outboundStreamIds.end() },
			               .errorMessage = std::string(errorMessage) });
		}

		void SocketDeferredListener::OnSocketInboundStreamsReset(std::span<const uint16_t> inboundStreamIds)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener.OnSocketInboundStreamsReset(streamReset.streamIds);
      },
			  StreamReset{ .streamIds = { inboundStreamIds.begin(), inboundStreamIds.end() } });
		}

		void SocketDeferredListener::OnSocketStreamBufferedAmountLow(uint16_t streamId)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  listener.OnSocketStreamBufferedAmountLow(std::get<uint16_t>(std::move(data)));
			  },
			  streamId);
		}

		void SocketDeferredListener::OnSocketTotalBufferedAmountLow()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, SocketListener& listener)
			  {
				  listener.OnSocketTotalBufferedAmountLow();
			  },
			  std::monostate{});
		}
	} // namespace SCTP
} // namespace RTC
