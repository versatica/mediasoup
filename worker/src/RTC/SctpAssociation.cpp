#define MS_CLASS "RTC::SctpAssociation"
// #define MS_LOG_DEV

#include "RTC/SctpAssociation.hpp"
#include "DepUsrSCTP.hpp"
#include "Logger.hpp"
// #include "MediaSoupErrors.hpp"
// #include "Utils.hpp"
// #include "Channel/Notifier.hpp"

namespace RTC
{
	/* Instance methods. */

	SctpAssociation::SctpAssociation(Listener* listener, uint16_t numSctpStreams, uint32_t maxSctpMessageSize)
	  : listener(listener), numSctpStreams(numSctpStreams), maxSctpMessageSize(maxSctpMessageSize)
	{
		MS_TRACE();

		DepUsrSCTP::IncreaseSctpAssociations();
	}

	SctpAssociation::~SctpAssociation()
	{
		MS_TRACE();

		DepUsrSCTP::DecreaseSctpAssociations();
	}

	void SctpAssociation::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add port (always 5000).
		jsonObject["port"] = 5000;

		// Add numStreams.
		jsonObject["numStreams"] = this->numSctpStreams;

		// Add maxMessageSize.
		jsonObject["maxMessageSize"] = this->maxSctpMessageSize;
	}

	void SctpAssociation::ProcessSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// TODO: Consume it using usrsctp_conninput() and so on.
		// TODO: Check that message len is not higher than this->maxSctpMessageSize.
	}

	void SctpAssociation::SendSctpMessage(RTC::DataConsumer* dataConsumer, const uint8_t* msg, size_t len)
	{
		MS_TRACE();

		// TODO: Check that len is not higher than this->maxSctpMessageSize.

		// TODO: send it using usrsctp_sendv() by reading dataConsumer->GetSctpStreamParameters()
		// and so on.
	}

	void SctpAssociation::OnUsrSctpSendSctpData(void* buffer, size_t len)
	{
		MS_TRACE();

		const uint8_t* data = static_cast<uint8_t*>(buffer);

		// TODO: accounting, rate, etc?

		this->listener->OnSctpAssociationSendData(this, data, len);
	}
} // namespace RTC
