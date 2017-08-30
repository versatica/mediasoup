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
			WORKER_CREATE_ROUTER,
			ROUTER_CLOSE,
			ROUTER_DUMP,
			ROUTER_CREATE_TRANSPORT,
			ROUTER_CREATE_PRODUCER,
			ROUTER_CREATE_CONSUMER,
			ROUTER_SET_AUDIO_LEVELS_EVENT,
			TRANSPORT_CLOSE,
			TRANSPORT_DUMP,
			TRANSPORT_SET_REMOTE_DTLS_PARAMETERS,
			TRANSPORT_SET_MAX_BITRATE,
			TRANSPORT_CHANGE_UFRAG_PWD,
			PRODUCER_CLOSE,
			PRODUCER_DUMP,
			PRODUCER_RECEIVE,
			PRODUCER_PAUSE,
			PRODUCER_RESUME,
			PRODUCER_SET_RTP_RAW_EVENT,
			PRODUCER_SET_RTP_OBJECT_EVENT,
			CONSUMER_CLOSE,
			CONSUMER_DUMP,
			CONSUMER_SEND,
			CONSUMER_PAUSE,
			CONSUMER_RESUME
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
