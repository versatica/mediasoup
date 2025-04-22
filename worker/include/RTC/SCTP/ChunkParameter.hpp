#ifndef MS_RTC_SCTP_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/PacketItemBase.hpp"
#include <string>
#include <unordered_map>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Chunk Parameter.
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Parameter Type         |       Parameter Length        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                        Parameter Value                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Parameter Type (16 bits).
		 * - Parameter Length (16 bits): Total length of the Parameter, including
		 *   the Parameter Type, Parameter Length and Parameter Value fields
		 *   (padding is excluded). Thus, a Parameter with a zero-length Parameter
		 *   Value field would have a Parameter Length field of 4.
		 * - Parameter Value (variable length).
		 * - Padding: Bytes of padding to make the Parameter total length be
		 *   multiple of 4 bytes.
		 */

		// Forward declaration.
		class Chunk;

		class ChunkParameter : public PacketItemBase
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parameter Type.
			 */
			enum class ChunkParameterType : uint16_t
			{
				HEARTBEAT_INFO          = 0x0001,
				IPV4_ADDRESS            = 0x0005,
				IPV6_ADDRESS            = 0x0006,
				COOKIE_PRESERVATIVE     = 0x0009,
				SUPPORTED_ADDRESS_TYPES = 0x000C,
				// TODO: Add more.
			};

			/**
			 * Action that is taken if the processing endpoint does not recognize the
			 * Chunk Parameter.
			 */
			enum class ActionForUnknownChunkParameterType : uint8_t
			{
				STOP            = 0b00,
				STOP_AND_REPORT = 0b01,
				SKIP            = 0b10,
				SKIP_AND_REPORT = 0b11
			};

			/**
			 * Struct of a SCTP Chunk Parameter Header.
			 */
			struct ChunkParameterHeader
			{
				ChunkParameterType type;
				/**
				 * The value of the Parameter Length field, which represents the total
				 * length of the Parameter in bytes, including the Parameter Type,
				 * Parameter Length and Parameter Value fields. So if the Parameter
				 * Value field is zero-length, the Length field must be 4. The
				 * Parameter Length field does not count any padding.
				 */
				uint16_t length;
			};

		public:
			static const size_t ChunkParameterHeaderLength{ 4 };

		public:
			/**
			 * Whether given buffer could be a a valid Chunk Parameter.
			 *
			 * @param buffer
			 * @param bufferLength - Can be greater than real Parameter length.
			 * @param parameterType - If given buffer is a valid Parameter then
			 *   `parameterType` is rewritten to parsed ChunkParameterType.
			 * @param parameterLength - If given buffer is a valid Parameter then
			 *   `parameterLength` is rewritten to the value of the Parameter Length
			 *    field.
			 * @param padding - If given buffer is a valid Parameter then `padding`
			 *   is rewritten to the number of padding bytes in the Parameter (only
			 *   the necessary ones to make total length multiple of 4).
			 */
			static bool IsChunkParameter(
			  const uint8_t* buffer,
			  size_t bufferLength,
			  ChunkParameterType& parameterType,
			  uint16_t& parameterLength,
			  uint8_t& padding);

			static const std::string& ChunkParameterType2String(ChunkParameterType parameterType);

		private:
			static std::unordered_map<ChunkParameterType, std::string> chunkParameterType2String;

		protected:
			/**
			 * Constructor is protected because we only want to create ChunkParameter
			 * instances via Parse() and Factory() in subclasses.
			 */
			ChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ChunkParameter() override;

			virtual void Dump(int indentation = 0) const override = 0;

			virtual ChunkParameter* Clone(uint8_t* buffer, size_t bufferLength) const override = 0;

			virtual ChunkParameterType GetType() const final
			{
				return static_cast<ChunkParameterType>(
				  uint16_t{ ntohs(static_cast<uint16_t>(GetHeaderPointer()->type)) });
			}

			/**
			 * False by default. UnknownParameterChunk class overrides this method to
			 * return true instead.
			 */
			virtual bool HasUnknownType() const
			{
				return false;
			}

			virtual ActionForUnknownChunkParameterType GetActionForUnknownChunkParameterType() const final
			{
				return static_cast<ActionForUnknownChunkParameterType>(GetBuffer()[0] >> 6);
			}

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpCommon(int indentation) const override final;

			virtual void SoftSerialize(const uint8_t* buffer) final;

			virtual ChunkParameter* SoftClone(const uint8_t* buffer) const = 0;

			virtual void SoftCloneInto(ChunkParameter* parameter) const final;

			virtual void InitializeHeader(ChunkParameterType parameterType, uint16_t lengthFieldValue) final;

			/**
			 * Chunk Parameter subclasses with header bigger than default one (4
			 * bytes) must override this method and return their header length
			 * (excluding variable-length field considered "value").
			 */
			virtual size_t GetHeaderLength() const override
			{
				return ChunkParameter::ChunkParameterHeaderLength;
			}

		private:
			/**
			 * NOTE: Return ChunkParameterHeader* instead of const
			 * ChunkParameterHeader* since we may want to modify its fields.
			 */
			virtual ChunkParameterHeader* GetHeaderPointer() const final
			{
				return reinterpret_cast<ChunkParameterHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			virtual void SetType(ChunkParameterType parameterType) final
			{
				GetHeaderPointer()->type =
				  static_cast<ChunkParameterType>(uint16_t{ htons(static_cast<uint16_t>(parameterType)) });
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
