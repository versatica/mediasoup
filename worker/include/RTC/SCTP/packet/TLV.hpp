#ifndef MS_RTC_SCTP_TLV_HPP
#define MS_RTC_SCTP_TLV_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Serializable.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP TLV (Type-Length-Value).
		 *
		 * This is the base class of all items in a SCTP Packet, this is:
		 * - SCTP Chunk,
		 * - SCTP Parameter, and
		 * - SCTP Error Cause.
		 *
		 * All those items have the same Length field and 4-byte padded length.
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

		class TLV : public Serializable
		{
		public:
			static const size_t TLVHeaderLength{ 4 };

		public:
			/**
			 * Whether given buffer could be a a valid SCTP TLV.
			 */
			static bool IsTLV(
			  const uint8_t* buffer, size_t bufferLength, uint16_t& itemLength, uint8_t& padding);

		protected:
			TLV(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~TLV() override;

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpCommon(int indentation) const;

			virtual void InitializeTLVHeader(uint16_t lengthFieldValue) final;

			/**
			 * Subclasses with header bigger than default one (4 bytes) must override
			 * this method and return their header length (excluding variable-length
			 * field considered "value", Optional/Variable-Length
			 * Parameters and Error Causes).
			 */
			virtual size_t GetHeaderLength() const
			{
				return TLV::TLVHeaderLength;
			}

			/**
			 * The value of the Length field, which includes the length of the header
			 * and content (padding excluded).
			 */
			virtual uint16_t GetLengthField() const final
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 2);
			}

			/**
			 * A pointer to the position in the buffer where the variable-length value
			 * (if any) starts or should start.
			 */
			virtual uint8_t* GetVariableLengthValuePointer() const final
			{
				return const_cast<uint8_t*>(GetBuffer()) + GetHeaderLength();
			}

			/**
			 * Whether this item contains a variable-length value.
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual bool HasVariableLengthValue() const final
			{
				return GetLengthField() > GetHeaderLength();
			}

			/**
			 * Variable-length value of this item.
			 *
			 * @remarks
			 * - The variable-length value starts after the fixed header, which can be
			 *   different and have different length in each item definition.
			 * - In the case of SCTP Chunk class and subclasses (which implements this
			 *   class) we assume that a Chunk having variable-length value does not
			 *   have Parameters or Error Causes.
			 */
			virtual const uint8_t* GetVariableLengthValue() const final
			{
				if (!HasVariableLengthValue())
				{
					return nullptr;
				}

				return GetVariableLengthValuePointer();
			}

			/**
			 * Set the variable-length value. It copies the given value into the
			 * the variable-length value of the item and updates both the length of
			 * the Serializable and the Length field.
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual void SetVariableLengthValue(const uint8_t* value, size_t valueLength) final;

			/**
			 * The length of the variable-length value.
			 */
			virtual uint16_t GetVariableLengthValueLength() const final
			{
				if (!HasVariableLengthValue())
				{
					return 0u;
				}

				return GetLengthField() - GetHeaderLength();
			}

			/**
			 * Set the length of the variable-length value. It doesn't copy any value
			 * into the variable-length value. This method is used in items that have
			 * variable-length value but it doesn't consist on a buffer + length, but
			 * instead is an structure with fields (with variable length).
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual void SetVariableLengthValueLength(size_t valueLength) final;

			/**
			 * This method doesn't really add an item into the item (that must be done
			 * by each subcass) but updates the length of the Serializable and the
			 * value of the Length field by incrementing it with the length of the
			 * given item.
			 */
			virtual void AddItem(const TLV* item) final;

		private:
			/**
			 * @throw MediaSoupError - If given `length` is higher than maximmun
			 *   allowed one (65535).
			 */
			virtual void SetLengthField(size_t lengthField) final;
		};
	} // namespace SCTP
} // namespace RTC

#endif
