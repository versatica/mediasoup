#ifndef MS_RTC_SCTP_CHUNK_HPP
#define MS_RTC_SCTP_CHUNK_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"
#include <string>
#include <unordered_map>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Chunk.
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
		 * - Chunk Type (8 bits): Unsigned integer.
		 * - Chunk Flags (8 bits).
		 * - Chunk Length (16 bits): Unsigned integer. Total length of the Chunk
		 *   excluding padding bytes. Minimum value is 4 (if Chunk Value is 0
		 *   bytes). Maximum value is 65535, which means 1 byte of padding.
		 * - Chunk Value (variable length).
		 * - Padding: Bytes of padding to make the Chunk length be multiple of 4
		 *   bytes.
		 */

		// Forward declaration.
		class Packet;

		class Chunk : public Serializable
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Chunk types.
			 */
			enum class ChunkType : uint8_t
			{
				DATA              = 0x00,
				INIT              = 0x01,
				INIT_ACK          = 0x02,
				SACK              = 0x03,
				HEARTBEAT         = 0x04,
				HEARTBEAT_ACK     = 0x05,
				ABORT             = 0x06,
				SHUTDOWN          = 0x07,
				SHUTDOWN_ACK      = 0x08,
				ERROR             = 0x09,
				COOKIE_ECHO       = 0x0A,
				COOKIE_ACK        = 0x0B,
				ECNE              = 0x0C,
				CWR               = 0x0D,
				SHUTDOWN_COMPLETE = 0x0E
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
				 * length of the chunk in bytes, including the Chunk Type, Chunk Flags,
				 * Chunk Length and Chunk Value fields. So if the Chunk Value field is
				 * zero-length, the Length field must be 4. The Chunk Length field does
				 * not count any chunk padding.
				 */
				uint16_t length;
			};

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
			 * @param chunkType - If given buffer is a valid FooItem then `chunkType`
			 *   is rewritten to parsed ChunkType.
			 * @param chunkLength - If given buffer is a valid Chunk then
			 *   `chunkLength` is rewritten to the value of the Chunk Length field.
			 * @param padding - If given buffer is a valid Chunk then `padding` is
			 *   rewritten to the number of padding bytes in the Chunk.
			 */
			static bool IsChunk(
			  const uint8_t* buffer,
			  size_t bufferLength,
			  ChunkType& chunkType,
			  size_t& chunkLength,
			  uint8_t& padding);

			static const std::string& ChunkType2String(ChunkType chunkType);

		private:
			static std::unordered_map<ChunkType, std::string> chunkType2String;

		protected:
			/**
			 * Constructor is protected because we only want to create Chunk
			 * instances via Parse() and Factory() in subclasses.
			 */
			Chunk(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~Chunk() override;

			/**
			 * NOTE: Should be overridden by each subclass.
			 */
			virtual void Dump() const override;

			/**
			 * Can be overridden by each subclass.
			 */
			virtual Chunk* Clone(uint8_t* buffer, size_t bufferLength) const override;

			virtual ChunkType GetType() const final
			{
				return GetHeaderPointer()->type;
			}

			virtual bool HasUnknownType() const final
			{
				auto type = GetType();

				return type < ChunkType::DATA || type > ChunkType::SHUTDOWN_COMPLETE;
			}

			virtual ActionForUnknownChunkType GetActionForUnknownChunkType() const final
			{
				return static_cast<ActionForUnknownChunkType>(static_cast<uint8_t>(GetType()) >> 6);
			}

			virtual uint8_t GetFlags() const final
			{
				return GetHeaderPointer()->flags;
			}

		protected:
			virtual void InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t lengthFieldValue) final;

			/**
			 * NOTE: Return ChunkHeader* instead of const ChunkHeader* since we may
			 * want to modify its fields.
			 */
			virtual ChunkHeader* GetHeaderPointer() const final
			{
				return reinterpret_cast<ChunkHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			virtual ChunkFlags* GetFlagsPointer() const final
			{
				return reinterpret_cast<ChunkFlags*>(const_cast<uint8_t*>(GetBuffer()) + 1);
			}

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
			 * Private private because it returns the value of the Value Length field,
			 * which is not useful for the application.
			 */
			virtual uint16_t GetLengthField() const final
			{
				return uint16_t{ ntohs(GetHeaderPointer()->length) };
			}

			virtual void SetLengthField(uint16_t length) final
			{
				GetHeaderPointer()->length = uint16_t{ htons(length) };
			}

			virtual bool HasValue() const final
			{
				return GetLengthField() > Chunk::ChunkHeaderLength;
			}

			virtual uint8_t* GetValuePointer() const final
			{
				return const_cast<uint8_t*>(GetBuffer()) + Chunk::ChunkHeaderLength;
			}

			virtual uint16_t GetValueLength() const final
			{
				if (!HasValue())
				{
					return 0u;
				}

				return GetLengthField() - Chunk::ChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
