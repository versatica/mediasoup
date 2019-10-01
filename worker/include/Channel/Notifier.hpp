#ifndef MS_CHANNEL_NOTIFIER_HPP
#define MS_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include <json.hpp>
#include <string>

namespace Channel
{
	class Notifier
	{
	public:
		static void ClassInit(Channel::UnixStreamSocket* channel);
		static void Emit(const std::string& targetId, const char* event);
		static void Emit(const std::string& targetId, const char* event, json& data);

	public:
		// Passed by argument.
		static Channel::UnixStreamSocket* channel;
	};
} // namespace Channel

#endif
