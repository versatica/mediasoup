#ifndef MS_RTC_NEW_RTCP_ANY_REPORT_PACKET_HPP
#define MS_RTC_NEW_RTCP_ANY_REPORT_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "Utils.hpp"
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Base class for ReceiverReportPacket and SenderReportPacket.
		 *
		 * @see RFC 3550.
		 *
		 * Both packets start with the Common Header followed by the SSRC of the
		 * packet sender, and both end with RC report blocks optionally followed
		 * by profile-specific extensions. The only difference is the length of
		 * the fixed header that precedes the report blocks (the Sender Report
		 * includes the sender info).
		 *
		 * Each report block:
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                 SSRC_n (SSRC of n-th source)                  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * | fraction lost |       cumulative number of packets lost       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           extended highest sequence number received           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      interarrival jitter                      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         last SR (LSR)                         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                   delay since last SR (DLSR)                  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * @remarks
		 * - Profile-specific extensions are accepted when parsing but they are
		 *   not exposed.
		 */
		class AnyReportPacket : public Packet
		{
		public:
			/**
			 * Length (in bytes) of a report block.
			 */
			static constexpr size_t ReportBlockLength{ 24 };
			/**
			 * Maximum number of report blocks in a packet (the RC field has 5 bits).
			 */
			static constexpr size_t MaxReportBlocks{ 31 };

		public:
			/**
			 * Report block.
			 */
			struct ReportBlock
			{
				/**
				 * SSRC of the source this report block refers to.
				 */
				uint32_t ssrc;
				/**
				 * Fraction of packets lost since the previous report, as a fixed
				 * point number with the binary point at the left edge.
				 */
				uint8_t fractionLost;
				/**
				 * Cumulative number of packets lost (24 bits signed in the packet).
				 */
				int32_t totalLost;
				/**
				 * Extended highest sequence number received.
				 */
				uint32_t extendedHighestSeq;
				/**
				 * Interarrival jitter.
				 */
				uint32_t jitter;
				/**
				 * Last SR timestamp (LSR).
				 */
				uint32_t lastSenderReport;
				/**
				 * Delay since last SR (DLSR).
				 */
				uint32_t delaySinceLastSenderReport;
			};

		protected:
			AnyReportPacket(uint8_t* buffer, size_t bufferLength);

		public:
			/**
			 * SSRC of the packet sender.
			 */
			uint32_t GetSsrc() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetSsrc(uint32_t ssrc);

			/**
			 * Value of the RC field.
			 */
			uint8_t GetNumberOfReportBlocks() const
			{
				return GetCount();
			}

			std::vector<ReportBlock> GetReportBlocks() const;

			/**
			 * Add a report block at the end of the packet.
			 *
			 * @remarks
			 * - `totalLost` is clamped to the range of a 24 bits signed integer as
			 *   stated in the appendix A.3 of RFC 3550.
			 *
			 * @throw MediaSoupTypeError - If the packet already has
			 *   `MaxReportBlocks` report blocks, if it has profile-specific
			 *   extensions or if there is no space in the buffer.
			 */
			void AddReportBlock(const ReportBlock& reportBlock);

		protected:
			/**
			 * Length (in bytes) of the fixed header that precedes the report blocks,
			 * Common Header included.
			 */
			virtual size_t GetHeaderLength() const = 0;

			/**
			 * Whether the fixed header and the report blocks announced by the RC
			 * field fit into the packet.
			 *
			 * @remarks
			 * - To be used by `ParseStrict()` in subclasses once the length of the
			 *   packet has been set.
			 */
			bool IsLengthValid() const;

			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			void DumpReportBlocks(int indentation) const;

		private:
			uint8_t* GetReportBlocksPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + GetHeaderLength();
			}

			/**
			 * Where the profile-specific extensions start (if any), which is right
			 * after the report blocks announced by the RC field.
			 */
			uint8_t* GetProfileSpecificExtensionsPointer() const
			{
				return GetReportBlocksPointer() +
				       (GetNumberOfReportBlocks() * AnyReportPacket::ReportBlockLength);
			}

			/**
			 * Whether there are profile-specific extensions after the report blocks.
			 */
			bool HasProfileSpecificExtensions() const;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
