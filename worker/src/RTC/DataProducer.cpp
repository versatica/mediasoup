#define MS_CLASS "RTC::DataProducer"
// #define MS_LOG_DEV

#include "RTC/DataProducer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	/* Instance methods. */

	DataProducer::DataProducer(const std::string& id, RTC::DataProducer::Listener* listener, json& data)
	  : id(id), listener(listener)
	{
		MS_TRACE();

		auto jsonSctpStreamParametersIt = data.find("sctpStreamParameters");

		if (jsonSctpStreamParametersIt == data.end() || !jsonSctpStreamParametersIt->is_object())
			MS_THROW_TYPE_ERROR("missing sctpStreamParameters");

		// This may throw.
		this->sctpStreamParameters = RTC::SctpStreamParameters(*jsonSctpStreamParametersIt);
	}

	DataProducer::~DataProducer()
	{
		MS_TRACE();
	}

	void DataProducer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add sctpStreamParameters.
		this->sctpStreamParameters.FillJson(jsonObject["sctpStreamParameters"]);
	}

	void DataProducer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// TODO
	}

	void DataProducer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::DATA_PRODUCER_DUMP:
			{
				json data = json::object();

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::DATA_PRODUCER_GET_STATS:
			{
				json data = json::array();

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}
} // namespace RTC
