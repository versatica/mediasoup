#ifndef MS_CHANNEL_REQUEST_HPP
#define MS_CHANNEL_REQUEST_HPP

#include "common.hpp"
#include <json/json.h>
#include <string>
#include <unordered_map>

namespace Channel
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class UnixStreamSocket;

	class Request
	{
	public:
		enum class MethodId
		{
			WORKER_DUMP = 1,
			WORKER_UPDATE_SETTINGS,
			WORKER_CREATE_ROOM,
			ROOM_CLOSE,
			ROOM_DUMP,
			ROOM_CREATE_PEER,
			PEER_CLOSE,
			PEER_DUMP,
			PEER_SET_CAPABILITIES,
			PEER_CREATE_TRANSPORT,
			PEER_CREATE_RTP_RECEIVER,
			TRANSPORT_CLOSE,
			TRANSPORT_DUMP,
			TRANSPORT_SET_REMOTE_DTLS_PARAMETERS,
			TRANSPORT_SET_MAX_BITRATE,
			TRANSPORT_CHANGE_UFRAG_PWD,
			RTP_RECEIVER_CLOSE,
			RTP_RECEIVER_DUMP,
			RTP_RECEIVER_RECEIVE,
			RTP_RECEIVER_SET_TRANSPORT,
			RTP_RECEIVER_SET_RTP_RAW_EVENT,
			RTP_RECEIVER_SET_RTP_OBJECT_EVENT,
			RTP_SENDER_DUMP,
			RTP_SENDER_SET_TRANSPORT,
			RTP_SENDER_DISABLE
		};

	private:
		static std::unordered_map<std::string, MethodId> string2MethodId;

	public:
		Request(Channel::UnixStreamSocket* channel, Json::Value& json);
		virtual ~Request();

		void Accept();
		void Accept(Json::Value& data);
		void Reject(std::string& reason);
		void Reject(const char* reason = nullptr);

	public:
		// Passed by argument.
		Channel::UnixStreamSocket* channel{ nullptr };
		uint32_t id{ 0 };
		std::string method;
		MethodId methodId;
		Json::Value internal;
		Json::Value data;
		// Others.
		bool replied{ false };
	};
} // namespace Channel

#endif
