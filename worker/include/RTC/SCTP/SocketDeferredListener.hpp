#ifndef MS_RTC_SCTP_SOCKET_DEFERRED_LISTENER_HPP
#define MS_RTC_SCTP_SOCKET_DEFERRED_LISTENER_HPP

#include "common.hpp"
#include "RTC/SCTP/Message.hpp"
#include "RTC/SCTP/SocketListener.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include <string>
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
			// TODO
			// struct Error
			// {
			// 	ErrorKind error;
			// 	std::string message;
			// };

			struct StreamReset
			{
				std::vector<uint16_t> streamIds;
				std::string errorMessage;
			};

			// Use a pre-sized variant for storage to avoid double heap allocation. This
			// variant can hold all cases of stored data.
			// TODO
			// using CallbackData = std::variant<std::monostate, DcSctpMessage, Error, StreamReset, StreamID>;
			using CallbackData = std::variant<std::monostate, Message, StreamReset, uint16_t>;

			using Callback = std::function<void(CallbackData, SocketListener*)>;

		public:
			explicit SocketDeferredListener(SocketListener* innerListener);

		private:
			void SetReady();

			void TriggerDeferredCallbacks();

		public:
			/* Pure virtual methods inherited from Socket::Listener. */
			bool OnSocketSendSctpPacket(const Socket* socket, Packet* packet) override;

			void OnConnected(const Socket* socket) override;

			void OnMessageReceived(const Socket* socket, Message message) override;

		private:
			SocketListener* innerListener{ nullptr };
			bool ready{ false };
			std::vector<std::pair<Callback, CallbackData>> deferredCallbacks;
		};
	} // namespace SCTP
} // namespace RTC

#endif
