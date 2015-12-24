#ifndef MS_CHANNEL_REQUEST_H
#define MS_CHANNEL_REQUEST_H

#include "Channel/UnixStreamSocket.h"
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace Channel
{
	// Avoid cyclic #include problem.
	class UnixStreamSocket;

	class Request
	{
	public:
		enum class MethodId
		{
			dumpWorker = 1,
			updateSettings,
			createRoom,
			closeRoom,
			dumpRoom,
			createPeer,
			closePeer,
			dumpPeer
		};

	private:
		static std::unordered_map<std::string, MethodId> string2MethodId;

	public:
		Request(Channel::UnixStreamSocket* channel, Json::Value& json);
		virtual ~Request();

		void Accept();
		void Accept(Json::Value &data);
		void Reject(unsigned int status, std::string& reason);
		void Reject(unsigned int status, const char* reason = nullptr);

	public:
		// Passed by argument.
		Channel::UnixStreamSocket* channel = nullptr;
		unsigned int id;
		std::string method;
		MethodId methodId;
		Json::Value data;
		// Others.
		bool replied = false;
	};
}

#endif
