#ifndef MS_RTC_SCTP_SOCKET_DEFERRED_LISTENER_HPP
#define MS_RTC_SCTP_SOCKET_DEFERRED_LISTENER_HPP

#include "common.hpp"
#include "RTC/SCTP/association/SocketListener.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include <string>
#include <utility> // std::pair
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
			using CallbackData = std::variant<std::monostate, StreamReset, uint16_t>;

			using Callback = void (*)(CallbackData, SocketListener*);

		public:
			explicit SocketDeferredListener(SocketListener* innerListener);

		private:
			void SetReady();

			void TriggerDeferredCallbacks();

		public:
			/* Pure virtual methods inherited from Socket::Listener. */
			void OnSocketSendSctpPacket(const Socket* socket, Packet* packet) const override;

			void OnConnected() override;

		private:
			SocketListener* innerListener{ nullptr };
			bool ready{ false };
			std::vector<std::pair<Callback, CallbackData>> deferredCallbacks;
		};
	} // namespace SCTP
} // namespace RTC

#endif
