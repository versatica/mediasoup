#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "FBS/worker_generated.h"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "PayloadChannel/PayloadChannelNotification.hpp"
#include "PayloadChannel/PayloadChannelRequest.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include "RTC/Router.hpp"
#include "RTC/WebRtcServer.hpp"
#include "handles/SignalsHandler.hpp"
#include <absl/container/flat_hash_map.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <string>

class Worker : public Channel::ChannelSocket::Listener,
               public PayloadChannel::PayloadChannelSocket::Listener,
               public SignalsHandler::Listener,
               public RTC::Router::Listener
{
public:
	explicit Worker(Channel::ChannelSocket* channel, PayloadChannel::PayloadChannelSocket* payloadChannel);
	~Worker();

private:
	void Close();
	flatbuffers::Offset<FBS::Worker::DumpResponse> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;
	flatbuffers::Offset<FBS::Worker::ResourceUsageResponse> FillBufferResourceUsage(
	  flatbuffers::FlatBufferBuilder& builder) const;
	void SetNewRouterId(std::string& routerId) const;
	RTC::WebRtcServer* GetWebRtcServer(const std::string& webRtcServerId) const;
	RTC::Router* GetRouter(const std::string& routerId) const;
	void CheckNoWebRtcServer(const std::string& webRtcServerId) const;
	void CheckNoRouter(const std::string& webRtcServerId) const;

	/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
public:
	void HandleRequest(Channel::ChannelRequest* request) override;

	/* Methods inherited from Channel::ChannelSocket::Listener. */
public:
	void OnChannelClosed(Channel::ChannelSocket* channel) override;

	/* Methods inherited from PayloadChannel::PayloadChannelSocket::RequestHandler. */
public:
	void HandleRequest(PayloadChannel::PayloadChannelRequest* request) override;

	/* Methods inherited from PayloadChannel::PayloadChannelSocket::NotificationHandler. */
public:
	void HandleNotification(PayloadChannel::PayloadChannelNotification* notification) override;

	/* Methods inherited from PayloadChannel::PayloadChannelSocket::Listener. */
public:
	void OnPayloadChannelClosed(PayloadChannel::PayloadChannelSocket* payloadChannel) override;

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

	/* Pure virtual methods inherited from RTC::Router::Listener. */
public:
	RTC::WebRtcServer* OnRouterNeedWebRtcServer(RTC::Router* router, std::string& webRtcServerId) override;

private:
	// Passed by argument.
	Channel::ChannelSocket* channel{ nullptr };
	PayloadChannel::PayloadChannelSocket* payloadChannel{ nullptr };
	// Allocated by this.
	SignalsHandler* signalsHandler{ nullptr };
	absl::flat_hash_map<std::string, RTC::WebRtcServer*> mapWebRtcServers;
	absl::flat_hash_map<std::string, RTC::Router*> mapRouters;
	// Others.
	bool closed{ false };
};

#endif
