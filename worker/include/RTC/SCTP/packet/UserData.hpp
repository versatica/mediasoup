#ifndef MS_RTC_SCTP_USER_DATA_HPP
#define MS_RTC_SCTP_USER_DATA_HPP

#include "common.hpp"
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Represents user data extracted from a DATA or I_DATA Chunk.
		 */
		class UserData
		{
		public:
			UserData(
			  uint16_t streamId,
			  uint16_t ssn,
			  uint32_t mid,
			  uint32_t fsn,
			  uint32_t ppid,
			  std::vector<uint8_t> payload,
			  bool isBeginning,
			  bool isEnd,
			  bool isUnordered);

			// Move constructor. No need to do anything special since std::vector
			// already implements move.
			UserData(UserData&& other) = default;

			// Move assignment. No need to do anything special since std::vector
			// already implements move.
			UserData& operator=(UserData&& other) = default;

			// Disable copy constructor.
			UserData(const UserData&) = delete;

			// Disable copy assignment.
			UserData& operator=(const UserData&) = delete;

			~UserData();

		public:
			void Dump(int indentation = 0) const;

			/**
			 * Stream Identifier (in DATA and I_DATA chunks).
			 */
			uint16_t GetStreamId() const
			{
				return this->streamId;
			}

			/**
			 * Stream Sequence Number (only in DATA chunks).
			 */
			uint16_t GetStreamSequenceNumber() const
			{
				return this->ssn;
			}

			/**
			 * Message Identifier (MID) (only in I_DATA chunks).
			 */
			uint32_t GetMessageId() const
			{
				return this->mid;
			}

			/**
			 * Fragment Sequence Number (FSN) (only in I_DATA chunks).
			 */
			uint32_t GetFragmentSequenceNumber() const
			{
				return this->fsn;
			}

			uint32_t GetPayloadProtocolId() const
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
			 * Useful to extract the payload and its ownership when destructing the
			 * Message.
			 *
			 * @remarks
			 * - && at the end means that it can only be called from a rvalue.
			 *
			 * @usage
			 * ```c++
			 * const auto payload = std::move(userData).ReleasePayload();
			 * ```
			 */
			std::vector<uint8_t> ReleasePayload() &&
			{
				return std::move(this->payload);
			}

			bool IsBeginning() const
			{
				return this->isBeginning;
			}

			bool IsEnd() const
			{
				return this->isEnd;
			}

			bool IsUnordered() const
			{
				return this->isUnordered;
			}

		private:
			uint16_t streamId{ 0 };
			uint16_t ssn{ 0 };
			uint32_t mid{ 0 };
			uint32_t fsn{ 0 };
			uint32_t ppid{ 0 };
			std::vector<uint8_t> payload;
			bool isBeginning{ false };
			bool isEnd{ false };
			bool isUnordered{ false };
		};
	} // namespace SCTP
} // namespace RTC

#endif
