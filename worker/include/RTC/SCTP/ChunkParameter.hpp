#ifndef MS_RTC_SCTP_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"
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
		 * - Parameter Type (16 bits): Unsigned integer.
		 * - Parameter Length (16 bits): Unsigned integer. Cnotains the size of the
		 *   parameter in bytes, including the Parameter Type, Parameter Length and
		 *   Parameter Value fields. Thus, a parameter with a zero-length Parameter
		 *   Value field would have a Parameter Length field of 4. The Parameter
		 *   Length does not include any padding bytes.
		 * - Parameter Value (variable length).
		 * - Padding: Bytes of padding to make the Parameter length be multiple of
		 *   4 bytes.
		 */

		// Forward declaration.
		class Chunk;

		class ChunkParameter : public Serializable
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parameter Type.
			 */
			enum class ChunkParameterType : uint16_t
			{
				HEARTBEAT_INFO      = 0x0001,
				IPV4_ADDRESS        = 0x0005,
				IPV6_ADDRESS        = 0x0006,
				COOKIE_PRESERVATIVE = 0x0009,
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
				 * Parameter Length field does not count any chunk padding.
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
			 * @param parameterType - If given buffer is a valid FooItem then
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

			virtual ChunkParameter* SoftClone(const uint8_t* buffer) const = 0;

			virtual void SoftCloneInto(ChunkParameter* parameter) const final;

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

			/**
			 * Whether the Parameter has a value (greater than 0 bytes).
			 *
			 * @remarks
			 * Let's make this method public since it's convenient for testing.
			 */
			virtual bool HasValue() const final
			{
				return GetLengthField() > ChunkParameter::ChunkParameterHeaderLength;
			}

			/**
			 * Length of the Parameter value.
			 *
			 * @remarks Let's make this method public since it's convenient for
			 * testing.
			 */
			virtual uint16_t GetValueLength() const final
			{
				if (!HasValue())
				{
					return 0u;
				}

				return GetLengthField() - ChunkParameter::ChunkParameterHeaderLength;
			}

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpCommon(int indentation) const final;

			virtual void SoftSerialize(const uint8_t* buffer) final;

			virtual void InitializeHeader(ChunkParameterType parameterType, uint16_t lengthFieldValue) final;

			virtual uint8_t* GetValuePointer() const final
			{
				return const_cast<uint8_t*>(GetBuffer()) + ChunkParameter::ChunkParameterHeaderLength;
			}

			virtual const uint8_t* GetValue() const final
			{
				if (!HasValue())
				{
					return nullptr;
				}

				return GetValuePointer();
			}

			virtual void SetValue(const uint8_t* value, uint16_t valueLength) final;

		private:
			/**
			 * NOTE: Return ChunkParameterHeader* instead of const
			 * ChunkParameterHeader* since we may want to modify its fields.
			 */
			virtual ChunkParameterHeader* GetHeaderPointer() const final
			{
				return reinterpret_cast<ChunkParameterHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			virtual void SetType(ChunkParameterType type) final
			{
				GetHeaderPointer()->type =
				  static_cast<ChunkParameterType>(uint16_t{ htons(static_cast<uint16_t>(type)) });
			}

			virtual uint16_t GetLengthField() const final
			{
				return uint16_t{ ntohs(GetHeaderPointer()->length) };
			}

			/**
			 * @throw MediaSoupError - If given `length` is higher than mazimmun
			 *   allowed one (65535).
			 */
			virtual void SetLengthField(size_t length) final;
		};
	} // namespace SCTP
} // namespace RTC

#endif
