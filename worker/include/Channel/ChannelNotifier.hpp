#ifndef MS_CHANNEL_NOTIFIER_HPP
#define MS_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "Channel/ChannelSocket.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace Channel
{
	class ChannelNotifier
	{
	public:
		static void ClassInit(Channel::ChannelSocket* channel);
		static void Emit(uint64_t targetId, const char* event);
		static void Emit(const std::string& targetId, const char* event);
		static void Emit(const std::string& targetId, const char* event, json& data);

	public:
		// Passed by argument.
		thread_local static Channel::ChannelSocket* channel;
	};
} // namespace Channel

#endif
