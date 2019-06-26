#ifndef MS_RTC_SCTP_ASSOCIATION_HPP
#define MS_RTC_SCTP_ASSOCIATION_HPP

#include "common.hpp"
#include "json.hpp"
// #include <usrsctp.h>

using json = nlohmann::json;

namespace RTC
{
	class SctpAssociation
	{
	public:
		class Listener
		{
		public:
			virtual void OnSctpAssociationSendData(
			  RTC::SctpAssociation* sctpAssociation, const uint8_t* data, size_t len) = 0;
			virtual void OnSctpAssociationMessageReceived(
			  RTC::SctpAssociation* sctpAssociation, uint16_t streamId, const uint8_t* msg, size_t len) = 0;
		};

	public:
		SctpAssociation(Listener* listener, uint32_t sctpMaxMessageSize);
		~SctpAssociation();

	public:
		void FillJson(json& jsonObject) const;
		void ProcessSctpData(const uint8_t* data, size_t len);
		void SendSctpMessage(const uint8_t* msg, size_t len);

		/* Callbacks fired by usrsctp events. */
	public:
		void OnUsrSctpSendSctpData(void* buffer, size_t len);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		uint32_t sctpMaxMessageSize{ 0 };
	};
} // namespace RTC

#endif
