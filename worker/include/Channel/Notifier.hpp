#ifndef MS_CHANNEL_NOTIFIER_HPP
#define MS_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include <string>
#include <json/json.h>

namespace Channel
{
	class Notifier
	{
	public:
		explicit Notifier(Channel::UnixStreamSocket* channel);

		void Emit(uint32_t targetId, std::string event);
		void Emit(uint32_t targetId, std::string event, Json::Value& data);
		void EmitWithBinary(uint32_t targetId, std::string event, Json::Value& data, const uint8_t* binary_data, size_t binary_len);

	public:
		// Passed by argument.
		Channel::UnixStreamSocket* channel = nullptr;
	};
}

#endif
