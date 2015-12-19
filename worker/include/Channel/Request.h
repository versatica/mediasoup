#ifndef MS_CHANNEL_REQUEST_H
#define MS_CHANNEL_REQUEST_H

#include "Channel/UnixStreamSocket.h"
#include <string>
#include <json/json.h>

namespace Channel
{
	class Request
	{
	public:
		static Channel::Request* Factory(Channel::UnixStreamSocket* channel, Json::Value& msg);

	public:
		Request(Channel::UnixStreamSocket* channel, std::string& id, std::string& method, Json::Value& data);
		virtual ~Request();

		void Reply(int status);

	public:
		// Passed by argument:
		Channel::UnixStreamSocket* channel = nullptr;
		std::string id;
		std::string method;
		Json::Value data;
		// Others:
		bool replied = false;
	};
}

#endif
