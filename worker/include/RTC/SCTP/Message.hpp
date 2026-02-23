#ifndef MS_RTC_SCTP_MESSAGE_HPP
#define MS_RTC_SCTP_MESSAGE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Packet.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * An SCTP message is a group of bytes sent or received as a whole on a
		 * specified stream identifier (`streamId`) and with a payload protocol
		 * identifier (`ppid`).
		 */
		class Message
		{
		public:
			Message(uint16_t streamId, uint32_t ppid, std::vector<uint8_t> payload);

			// Move constructor. No need to do anything special since std::vector
			// already implements move.
			Message(Message&& other) = default;

			// Move assignment. No need to do anything special since std::vector
			// already implements move.
			Message& operator=(Message&& other) = default;

			// Disable copy constructor.
			Message(const Message&) = delete;

			// Disable copy assignment.
			Message& operator=(const Message&) = delete;

			~Message();

		public:
			void Dump(int indentation = 0) const;

			uint16_t GetStreamId() const
			{
				return this->streamId;
			}

			uint32_t GetPayloadProtocolIdentifier() const
			{
				return this->ppid;
			}

			const uint8_t* GetPayload() const
			{
				return this->payload.data();
			}

			size_t GetPayloadLength() const
			{
				return this->payload.size();
			}

			/**
			 * Useful to extract the payload and its ownership When destructing the
			 * Message.
			 */
			std::vector<uint8_t> ReleasePayload() &&
			{
				return std::move(this->payload);
			}

		private:
			uint16_t streamId{ 0 };
			uint32_t ppid{ 0 };
			std::vector<uint8_t> payload;
		};
	} // namespace SCTP
} // namespace RTC

#endif
