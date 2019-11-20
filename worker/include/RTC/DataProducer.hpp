#ifndef MS_RTC_DATA_PRODUCER_HPP
#define MS_RTC_DATA_PRODUCER_HPP

#include "common.hpp"
#include "Channel/Request.hpp"
#include "RTC/SctpDictionaries.hpp"
#include <json.hpp>
#include <string>

namespace RTC
{
	class DataProducer
	{
	public:
		class Listener
		{
		public:
			virtual void OnDataProducerSctpMessageReceived(
			  RTC::DataProducer* dataProducer, uint32_t ppid, const uint8_t* msg, size_t len) = 0;
		};

	public:
		DataProducer(const std::string& id, RTC::DataProducer::Listener* listener, json& data);
		virtual ~DataProducer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonArray) const;
		void HandleRequest(Channel::Request* request);
		const RTC::SctpStreamParameters& GetSctpStreamParameters() const;
		void ReceiveSctpMessage(uint32_t ppid, const uint8_t* msg, size_t len);

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		RTC::DataProducer::Listener* listener{ nullptr };
		// Others.
		RTC::SctpStreamParameters sctpStreamParameters;
		std::string label;
		std::string protocol;
		size_t messagesReceived{ 0 };
		size_t bytesReceived{ 0 };
	};

	/* Inline methods. */

	inline const RTC::SctpStreamParameters& DataProducer::GetSctpStreamParameters() const
	{
		return this->sctpStreamParameters;
	}
} // namespace RTC

#endif
