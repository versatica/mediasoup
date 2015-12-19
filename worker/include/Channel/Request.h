#ifndef MS_CHANNEL_REQUEST_H
#define MS_CHANNEL_REQUEST_H

#include <string>
#include <json/json.h>

namespace Channel
{
	class Request
	{
	public:
		static Channel::Request* Factory(Json::Value& msg);

	public:
		Request(std::string& id, std::string& method, Json::Value& data);
		virtual ~Request();

		void Reply(int status);

	public:
		// Passed by argument:
		std::string id;
		std::string method;
		Json::Value data;
	};
}

#endif
