#define MS_CLASS "RTC::SCTP::AssociationDeferredListener"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/AssociationDeferredListener.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		AssociationDeferredListener::ScopedDeferred::ScopedDeferred(
		  AssociationDeferredListener& deferredListener)
		  : deferredListener(deferredListener)
		{
			MS_TRACE();

			this->deferredListener.SetReady();
		}

		AssociationDeferredListener::ScopedDeferred::~ScopedDeferred()
		{
			MS_TRACE();

			this->deferredListener.TriggerDeferredCallbacks();
		}

		AssociationDeferredListener::AssociationDeferredListener(AssociationListener* innerListener)
		  : innerListener(innerListener)
		{
			MS_TRACE();

			this->deferredCallbacks.reserve(8);
		}

		void AssociationDeferredListener::SetReady()
		{
			MS_TRACE();

			this->ready = true;
		}

		void AssociationDeferredListener::TriggerDeferredCallbacks()
		{
			MS_TRACE();

			if (!this->ready)
			{
				return;
			}

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

		bool AssociationDeferredListener::OnAssociationSendData(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			// Will not be deferred but called directly.
			return this->innerListener->OnAssociationSendData(data, len);
		}

		void AssociationDeferredListener::OnAssociationConnecting()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData /*data*/, AssociationListener* listener)
			  {
				  listener->OnAssociationConnecting();
			  },
			  std::monostate{});
		}

		void AssociationDeferredListener::OnAssociationConnected()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData /*data*/, AssociationListener* listener)
			  {
				  listener->OnAssociationConnected();
			  },
			  std::monostate{});
		}

		void AssociationDeferredListener::OnAssociationFailed(
		  Types::ErrorKind errorKind, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  const Error error = std::get<Error>(std::move(data));
				  listener->OnAssociationFailed(error.errorKind, error.message);
			  },
			  Error{ .errorKind = errorKind, .message = std::string(errorMessage) });
		}

		void AssociationDeferredListener::OnAssociationClosed(
		  Types::ErrorKind errorKind, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  const Error error = std::get<Error>(std::move(data));
				  listener->OnAssociationClosed(error.errorKind, error.message);
			  },
			  Error{ .errorKind = errorKind, .message = std::string(errorMessage) });
		}

		void AssociationDeferredListener::OnAssociationRestarted()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData /*data*/, AssociationListener* listener)
			  {
				  listener->OnAssociationRestarted();
			  },
			  std::monostate{});
		}

		void AssociationDeferredListener::OnAssociationError(
		  Types::ErrorKind errorKind, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  const Error error = std::get<Error>(std::move(data));
				  listener->OnAssociationError(error.errorKind, error.message);
			  },
			  Error{ .errorKind = errorKind, .message = std::string(errorMessage) });
		}

		void AssociationDeferredListener::OnAssociationMessageReceived(Message message)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  listener->OnAssociationMessageReceived(std::get<Message>(std::move(data)));
			  },
			  std::move(message));
		}

		void AssociationDeferredListener::OnAssociationStreamsResetPerformed(
		  std::span<const uint16_t> outboundStreamIds)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener->OnAssociationStreamsResetPerformed(streamReset.streamIds);
      },
			  StreamReset{ .streamIds = { outboundStreamIds.begin(), outboundStreamIds.end() } });
		}

		void AssociationDeferredListener::OnAssociationStreamsResetFailed(
		  std::span<const uint16_t> outboundStreamIds, std::string_view errorMessage)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener->OnAssociationStreamsResetFailed(streamReset.streamIds, streamReset.errorMessage);
      },
			  StreamReset{ .streamIds    = { outboundStreamIds.begin(), outboundStreamIds.end() },
			               .errorMessage = std::string(errorMessage) });
		}

		void AssociationDeferredListener::OnAssociationInboundStreamsReset(
		  std::span<const uint16_t> inboundStreamIds)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  StreamReset streamReset = std::get<StreamReset>(std::move(data));
				  listener->OnAssociationInboundStreamsReset(streamReset.streamIds);
      },
			  StreamReset{ .streamIds = { inboundStreamIds.begin(), inboundStreamIds.end() } });
		}

		void AssociationDeferredListener::OnAssociationStreamBufferedAmountLow(uint16_t streamId)
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData data, AssociationListener* listener)
			  {
				  listener->OnAssociationStreamBufferedAmountLow(std::get<uint16_t>(std::move(data)));
			  },
			  streamId);
		}

		void AssociationDeferredListener::OnAssociationTotalBufferedAmountLow()
		{
			MS_TRACE();

			MS_ASSERT(this->ready, "not ready");

			this->deferredCallbacks.emplace_back(
			  [](CallbackData /*data*/, AssociationListener* listener)
			  {
				  listener->OnAssociationTotalBufferedAmountLow();
			  },
			  std::monostate{});
		}
	} // namespace SCTP
} // namespace RTC
