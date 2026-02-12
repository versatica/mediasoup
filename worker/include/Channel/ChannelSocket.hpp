#ifndef MS_CHANNEL_SOCKET_HPP
#define MS_CHANNEL_SOCKET_HPP

#include "common.hpp"
#include "Channel/ChannelNotification.hpp"
#include "Channel/ChannelRequest.hpp"
#include "handles/UnixStreamSocketHandle.hpp"

namespace Channel
{
	class Socket : public UnixStreamSocketHandle
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnSocketMessage(const Socket* socket, char* msg, size_t msgLen) = 0;
			virtual void OnSocketClosed(const Socket* socket)                            = 0;
		};

	public:
		Socket(int fd, size_t bufferSize, Listener* listener);
		~Socket() override;

		/* Pure virtual methods inherited from UnixStreamSocketHandle. */
	public:
		void UserOnUnixStreamRead() override;
		void UserOnUnixStreamSocketClosed() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
	};

	class ChannelSocket : public Socket::Listener
	{
	public:
		class RequestHandler
		{
		public:
			virtual ~RequestHandler() = default;

		public:
			virtual void HandleRequest(Channel::ChannelRequest* request) = 0;
		};

		class NotificationHandler
		{
		public:
			virtual ~NotificationHandler() = default;

		public:
			virtual void HandleNotification(Channel::ChannelNotification* notification) = 0;
		};

		class Listener : public RequestHandler, public NotificationHandler
		{
		public:
			~Listener() override = default;

		public:
			virtual void OnChannelClosed(Channel::ChannelSocket* channel) = 0;
		};

	public:
#ifdef MS_TEST
		explicit ChannelSocket();
#endif
		explicit ChannelSocket(int channelFd);
		explicit ChannelSocket(
		  ChannelReadFn channelReadFn,
		  ChannelReadCtx channelReadCtx,
		  ChannelWriteFn channelWriteFn,
		  ChannelWriteCtx channelWriteCtx);
		~ChannelSocket() override;

	public:
		void Close();
		void SetListener(Listener* listener);
		void Send(const uint8_t* data, uint32_t dataLen);
		void SendLog(const char* data, uint32_t dataLen);
		bool CallbackRead();

	private:
		void SendImpl(const uint8_t* payload, uint32_t payloadLen);

		/* Pure virtual methods inherited from Socket::Listener. */
	public:
		void OnSocketMessage(const Socket* socket, char* msg, size_t msgLen) override;
		void OnSocketClosed(const Socket* socket) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		bool closed{ false };
		Socket* socket{ nullptr };
		ChannelReadFn channelReadFn{ nullptr };
		ChannelReadCtx channelReadCtx{ nullptr };
		ChannelWriteFn channelWriteFn{ nullptr };
		ChannelWriteCtx channelWriteCtx{ nullptr };
		uv_async_t* uvReadHandle{ nullptr };
		flatbuffers::FlatBufferBuilder bufferBuilder;
	};
} // namespace Channel

#endif
