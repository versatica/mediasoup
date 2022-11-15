#ifndef MS_RTC_DATA_PRODUCER_HPP
#define MS_RTC_DATA_PRODUCER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/SctpDictionaries.hpp"
#include "RTC/Shared.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace RTC
{
	class DataProducer : public Channel::ChannelSocket::RequestHandler,
	                     public PayloadChannel::PayloadChannelSocket::NotificationHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnDataProducerReceiveData(RTC::DataProducer* producer, size_t len) = 0;
			virtual void OnDataProducerMessageReceived(
			  RTC::DataProducer* dataProducer, uint32_t ppid, const uint8_t* msg, size_t len) = 0;
		};

	public:
		enum class Type : uint8_t
		{
			SCTP = 0,
			DIRECT
		};

	public:
		DataProducer(
		  RTC::Shared* shared,
		  const std::string& id,
		  size_t maxMessageSize,
		  RTC::DataProducer::Listener* listener,
		  json& data);
		virtual ~DataProducer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonArray) const;
		Type GetType() const
		{
			return this->type;
		}
		const RTC::SctpStreamParameters& GetSctpStreamParameters() const
		{
			return this->sctpStreamParameters;
		}
		void ReceiveMessage(uint32_t ppid, const uint8_t* msg, size_t len);

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

		/* Methods inherited from PayloadChannel::PayloadChannelSocket::NotificationHandler. */
	public:
		void HandleNotification(PayloadChannel::PayloadChannelNotification* notification) override;

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		RTC::Shared* shared{ nullptr };
		size_t maxMessageSize{ 0u };
		RTC::DataProducer::Listener* listener{ nullptr };
		// Others.
		Type type;
		std::string typeString;
		RTC::SctpStreamParameters sctpStreamParameters;
		std::string label;
		std::string protocol;
		size_t messagesReceived{ 0u };
		size_t bytesReceived{ 0u };
	};
} // namespace RTC

#endif
