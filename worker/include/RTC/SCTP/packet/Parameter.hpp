#ifndef MS_RTC_SCTP_PARAMETER_HPP
#define MS_RTC_SCTP_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/TLV.hpp"
#include <string>
#include <unordered_map>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Parameter.
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

		class Parameter : public TLV
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parameter Type.
			 */
			enum class ParameterType : uint16_t
			{
				HEARTBEAT_INFO               = 0x0001,
				IPV4_ADDRESS                 = 0x0005,
				IPV6_ADDRESS                 = 0x0006,
				STATE_COOKIE                 = 0x0007,
				UNRECOGNIZED_PARAMETER       = 0x0008,
				COOKIE_PRESERVATIVE          = 0x0009,
				SUPPORTED_ADDRESS_TYPES      = 0x000C,
				FORWARD_TSN_SUPPORTED        = 0xC000, // Type 49152, RFC 3758
				SUPPORTED_EXTENSIONS         = 0x8008, // Type 32776, RFC 5061
				OUTGOING_SSN_RESET_REQUEST   = 0x000D, // Type 13, RFC 6525
				INCOMING_SSN_RESET_REQUEST   = 0x000E, // Type 14, RFC 6525
				SSN_TSN_RESET_REQUEST        = 0x000F, // Type 15, RFC 6525
				RECONFIGURATION_RESPONSE     = 0x0010, // Type 16, RFC 6525
				ADD_OUTGOING_STREAMS_REQUEST = 0x0011, // Type 17, RFC 6525
				ADD_INCOMING_STREAMS_REQUEST = 0x0012, // Type 18, RFC 6525
				ZERO_CHECKSUM_ACCEPTABLE     = 0x8001, // Type 32769, RFC 9653
			};

			/**
			 * Action that is taken if the processing endpoint does not recognize the
			 * Parameter.
			 */
			enum class ActionForUnknownParameterType : uint8_t
			{
				STOP            = 0b00,
				STOP_AND_REPORT = 0b01,
				SKIP            = 0b10,
				SKIP_AND_REPORT = 0b11
			};

			/**
			 * Struct of a SCTP Parameter Header.
			 */
			struct ParameterHeader
			{
				ParameterType type;
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
			static const size_t ParameterHeaderLength{ 4 };

		public:
			/**
			 * Whether given buffer could be a a valid Parameter.
			 *
			 * @param buffer
			 * @param bufferLength - Can be greater than real Parameter length.
			 * @param parameterType - If given buffer is a valid Parameter then
			 *   `parameterType` is rewritten to parsed ParameterType.
			 * @param parameterLength - If given buffer is a valid Parameter then
			 *   `parameterLength` is rewritten to the value of the Parameter Length
			 *    field.
			 * @param padding - If given buffer is a valid Parameter then `padding`
			 *   is rewritten to the number of padding bytes in the Parameter (only
			 *   the necessary ones to make total length multiple of 4).
			 */
			static bool IsParameter(
			  const uint8_t* buffer,
			  size_t bufferLength,
			  ParameterType& parameterType,
			  uint16_t& parameterLength,
			  uint8_t& padding);

			static const std::string& ParameterType2String(ParameterType parameterType);

		private:
			static std::unordered_map<ParameterType, std::string> parameterType2String;

		protected:
			/**
			 * Constructor is protected because we only want to create Parameter
			 * instances via Parse() and Factory() in subclasses.
			 */
			Parameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~Parameter() override;

			virtual void Dump(int indentation = 0) const override = 0;

			virtual Parameter* Clone(uint8_t* buffer, size_t bufferLength) const override = 0;

			virtual ParameterType GetType() const final
			{
				return static_cast<ParameterType>(
				  uint16_t{ ntohs(static_cast<uint16_t>(GetHeaderPointer()->type)) });
			}

			/**
			 * False by default. UnknownParameter class overrides this method to
			 * return true instead.
			 */
			virtual bool HasUnknownType() const
			{
				return false;
			}

			virtual ActionForUnknownParameterType GetActionForUnknownParameterType() const final
			{
				return static_cast<ActionForUnknownParameterType>(GetBuffer()[0] >> 6);
			}

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpCommon(int indentation) const override final;

			virtual void SoftSerialize(const uint8_t* buffer) final;

			virtual Parameter* SoftClone(const uint8_t* buffer) const = 0;

			virtual void SoftCloneInto(Parameter* parameter) const final;

			virtual void InitializeHeader(ParameterType parameterType, uint16_t lengthFieldValue) final;

			/**
			 * Parameter subclasses with header bigger than default one (4 bytes)
			 * must override this method and return their header length (excluding
			 * variable-length field considered "value").
			 */
			virtual size_t GetHeaderLength() const override
			{
				return Parameter::ParameterHeaderLength;
			}

		private:
			/**
			 * NOTE: Return ParameterHeader* instead of const ParameterHeader* since
			 * we may want to modify its fields.
			 */
			virtual ParameterHeader* GetHeaderPointer() const final
			{
				return reinterpret_cast<ParameterHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			virtual void SetType(ParameterType parameterType) final
			{
				GetHeaderPointer()->type =
				  static_cast<ParameterType>(uint16_t{ htons(static_cast<uint16_t>(parameterType)) });
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
