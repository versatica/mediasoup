#ifndef MS_RTC_SCTP_ASSOCIATION_DEFERRED_LISTENER_HPP
#define MS_RTC_SCTP_ASSOCIATION_DEFERRED_LISTENER_HPP

#include "common.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		class AssociationDeferredListener : public AssociationListener
		{
		public:
			class ScopedDeferred
			{
			public:
				explicit ScopedDeferred(AssociationDeferredListener& deferredListener);

				~ScopedDeferred();

			private:
				AssociationDeferredListener& deferredListener;
			};

		private:
			struct Error
			{
				Types::ErrorKind errorKind;
				std::string message;
			};

			struct StreamReset
			{
				std::vector<uint16_t> streamIds;
				std::string errorMessage;
			};

			// Use a pre-sized variant for storage to avoid double heap allocation. This
			// variant can hold all cases of stored data.
			using CallbackData = std::variant<std::monostate, Message, Error, StreamReset, uint16_t>;

			using Callback = std::function<void(CallbackData, AssociationListener*)>;

		public:
			explicit AssociationDeferredListener(AssociationListener* innerListener);

		private:
			void SetReady();

			void TriggerDeferredCallbacks();

		public:
			/* Pure virtual methods inherited from RTC::STCP::AssociationListener. */
			bool OnAssociationSendData(const uint8_t* data, size_t len) override;

			void OnAssociationConnecting() override;

			void OnAssociationConnected() override;

			void OnAssociationFailed(Types::ErrorKind errorKind, std::string_view errorMessage) override;

			void OnAssociationClosed(Types::ErrorKind errorKind, std::string_view errorMessage) override;

			void OnAssociationRestarted() override;

			void OnAssociationError(Types::ErrorKind errorKind, std::string_view errorMessage) override;

			void OnAssociationMessageReceived(Message message) override;

			void OnAssociationStreamsResetPerformed(std::span<const uint16_t> outboundStreamIds) override;

			void OnAssociationStreamsResetFailed(
			  std::span<const uint16_t> outboundStreamIds, std::string_view errorMessage) override;

			void OnAssociationInboundStreamsReset(std::span<const uint16_t> inboundStreamIds) override;

			void OnAssociationStreamBufferedAmountLow(uint16_t streamId) override;

			void OnAssociationTotalBufferedAmountLow() override;

		private:
			AssociationListener* innerListener;
			bool ready{ false };
			std::vector<std::pair<Callback, CallbackData>> deferredCallbacks;
		};
	} // namespace SCTP
} // namespace RTC

#endif
