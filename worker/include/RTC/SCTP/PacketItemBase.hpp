#ifndef MS_RTC_SCTP_PACKET_ITEM_BASE_HPP
#define MS_RTC_SCTP_PACKET_ITEM_BASE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Serializable.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Packet Item Base.
		 *
		 * This is the base class of all items in a SCTP Packet, this is:
		 * - SCTP Chunk,
		 * - SCTP Chunk Parameter, and
		 * - SCTP Error Cause.
		 *
		 * All those items have the same Length field with same meaning.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                               |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                             Value                             /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class PacketItemBase : public Serializable
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t PacketItemBaseHeaderLength{ 4 };

		public:
			/**
			 * Whether given buffer could be a a valid SCTP PacketItemBase.
			 */
			static bool IsPacketItemBase(
			  const uint8_t* buffer, size_t bufferLength, uint16_t& itemLength, uint8_t& padding);

		protected:
			PacketItemBase(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~PacketItemBase() override;

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpCommon(int indentation) const;

			virtual void InitializePacketBaseItemHeader(uint16_t lengthFieldValue) final;

			/**
			 * Subclasses with header bigger than default one (4 bytes) must override
			 * this method and return their header length (excluding variable-length
			 * field considered "value", Optional/Variable-Length
			 * Parameters and Error Causes).
			 */
			virtual size_t GetHeaderLength() const
			{
				return PacketItemBase::PacketItemBaseHeaderLength;
			}

			virtual uint8_t* GetValuePointer() const final
			{
				return const_cast<uint8_t*>(GetBuffer()) + GetHeaderLength();
			}

			virtual bool HasValue() const final
			{
				return GetLengthField() > GetHeaderLength();
			}

			virtual const uint8_t* GetValue() const final
			{
				if (!HasValue())
				{
					return nullptr;
				}

				return GetValuePointer();
			}

			virtual void SetValue(const uint8_t* value, size_t valueLength) final;

			virtual uint16_t GetValueLength() const final
			{
				if (!HasValue())
				{
					return 0u;
				}

				return GetLengthField() - GetHeaderLength();
			}

			virtual void SetValueLength(size_t valueLength) final;

		private:
			virtual uint16_t GetLengthField() const final
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 2);
			}

			/**
			 * @throw MediaSoupError - If given `length` is higher than maximmun
			 *   allowed one (65535).
			 */
			virtual void SetLengthField(size_t length) final;
		};
	} // namespace SCTP
} // namespace RTC

#endif
