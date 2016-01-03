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
			worker_dump = 1,
			worker_updateSettings,
			worker_createRoom,
			room_close,
			room_dump,
			room_createPeer,
			peer_close,
			peer_dump,
			peer_createTransport,
			peer_createAssociatedTransport,
			transport_close,
			transport_dump,
			transport_start
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
