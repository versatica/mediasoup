#ifndef MS_CHANNEL_NOTIFIER_HPP
#define MS_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include <json/json.h>
#include <string>

namespace Channel
{
	class Notifier
	{
	public:
		explicit Notifier(Channel::UnixStreamSocket* channel);

		void Emit(uint32_t targetId, const std::string& event);
		void Emit(uint32_t targetId, const std::string& event, Json::Value& data);
		void EmitWithBinary(
		    uint32_t targetId,
		    const std::string& event,
		    Json::Value& data,
		    const uint8_t* binaryData,
		    size_t binaryLen);

	public:
		// Passed by argument.
		Channel::UnixStreamSocket* channel{ nullptr };
	};
} // namespace Channel

#endif
