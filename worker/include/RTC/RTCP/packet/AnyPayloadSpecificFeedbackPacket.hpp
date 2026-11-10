#ifndef MS_RTC_RTCP_ANY_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP
#define MS_RTC_RTCP_ANY_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/AnyFeedbackPacket.hpp"
#include <ankerl/unordered_dense.h>
#include <string>

namespace RTC
{
	namespace RTCP
	{
		/**
		 * Base class for Payload-Specific Feedback packets (PSFB, 206).
		 *
		 * @see RFC 4585.
		 *
		 * The FMT field tells the type of the feedback message.
		 */
		class AnyPayloadSpecificFeedbackPacket : public AnyFeedbackPacket
		{
		public:
			/**
			 * Payload-Specific Feedback message type (FMT field).
			 */
			enum class MessageType : uint8_t
			{
				PLI = 1,
				FIR = 4,
				AFB = 15
			};

		public:
			static const std::string& MessageTypeToString(MessageType messageType);

		private:
			static const ankerl::unordered_dense::map<MessageType, std::string> MessageType2String;

		protected:
			AnyPayloadSpecificFeedbackPacket(uint8_t* buffer, size_t bufferLength);

		public:
			virtual MessageType GetMessageType() const final
			{
				return static_cast<MessageType>(GetCount());
			}

			/**
			 * False by default. UnknownPayloadSpecificFeedbackPacket class overrides
			 * this method to return true instead.
			 */
			virtual bool HasUnknownMessageType() const
			{
				return false;
			}
		};
	} // namespace RTCP
} // namespace RTC

#endif
