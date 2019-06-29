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
		// TODO: Let's see which states are needed.
		enum class SctpState
		{
			NEW = 1,
			CONNECTING,
			CONNECTED,
			FAILED,
			CLOSED
		};

	public:
		class Listener
		{
		public:
			virtual void OnSctpAssociationConnected(RTC::SctpAssociation* sctpAssociation) = 0;
			virtual void OnSctpAssociationClosed(RTC::SctpAssociation* sctpAssociation)    = 0;
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
		bool Run();
		SctpState GetState() const;
		void ProcessSctpData(const uint8_t* data, size_t len);
		void SendSctpMessage(RTC::DataConsumer* dataConsumer, const uint8_t* msg, size_t len);

		/* Callbacks fired by usrsctp events. */
	public:
		void OnUsrSctpSendSctpData(void* buffer, size_t len);
		void OnUsrSctpReceiveSctpData(uint16_t streamId, const uint8_t* msg, size_t len);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		uint16_t numSctpStreams{ 65535 };
		uint32_t maxSctpMessageSize{ 262144 };
		// Others.
		SctpState state{ SctpState::NEW };
		struct socket* socket{ nullptr };
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

	/* Inline instance methods. */
	inline SctpAssociation::SctpState SctpAssociation::GetState() const
	{
		return this->state;
	}
} // namespace RTC

#endif
