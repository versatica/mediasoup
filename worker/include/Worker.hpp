#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "FBS/worker.h"
#include "handles/SignalHandle.hpp"
#include "RTC/Router.hpp"
#include "RTC/WebRtcServer.hpp"
#include "SharedInterface.hpp"
#include <flatbuffers/flatbuffer_builder.h>
#include <ankerl/unordered_dense.h>
#include <string>

class Worker : public Channel::ChannelSocket::Listener,
               public SignalHandle::Listener,
               public RTC::Router::Listener
{
public:
	Worker(Channel::ChannelSocket* channel, SharedInterface* shared);

	~Worker() override;

private:
	void Close();

	flatbuffers::Offset<FBS::Worker::DumpResponse> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;

	flatbuffers::Offset<FBS::Worker::ResourceUsageResponse> FillBufferResourceUsage(
	  flatbuffers::FlatBufferBuilder& builder) const;

	RTC::WebRtcServer* AssertAndGetWebRtcServerById(
	  const std::string& webRtcServerId, const std::string& method) const;

	RTC::Router* AssertAndGetRouterById(const std::string& routerId, const std::string& method) const;

	void CheckNoWebRtcServer(const std::string& webRtcServerId, const std::string& method) const;

	void CheckNoRouter(const std::string& routerId, const std::string& method) const;

	/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
public:
	void HandleRequest(Channel::ChannelRequest* request) override;

	void HandleNotification(Channel::ChannelNotification* notification) override;

	/* Methods inherited from Channel::ChannelSocket::Listener. */
public:
	void OnChannelClosed(Channel::ChannelSocket* channel) override;

	/* Methods inherited from SignalHandle::Listener. */
public:
	void OnSignal(SignalHandle* signalHandle, int signum) override;

	/* Pure virtual methods inherited from RTC::Router::Listener. */
public:
	RTC::WebRtcServer* OnRouterNeedWebRtcServer(
	  const RTC::Router* router, const std::string& webRtcServerId) override;

private:
	// Passed by argument.
	Channel::ChannelSocket* const channel;
	SharedInterface* const shared;
	// Allocated by this.
	std::unique_ptr<SignalHandle> signalHandle;
	ankerl::unordered_dense::map<std::string, std::unique_ptr<RTC::WebRtcServer>> mapWebRtcServers;
	ankerl::unordered_dense::map<std::string, std::unique_ptr<RTC::Router>> mapRouters;
	// Others.
	bool closed{ false };
};

#endif
