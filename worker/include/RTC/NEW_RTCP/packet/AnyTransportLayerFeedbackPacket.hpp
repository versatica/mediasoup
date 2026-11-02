#ifndef MS_RTC_NEW_RTCP_ANY_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP
#define MS_RTC_NEW_RTCP_ANY_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/AnyFeedbackPacket.hpp"
#include <ankerl/unordered_dense.h>
#include <string>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Base class for Transport Layer Feedback packets (RTPFB, 205).
		 *
		 * @see RFC 4585.
		 *
		 * The FMT field tells the type of the feedback message.
		 */
		class AnyTransportLayerFeedbackPacket : public AnyFeedbackPacket
		{
		public:
			/**
			 * Transport Layer Feedback message type (FMT field).
			 */
			enum class MessageType : uint8_t
			{
				NACK   = 1,
				TMMBR  = 3,
				TMMBN  = 4,
				SR_REQ = 5,
				RAMS   = 6,
				TLLEI  = 7,
				ECN    = 8,
				PS     = 9,
				TCC    = 15,
				EXT    = 31
			};

		public:
			static const std::string& MessageTypeToString(MessageType messageType);

		private:
			static const ankerl::unordered_dense::map<MessageType, std::string> MessageType2String;

		protected:
			AnyTransportLayerFeedbackPacket(uint8_t* buffer, size_t bufferLength);

		public:
			virtual MessageType GetMessageType() const final
			{
				return static_cast<MessageType>(GetCount());
			}

			/**
			 * False by default. UnknownTransportLayerFeedbackPacket class overrides
			 * this method to return true instead.
			 */
			virtual bool HasUnknownMessageType() const
			{
				return false;
			}
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
