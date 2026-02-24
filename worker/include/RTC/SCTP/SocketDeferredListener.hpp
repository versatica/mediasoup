#ifndef MS_RTC_SCTP_SOCKET_DEFERRED_LISTENER_HPP
#define MS_RTC_SCTP_SOCKET_DEFERRED_LISTENER_HPP

#include "common.hpp"
#include "RTC/SCTP/Message.hpp"
#include "RTC/SCTP/SocketListener.hpp"
#include "RTC/SCTP/Types.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		class SocketDeferredListener : public SocketListener
		{
		public:
			class ScopedDeferred
			{
			public:
				explicit ScopedDeferred(SocketDeferredListener* deferredListener);

				~ScopedDeferred();

			private:
				SocketDeferredListener* deferredListener;
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

			using Callback = std::function<void(CallbackData, SocketListener*)>;

		public:
			explicit SocketDeferredListener(SocketListener* innerListener);

		private:
			void SetReady();

			void TriggerDeferredCallbacks();

		public:
			/* Pure virtual methods inherited from Socket::Listener. */
			bool OnSocketSendSctpPacket(Socket* socket, Packet* packet) override;

			void OnSocketConnected(Socket* socket) override;

			void OnSocketClosed(Socket* socket) override;

			void OnSocketConnectionRestarted(Socket* socket) override;

			void OnSocketError(
			  Socket* socket, Types::ErrorKind errorKind, std::string_view errorMessage) override;

			void OnSocketAborted(
			  Socket* socket, Types::ErrorKind errorKind, std::string_view errorMessage) override;

			void OnSocketMessageReceived(Socket* socket, Message message) override;

			void OnSocketStreamsResetPerformed(
			  Socket* socket, std::span<const uint16_t> outboundStreamIds) override;

			void OnSocketStreamsResetFailed(
			  Socket* socket,
			  std::span<const uint16_t> outboundStreamIds,
			  std::string_view errorMessage) override;

			void OnSocketInboundStreamsReset(Socket* socket, std::span<const uint16_t> inboundStreamIds) override;

			void OnSocketBufferedAmountLow(Socket* socket, uint16_t streamId) override;

			void OnSocketTotalBufferedAmountLow(Socket* socket) override;

		private:
			SocketListener* innerListener{ nullptr };
			bool ready{ false };
			std::vector<std::pair<Callback, CallbackData>> deferredCallbacks;
		};
	} // namespace SCTP
} // namespace RTC

#endif
