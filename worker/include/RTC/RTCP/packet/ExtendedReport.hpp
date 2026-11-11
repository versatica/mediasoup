#ifndef MS_RTC_RTCP_EXTENDED_REPORT_HPP
#define MS_RTC_RTCP_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"
#include "Utils.hpp"
#include <ankerl/unordered_dense.h>
#include <string>

namespace RTC
{
	namespace RTCP
	{
		/**
		 * RTCP XR Extended Report block.
		 *
		 * @see RFC 3611.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |      BT       | type-specific |         block length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :             type-specific block contents                      :
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Block type (BT) (8 bits): Identifies the block format.
		 * - Type-specific (8 bits): Its use is determined by the block type
		 *   definition.
		 * - Block length (16 bits): Length of the block in 32-bit words minus
		 *   one, including the header. Therefore the total size in bytes is
		 *   (block length + 1) * 4.
		 * - Type-specific block contents (variable length): Its format depends
		 *   on the block type (BT). It's a multiple of 32 bits long.
		 */

		// Forward declaration.
		class XrPacket;

		class ExtendedReport : public Serializable
		{
			// We need that XrPacket calls protected and private methods in this
			// class.
			friend class XrPacket;

		public:
			/**
			 * Extended Report block type.
			 */
			enum class BlockType : uint8_t
			{
				/**
				 * Receiver Reference Time Report Block.
				 */
				RRT = 4,
				/**
				 * DLRR Report Block.
				 */
				DLRR = 5,
			};

			/**
			 * Struct of an Extended Report block header.
			 *
			 * @remarks
			 * - This struct is guaranteed to be aligned to 2 bytes.
			 */
			struct ExtendedReportHeader
			{
				BlockType blockType;
				uint8_t typeSpecific;
				uint16_t length;
			};

		public:
			/**
			 * Length (in bytes) of the Extended Report block header.
			 */
			static constexpr size_t ExtendedReportHeaderLength{ 4 };

			static const std::string& BlockTypeToString(BlockType blockType);

		protected:
			/**
			 * Whether given buffer could be a valid Extended Report block.
			 *
			 * @param buffer
			 * @param bufferLength - Can be greater than real Extended Report block
			 *   length.
			 * @param blockType - If given buffer is a valid Extended Report block then
			 *   `blockType` is rewritten to parsed BlockType.
			 * @param extendedReportLength - If given buffer is a valid Extended
			 *   Report block then `extendedReportLength` is rewritten to the computed
			 *   length of the Extended Report block.
			 */
			static bool IsExtendedReport(
			  const uint8_t* buffer, size_t bufferLength, BlockType& blockType, size_t& extendedReportLength);

		private:
			static const ankerl::unordered_dense::map<BlockType, std::string> BlockType2String;

		protected:
			/**
			 * Constructor is protected because we only want to create Extended
			 * Report block instances via Parse() and Factory() in subclasses.
			 */
			ExtendedReport(uint8_t* buffer, size_t bufferLength);

		public:
			~ExtendedReport() override;

			/**
			 * Must be overridden by each subclass.
			 */
			void Dump(int indentation = 0) const override = 0;

			/**
			 * Must be overridden by each subclass.
			 */
			ExtendedReport* Clone(uint8_t* buffer, size_t bufferLength) const override = 0;

			BlockType GetType() const
			{
				return GetHeaderPointer()->blockType;
			}

			/**
			 * False by default. UnknownExtendedReport class overrides this method to
			 * return true instead.
			 */
			virtual bool HasUnknownType() const
			{
				return false;
			}

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			void DumpCommon(int indentation) const;

			virtual void SoftSerialize(const uint8_t* buffer) final;

			/**
			 * Must be overridden by each subclass.
			 */
			virtual ExtendedReport* SoftClone(const uint8_t* buffer) const = 0;

			virtual void SoftCloneInto(ExtendedReport* extendedReport) const final;

			virtual void InitializeHeader(BlockType blockType, uint8_t typeSpecific, size_t length) final;

			virtual uint8_t GetTypeSpecific() const final
			{
				return GetHeaderPointer()->typeSpecific;
			}

			virtual void SetTypeSpecific(uint8_t typeSpecific) final;

			/**
			 * Value of the block length field, which is the length of the Extended
			 * Report block in 32-bit words minus one, including the header.
			 * Therefore the total size in bytes is (block length + 1) * 4.
			 */
			virtual uint16_t GetLengthField() const final
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 2);
			}

			/**
			 * Computed value (in bytes) of the block length field.
			 */
			virtual size_t GetLengthFieldComputed() const final
			{
				return (static_cast<size_t>(GetLengthField()) + 1) * 4;
			}

			/**
			 * Set the block length field given the total Extended Report block
			 * length in bytes. It encodes it as 32-bit words minus one as required
			 * by the block length field.
			 *
			 * @throw
			 * - MediaSoupTypeError - If given `length` is lower than the Extended
			 *   Report block header length (4 bytes).
			 * - MediaSoupTypeError - If given `length` is not multiple of 4 bytes.
			 * - MediaSoupTypeError - If given `length` is higher than the maximum
			 *   representable Extended Report block length (262144 bytes).
			 */
			virtual void SetLengthField(size_t length) final;

			/**
			 * A pointer to the position in the buffer where the variable-length value
			 * (if any) starts or should start.
			 */
			virtual uint8_t* GetVariableLengthValuePointer() const final
			{
				return const_cast<uint8_t*>(GetBuffer()) + ExtendedReport::ExtendedReportHeaderLength;
			}

			/**
			 * Whether this Extended Report block contains a variable-length value.
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual bool HasVariableLengthValue() const final
			{
				return GetLengthFieldComputed() > ExtendedReport::ExtendedReportHeaderLength;
			}

			/**
			 * Variable-length value of this Extended Report block.
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
			 * The length of the variable-length value.
			 */
			virtual size_t GetVariableLengthValueLength() const final
			{
				if (!HasVariableLengthValue())
				{
					return 0;
				}

				return GetLengthFieldComputed() - ExtendedReport::ExtendedReportHeaderLength;
			}

			/**
			 * Set the length of the variable-length value. It doesn't copy any value
			 * into the variable-length value.
			 *
			 * @throw
			 * - MediaSoupTypeError - If given `valueLength` is higher than available
			 *   length.
			 * - MediaSoupTypeError - If given `valueLength` is not multiple of 4
			 *   bytes.
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual void SetVariableLengthValueLength(size_t valueLength) final;

		private:
			/**
			 * @remarks
			 * - Returns ExtendedReportHeader* instead of const ExtendedReportHeader*
			 *   since we may want to modify its fields.
			 */
			ExtendedReportHeader* GetHeaderPointer() const
			{
				return reinterpret_cast<ExtendedReportHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}
		};
	} // namespace RTCP
} // namespace RTC

#endif
