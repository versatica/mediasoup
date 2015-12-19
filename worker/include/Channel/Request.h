#ifndef MS_CHANNEL_REQUEST_H
#define MS_CHANNEL_REQUEST_H

#include "Channel/UnixStreamSocket.h"
#include <string>
#include <json/json.h>

namespace Channel
{
	// Avoid cyclic #include problem.
	class UnixStreamSocket;

	class Request
	{
	public:
		static Channel::Request* Factory(Channel::UnixStreamSocket* channel, Json::Value& json);

	public:
		Request(Channel::UnixStreamSocket* channel, unsigned int id, std::string& method, Json::Value& data);
		virtual ~Request();

		// void Accept(Json::Value data = Json::Value(Json::nullValue));
		void Accept();
		void Accept(Json::Value &data);
		void Reject(unsigned int status, std::string& reason);
		void Reject(unsigned int status, const char* reason = nullptr);

	public:
		// Passed by argument:
		Channel::UnixStreamSocket* channel = nullptr;
		unsigned int id;
		std::string method;
		Json::Value data;
		// Others:
		bool replied = false;
	};
}

#endif
