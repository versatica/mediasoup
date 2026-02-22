#define MS_CLASS "RTC::SCTP::SocketDeferredListener"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/SocketDeferredListener.hpp"
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

		void SocketDeferredListener::OnSocketSendSctpPacket(const Socket* socket, Packet* packet) const
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			// Will not be deferred but called directly.
			this->innerListener->OnSocketSendSctpPacket(socket, packet);
		}

		void SocketDeferredListener::OnConnected()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  +[](CallbackData /*data*/, SocketListener* callback) { return callback->OnConnected(); },
			  std::monostate{});
		}
	} // namespace SCTP
} // namespace RTC
