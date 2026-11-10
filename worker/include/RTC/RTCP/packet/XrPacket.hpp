#ifndef MS_RTC_RTCP_XR_PACKET_HPP
#define MS_RTC_RTCP_XR_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/ExtendedReport.hpp"
#include "RTC/RTCP/packet/Packet.hpp"
#include "Utils.hpp"
#include <typeinfo> // typeid
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		/**
		 * XR RTCP Packet (207).
		 *
		 * @see RFC 3611.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|reserved |   PT=XR=207   |             length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              SSRC                             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :                         report blocks                         :
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Reserved (5 bits): Must be set to zero and must be ignored by the
		 *   receiver.
		 * - Packet type (8 bits): 207.
		 * - SSRC (32 bits): The synchronization source identifier for the
		 *   originator of this XR packet.
		 * - Report blocks (variable length): Zero or more Extended Report blocks.
		 *
		 * @remarks
		 * - Extended Report blocks with an unknown or not implemented block type
		 *   are kept as UnknownExtendedReport and parsing continues with the next
		 *   block.
		 */

		class XrPacket : public Packet
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Length (in bytes) of the XR header (Common Header included).
			 */
			static constexpr size_t XrHeaderLength{ 8 };

		public:
			using ExtendedReportsIterator = std::vector<ExtendedReport*>::const_iterator;

		public:
			/**
			 * Parse a XrPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static XrPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a XrPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than the
			 *   XR header length (8 bytes).
			 */
			static XrPacket* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Length (in bytes) of a XrPacket carrying the given Extended Reports.
			 *
			 * @param extendedReportsLength - Sum of what `ComputeLengthFor()` of each
			 *   Extended Report the packet will carry returns.
			 */
			static size_t ComputeLengthFor(size_t extendedReportsLength);

		private:
			/**
			 * Parse a XrPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static XrPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			XrPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~XrPacket() override;

			void Dump(int indentation = 0) const final;

			void Serialize(uint8_t* buffer, size_t bufferLength) final;

			XrPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			/**
			 * SSRC of the originator of this XR packet.
			 */
			uint32_t GetSsrc() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetSsrc(uint32_t ssrc);

			bool HasExtendedReports() const
			{
				return this->extendedReports.size() > 0;
			}

			size_t GetExtendedReportsCount() const
			{
				return this->extendedReports.size();
			}

			ExtendedReportsIterator ExtendedReportsBegin() const
			{
				return this->extendedReports.begin();
			}

			ExtendedReportsIterator ExtendedReportsEnd() const
			{
				return this->extendedReports.end();
			}

			const ExtendedReport* GetExtendedReportAt(size_t idx) const
			{
				if (idx >= this->extendedReports.size())
				{
					return nullptr;
				}

				return this->extendedReports[idx];
			}

			template<typename T>
			const T* GetFirstExtendedReportOfType() const
			{
				for (const auto* extendedReport : this->extendedReports)
				{
					if (typeid(*extendedReport) == typeid(T))
					{
						return static_cast<const T*>(extendedReport);
					}
				}

				return nullptr;
			}

			/**
			 * Clone given Extended Report block into XR packet's buffer.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 * - Once this method is called, the caller may want to free the original
			 *   given Extended Report block (otherwise it will leak since the XR
			 *   packet manages a clone of it).
			 *
			 * @throw
			 * - MediaSoupError - If `BuildExtendedReportInPlace()` was called before
			 *   and the caller didn't invoke `Consolidate()` or `Discard()` on the
			 *   returned Extended Report block yet.
			 * - MediaSoupTypeError - If there is no space in the buffer for given
			 *   Extended Report block.
			 */
			void AddExtendedReport(const ExtendedReport* extendedReport);

			/**
			 * Build an Extended Report block within the XR packet's buffer, which is
			 * appended to the list of Extended Report blocks once `Consolidate()` is
			 * invoked on it. The caller can perform modifications in that Extended
			 * Report block and those will affect the XR packet body where the
			 * Extended Report block is serialized. The desired Extended Report block
			 * class type is given via template argument.
			 *
			 * @returns Pointer of the created Extended Report block specific class.
			 *
			 * @throw
			 * - MediaSoupError - If `BuildExtendedReportInPlace()` was called before
			 *   and the caller didn't invoke `Consolidate()` or `Discard()` on the
			 *   returned Extended Report block yet.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 * - The caller MUST invoke `Consolidate()` once the Extended Report block
			 *   is completed, or `Discard()` to remove it from the XR packet (which
			 *   deletes it). Meanwhile, `GetLength()` of the XR packet does not
			 *   include the length of the Extended Report block.
			 * - The caller MUST NOT free the obtained Extended Report block pointer
			 *   since it's now part of the XR packet.
			 * - The caller MUST free the obtained Extended Report block only in case
			 *   the `Consolidate()` method on the Extended Report block throws.
			 * - The caller MUST NOT invoke `Consolidate()` or `Discard()` on the
			 *   obtained Extended Report block once the XR packet has been destroyed,
			 *   since the Extended Report block holds a listener that refers to the
			 *   XR packet.
			 * - Method implemented in header file due to C++ template usage.
			 *
			 * @example
			 * ```c++
			 * auto* fooExtendedReport =
			 *   xrPacket->BuildExtendedReportInPlace<FooExtendedReport>();
			 * ```
			 */
			template<typename T>
			T* BuildExtendedReportInPlace()
			{
				AssertDoesNotNeedConsolidation();

				// Remove padding (if any) so the new Extended Report block goes right
				// after the last one.
				// NOTE: This doesn't throw since the length doesn't grow.
				SetVariableLengthValueLength(GetVariableLengthValueLength());

				auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();
				// The remaining length in the buffer is the potential buffer length
				// of the Extended Report block.
				const size_t extendedReportMaxBufferLength = GetBufferLength() - (ptr - GetBuffer());

				auto* extendedReport = T::Factory(ptr, extendedReportMaxBufferLength);

				// NOTE: Do not fix/update the Extended Report block buffer length since
				// the caller probably wants to modify the Extended Report block.

				HandleInPlaceExtendedReport(extendedReport);

				return extendedReport;
			}

		private:
			void SoftSerialize(const uint8_t* buffer) final;

			XrPacket* SoftClone(const uint8_t* buffer) const final;

			void SoftCloneInto(Packet* packet) const final;

			void HandleInPlaceExtendedReport(ExtendedReport* extendedReport);

		private:
			// Extended Report blocks.
			std::vector<ExtendedReport*> extendedReports;
		};
	} // namespace RTCP
} // namespace RTC

#endif
