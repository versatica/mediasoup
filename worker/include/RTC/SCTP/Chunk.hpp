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
		 *   bytes).
		 * - Chunk Value (variable length).
		 * - Padding: Bytes of padding to make the Packet length be multiple of 4
		 *   bytes.
		 */

		// Forward declaration.
		class Packet;

		class Chunk : public Serializable
		{
		private:
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
			 * Struct of a SCTP Chunk Header.
			 */
			struct ChunkHeader
			{
				ChunkType type;
				uint8_t flags;
				/**
				 * The value of the Chunk Length field, which represents the size of the
				 * chunk in bytes, including the Chunk Type, Chunk Flags, Chunk Length
				 * and Chunk Value fields. So if the Chunk Value field is zero-length,
				 * the Length field must be 4. The Chunk Length field does not count any
				 * chunk padding.
				 */
				uint16_t length;
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
			 *   `chunkLength` is rewritten to the length of the Chunk (including
			 *   padding bytes, so it will be multiple of 4 bytes).
			 */
			static bool IsChunk(
			  const uint8_t* buffer, size_t bufferLength, ChunkType& chunkType, uint16_t& chunkLength);

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

			virtual uint8_t GetFlags() const final
			{
				return GetHeaderPointer()->flags;
			}

			virtual bool HasValue() const final
			{
				return GetLengthField() > Chunk::ChunkHeaderLength;
			}

			virtual uint16_t GetValueLength() const final
			{
				if (!HasValue())
				{
					return 0u;
				}

				return GetLengthField() - Chunk::ChunkHeaderLength;
			}

		protected:
			virtual void InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t valueLength) final;

			/**
			 * NOTE: Return ChunkHeader* instead of const ChunkHeader* since we may
			 * want to modify its fields.
			 */
			virtual ChunkHeader* GetHeaderPointer() const final
			{
				return reinterpret_cast<ChunkHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			/**
			 * Private private because it returns the value of the Value Length field,
			 * which is not useful for the application.
			 */
			virtual uint16_t GetLengthField() const final
			{
				return GetHeaderPointer()->length;
			}

			virtual void SetLengthField(uint16_t length) final
			{
				GetHeaderPointer()->length = length;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
