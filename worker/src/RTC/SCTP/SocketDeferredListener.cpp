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

		bool SocketDeferredListener::OnSocketSendSctpPacket(const Socket* socket, Packet* packet)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			// Will not be deferred but called directly.
			return this->innerListener->OnSocketSendSctpPacket(socket, packet);
		}

		void SocketDeferredListener::OnConnected(const Socket* socket)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData /*data*/, SocketListener* listener) { listener->OnConnected(socket); },
			  std::monostate{});
		}

		void SocketDeferredListener::OnMessageReceived(const Socket* socket, Message message)
		{
			MS_TRACE();

			this->deferredCallbacks.emplace_back(
			  [socket](CallbackData data, SocketListener* listener)
			  { listener->OnMessageReceived(socket, std::get<Message>(std::move(data))); },
			  std::move(message));
		}
	} // namespace SCTP
} // namespace RTC
