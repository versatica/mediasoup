#define MS_CLASS "RTC::SctpAssociation"
// #define MS_LOG_DEV

#include "RTC/SctpAssociation.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
// #include "MediaSoupErrors.hpp"
// #include "Utils.hpp"
// #include "Channel/Notifier.hpp"

namespace RTC
{
	/* Instance methods. */

	SctpAssociation::SctpAssociation(Listener* listener, uint32_t sctpMaxMessageSize)
	  : listener(listener), sctpMaxMessageSize(sctpMaxMessageSize)
	{
		MS_TRACE();
	}

	SctpAssociation::~SctpAssociation()
	{
		MS_TRACE();
	}

	void SctpAssociation::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add port (always 5000).
		jsonObject["port"] = 5000;

		// Add maxMessageSize.
		jsonObject["maxMessageSize"] = this->sctpMaxMessageSize;
	}

	void SctpAssociation::ProcessSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}
} // namespace RTC
