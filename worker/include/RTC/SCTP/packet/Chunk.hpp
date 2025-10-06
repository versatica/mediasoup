#ifndef MS_RTC_SCTP_CHUNK_HPP
#define MS_RTC_SCTP_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/TLV.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Chunk.
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Chunk Type   |  Chunk Flags  |         Chunk Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                          Chunk Value                          /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits).
		 * - Chunk Flags (8 bits).
		 * - Chunk Length (16 bits): Total length of the Chunk
		 *   excluding padding bytes. Minimum value is 4 (if Chunk Value is 0
		 *   bytes). Maximum value is 65535, which means 1 byte of padding.
		 * - Chunk Value (variable length).
		 * - Padding: Bytes of padding to make the Chunk total length be
		 *   multiple of 4 bytes.
		 */

		// Forward declaration.
		class Packet;

		class Chunk : public TLV
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			using parametersIterator  = typename std::vector<Parameter*>::const_iterator;
			using ErrorCausesIterator = typename std::vector<ErrorCause*>::const_iterator;

		public:
			/**
			 * Chunk Type.
			 */
			enum class ChunkType : uint8_t
			{
				DATA              = 0x00,
				INIT              = 0x01,
				INIT_ACK          = 0x02,
				SACK              = 0x03,
				HEARTBEAT_REQUEST = 0x04,
				HEARTBEAT_ACK     = 0x05,
				ABORT             = 0x06,
				SHUTDOWN          = 0x07,
				SHUTDOWN_ACK      = 0x08,
				OPERATION_ERROR   = 0x09, // NOTE: Cannot use ERROR (MSVC complains).
				COOKIE_ECHO       = 0x0A,
				COOKIE_ACK        = 0x0B,
				ECNE              = 0x0C, // NOTE: Not implemented.
				CWR               = 0x0D, // NOTE: Not implemented.
				SHUTDOWN_COMPLETE = 0x0E,
				FORWARD_TSN       = 0xC0, // Type: 192, RFC 3758
				RE_CONFIG         = 0x82, // Type 130, RFC 6525
				I_DATA            = 0x40, // Type: 64, RFC 8260
				I_FORWARD_TSN     = 0xC2, // Type: 194, RFC 8260
			};

			/**
			 * Action that is taken if the processing endpoint does not recognize the
			 * Chunk Type.
			 */
			enum class ActionForUnknownChunkType : uint8_t
			{
				STOP            = 0b00,
				STOP_AND_REPORT = 0b01,
				SKIP            = 0b10,
				SKIP_AND_REPORT = 0b11
			};

			/**
			 * Struct of a SCTP Chunk Header.
			 */
			struct ChunkHeader
			{
				ChunkType type;
				uint8_t flags;
				/**
				 * The value of the Chunk Length field, which represents the total
				 * length of the Chunk in bytes, including the Chunk Type, Chunk Flags,
				 * Chunk Length and Chunk Value fields. So if the Chunk Value field is
				 * zero-length, the Length field must be 4. The Chunk Length field does
				 * not count any padding.
				 */
				uint16_t length;
			};

		private:
			/**
			 * Access to individual bit in the Chunk Flags field. bit0 corresponds
			 * to the least significant bit.
			 */
			struct ChunkFlags
			{
#if defined(MS_LITTLE_ENDIAN)
				uint8_t bit0 : 1;
				uint8_t bit1 : 1;
				uint8_t bit2 : 1;
				uint8_t bit3 : 1;
				uint8_t bit4 : 1;
				uint8_t bit5 : 1;
				uint8_t bit6 : 1;
				uint8_t bit7 : 1;
#elif defined(MS_BIG_ENDIAN)
				uint8_t bit7 : 1;
				uint8_t bit6 : 1;
				uint8_t bit5 : 1;
				uint8_t bit4 : 1;
				uint8_t bit3 : 1;
				uint8_t bit2 : 1;
				uint8_t bit1 : 1;
				uint8_t bit0 : 1;
#endif
			};

		public:
			static const size_t ChunkHeaderLength{ 4 };

		public:
			/**
			 * Whether given buffer could be a a valid Chunk.
			 *
			 * @param buffer
			 * @param bufferLength - Can be greater than real Chunk length.
			 * @param chunkType - If given buffer is a valid Chunk then `chunkType`
			 *   is rewritten to parsed ChunkType.
			 * @param chunkLength - If given buffer is a valid Chunk then
			 *   `chunkLength` is rewritten to the value of the Chunk Length field.
			 * @param padding - If given buffer is a valid Chunk then `padding` is
			 *   rewritten to the number of padding bytes in the Chunk (only the
			 *   necessary ones to make total length multiple of 4).
			 */
			static bool IsChunk(
			  const uint8_t* buffer,
			  size_t bufferLength,
			  ChunkType& chunkType,
			  uint16_t& chunkLength,
			  uint8_t& padding);

			static const std::string& ChunkType2String(ChunkType chunkType);

		private:
			static std::unordered_map<ChunkType, std::string> chunkType2String;

		protected:
			/**
			 * Constructor is protected because we only want to create Chunk
			 * instances via Parse() and Factory() in subclasses.
			 */
			Chunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~Chunk() override;

			virtual void Dump(int indentation = 0) const override = 0;

			virtual void Serialize(uint8_t* buffer, size_t bufferLength) override final;

			/**
			 * Can be overridden by each subclass.
			 */
			virtual Chunk* Clone(uint8_t* buffer, size_t bufferLength) const override = 0;

			virtual ChunkType GetType() const final
			{
				return GetHeaderPointer()->type;
			}

			/**
			 * False by default. UnknownChunk class overrides this method to return
			 * true instead.
			 */
			virtual bool HasUnknownType() const
			{
				return false;
			}

			virtual ActionForUnknownChunkType GetActionForUnknownChunkType() const final
			{
				return static_cast<ActionForUnknownChunkType>(GetBuffer()[0] >> 6);
			}

			virtual uint8_t GetFlags() const final
			{
				return GetHeaderPointer()->flags;
			}

			/**
			 * Whether this type of Chunk can have Parameters. Subclasses must
			 * override this method if they can have Parameters.
			 */
			virtual bool CanHaveParameters() const
			{
				return false;
			}

			virtual bool HasParameters() const final
			{
				return this->parameters.size() > 0;
			}

			virtual size_t GetParametersCount() const final
			{
				return this->parameters.size();
			}

			virtual parametersIterator ParametersBegin() const final
			{
				return this->parameters.begin();
			}

			virtual parametersIterator ParametersEnd() const final
			{
				return this->parameters.end();
			}

			virtual const Parameter* GetParameterAt(size_t idx) const final
			{
				if (idx >= this->parameters.size())
				{
					return nullptr;
				}

				return this->parameters[idx];
			}

			template<typename T>
			const T* GetFirstParameterOfType() const
			{
				for (const auto* parameter : this->parameters)
				{
					if (typeid(*parameter) == typeid(T))
					{
						return static_cast<const T*>(parameter);
					}
				}

				return nullptr;
			}

			/**
			 * Clone given Parameter into Chunk's buffer.
			 *
			 * @remarks
			 * Once this method is called, the caller may want to free the original
			 * given Parameter (otherwise it will leak since the manages a clone of
			 * it).
			 *
			 * @throw MediaSoupError - If the Chunk subclass cannot have Parameters.
			 */
			virtual void AddParameter(const Parameter* parameter) final;

			/**
			 * Build a Parameter within the Chunk's buffer and append it to the list
			 * of Parameters. The caller can perform modifications in that Parameter
			 * and those will affect the Chunk body where the Parameter is serialized.
			 * The desired Parameter class type is given via template argument.
			 *
			 * @returns Pointer of the created Parameter specific class.
			 *
			 * @throw MediaSoupError - If the Chunk subclass cannot have Parameters.
			 *
			 * @remarks
			 * - The caller MUST invoke `Consolidate()` once the Parameter is
			 *   completed.
			 * - The caller MUST NOT call `BuildChunkInPlace()` while other Parameter
			 *   is in progress.
			 * - The caller MUST NOT free the obtained Parameter pointer since it's
			 *   now part of the Chunk.
			 * - Method implemented in header file due to C++ template usage.
			 *
			 * @example
			 * ```c++
			 * auto* ipv4Parameter =
			 *   chunk->BuildParameterInPlace<IPv4AddressParameter>();
			 * ```
			 */
			template<typename T>
			T* BuildParameterInPlace()
			{
				AssertNotFrozen();
				AssertCanHaveParameters();

				// The new Parameter will be added after other Parameters in the Chunk,
				// this is, at the end of the Chunk, whose length we know it's padded to
				// 4 bytes, and each Parameter total length is also multiple of 4 bytes.
				auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();
				// The remaining length in the buffer is the potential buffer length
				// of the Parameter.
				size_t parameterMaxBufferLength = GetBufferLength() - (ptr - GetBuffer());

				auto* parameter = T::Factory(ptr, parameterMaxBufferLength);

				// NOTE: Do not fix/update the Parameter buffer length since the caller
				// probably wants to modify the Parameter.

				HandleInPlaceParameter(parameter);

				return parameter;
			}

			/**
			 * Whether this type of Chunk can have Error Causes. Subclasses must
			 * override this method if they can have Error Causes.
			 */
			virtual bool CanHaveErrorCauses() const
			{
				return false;
			}

			virtual bool HasErrorCauses() const final
			{
				return this->errorCauses.size() > 0;
			}

			virtual size_t GetErrorCausesCount() const final
			{
				return this->errorCauses.size();
			}

			virtual ErrorCausesIterator ErrorCausesBegin() const final
			{
				return this->errorCauses.begin();
			}

			virtual ErrorCausesIterator ErrorCausesEnd() const final
			{
				return this->errorCauses.end();
			}

			virtual const ErrorCause* GetErrorCauseAt(size_t idx) const final
			{
				if (idx >= this->errorCauses.size())
				{
					return nullptr;
				}

				return this->errorCauses[idx];
			}

			template<typename T>
			const T* GetFirstErrorCauseOfCode() const
			{
				for (const auto* errorCause : this->errorCauses)
				{
					if (typeid(*errorCause) == typeid(T))
					{
						return static_cast<const T*>(errorCause);
					}
				}

				return nullptr;
			}

			/**
			 * Clone given Error Cause into Chunk's buffer.
			 *
			 * @remarks
			 * Once this method is called, the caller may want to free the original
			 * given Error Cause (otherwise it will leak since the Chunk manages
			 * a clone of it).
			 *
			 * @throw MediaSoupError - If the Chunk subclass cannot have Error Causes.
			 */
			virtual void AddErrorCause(const ErrorCause* errorCause) final;

			/**
			 * Build a Error Cause within the Chunk's buffer and append it to the
			 * list of Error Causes. The caller can perform modifications in that
			 * Error Cause and those will affect the Chunk body where the Error Cause
			 * is serialzed. The desired Error Cause class type is given via template
			 * argument.
			 *
			 * @returns Pointer of the created Error Cause specific class.
			 *
			 * @throw MediaSoupError - If the Chunk subclass cannot have Error Causes.
			 *
			 * @remarks
			 * - The caller MUST invoke `Consolidate()` once the Error Cause is
			 *   completed.
			 * - The caller MUST NOT call `BuildChunkInPlace()` while other Error
			 *   Cause is in progress.
			 * - The caller MUST NOT free the obtained Error Cause pointer since it's
			 *   now part of the Chunk.
			 * - Method implemented in header file due to C++ template usage.
			 *
			 * @example
			 * ```c++
			 * auto* noUserDataErrorCause =
			 *   chunk->BuildErrorCauseInPlace<NoUserDataErrorCause>();
			 * ```
			 */
			template<typename T>
			T* BuildErrorCauseInPlace()
			{
				AssertNotFrozen();
				AssertCanHaveErrorCauses();

				// The new Error Cause will be added after other Error Causes in the
				// Chunk, this is, at the end of the Chunk, whose length we know it's
				// padded to 4 bytes, and each Error Cause total length is also
				// multiple of 4 bytes.
				auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();
				// The remaining length in the buffer is the potential buffer length
				// of the Error Cause.
				size_t errorCauseMaxBufferLength = GetBufferLength() - (ptr - GetBuffer());

				auto* errorCause = T::Factory(ptr, errorCauseMaxBufferLength);

				// NOTE: Do not fix/update the Error Cause buffer length since the
				// caller probably wants to modify the Error Cause.

				HandleInPlaceErrorCause(errorCause);

				return errorCause;
			}

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpCommon(int indentation) const override final;

			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpParameters(int indentation) const final;

			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpErrorCauses(int indentation) const final;

			virtual void SoftSerialize(const uint8_t* buffer) final;

			/**
			 * Can be overridden by each subclass.
			 */
			virtual Chunk* SoftClone(const uint8_t* buffer) const = 0;

			virtual void SoftCloneInto(Chunk* chunk) const final;

			virtual void InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t lengthFieldValue) final;

			virtual bool GetBit0() const final
			{
				return GetFlagsPointer()->bit0;
			}

			virtual void SetBit0(bool flag) final
			{
				GetFlagsPointer()->bit0 = flag;
			}

			virtual bool GetBit1() const final
			{
				return GetFlagsPointer()->bit1;
			}

			virtual void SetBit1(bool flag) final
			{
				GetFlagsPointer()->bit1 = flag;
			}

			virtual bool GetBit2() const final
			{
				return GetFlagsPointer()->bit2;
			}

			virtual void SetBit2(bool flag) final
			{
				GetFlagsPointer()->bit2 = flag;
			}

			virtual bool GetBit3() const final
			{
				return GetFlagsPointer()->bit3;
			}

			virtual void SetBit3(bool flag) final
			{
				GetFlagsPointer()->bit3 = flag;
			}

			virtual bool GetBit4() const final
			{
				return GetFlagsPointer()->bit4;
			}

			virtual void SetBit4(bool flag) final
			{
				GetFlagsPointer()->bit4 = flag;
			}

			virtual bool GetBit5() const final
			{
				return GetFlagsPointer()->bit5;
			}

			virtual void SetBit5(bool flag) final
			{
				GetFlagsPointer()->bit5 = flag;
			}

			virtual bool GetBit6() const final
			{
				return GetFlagsPointer()->bit6;
			}

			virtual void SetBit6(bool flag) final
			{
				GetFlagsPointer()->bit6 = flag;
			}

			virtual bool GetBit7() const final
			{
				return GetFlagsPointer()->bit7;
			}

			virtual void SetBit7(bool flag) final
			{
				GetFlagsPointer()->bit7 = flag;
			}

			/**
			 * Chunk subclasses with header bigger than default one (4 bytes) must
			 * override this method and return their header length (excluding
			 * variable-length field considered "value", Optional/Variable-Length
			 * Parameters and Error Causes).
			 */
			virtual size_t GetHeaderLength() const override
			{
				return Chunk::ChunkHeaderLength;
			}

			/**
			 * To be called by each subclass of Chunk if Parameters parsing is
			 * needed. It creates Parameter subclasses and adds them to the Chunk.
			 *
			 * @remarks
			 * This method assumes that the Chunk basic parsing has been made already
			 * so current length of the Chunk is the fixed length of the specific
			 * Chunk class.
			 *
			 * @return True if no error happened while parsing Parameters.
			 *
			 * @throw MediaSoupError - If the Chunk subclass cannot have Chunk
			 *   Parameters.
			 */
			virtual bool ParseParameters() final;

			/**
			 * To be called by each subclass of Chunk if Error Causes parsing is
			 * needed. It creates ErrorCause subclasses and adds them to the Chunk.
			 *
			 * @remarks
			 * This method assumes that the Chunk basic parsing has been made already
			 * so current length of the Chunk is the fixed length of the specific
			 * Chunk class.
			 *
			 * @return True if no error happened while parsing Error Causes.
			 *
			 * @throw MediaSoupError - If the Chunk subclass cannot have Chunk
			 *   Parameters.
			 */
			virtual bool ParseErrorCauses() final;

		private:
			/**
			 * NOTE: Return ChunkHeader* instead of const ChunkHeader* since we may
			 * want to modify its fields.
			 */
			virtual ChunkHeader* GetHeaderPointer() const final
			{
				return reinterpret_cast<ChunkHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			virtual void SetType(ChunkType chunkType) final
			{
				GetHeaderPointer()->type = chunkType;
			}

			virtual void SetFlags(uint8_t flags) final
			{
				GetHeaderPointer()->flags = flags;
			}

			virtual ChunkFlags* GetFlagsPointer() const final
			{
				return reinterpret_cast<ChunkFlags*>(const_cast<uint8_t*>(GetBuffer()) + 1);
			}

			virtual void HandleInPlaceParameter(Parameter* parameter) final;

			virtual void HandleInPlaceErrorCause(ErrorCause* errorCause) final;

			virtual void AssertCanHaveParameters() const final;

			virtual void AssertCanHaveErrorCauses() const final;

		private:
			// Parameters.
			std::vector<Parameter*> parameters;
			// Error Causes.
			std::vector<ErrorCause*> errorCauses;
		};
	} // namespace SCTP
} // namespace RTC

#endif
