#ifndef MS_RTC_SCTP_ASSOCIATION_HPP
#define MS_RTC_SCTP_ASSOCIATION_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "json.hpp"
#include "RTC/DataConsumer.hpp"
#include <usrsctp.h>

using json = nlohmann::json;

namespace RTC
{
	class SctpAssociation
	{
	public:
		enum class SctpState
		{
			NEW = 1,
			CONNECTING,
			CONNECTED,
			FAILED,
			CLOSED
		};

		class Listener
		{
		public:
			virtual void OnSctpAssociationSendData(
			  RTC::SctpAssociation* sctpAssociation, const uint8_t* data, size_t len) = 0;
			virtual void OnSctpAssociationMessageReceived(
			  RTC::SctpAssociation* sctpAssociation, uint16_t streamId, const uint8_t* msg, size_t len) = 0;
		};

	public:
		static bool IsSctp(const uint8_t* data, size_t len);

	public:
		SctpAssociation(Listener* listener, uint16_t numSctpStreams, uint32_t maxSctpMessageSize);
		~SctpAssociation();

	public:
		void FillJson(json& jsonObject) const;
		void ProcessSctpData(const uint8_t* data, size_t len);
		bool Run();
		SctpState GetState() const;
		void SendSctpMessage(RTC::DataConsumer* dataConsumer, const uint8_t* msg, size_t len);

		/* Callbacks fired by usrsctp events. */
	public:
		void OnUsrSctpSendSctpData(void* buffer, size_t len);
		void OnUsrSctpReceiveSctpData(uint16_t streamId, const uint8_t* msg, size_t len);
		void OnUsrSctpReceiveSctpNotification(union sctp_notification* notification, size_t len);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by usrsctp.
		struct socket* socket{ nullptr };
		// Others.
		uint16_t numSctpStreams{ 65535 };
		uint32_t maxSctpMessageSize{ 262144 };
		SctpState state{ SctpState::NEW };
	};

	/* Inline static methods. */

	inline bool SctpAssociation::IsSctp(const uint8_t* data, size_t len)
	{
		// clang-format off
		return (
			(len >= 12) &&
			// Must have Source Port Number and Destination Port Number set to 5000 (hack).
			(Utils::Byte::Get2Bytes(data, 0) == 5000) &&
			(Utils::Byte::Get2Bytes(data, 2) == 5000)
		);
		// clang-format on
	}
} // namespace RTC

#endif
