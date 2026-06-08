#ifndef MS_RTC_SCTP_USER_DATA_HPP
#define MS_RTC_SCTP_USER_DATA_HPP

#include "common.hpp"
#include <ostream>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Represents user data extracted from a DATA or I-DATA Chunk.
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

			/**
			 * Move constructor. No need to do anything special since std::vector
			 * already implements move.
			 */
			UserData(UserData&& other) = default;

			/**
			 * Move assignment. No need to do anything special since std::vector
			 * already implements move.
			 */
			UserData& operator=(UserData&& other) = default;

			/**
			 * Copy constructor disabled.
			 */
			UserData(const UserData&) = delete;

			/**
			 * Copy assignment disabled.
			 */
			UserData& operator=(const UserData&) = delete;

			bool operator==(const UserData& other) const
			{
				return (
				  this->streamId == other.streamId && this->ssn == other.ssn && this->mid == other.mid &&
				  this->fsn == other.fsn && this->ppid == other.ppid && this->payload == other.payload &&
				  this->isBeginning == other.isBeginning && this->isEnd == other.isEnd &&
				  this->isUnordered == other.isUnordered);
			}

			~UserData();

		public:
			void Dump(int indentation = 0) const;

			/**
			 * Stream Identifier (in DATA and I-DATA chunks).
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
			 * Message Identifier (MID) (only in I-DATA chunks).
			 */
			uint32_t GetMessageId() const
			{
				return this->mid;
			}

			/**
			 * Fragment Sequence Number (FSN) (only in I-DATA chunks).
			 */
			uint32_t GetFragmentSequenceNumber() const
			{
				return this->fsn;
			}

			uint32_t GetPayloadProtocolId() const
			{
				return this->ppid;
			}

			std::vector<uint8_t>& GetPayload()
			{
				return this->payload;
			}

			size_t GetPayloadLength() const
			{
				return this->payload.size();
			}

			/**
			 * Useful to extract the payload and its ownership when destructing the
			 * UserData.
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
				// NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
				return std::move(this->payload);
			}

			UserData Clone() const
			{
				return UserData(
				  this->streamId,
				  this->ssn,
				  this->mid,
				  this->fsn,
				  this->ppid,
				  this->payload,
				  this->isBeginning,
				  this->isEnd,
				  this->isUnordered);
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
			uint16_t streamId;
			uint16_t ssn;
			uint32_t mid;
			uint32_t fsn;
			uint32_t ppid;
			std::vector<uint8_t> payload;
			bool isBeginning;
			bool isEnd;
			bool isUnordered;
		};

		/**
		 * For Catch2 to print it nicely.
		 */
		inline std::ostream& operator<<(std::ostream& os, const UserData& d)
		{
			return os << "{streamId:" << d.GetStreamId() << ", ssn:" << d.GetStreamSequenceNumber()
			          << ", mid:" << d.GetMessageId() << ", fsn:" << d.GetFragmentSequenceNumber()
			          << ", ppid:" << d.GetPayloadProtocolId() << ", payloadLen:" << d.GetPayloadLength()
			          << ", B:" << d.IsBeginning() << ", E:" << d.IsEnd() << ", U:" << d.IsUnordered()
			          << "}";
		}
	} // namespace SCTP
} // namespace RTC

#endif
