#ifndef MS_RTC_SCTP_HEARTBEAT_INFO_PARAMETER_HPP
#define MS_RTC_SCTP_HEARTBEAT_INFO_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP HeartbeatInfo Parameter (HEARBEAT_INFO) (1).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 1            |        HB Info Length         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                Sender-Specific Heartbeat Info                 /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class HeartbeatInfoParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a HeartbeatInfoParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static HeartbeatInfoParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a HeartbeatInfoParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static HeartbeatInfoParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a HeartbeatInfoParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static HeartbeatInfoParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			HeartbeatInfoParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~HeartbeatInfoParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual HeartbeatInfoParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasInfo() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetInfo() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetInfoLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetInfo(const uint8_t* info, uint16_t infoLength);

		protected:
			virtual HeartbeatInfoParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
