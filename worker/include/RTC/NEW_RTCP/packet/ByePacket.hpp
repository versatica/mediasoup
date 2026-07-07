#ifndef MS_RTC_NEW_RTCP_BYE_PACKET_HPP
#define MS_RTC_NEW_RTCP_BYE_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "Utils.hpp"
#include <string_view>
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * BYE RTCP Packet (203)
		 *
		 * @see RFC 3550.
		 *
		 *        0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|    SC   |   PT=BYE=203  |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           SSRC/CSRC                           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :                              ...                              :
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * | Reason length |               Reason for leaving            ...
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Packet type (8 bits): 203.
		 * - Source count (SC) (5 bits): The number of SSRC/CSRC identifiers
		 *   included in this BYE packet.
		 * - Reason length (8 bits): Number of bytes of text indicating the reason
		 *   for leaving.
		 * - Reason for leaving (variable length): If the string fills the packet
		 *   to the next 32-bit boundary, the string is not null terminated. If not,
		 *   the BYE packet MUST be padded with null octets to the next 32-bit
		 *   boundary.
		 */

		// Forward declaration.
		class Packet;

		class ByePacket : public Packet
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Parse a ByePacket.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the packet.
			 */
			static ByePacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ByePacket.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the packet real length.
			 */
			static ByePacket* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ByePacket.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static ByePacket* ParseStrict(const uint8_t* buffer, size_t bufferLength, size_t packetLength);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ByePacket(uint8_t* buffer, size_t bufferLength);

		public:
			~ByePacket() override;

			void Dump(int indentation = 0) const final;

			ByePacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			std::vector<uint32_t> GetSsrcs() const;

			void AddSsrc(uint32_t ssrc);

			bool HasReason() const
			{
				return (GetBuffer() + GetLength()) > GetReasonLengthPointer();
			}

			const std::string_view GetReason() const
			{
				if (!HasReason())
				{
					return {};
				}

				const uint8_t reasonLength = GetReasonLength();
				const auto* reason         = GetReasonPointer();

				if (!reason)
				{
					return {};
				}

				// NOTE: Validation of the Reason length field is done outside this
				// method.

				return std::string_view(reinterpret_cast<const char*>(reason), reasonLength);
			}

			void SetReason(const std::string_view& reason);

		protected:
			ByePacket* SoftClone(const uint8_t* buffer) const final;

		private:
			uint8_t* GetSsrcsPointer() const
			{
				return GetVariableLengthValuePointer();
			}

			uint32_t GetSsrcAt(uint8_t idx) const
			{
				return Utils::Byte::Get4Bytes(GetSsrcsPointer(), (idx * 4));
			}

			uint8_t* GetReasonLengthPointer() const
			{
				return GetVariableLengthValuePointer() + (GetCount() * 4);
			}

			uint8_t* GetReasonPointer() const
			{
				return GetReasonLengthPointer() + 1;
			}

			/**
			 * Returns the value of the Reason length field.
			 *
			 * @remarks
			 * - This value must be validated outside.
			 */
			uint8_t GetReasonLength() const
			{
				if (!HasReason())
				{
					return 0;
				}

				return Utils::Byte::Get1Byte(GetReasonLengthPointer(), 0);
			}
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
